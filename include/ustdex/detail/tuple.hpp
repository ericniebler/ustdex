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

#ifndef USTDEX_ASYNC_DETAIL_TUPLE
#define USTDEX_ASYNC_DETAIL_TUPLE

#include "config.hpp"
#include "meta.hpp"
#include "type_traits.hpp"

#include "prologue.hpp"

namespace ustdex
{
template <std::size_t Idx, class Ty>
struct _box
{
  // Too many compiler bugs with [[no_unique_address]] to use it here.
  // E.g., https://github.com/llvm/llvm-project/issues/88077
  // USTDEX_NO_UNIQUE_ADDRESS
  Ty _value_;
};

template <std::size_t Idx, class Ty>
USTDEX_TRIVIAL_API constexpr auto _cget(_box<Idx, Ty> const& _box) noexcept -> Ty const&
{
  return _box._value_;
}

template <class Idx, class... Ts>
struct _tupl;

template <std::size_t... Idx, class... Ts>
struct _tupl<std::index_sequence<Idx...>, Ts...> : _box<Idx, Ts>...
{
  template <class Fn, class Self, class... Us>
  USTDEX_TRIVIAL_API static auto apply(Fn&& _fn, Self&& _self, Us&&... _us) //
    noexcept(_nothrow_callable<Fn, Us..., _copy_cvref_t<Self, Ts>...>)
      -> _call_result_t<Fn, Us..., _copy_cvref_t<Self, Ts>...>
  {
    return static_cast<Fn&&>(_fn)( //
      static_cast<Us&&>(_us)...,
      static_cast<Self&&>(_self)._box<Idx, Ts>::_value_...);
  }

  template <class Fn, class Self, class... Us>
  USTDEX_TRIVIAL_API static auto for_each(Fn&& _fn, Self&& _self, Us&&... _us) //
    noexcept((_nothrow_callable<Fn, Us..., _copy_cvref_t<Self, Ts>> && ...))
      -> std::enable_if_t<(_callable<Fn, Us..., _copy_cvref_t<Self, Ts>> && ...)>
  {
    return (static_cast<Fn&&>(_fn)(static_cast<Us&&>(_us)..., static_cast<Self&&>(_self)._box<Idx, Ts>::_value_), ...);
  }
};

template <class... Ts>
_tupl(Ts...) //
  ->_tupl<std::make_index_sequence<sizeof...(Ts)>, Ts...>;

template <class Fn, class Tupl, class... Us>
using _apply_result_t = decltype(declval<Tupl>().apply(declval<Fn>(), declval<Tupl>(), declval<Us>()...));

#if USTDEX_MSVC()
template <class... Ts>
struct _mk_tuple_
{
  using _indices_t = std::make_index_sequence<sizeof...(Ts)>;
  using type       = _tupl<_indices_t, Ts...>;
};

template <class... Ts>
using _tuple = typename _mk_tuple_<Ts...>::type;
#else
template <class... Ts>
using _tuple = _tupl<std::make_index_sequence<sizeof...(Ts)>, Ts...>;
#endif

template <class... Ts>
using _decayed_tuple = _tuple<USTDEX_DECAY(Ts)...>;

// A very simple pair type
template <class First, class Second>
struct _pair
{
  First first;
  Second second;
};

template <class First, class Second>
_pair(First, Second) -> _pair<First, Second>;

} // namespace ustdex

#include "epilogue.hpp"

#endif
