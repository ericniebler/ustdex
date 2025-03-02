/*
 * SPDX-FileCopyrightText: Copyright (c) 2025 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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

#ifndef USTDEX_ASYNC_DETAIL_UTILITY
#define USTDEX_ASYNC_DETAIL_UTILITY

#include "config.hpp"
#include "meta.hpp"
#include "type_traits.hpp"

#include "prologue.hpp"

namespace ustdex
{
USTDEX_DEVICE_CONSTANT constexpr std::size_t _npos = static_cast<std::size_t>(-1);

struct _ignore
{
  _ignore() = default;

  template <class First, class... Rest>
  constexpr _ignore(First&&, Rest&&...) noexcept
  {}

  template <class Ty>
  constexpr _ignore& operator=(Ty&&) noexcept
  {
    return *this;
  }
};

struct _empty
{};

struct _nil
{};

struct _immovable
{
  _immovable() = default;
  USTDEX_IMMOVABLE(_immovable);
};

USTDEX_API constexpr std::size_t _maximum(std::initializer_list<std::size_t> _il) noexcept
{
  std::size_t _max = 0;
  for (auto i : _il)
  {
    if (i > _max)
    {
      _max = i;
    }
  }
  return _max;
}

USTDEX_API constexpr std::size_t _find_pos(bool const* const _begin, bool const* const _end) noexcept
{
  for (bool const* _where = _begin; _where != _end; ++_where)
  {
    if (*_where)
    {
      return static_cast<std::size_t>(_where - _begin);
    }
  }
  return _npos;
}

template <class Ty, class... Ts>
USTDEX_API constexpr std::size_t _index_of() noexcept
{
  constexpr bool _same[] = {std::is_same_v<Ty, Ts>...};
  return ustdex::_find_pos(_same, _same + sizeof...(Ts));
}

template <class Ty, class Uy = Ty>
USTDEX_API constexpr Ty _exchange(Ty& _obj, Uy&& _new_value) noexcept
{
  constexpr bool _is_nothrow =                      //
    noexcept(Ty(static_cast<Ty&&>(_obj))) &&        //
    noexcept(_obj = static_cast<Uy&&>(_new_value)); //
  static_assert(_is_nothrow);

  Ty old_value = static_cast<Ty&&>(_obj);
  _obj         = static_cast<Uy&&>(_new_value);
  return old_value;
}

template <class Ty>
USTDEX_API constexpr void _swap(Ty& _left, Ty& _right) noexcept
{
  constexpr bool _is_nothrow =                   //
    noexcept(Ty(static_cast<Ty&&>(_left))) &&    //
    noexcept(_left = static_cast<Ty&&>(_right)); //
  static_assert(_is_nothrow);

  Ty _tmp = static_cast<Ty&&>(_left);
  _left   = static_cast<Ty&&>(_right);
  _right  = static_cast<Ty&&>(_tmp);
}

template <class Ty>
USTDEX_API constexpr std::decay_t<Ty> _decay_copy(Ty&& _ty) noexcept(_nothrow_decay_copyable<Ty>)
{
  return static_cast<Ty&&>(_ty);
}

[[noreturn]] inline void _unreachable()
{
  // Uses compiler specific extensions if possible. Even if no extension is
  // used, undefined behavior is still raised by an empty function body and the
  // noreturn attribute.
#if USTDEX_MSVC() && !USTDEX_CLANG_CL()
  __assume(false);
#else
  __builtin_unreachable();
#endif
}

USTDEX_PRAGMA_PUSH()
USTDEX_PRAGMA_IGNORE_GNU("-Wnon-template-friend")
USTDEX_PRAGMA_IGNORE_EDG(probable_guiding_friend)

// _zip/_unzip is for keeping type names short. It has the unfortunate side
// effect of obfuscating the types.
namespace
{
template <std::size_t Ny>
struct _slot
{
  friend constexpr auto _slot_allocated(_slot<Ny>);
};

template <class Type, std::size_t Ny>
struct _allocate_slot
{
  static constexpr std::size_t _value = Ny;

  friend constexpr auto _slot_allocated(_slot<Ny>)
  {
    return static_cast<Type (*)()>(nullptr);
  }
};

template <class Type, std::size_t Id = 0, std::size_t Pow2 = 0>
constexpr std::size_t _next(long);

// If _slot_allocated(_slot<Id>) has NOT been defined, then SFINAE will keep
// this function out of the overload set...
template <class Type, //
          std::size_t Id   = 0,
          std::size_t Pow2 = 0,
          bool             = !_slot_allocated(_slot<Id + (1 << Pow2) - 1>())>
constexpr std::size_t _next(int)
{
  return ustdex::_next<Type, Id, Pow2 + 1>(0);
}

template <class Type, std::size_t Id, std::size_t Pow2>
constexpr std::size_t _next(long)
{
  if constexpr (Pow2 == 0)
  {
    return _allocate_slot<Type, Id>::_value;
  }
  else
  {
    return ustdex::_next<Type, Id + (1 << (Pow2 - 1)), 0>(0);
  }
}

// Prior to Clang 12, we can't use the _slot trick to erase long type names
// because of a compiler bug. We'll just use the original type name in that case.
#if USTDEX_CLANG() && (__clang__ < 12)

template <class Type>
using _zip = Type;

template <class Id>
using _unzip = Id;

#else

template <class Type, std::size_t Val = ustdex::_next<Type>(0)>
using _zip = _slot<Val>;

template <class Id>
using _unzip = decltype(_slot_allocated(Id())());

#endif

// burn the first slot
using _ignore_this_typedef [[maybe_unused]] = _zip<void>;
} // namespace

USTDEX_PRAGMA_POP()

} // namespace ustdex

#include "epilogue.hpp"

#endif
