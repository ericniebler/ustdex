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

#include <cstddef>
#include <new>
#include <type_traits>

#include "meta.hpp"
#include "type_traits.hpp"
#include "utility.hpp"

// This must be the last #include
#include "prologue.hpp"

namespace USTDEX_NAMESPACE
{
namespace _detail
{
template <class Ty>
USTDEX_HOST_DEVICE void _destroy(Ty& ty) noexcept
{
  ty.~Ty();
}
} // namespace _detail
template <class Idx, class... Ts>
class _variant_impl;

template <>
class _variant_impl<::std::index_sequence<>>
{
public:
  template <class Fn, class... Us>
  USTDEX_HOST_DEVICE void visit(Fn&&, Us&&...) const noexcept
  {}
};

template <::std::size_t... Idx, class... Ts>
class _variant_impl<::std::index_sequence<Idx...>, Ts...>
{
  static constexpr ::std::size_t _max_size = _max({sizeof(Ts)...});
  static_assert(_max_size != 0);
  ::std::size_t _index{_npos};
  alignas(Ts...) unsigned char _storage[_max_size];

  template <::std::size_t Ny>
  using _at = _m_at_c<Ny, Ts...>;

  USTDEX_HOST_DEVICE void _destroy() noexcept
  {
    if (_index != _npos)
    {
      // make this local in case destroying the sub-object destroys *this
      const auto index = ustdex::_exchange(_index, _npos);
#if USTDEX_NVHPC()
      // Unknown nvc++ name lookup bug
      ((Idx == index ? static_cast<_at<Idx>*>(_ptr())->Ts::~Ts() : void(0)), ...);
#elif USTDEX_NVCC()
      // Unknown nvcc name lookup bug
      ((Idx == index ? _detail::_destroy(*static_cast<_at<Idx>*>(_ptr())) : void(0)), ...);
#else
      // casting the destructor expression to void is necessary for MSVC in
      // /permissive- mode.
      ((Idx == index ? void((*static_cast<_at<Idx>*>(_ptr())).~Ts()) : void(0)), ...);
#endif
    }
  }

public:
  USTDEX_IMMOVABLE(_variant_impl);

  USTDEX_HOST_DEVICE _variant_impl() noexcept {}

  USTDEX_HOST_DEVICE ~_variant_impl()
  {
    _destroy();
  }

  USTDEX_HOST_DEVICE USTDEX_INLINE void* _ptr() noexcept
  {
    return _storage;
  }

  USTDEX_HOST_DEVICE USTDEX_INLINE ::std::size_t index() const noexcept
  {
    return _index;
  }

  template <class Ty, class... As>
  USTDEX_HOST_DEVICE Ty& emplace(As&&... as) //
    noexcept(_nothrow_constructible<Ty, As...>)
  {
    constexpr ::std::size_t _new_index = ustdex::_index_of<Ty, Ts...>();
    static_assert(_new_index != _npos, "Type not in variant");

    _destroy();
    ::new (_ptr()) Ty{static_cast<As&&>(as)...};
    _index = _new_index;
    return *static_cast<Ty*>(_ptr());
  }

  template <::std::size_t Ny, class... As>
  USTDEX_HOST_DEVICE _at<Ny>& emplace_at(As&&... as) //
    noexcept(_nothrow_constructible<_at<Ny>, As...>)
  {
    static_assert(Ny < sizeof...(Ts), "variant index is too large");

    _destroy();
    ::new (_ptr()) _at<Ny>{static_cast<As&&>(as)...};
    _index = Ny;
    return *static_cast<_at<Ny>*>(_ptr());
  }

  template <class Fn, class... As>
  USTDEX_HOST_DEVICE auto emplace_from(Fn&& fn, As&&... as) //
    noexcept(_nothrow_callable<Fn, As...>) -> _call_result_t<Fn, As...>&
  {
    using _result_t                  = _call_result_t<Fn, As...>;
    constexpr ::std::size_t _new_index = ustdex::_index_of<_result_t, Ts...>();
    static_assert(_new_index != _npos, "Type not in variant");

    _destroy();
    ::new (_ptr()) _result_t(static_cast<Fn&&>(fn)(static_cast<As&&>(as)...));
    _index = _new_index;
    return *static_cast<_result_t*>(_ptr());
  }

  template <class Fn, class Self, class... As>
  USTDEX_HOST_DEVICE static void visit(Fn&& fn, Self&& self, As&&... as) //
    noexcept((_nothrow_callable<Fn, As..., _copy_cvref_t<Self, Ts>> && ...))
  {
    // make this local in case destroying the sub-object destroys *this
    const auto index = self._index;
    USTDEX_ASSERT(index != _npos);
    ((Idx == index ? static_cast<Fn&&>(fn)(static_cast<As&&>(as)..., static_cast<Self&&>(self).template get<Idx>())
                   : void()),
     ...);
  }

  template <::std::size_t Ny>
  USTDEX_HOST_DEVICE _at<Ny>&& get() && noexcept
  {
    USTDEX_ASSERT(Ny == _index);
    return static_cast<_at<Ny>&&>(*static_cast<_at<Ny>*>(_ptr()));
  }

  template <::std::size_t Ny>
  USTDEX_HOST_DEVICE _at<Ny>& get() & noexcept
  {
    USTDEX_ASSERT(Ny == _index);
    return *static_cast<_at<Ny>*>(_ptr());
  }

  template <::std::size_t Ny>
  USTDEX_HOST_DEVICE const _at<Ny>& get() const& noexcept
  {
    USTDEX_ASSERT(Ny == _index);
    return *static_cast<const _at<Ny>*>(_ptr());
  }
};

template <class... Ts>
using _variant = _variant_impl<::std::make_index_sequence<sizeof...(Ts)>, Ts...>;

template <class... Ts>
using _decayed_variant = _variant<_decay_t<Ts>...>;
} // namespace USTDEX_NAMESPACE

#include "epilogue.hpp"
