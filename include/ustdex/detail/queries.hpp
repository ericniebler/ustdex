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

#include "stop_token.hpp"
#include "utility.hpp"

namespace ustdex {
  USTDEX_DEVICE constexpr struct get_stop_token_t {
    template <class Env>
    USTDEX_HOST_DEVICE
    auto
      operator()(const Env &e) const noexcept -> decltype(e.query(*this)) {
      static_assert(noexcept(e.query(*this)));
      return e.query(*this);
    }

    USTDEX_HOST_DEVICE
    auto
      operator()(_ignore) const noexcept -> never_stop_token {
      return {};
    }
  } get_stop_token{};

  template <class Tag>
  struct get_completion_scheduler_t {
    template <class Env>
    USTDEX_HOST_DEVICE
    auto
      operator()(const Env &e) const noexcept -> decltype(e.query(*this)) {
      static_assert(noexcept(e.query(*this)));
      return e.query(*this);
    }
  };

  template <class Tag>
  USTDEX_DEVICE constexpr get_completion_scheduler_t<Tag> get_completion_scheduler{};

  USTDEX_DEVICE constexpr struct get_scheduler_t {
    template <class Env>
    USTDEX_HOST_DEVICE
    auto
      operator()(const Env &e) const noexcept -> decltype(e.query(*this)) {
      static_assert(noexcept(e.query(*this)));
      return e.query(*this);
    }
  } get_scheduler{};

  USTDEX_DEVICE constexpr struct get_delegatee_scheduler_t {
    template <class Env>
    USTDEX_HOST_DEVICE
    auto
      operator()(const Env &e) const noexcept -> decltype(e.query(*this)) {
      static_assert(noexcept(e.query(*this)));
      return e.query(*this);
    }
  } get_delegatee_scheduler{};

  enum class forward_progress_guarantee {
    concurrent,
    parallel,
    weakly_parallel
  };

  USTDEX_DEVICE constexpr struct get_forward_progress_guarantee_t {
    template <class Env>
    USTDEX_HOST_DEVICE
    auto
      operator()(const Env &e) const noexcept -> decltype(e.query(*this)) {
      static_assert(noexcept(e.query(*this)));
      return e.query(*this);
    }

    USTDEX_HOST_DEVICE
    auto
      operator()(_ignore) const noexcept -> forward_progress_guarantee {
      return forward_progress_guarantee::weakly_parallel;
    }
  } get_forward_progress_guarantee{};

} // namespace ustdex
