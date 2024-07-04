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

#include <utility>

#include "config.hpp"
#include "type_traits.hpp"

// This must be the last #include
#include "prologue.hpp"

namespace USTDEX_NAMESPACE
{
template <::std::size_t Idx, class Ty>
struct _box
{
  // Too many compiler bugs with [[no_unique_address]] to use it here.
  // E.g., https://github.com/llvm/llvm-project/issues/88077
  // USTDEX_NO_UNIQUE_ADDRESS
  Ty _value;
};

template <::std::size_t Idx, class Ty>
USTDEX_INLINE USTDEX_HOST_DEVICE constexpr auto _cget(_box<Idx, Ty> const& box) noexcept -> Ty const&
{
  return box._value;
}

template <class Idx, class... Ts>
struct _tupl;

template <::std::size_t... Idx, class... Ts>
struct _tupl<::std::index_sequence<Idx...>, Ts...> : _box<Idx, Ts>...
{
  template <class Fn, class Self, class... Us>
  USTDEX_INLINE USTDEX_HOST_DEVICE static auto apply(Fn&& fn, Self&& self, Us&&... us) //
    noexcept(
      noexcept(static_cast<Fn&&>(fn)(static_cast<Us&&>(us)..., static_cast<Self&&>(self)._box<Idx, Ts>::_value...)))
      -> decltype(static_cast<Fn&&>(fn)(static_cast<Us&&>(us)..., static_cast<Self&&>(self)._box<Idx, Ts>::_value...))
  {
    return static_cast<Fn&&>(fn)(static_cast<Us&&>(us)..., static_cast<Self&&>(self)._box<Idx, Ts>::_value...);
  }

  template <class Fn, class Self, class... Us>
  USTDEX_INLINE USTDEX_HOST_DEVICE static auto for_each(Fn&& fn, Self&& self, Us&&... us) //
    noexcept((_nothrow_callable<Fn, Us..., _copy_cvref_t<Self, Ts>> && ...))
      -> _mif<(_callable<Fn, Us..., _copy_cvref_t<Self, Ts>> && ...)>
  {
    return (static_cast<Fn&&>(fn)(static_cast<Us&&>(us)..., static_cast<Self&&>(self)._box<Idx, Ts>::_value), ...);
  }
};

template <class... Ts>
USTDEX_HOST_DEVICE _tupl(Ts...) //
  ->_tupl<::std::make_index_sequence<sizeof...(Ts)>, Ts...>;

template <class... Ts>
using _tuple = _tupl<::std::make_index_sequence<sizeof...(Ts)>, Ts...>;

template <class Fn, class Tupl, class... Us>
using _apply_result_t = decltype(DECLVAL(Tupl).apply(DECLVAL(Fn), DECLVAL(Tupl), DECLVAL(Us)...));

template <class First, class Second>
struct _pair
{
  First first;
  Second second;
};

template <class... Ts>
using _decayed_tuple = _tuple<_decay_t<Ts>...>;
} // namespace USTDEX_NAMESPACE

#include "epilogue.hpp"
