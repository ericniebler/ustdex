//===----------------------------------------------------------------------===//
//
// Part of CUDA Experimental in CUDA C++ Core Libraries,
// under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// SPDX-FileCopyrightText: Copyright (c) 2024 NVIDIA CORPORATION & AFFILIATES.
//
//===----------------------------------------------------------------------===//
#pragma once

#include "meta.hpp"
#include "stop_token.hpp"
#include "type_traits.hpp"
#include "utility.hpp"

#include <memory>

// Must be the last include
#include "prologue.hpp"

namespace USTDEX_NAMESPACE
{
template <class Ty, class Query>
auto _query_result_() -> decltype(DECLVAL(Ty).query(Query()));

template <class Ty, class Query>
using _query_result_t = decltype(_query_result_<Ty, Query>());

template <class Ty, class Query>
inline constexpr bool _queryable = _mvalid_q<_query_result_t, Ty, Query>;

#if defined(__CUDA_ARCH__)
template <class Ty, class Query>
inline constexpr bool _nothrow_queryable = true;
#else
template <class Ty, class Query>
using _nothrow_queryable_ = _mif<noexcept(DECLVAL(Ty).query(Query()))>;

template <class Ty, class Query>
inline constexpr bool _nothrow_queryable = _mvalid_q<_nothrow_queryable_, Ty, Query>;
#endif

USTDEX_DEVICE_CONSTANT constexpr struct get_allocator_t
{
  template <class Env>
  USTDEX_HOST_DEVICE auto operator()(const Env& env) const noexcept //
    -> decltype(env.query(*this))
  {
    static_assert(noexcept(env.query(*this)));
    return env.query(*this);
  }

  USTDEX_HOST_DEVICE auto operator()(_ignore) const noexcept -> ::std::allocator<void>
  {
    return {};
  }
} get_allocator{};

USTDEX_DEVICE_CONSTANT constexpr struct get_stop_token_t
{
  template <class Env>
  USTDEX_HOST_DEVICE auto operator()(const Env& env) const noexcept //
    -> decltype(env.query(*this))
  {
    static_assert(noexcept(env.query(*this)));
    return env.query(*this);
  }

  USTDEX_HOST_DEVICE auto operator()(_ignore) const noexcept -> never_stop_token
  {
    return {};
  }
} get_stop_token{};

template <class T>
using stop_token_of_t = _decay_t<_call_result_t<get_stop_token_t, T>>;

template <class Tag>
struct get_completion_scheduler_t
{
  template <class Env>
  USTDEX_HOST_DEVICE auto operator()(const Env& env) const noexcept //
    -> decltype(env.query(*this))
  {
    static_assert(noexcept(env.query(*this)));
    return env.query(*this);
  }
};

template <class Tag>
USTDEX_DEVICE_CONSTANT constexpr get_completion_scheduler_t<Tag> get_completion_scheduler{};

USTDEX_DEVICE_CONSTANT constexpr struct get_scheduler_t
{
  template <class Env>
  USTDEX_HOST_DEVICE auto operator()(const Env& env) const noexcept //
    -> decltype(env.query(*this))
  {
    static_assert(noexcept(env.query(*this)));
    return env.query(*this);
  }
} get_scheduler{};

USTDEX_DEVICE_CONSTANT constexpr struct get_delegatee_scheduler_t
{
  template <class Env>
  USTDEX_HOST_DEVICE auto operator()(const Env& env) const noexcept //
    -> decltype(env.query(*this))
  {
    static_assert(noexcept(env.query(*this)));
    return env.query(*this);
  }
} get_delegatee_scheduler{};

enum class forward_progress_guarantee
{
  concurrent,
  parallel,
  weakly_parallel
};

USTDEX_DEVICE_CONSTANT constexpr struct get_forward_progress_guarantee_t
{
  template <class Sch>
  USTDEX_HOST_DEVICE auto operator()(const Sch& sch) const noexcept //
    -> decltype(ustdex::_decay_copy(sch.query(*this)))
  {
    static_assert(noexcept(sch.query(*this)));
    return sch.query(*this);
  }

  USTDEX_HOST_DEVICE auto operator()(_ignore) const noexcept -> forward_progress_guarantee
  {
    return forward_progress_guarantee::weakly_parallel;
  }
} get_forward_progress_guarantee{};

USTDEX_DEVICE_CONSTANT constexpr struct get_domain_t
{
  template <class Sch>
  USTDEX_HOST_DEVICE constexpr auto operator()(const Sch& sch) const noexcept //
    -> decltype(ustdex::_decay_copy(sch.query(*this)))
  {
    return {};
  }
} get_domain{};

template <class Sch>
using domain_of_t = _call_result_t<get_domain_t, Sch>;

} // namespace USTDEX_NAMESPACE

#include "epilogue.hpp"
