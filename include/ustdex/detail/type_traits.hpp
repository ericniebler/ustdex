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

#ifndef USTDEX_ASYNC_DETAIL_TYPE_TRAITS
#define USTDEX_ASYNC_DETAIL_TYPE_TRAITS

#include "config.hpp"
#include "meta.hpp"

#include <type_traits> // IWYU pragma: keep

#include "prologue.hpp"

namespace ustdex
{
//////////////////////////////////////////////////////////////////////////////////////////////////
// _copy_cvref_t: For copying cvref from one type to another
struct _cp
{
  template <class Tp>
  using call = Tp;
};

struct _cpc
{
  template <class Tp>
  using call = const Tp;
};

struct _cplr
{
  template <class Tp>
  using call = Tp&;
};

struct _cprr
{
  template <class Tp>
  using call = Tp&&;
};

struct _cpclr
{
  template <class Tp>
  using call = const Tp&;
};

struct _cpcrr
{
  template <class Tp>
  using call = const Tp&&;
};

template <class>
extern _cp _cpcvr;
template <class Tp>
extern _cpc _cpcvr<const Tp>;
template <class Tp>
extern _cplr _cpcvr<Tp&>;
template <class Tp>
extern _cprr _cpcvr<Tp&&>;
template <class Tp>
extern _cpclr _cpcvr<const Tp&>;
template <class Tp>
extern _cpcrr _cpcvr<const Tp&&>;
template <class Tp>
using _copy_cvref_fn = decltype(_cpcvr<Tp>);

template <class From, class To>
using _copy_cvref_t = typename _copy_cvref_fn<From>::template call<To>;

template <class Fn, class... As>
using _call_result_t = decltype(declval<Fn>()(declval<As>()...));

template <class Fn, class... As>
inline constexpr bool _callable = _m_callable_q<_call_result_t, Fn, As...>;

template <class Ty>
using _identity_t = Ty;

template <class Ty>
using _decay_copyable_ = decltype(USTDEX_DECAY(Ty)(declval<Ty>()));

template <class... As>
inline constexpr bool _decay_copyable = (_m_callable_q<_decay_copyable_, As> && ...);

template <class Ty, template <class...> class Fn>
inline constexpr bool _is_specialization_of = false;

template <template <class...> class Fn, class... Ts>
inline constexpr bool _is_specialization_of<Fn<Ts...>, Fn> = true;

template <bool Init, bool... Bs>
inline constexpr bool _fold_expr_and = (Init and ... and Bs);

#if defined(__CUDA_ARCH__)
template <class Fn, class... As>
inline constexpr bool _nothrow_callable = true;

template <class Ty, class... As>
inline constexpr bool _nothrow_constructible = true;

template <class... As>
inline constexpr bool _nothrow_decay_copyable = true;

template <class... As>
inline constexpr bool _nothrow_movable = true;

template <class... As>
inline constexpr bool _nothrow_copyable = true;
#else
template <class Fn, class... As>
using _nothrow_callable_ = std::enable_if_t<noexcept(declval<Fn>()(declval<As>()...))>;

template <class Fn, class... As>
inline constexpr bool _nothrow_callable = _m_callable_q<_nothrow_callable_, Fn, As...>;

template <class Ty, class... As>
using _nothrow_constructible_ = std::enable_if_t<noexcept(Ty{declval<As>()...})>;

template <class Ty, class... As>
inline constexpr bool _nothrow_constructible = _m_callable_q<_nothrow_constructible_, Ty, As...>;

template <class Ty>
using _nothrow_decay_copyable_ = std::enable_if_t<noexcept(USTDEX_DECAY(Ty)(declval<Ty>()))>;

template <class... As>
inline constexpr bool _nothrow_decay_copyable = (_m_callable_q<_nothrow_decay_copyable_, As> && ...);

template <class Ty>
using _nothrow_movable_ = std::enable_if_t<noexcept(Ty(declval<Ty>()))>;

template <class... As>
inline constexpr bool _nothrow_movable = (_m_callable_q<_nothrow_movable_, As> && ...);

template <class Ty>
using _nothrow_copyable_ = std::enable_if_t<noexcept(Ty(declval<const Ty&>()))>;

template <class... As>
inline constexpr bool _nothrow_copyable = (_m_callable_q<_nothrow_copyable_, As> && ...);
#endif

template <class... As>
using _nothrow_decay_copyable_t = std::bool_constant<_nothrow_decay_copyable<As...>>;
} // namespace ustdex

#include "epilogue.hpp"

#endif
