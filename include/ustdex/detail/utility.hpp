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

template <class Type, ::std::size_t Id = 0, ::std::size_t Pow2 = 0>
constexpr ::std::size_t _next(long);

// If _slot_allocated(_slot<Id>) has NOT been defined, then SFINAE will keep this function out of the overload set...
template <class Type, //
          ::std::size_t Id   = 0,
          ::std::size_t Pow2 = 0,
          bool               = !_slot_allocated(_slot<Id + (1 << Pow2) - 1>())>
constexpr ::std::size_t _next(int)
{
  return ustdex::_next<Type, Id, Pow2 + 1>(0);
}

template <class Type, ::std::size_t Id, ::std::size_t Pow2>
constexpr ::std::size_t _next(long)
{
  if constexpr (Pow2 == 0)
  {
    return _allocate_slot<Type, Id>::value;
  }
  else
  {
    return ustdex::_next<Type, Id + (1 << (Pow2 - 1)), 0>(0);
  }
}

template <class Type, ::std::size_t Val = ustdex::_next<Type>(0)>
using _zip = _slot<Val>;

template <class Id>
using _unzip = decltype(_slot_allocated(Id())());

// burn the first slot
using _ignore_this_typedef [[maybe_unused]] = _zip<void>;
} // namespace

USTDEX_PRAGMA_POP()

} // namespace USTDEX_NAMESPACE

#include "epilogue.hpp"
