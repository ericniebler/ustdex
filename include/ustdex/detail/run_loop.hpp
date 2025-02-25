/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
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

#ifndef USTDEX_ASYNC_DETAIL_RUN_LOOP
#define USTDEX_ASYNC_DETAIL_RUN_LOOP

#include "config.hpp"

// libcu++ does not have <cuda/std/mutex> or <cuda/std/condition_variable>
#if !defined(__CUDA_ARCH__)

#  include "completion_signatures.hpp"
#  include "env.hpp"
#  include "exception.hpp"
#  include "queries.hpp"
#  include "utility.hpp"

#  include <condition_variable>
#  include <mutex>

#  include "prologue.hpp"

namespace ustdex
{
class run_loop;

struct _task : _immovable
{
  using _execute_fn_t = void(_task*) noexcept;

  _task()             = default;

  USTDEX_API explicit _task(_task* _next, _task* _tail) noexcept
      : _next_{_next}
      , _tail_{_tail}
  {}

  USTDEX_API explicit _task(_task* _next, _execute_fn_t* _execute) noexcept
      : _next_{_next}
      , _execute_fn_{_execute}
  {}

  _task* _next_ = this;

  union
  {
    _task* _tail_ = nullptr;
    _execute_fn_t* _execute_fn_;
  };

  USTDEX_API void _execute() noexcept
  {
    (*_execute_fn_)(this);
  }
};

template <class Rcvr>
struct _operation : _task
{
  run_loop* _loop_;
  USTDEX_NO_UNIQUE_ADDRESS Rcvr _rcvr_;

  USTDEX_API static void _execute_impl(_task* _p) noexcept
  {
    auto& _rcvr = static_cast<_operation*>(_p)->_rcvr_;
    USTDEX_TRY( //
      ({        //
        if (get_stop_token(get_env(_rcvr)).stop_requested())
        {
          set_stopped(static_cast<Rcvr&&>(_rcvr));
        }
        else
        {
          set_value(static_cast<Rcvr&&>(_rcvr));
        }
      }),
      USTDEX_CATCH(...) //
      ({                //
        set_error(static_cast<Rcvr&&>(_rcvr), ::std::current_exception());
      })                //
    )
  }

  USTDEX_API explicit _operation(_task* _tail_) noexcept
      : _task{this, _tail_}
  {}

  USTDEX_API _operation(_task* _next_, run_loop* _loop, Rcvr _rcvr)
      : _task{_next_, &_execute_impl}
      , _loop_{_loop}
      , _rcvr_{static_cast<Rcvr&&>(_rcvr)}
  {}

  USTDEX_API void start() & noexcept;
};

class run_loop
{
  template <class... Ts>
  using _completion_signatures = completion_signatures<Ts...>;

  template <class>
  friend struct _operation;

public:
  run_loop() noexcept
  {
    _head._next_ = _head._tail_ = &_head;
  }

  class _scheduler
  {
    struct _schedule_task
    {
      using _t             = _schedule_task;
      using _id            = _schedule_task;
      using sender_concept = sender_t;

      template <class Rcvr>
      USTDEX_API auto connect(Rcvr _rcvr) const noexcept -> _operation<Rcvr>
      {
        return {&_loop_->_head, _loop_, static_cast<Rcvr&&>(_rcvr)};
      }

      template <class Self>
      USTDEX_API static constexpr auto get_completion_signatures() noexcept
      {
        return completion_signatures<set_value_t(), set_error_t(::std::exception_ptr), set_stopped_t()>();
      }

    private:
      friend _scheduler;

      struct _env
      {
        run_loop* _loop_;

        template <class Tag>
        USTDEX_API auto query(get_completion_scheduler_t<Tag>) const noexcept -> _scheduler
        {
          return _loop_->get_scheduler();
        }
      };

      USTDEX_API auto get_env() const noexcept -> _env
      {
        return _env{_loop_};
      }

      USTDEX_API explicit _schedule_task(run_loop* _loop) noexcept
          : _loop_(_loop)
      {}

      run_loop* const _loop_;
    };

    friend run_loop;

    USTDEX_API explicit _scheduler(run_loop* _loop) noexcept
        : _loop_(_loop)
    {}

    USTDEX_API auto query(get_forward_progress_guarantee_t) const noexcept -> forward_progress_guarantee
    {
      return forward_progress_guarantee::parallel;
    }

    run_loop* _loop_;

  public:
    using scheduler_concept = scheduler_t;

    [[nodiscard]] USTDEX_API auto schedule() const noexcept -> _schedule_task
    {
      return _schedule_task{_loop_};
    }

    USTDEX_API friend bool operator==(const _scheduler& _a, const _scheduler& _b) noexcept
    {
      return _a._loop_ == _b._loop_;
    }

    USTDEX_API friend bool operator!=(const _scheduler& _a, const _scheduler& _b) noexcept
    {
      return _a._loop_ != _b._loop_;
    }
  };

  USTDEX_API auto get_scheduler() noexcept -> _scheduler
  {
    return _scheduler{this};
  }

  USTDEX_API void run();

  USTDEX_API void finish();

private:
  USTDEX_API void _push_back(_task* _tsk);
  USTDEX_API auto _pop_front() -> _task*;

  ::std::mutex _mutex{};
  ::std::condition_variable _cv{};
  _task _head{};
  bool _stop = false;
};

template <class Rcvr>
USTDEX_API inline void _operation<Rcvr>::start() & noexcept {
  USTDEX_TRY(                                                             //
    ({                                                                    //
      _loop_->_push_back(this);                                           //
    }),                                                                   //
    USTDEX_CATCH(...)                                                     //
    ({                                                                    //
      set_error(static_cast<Rcvr&&>(_rcvr_), ::std::current_exception()); //
    })                                                                    //
    )                                                                     //
}

USTDEX_API inline void run_loop::run()
{
  for (_task* _tsk = _pop_front(); _tsk != &_head; _tsk = _pop_front())
  {
    _tsk->_execute();
  }
}

USTDEX_API inline void run_loop::finish()
{
  ::std::unique_lock _lock{_mutex};
  _stop = true;
  _cv.notify_all();
}

USTDEX_API inline void run_loop::_push_back(_task* _tsk)
{
  ::std::unique_lock _lock{_mutex};
  _tsk->_next_ = &_head;
  _head._tail_ = _head._tail_->_next_ = _tsk;
  _cv.notify_one();
}

USTDEX_API inline auto run_loop::_pop_front() -> _task*
{
  ::std::unique_lock _lock{_mutex};
  _cv.wait(_lock, [this] {
    return _head._next_ != &_head || _stop;
  });
  if (_head._tail_ == _head._next_)
  {
    _head._tail_ = &_head;
  }
  return ustdex::_exchange(_head._next_, _head._next_->_next_);
}
} // namespace ustdex

#  include "epilogue.hpp"

#endif // !defined(__CUDA_ARCH__)

#endif
