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

USTDEX_PRAGMA_PUSH()
USTDEX_PRAGMA_IGNORE_GNU("-Wnon-template-friend")
USTDEX_PRAGMA_IGNORE_EDG(probable_guiding_friend)

// _zip/_unzip is for keeping type names short. It has the unfortunate side
// effect of obfuscating the types.
namespace
{
template <::std::size_t N>
struct _slot
{
  friend constexpr auto _slot_allocated(_slot<N>);
  static constexpr ::std::size_t value = N;
};

template <class Type, ::std::size_t N>
struct _allocate_slot : _slot<N>
{
  friend constexpr auto _slot_allocated(_slot<N>)
  {
    return static_cast<Type (*)()>(nullptr);
  }
};

inline constexpr struct adl_t
{
}* adl = nullptr;

// If _slot_allocated(_slot<I>) has NOT been defined, then SFINAE will keep this function out of the overload set...
template <typename Type, ::std::size_t I = 0, ::std::size_t P = 0, bool = !_slot_allocated(_slot<I + (1 << P) - 1>())>
constexpr ::std::size_t _next(adl_t*)
{
  return _next<Type, I, P + 1>(adl);
}

template <class Type, ::std::size_t I = 0, ::std::size_t P = 0>
constexpr ::std::size_t _next(void*)
{
  if constexpr (P == 0)
  {
    return _allocate_slot<Type, I>::value;
  }
  else
  {
    return _next<Type, I + (1 << (P - 1)), 0>(adl);
  }
}

template <class Type, std::size_t Val = _next<Type>(adl)>
using _zip = _slot<Val>;

template <class Id>
using _unzip = decltype(_slot_allocated(Id())());

// burn the first slot
using _ignore_this_typedef [[maybe_unused]] = _zip<void>;
} // namespace

USTDEX_PRAGMA_POP()

} // namespace USTDEX_NAMESPACE

#include "epilogue.hpp"
