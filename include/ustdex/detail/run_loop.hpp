/*
 * Copyright (c) 2024 NVIDIA Corporation
 *
 * Licensed under the Apache License Version 2.0 with LLVM Exceptions
 * (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 *
 *   https://llvm.org/LICENSE.txt
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once

#include "config.hpp"

// libcu++ does not have <cuda/std/mutex> or <cuda/std/condition_variable>
#ifndef USTDEX_CUDA

#  include "completion_signatures.hpp"
#  include "env.hpp"
#  include "queries.hpp"

#  include <condition_variable>
#  include <exception>
#  include <mutex>
#  include <utility>

namespace ustdex {
  class run_loop;

  struct _task : _immovable {
    _task *_next = this;

    union {
      _task *_tail;
      void (*_execute_fn)(_task *) noexcept;
    };

    USTDEX_HOST_DEVICE void _execute() noexcept {
      (*_execute_fn)(this);
    }
  };

  template <class Rcvr>
  struct _operation : _task {
    run_loop *_loop;
    USTDEX_NO_UNIQUE_ADDRESS Rcvr _rcvr;

    USTDEX_HOST_DEVICE static void _execute_impl(_task *_p) noexcept {
      auto &_rcvr = static_cast<_operation *>(_p)->_rcvr;
      USTDEX_TRY(
        ({
          if (get_stop_token(get_env(_rcvr)).stop_requested()) {
            set_stopped(static_cast<Rcvr &&>(_rcvr));
          } else {
            set_value(static_cast<Rcvr &&>(_rcvr));
          }
        }),
        USTDEX_CATCH(...)({
          set_error(static_cast<Rcvr &&>(_rcvr), std::current_exception());
        }))
    }

    USTDEX_HOST_DEVICE explicit _operation(_task *_tail) noexcept
      : _task{{}, this, _tail} {
    }

    USTDEX_HOST_DEVICE _operation(_task *_next, run_loop *_loop, Rcvr _rcvr)
      : _task{{}, _next}
      , _loop{_loop}
      , _rcvr{static_cast<Rcvr &&>(_rcvr)} {
      _execute_fn = &_execute_impl;
    }

    USTDEX_HOST_DEVICE void start() & noexcept;
  };

  class run_loop {
    template <class... Ts>
    using _completion_signatures = completion_signatures<Ts...>;

    template <class>
    friend struct _operation;

   public:
    class _scheduler {
      struct _schedule_task {
        using _t = _schedule_task;
        using _id = _schedule_task;
        using sender_concept = sender_t;

        template <class... Env>
        auto get_completion_signatures(const Env &...) const //
          -> completion_signatures<
            set_value_t(),
            set_error_t(std::exception_ptr),
            set_stopped_t()>;

        template <class Rcvr>
        USTDEX_HOST_DEVICE auto connect(Rcvr _rcvr) const noexcept -> _operation<Rcvr> {
          return {&_loop->_head, _loop, static_cast<Rcvr &&>(_rcvr)};
        }

       private:
        friend _scheduler;

        struct _env {
          run_loop *_loop;

          template <class Tag>
          USTDEX_HOST_DEVICE auto
            query(get_completion_scheduler_t<Tag>) const noexcept -> _scheduler {
            return _loop->get_scheduler();
          }
        };

        USTDEX_HOST_DEVICE auto get_env() const noexcept -> _env {
          return _env{_loop};
        }

        USTDEX_HOST_DEVICE explicit _schedule_task(run_loop *_loop) noexcept
          : _loop(_loop) {
        }

        run_loop *const _loop;
      };

      friend run_loop;

      USTDEX_HOST_DEVICE explicit _scheduler(run_loop *_loop) noexcept
        : _loop(_loop) {
      }

      USTDEX_HOST_DEVICE auto query(get_forward_progress_guarantee_t) const noexcept
        -> forward_progress_guarantee {
        return forward_progress_guarantee::parallel;
      }

      run_loop *_loop;

     public:
      using scheduler_concept = scheduler_t;

      [[nodiscard]]
      USTDEX_HOST_DEVICE auto schedule() const noexcept -> _schedule_task {
        return _schedule_task{_loop};
      }

      USTDEX_HOST_DEVICE friend bool
        operator==(const _scheduler &_a, const _scheduler &_b) noexcept {
        return _a._loop == _b._loop;
      }

      USTDEX_HOST_DEVICE friend bool
        operator!=(const _scheduler &_a, const _scheduler &_b) noexcept {
        return _a._loop != _b._loop;
      }
    };

    USTDEX_HOST_DEVICE auto get_scheduler() noexcept -> _scheduler {
      return _scheduler{this};
    }

    USTDEX_HOST_DEVICE void run();

    USTDEX_HOST_DEVICE void finish();

   private:
    USTDEX_HOST_DEVICE void _push_back(_task *_task);
    USTDEX_HOST_DEVICE auto _pop_front() -> _task *;

    std::mutex _mutex;
    std::condition_variable _cv;
    _task _head{{}, &_head, &_head};
    bool _stop = false;
  };

  template <class Rcvr>
  USTDEX_HOST_DEVICE inline void _operation<Rcvr>::start() & noexcept {
    USTDEX_TRY(
      ({                                                                  //
        _loop->_push_back(this);                                          //
      }),                                                                 //
      USTDEX_CATCH(...)({                                                 //
        set_error(static_cast<Rcvr &&>(_rcvr), std::current_exception()); //
      }))                                                                 //
  }

  USTDEX_HOST_DEVICE inline void run_loop::run() {
    for (_task *_task; (_task = _pop_front()) != &_head;) {
      _task->_execute();
    }
  }

  USTDEX_HOST_DEVICE inline void run_loop::finish() {
    std::unique_lock _lock{_mutex};
    _stop = true;
    _cv.notify_all();
  }

  USTDEX_HOST_DEVICE inline void run_loop::_push_back(_task *_task) {
    std::unique_lock _lock{_mutex};
    _task->_next = &_head;
    _head._tail = _head._tail->_next = _task;
    _cv.notify_one();
  }

  USTDEX_HOST_DEVICE inline auto run_loop::_pop_front() -> _task * {
    std::unique_lock _lock{_mutex};
    _cv.wait(_lock, [this] { return _head._next != &_head || _stop; });
    if (_head._tail == _head._next)
      _head._tail = &_head;
    return std::exchange(_head._next, _head._next->_next);
  }
} // namespace ustdex

#endif // USTDEX_CUDA
