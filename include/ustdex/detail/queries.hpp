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

#include "meta.hpp"
#include "stop_token.hpp"
#include "type_traits.hpp"
#include "utility.hpp"

#include <memory>

namespace ustdex {
  template <class Query, class Ty>
  auto _query_result_() -> decltype(DECLVAL(Ty).query(Query()));

  template <class Query, class Ty>
  using _query_result_t = decltype(_query_result_<Query, Ty>());

  template <class Query, class Ty>
  USTDEX_DEVICE constexpr bool _has_query = _mvalid_q<_query_result_t, Query, Ty>;

  USTDEX_DEVICE constexpr struct get_allocator_t {
    template <class Env>
    USTDEX_HOST_DEVICE auto operator()(const Env &env) const noexcept //
      -> decltype(env.query(*this)) {
      static_assert(noexcept(env.query(*this)));
      return env.query(*this);
    }

    USTDEX_HOST_DEVICE auto
      operator()(_ignore_t) const noexcept -> std::allocator<void> {
      return {};
    }
  } get_allocator{};

  USTDEX_DEVICE constexpr struct get_stop_token_t {
    template <class Env>
    USTDEX_HOST_DEVICE auto operator()(const Env &env) const noexcept //
      -> decltype(env.query(*this)) {
      static_assert(noexcept(env.query(*this)));
      return env.query(*this);
    }

    USTDEX_HOST_DEVICE auto
      operator()(_ignore_t) const noexcept -> never_stop_token {
      return {};
    }
  } get_stop_token{};

  template <class T>
  using stop_token_of_t = _decay_t<_call_result_t<get_stop_token_t, T>>;

  template <class Tag>
  struct get_completion_scheduler_t {
    template <class Env>
    USTDEX_HOST_DEVICE auto operator()(const Env &env) const noexcept //
      -> decltype(env.query(*this)) {
      static_assert(noexcept(env.query(*this)));
      return env.query(*this);
    }
  };

  template <class Tag>
  USTDEX_DEVICE constexpr get_completion_scheduler_t<Tag> get_completion_scheduler{};

  USTDEX_DEVICE constexpr struct get_scheduler_t {
    template <class Env>
    USTDEX_HOST_DEVICE auto operator()(const Env &env) const noexcept //
      -> decltype(env.query(*this)) {
      static_assert(noexcept(env.query(*this)));
      return env.query(*this);
    }
  } get_scheduler{};

  USTDEX_DEVICE constexpr struct get_delegatee_scheduler_t {
    template <class Env>
    USTDEX_HOST_DEVICE auto operator()(const Env &env) const noexcept //
      -> decltype(env.query(*this)) {
      static_assert(noexcept(env.query(*this)));
      return env.query(*this);
    }
  } get_delegatee_scheduler{};

  enum class forward_progress_guarantee {
    concurrent,
    parallel,
    weakly_parallel
  };

  USTDEX_DEVICE constexpr struct get_forward_progress_guarantee_t {
    template <class Sch>
    USTDEX_HOST_DEVICE auto operator()(const Sch &sch) const noexcept //
      -> decltype(sch.query(*this)) {
      static_assert(noexcept(sch.query(*this)));
      return sch.query(*this);
    }

    USTDEX_HOST_DEVICE auto
      operator()(_ignore_t) const noexcept -> forward_progress_guarantee {
      return forward_progress_guarantee::weakly_parallel;
    }
  } get_forward_progress_guarantee{};

  USTDEX_DEVICE constexpr struct get_domain_t {
    template <class Sch>
    USTDEX_HOST_DEVICE constexpr auto operator()(const Sch &sch) const noexcept //
      -> decltype(sch.query(*this)) {
      static_assert(noexcept(sch.query(*this)));
      return sch.query(*this);
    }
  } get_domain{};

  template <class Sch>
  using domain_of_t = _call_result_t<get_domain_t, Sch>;

} // namespace ustdex
