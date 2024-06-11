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

#include "ustdex/detail/config.hpp"
#include "ustdex/detail/just.hpp"
#include "ustdex/detail/when_all.hpp"

#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>

namespace {
  //! Scheduler that will send impulses on user's request.
  //! One can obtain senders from this, connect them to receivers and start the operation states.
  //! Until the scheduler is told to start the next operation, the actions in the operation states are
  //! not executed. This is similar to a task scheduler, but it's single threaded. It has basic
  //! thread-safety to allow it to be run with `sync_wait` (which makes us not control when the
  //! operation_state object is created and started).
  struct impulse_scheduler {
   private:
    //! Command type that can store the action of firing up a sender
    using cmd_t = std::function<void()>;
    using cmd_vec_t = std::vector<cmd_t>;

    struct data_t : std::enable_shared_from_this<data_t> {
      explicit data_t(int id)
        : id_(id) {
      }

      int id_;
      std::mutex mutex_;
      std::condition_variable cv_;
      std::vector<std::function<void()>> all_commands_;
    };

    //! That data_t shared between the operation state and the actual scheduler
    //! Shared pointer to allow the scheduler to be copied (not the best semantics, but it will do)
    std::shared_ptr<data_t> data_{};

    template <class Rcvr>
    struct opstate_t : ustdex::_immovable {
      data_t* data_;
      Rcvr rcvr_;

      opstate_t(data_t* data, Rcvr&& rcvr)
        : data_(data)
        , rcvr_(static_cast<Rcvr&&>(rcvr)) {
      }

      void start() & noexcept {
        // Enqueue another command to the list of all commands
        // The scheduler will start this, whenever start_next() is called
        std::unique_lock lock{data_->mutex_};
        data_->all_commands_.emplace_back([this]() {
          if (ustdex::get_stop_token(ustdex::get_env(rcvr_)).stop_requested()) {
            ustdex::set_stopped(static_cast<Rcvr&&>(rcvr_));
          } else {
            ustdex::set_value(static_cast<Rcvr&&>(rcvr_));
          }
        });
        data_->cv_.notify_all();
      }
    };

    struct env_t {
      data_t* data_;

      impulse_scheduler
        query(ustdex::get_completion_scheduler_t<ustdex::set_value_t>) const noexcept {
        return impulse_scheduler{data_};
      }

      impulse_scheduler
        query(ustdex::get_completion_scheduler_t<ustdex::set_stopped_t>) const noexcept {
        return impulse_scheduler{data_};
      }
    };

    struct sndr_t {
      using sender_concept = ustdex::sender_t;

      data_t* data_;

      auto get_completion_signatures(ustdex::_ignore = {}) const
        -> ustdex::completion_signatures<ustdex::set_value_t(), ustdex::set_stopped_t()>;

      template <class Rcvr>
      opstate_t<Rcvr> connect(Rcvr rcvr) {
        return {data_, static_cast<Rcvr&&>(rcvr)};
      }

      auto get_env() const noexcept {
        return env_t{data_};
      }
    };

    explicit impulse_scheduler(data_t* data)
      : data_(data->shared_from_this()) {
    }

   public:
    using scheduler_concept = ustdex::scheduler_t;

    impulse_scheduler()
      : data_(std::make_shared<data_t>(0)) {
    }

    explicit impulse_scheduler(int id)
      : data_(std::make_shared<data_t>(id)) {
    }

    ~impulse_scheduler() = default;

    //! Actually start the command from the last started operation_state
    //! Returns immediately if no command registered (i.e., no operation state started)
    bool try_start_next() {
      // Wait for a command that we can execute
      std::unique_lock lock{data_->mutex_};

      // If there are no commands in the queue, return false
      if (data_->all_commands_.empty()) {
        return false;
      }

      // Pop one command from the queue
      auto cmd = std::move(data_->all_commands_.front());
      data_->all_commands_.erase(data_->all_commands_.begin());
      // Exit the lock before executing the command
      lock.unlock();
      // Execute the command, i.e., send an impulse to the connected sender
      cmd();
      // Return true to signal that we started a command
      return true;
    }

    //! Actually start the command from the last started operation_state
    //! Blocks if no command registered (i.e., no operation state started)
    void start_next() {
      // Wait for a command that we can execute
      std::unique_lock lock{data_->mutex_};
      while (data_->all_commands_.empty()) {
        data_->cv_.wait(lock);
      }

      // Pop one command from the queue
      auto cmd = std::move(data_->all_commands_.front());
      data_->all_commands_.erase(data_->all_commands_.begin());
      // Exit the lock before executing the command
      lock.unlock();
      // Execute the command, i.e., send an impulse to the connected sender
      cmd();
    }

    sndr_t schedule() const noexcept {
      return sndr_t{data_.get()};
    }

    friend bool operator==(impulse_scheduler, impulse_scheduler) noexcept {
      return true;
    }

    friend bool operator!=(impulse_scheduler, impulse_scheduler) noexcept {
      return false;
    }
  };
} // namespace
