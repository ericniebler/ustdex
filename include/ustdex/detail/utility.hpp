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

#include <initializer_list>

#include "config.hpp"
#include "meta.hpp"
#include "type_traits.hpp"

// This must be the last #include
#include "prologue.hpp"

namespace USTDEX_NAMESPACE
{
USTDEX_DEVICE_CONSTANT constexpr ::std::size_t _npos = ~0UL;

struct _ignore
{
  template <class... As>
  USTDEX_HOST_DEVICE constexpr _ignore(As&&...) noexcept {};
};

template <class...>
struct _undefined;

struct _empty
{};

struct [[deprecated]] _deprecated
{};

struct _nil
{};

struct _immovable
{
  _immovable() = default;
  USTDEX_IMMOVABLE(_immovable);
};

USTDEX_HOST_DEVICE constexpr ::std::size_t _max(::std::initializer_list<::std::size_t> il) noexcept
{
  ::std::size_t max = 0;
  for (auto i : il)
  {
    if (i > max)
    {
      max = i;
    }
  }
  return max;
}

USTDEX_HOST_DEVICE constexpr ::std::size_t _find_pos(bool const* const begin, bool const* const end) noexcept
{
  for (bool const* where = begin; where != end; ++where)
  {
    if (*where)
    {
      return static_cast<::std::size_t>(where - begin);
    }
  }
  return _npos;
}

template <class Ty, class... Ts>
USTDEX_HOST_DEVICE constexpr ::std::size_t _index_of() noexcept
{
  constexpr bool _same[] = {USTDEX_IS_SAME(Ty, Ts)...};
  return ustdex::_find_pos(_same, _same + sizeof...(Ts));
}

template <class Ty, class Uy = Ty>
USTDEX_HOST_DEVICE constexpr Ty _exchange(Ty& obj, Uy&& new_value) noexcept
{
  constexpr bool _nothrow = //
    noexcept(Ty(static_cast<Ty&&>(obj)))&& //
    noexcept(obj = static_cast<Uy&&>(new_value)); //
  static_assert(_nothrow);

  Ty old_value = static_cast<Ty&&>(obj);
  obj          = static_cast<Uy&&>(new_value);
  return old_value;
}

template <class Ty>
USTDEX_HOST_DEVICE constexpr void _swap(Ty& left, Ty& right) noexcept
{
  constexpr bool _nothrow = //
    noexcept(Ty(static_cast<Ty&&>(left)))&& //
    noexcept(left = static_cast<Ty&&>(right)); //
  static_assert(_nothrow);

  Ty tmp = static_cast<Ty&&>(left);
  left   = static_cast<Ty&&>(right);
  right  = static_cast<Ty&&>(tmp);
}

template <class Ty>
USTDEX_HOST_DEVICE constexpr Ty _decay_copy(Ty&& ty) noexcept(_nothrow_decay_copyable<Ty>)
{
  return static_cast<Ty&&>(ty);
}

// _ref is for keeping type names short in compiler output.
// It has the unfortunate side effect of obfuscating the types.
#if USTDEX_NVHPC() || USTDEX_EDG()
// This version of _ref is for compilers that display the return
// type of a monomorphic lambda in the compiler output.
inline constexpr auto _ref = [](auto fn) noexcept {
  return [fn](auto...) noexcept -> decltype(auto) {
    return fn;
  };
};
#else
inline constexpr auto _ref = [](auto fn) noexcept {
  return [fn]() noexcept -> decltype(auto) {
    return fn;
  };
};
#endif

template <class Ty>
using _ref_t = decltype(_ref(static_cast<Ty (*)()>(nullptr)));

template <class Ref>
using _deref_t = decltype(_declval<Ref>()()());
} // namespace USTDEX_NAMESPACE

#include "epilogue.hpp"
