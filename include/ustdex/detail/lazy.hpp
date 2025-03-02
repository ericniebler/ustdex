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

#ifndef USTDEX_ASYNC_DETAIL_LAZY
#define USTDEX_ASYNC_DETAIL_LAZY

#include "config.hpp"
#include "meta.hpp"
#include "type_traits.hpp"

#include <memory>
#include <new> // IWYU pragma: keep

namespace ustdex
{
/// @brief A lazy type that can be used to delay the construction of a type.
template <class Ty>
struct _lazy
{
  USTDEX_API _lazy() noexcept {}

  USTDEX_API ~_lazy() {}

  template <class... Ts>
  USTDEX_API Ty& construct(Ts&&... _ts) noexcept(_nothrow_constructible<Ty, Ts...>)
  {
    Ty* _value_ = ::new (static_cast<void*>(std::addressof(_value_))) Ty{static_cast<Ts&&>(_ts)...};
    return *std::launder(_value_);
  }

  template <class Fn, class... Ts>
  USTDEX_API Ty& construct_from(Fn&& _fn, Ts&&... _ts) noexcept(_nothrow_callable<Fn, Ts...>)
  {
    Ty* _value_ = ::new (static_cast<void*>(std::addressof(_value_)))
      Ty{static_cast<Fn&&>(_fn)(static_cast<Ts&&>(_ts)...)};
    return *std::launder(_value_);
  }

  USTDEX_API void destroy() noexcept
  {
    std::destroy_at(&_value_);
  }

  union
  {
    Ty _value_;
  };
};

namespace detail
{
template <std::size_t Idx, std::size_t Size, std::size_t Align>
struct _lazy_box_
{
  static_assert(Size != 0);
  alignas(Align) unsigned char _data_[Size];
};

template <std::size_t Idx, class Ty>
using _lazy_box = _lazy_box_<Idx, sizeof(Ty), alignof(Ty)>;
} // namespace detail

template <class Idx, class... Ts>
struct _lazy_tupl;

template <>
struct _lazy_tupl<std::index_sequence<>>
{
  template <class Fn, class Self, class... Us>
  USTDEX_TRIVIAL_API static auto apply(Fn&& _fn, Self&&, Us&&... _us) //
    noexcept(_nothrow_callable<Fn, Us...>) -> _call_result_t<Fn, Us...>
  {
    return static_cast<Fn&&>(_fn)(static_cast<Us&&>(_us)...);
  }
};

template <std::size_t... Idx, class... Ts>
struct _lazy_tupl<std::index_sequence<Idx...>, Ts...> : detail::_lazy_box<Idx, Ts>...
{
  template <std::size_t Ny>
  using _at = _m_index<Ny, Ts...>;

  USTDEX_TRIVIAL_API _lazy_tupl() noexcept {}

  USTDEX_API ~_lazy_tupl()
  {
    ((_engaged_[Idx] ? std::destroy_at(_get<Idx, Ts>()) : void(0)), ...);
  }

  template <std::size_t Ny, class Ty>
  USTDEX_TRIVIAL_API Ty* _get() noexcept
  {
    return reinterpret_cast<Ty*>(this->detail::_lazy_box<Ny, Ty>::_data_);
  }

  template <std::size_t Ny, class... Us>
  USTDEX_TRIVIAL_API _at<Ny>& _emplace(Us&&... _us) //
    noexcept(_nothrow_constructible<_at<Ny>, Us...>)
  {
    using Ty      = _at<Ny>;
    Ty* _value_   = ::new (static_cast<void*>(_get<Ny, Ty>())) Ty{static_cast<Us&&>(_us)...};
    _engaged_[Ny] = true;
    return *std::launder(_value_);
  }

  template <class Fn, class Self, class... Us>
  USTDEX_TRIVIAL_API static auto apply(Fn&& _fn, Self&& _self, Us&&... _us) //
    noexcept(_nothrow_callable<Fn, Us..., _copy_cvref_t<Self, Ts>...>)
      -> _call_result_t<Fn, Us..., _copy_cvref_t<Self, Ts>...>
  {
    return static_cast<Fn&&>(
      _fn)(static_cast<Us&&>(_us)..., static_cast<_copy_cvref_t<Self, Ts>&&>(*_self.template _get<Idx, Ts>())...);
  }

  bool _engaged_[sizeof...(Ts)] = {};
};

#if USTDEX_MSVC()
template <class... Ts>
struct _mk_lazy_tuple_
{
  using _indices_t = std::make_index_sequence<sizeof...(Ts)>;
  using type       = _lazy_tupl<_indices_t, Ts...>;
};

template <class... Ts>
using _lazy_tuple = typename _mk_lazy_tuple_<Ts...>::type;
#else
template <class... Ts>
using _lazy_tuple = _lazy_tupl<std::make_index_sequence<sizeof...(Ts)>, Ts...>;
#endif

template <class... Ts>
using _decayed_lazy_tuple = _lazy_tuple<USTDEX_DECAY(Ts)...>;

} // namespace ustdex

#endif
