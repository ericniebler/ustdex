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

#include "ustdex/detail/completion_signatures.hpp"
#include "ustdex/detail/cpos.hpp"
#include "ustdex/detail/queries.hpp"
#include "ustdex/detail/utility.hpp"

namespace {
  //! Scheduler that returns a sender that always completes with stopped.
  struct stopped_scheduler {
   private:
    struct env_t {
      auto query(ustdex::get_completion_scheduler_t<ustdex::set_value_t>) const noexcept {
        return stopped_scheduler{};
      }

      auto
        query(ustdex::get_completion_scheduler_t<ustdex::set_stopped_t>) const noexcept {
        return stopped_scheduler{};
      }
    };

    template <typename Rcvr>
    struct opstate_t : ustdex::_immovable {
      using operation_state_concept = ustdex::operation_state_t;

      Rcvr _rcvr;

      void start() noexcept {
        ustdex::set_stopped(static_cast<Rcvr&&>(_rcvr));
      }
    };

    struct sndr_t {
      using sender_concept = ustdex::sender_t;

      auto get_completion_signatures(ustdex::_ignore_t = {})
        -> ustdex::completion_signatures<
          ustdex::set_value_t(),
          ustdex::set_stopped_t()>;

      template <typename Rcvr>
      opstate_t<Rcvr> connect(Rcvr rcvr) const {
        return {{}, static_cast<Rcvr&&>(rcvr)};
      }

      env_t get_env() const noexcept {
        return {};
      }
    };

    friend bool operator==(stopped_scheduler, stopped_scheduler) noexcept {
      return true;
    }

    friend bool operator!=(stopped_scheduler, stopped_scheduler) noexcept {
      return false;
    }

   public:
    using scheduler_concept = ustdex::scheduler_t;

    stopped_scheduler() = default;

    sndr_t schedule() const noexcept {
      return {};
    }
  };
} // namespace
