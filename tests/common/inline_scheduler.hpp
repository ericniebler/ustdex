/*
 * SPDX-FileCopyrightText: Copyright (c) 2025 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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

#pragma once

#include "utility.hpp"

#include "ustdex/ustdex.hpp"

namespace
{
//! Scheduler that returns a sender that always completes inline
//! (successfully).
struct inline_scheduler
{
private:
  struct env_t
  {
    HOST_DEVICE auto query(ustdex::get_completion_scheduler_t<ustdex::set_value_t>) const noexcept
    {
      return inline_scheduler{};
    }
  };

  template <class Rcvr>
  struct opstate_t : ustdex::_immovable
  {
    using operation_state_concept = ustdex::operation_state_t;
    using completion_signatures   = ustdex::completion_signatures<ustdex::set_value_t()>;

    Rcvr _rcvr;

    HOST_DEVICE void start() noexcept
    {
      ustdex::set_value(static_cast<Rcvr&&>(_rcvr));
    }
  };

  struct sndr_t
  {
    using sender_concept = ustdex::sender_t;

    template <class>
    static constexpr auto get_completion_signatures()
    {
      return ustdex::completion_signatures<ustdex::set_value_t()>{};
    }

    template <class Rcvr>
    HOST_DEVICE opstate_t<Rcvr> connect(Rcvr rcvr) const
    {
      return {{}, static_cast<Rcvr&&>(rcvr)};
    }

    HOST_DEVICE env_t get_env() const noexcept
    {
      return {};
    }
  };

  HOST_DEVICE friend bool operator==(inline_scheduler, inline_scheduler) noexcept
  {
    return true;
  }

  HOST_DEVICE friend bool operator!=(inline_scheduler, inline_scheduler) noexcept
  {
    return false;
  }

public:
  using scheduler_concept = ustdex::scheduler_t;

  inline_scheduler()      = default;

  HOST_DEVICE sndr_t schedule() const noexcept
  {
    return {};
  }
};
} // namespace
