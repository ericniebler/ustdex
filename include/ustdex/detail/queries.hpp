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

#ifndef USTDEX_ASYNC_DETAIL_QUERIES
#define USTDEX_ASYNC_DETAIL_QUERIES

#include "meta.hpp"
#include "stop_token.hpp"
#include "type_traits.hpp"
#include "utility.hpp"

#include <memory>

#include "prologue.hpp"

namespace ustdex
{
template <class Ty, class Query>
auto _query_result_() -> decltype(declval<Ty>().query(Query()));

template <class Ty, class Query>
using _query_result_t = decltype(_query_result_<Ty, Query>());

template <class Ty, class Query>
inline constexpr bool _queryable_with = _m_callable_q<_query_result_t, Ty, Query>;

#if defined(__CUDA_ARCH__)
template <class Ty, class Query>
inline constexpr bool _nothrow_queryable_with = true;
#else
template <class Ty, class Query>
using _nothrow_queryable_ = std::enable_if_t<noexcept(declval<Ty>().query(Query()))>;

template <class Ty, class Query>
inline constexpr bool _nothrow_queryable_with = _m_callable_q<_nothrow_queryable_, Ty, Query>;
#endif

inline constexpr struct get_allocator_t
{
  template <class Env>
  USTDEX_API auto operator()(const Env& _env) const noexcept //
    -> decltype(_env.query(*this))
  {
    static_assert(noexcept(_env.query(*this)));
    return _env.query(*this);
  }

  USTDEX_API auto operator()(_ignore) const noexcept -> std::allocator<void>
  {
    return {};
  }
} get_allocator{};

inline constexpr struct get_stop_token_t
{
  template <class Env>
  USTDEX_API auto operator()(const Env& _env) const noexcept //
    -> decltype(_env.query(*this))
  {
    static_assert(noexcept(_env.query(*this)));
    return _env.query(*this);
  }

  USTDEX_API auto operator()(_ignore) const noexcept -> never_stop_token
  {
    return {};
  }
} get_stop_token{};

template <class Ty>
using stop_token_of_t = USTDEX_DECAY(_call_result_t<get_stop_token_t, Ty>);

template <class Tag>
struct get_completion_scheduler_t
{
  template <class Env>
  USTDEX_API auto operator()(const Env& _env) const noexcept //
    -> decltype(_env.query(*this))
  {
    static_assert(noexcept(_env.query(*this)));
    return _env.query(*this);
  }
};

template <class Tag>
inline constexpr get_completion_scheduler_t<Tag> get_completion_scheduler{};

inline constexpr struct get_scheduler_t
{
  template <class Env>
  USTDEX_API auto operator()(const Env& _env) const noexcept //
    -> decltype(_env.query(*this))
  {
    static_assert(noexcept(_env.query(*this)));
    return _env.query(*this);
  }
} get_scheduler{};

inline constexpr struct get_delegation_scheduler_t
{
  template <class Env>
  USTDEX_API auto operator()(const Env& _env) const noexcept //
    -> decltype(_env.query(*this))
  {
    static_assert(noexcept(_env.query(*this)));
    return _env.query(*this);
  }
} get_delegation_scheduler{};

enum class forward_progress_guarantee
{
  concurrent,
  parallel,
  weakly_parallel
};

inline constexpr struct get_forward_progress_guarantee_t
{
  template <class Sch>
  USTDEX_API auto operator()(const Sch& _sch) const noexcept //
    -> decltype(ustdex::_decay_copy(_sch.query(*this)))
  {
    static_assert(noexcept(_sch.query(*this)));
    return _sch.query(*this);
  }

  USTDEX_API auto operator()(_ignore) const noexcept -> forward_progress_guarantee
  {
    return forward_progress_guarantee::weakly_parallel;
  }
} get_forward_progress_guarantee{};

inline constexpr struct get_domain_t
{
  template <class Sch>
  USTDEX_API constexpr auto operator()(const Sch& _sch) const noexcept //
    -> decltype(ustdex::_decay_copy(_sch.query(*this)))
  {
    return {};
  }
} get_domain{};

template <class Sch>
using domain_of_t = _call_result_t<get_domain_t, Sch>;

} // namespace ustdex

#include "epilogue.hpp"

#endif
