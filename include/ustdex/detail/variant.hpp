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

#ifndef USTDEX_ASYNC_DETAIL_VARIANT
#define USTDEX_ASYNC_DETAIL_VARIANT

#include "meta.hpp"
#include "type_traits.hpp"
#include "utility.hpp"

#include <memory>
#include <new> // IWYU pragma: keep

#include "prologue.hpp"

namespace ustdex
{
/********************************************************************************/
/* NB: The variant type implemented here default-constructs into the valueless  */
/* state. This is different from std::variant which default-constructs into the */
/* first alternative. This is done to simplify the implementation and to avoid  */
/* the need for a default constructor for each alternative type.                */
/********************************************************************************/

template <class Idx, class... Ts>
class _variant_impl;

template <>
class _variant_impl<std::index_sequence<>>
{
public:
  template <class Fn, class... Us>
  USTDEX_API void _visit(Fn&&, Us&&...) const noexcept
  {}
};

template <std::size_t... Idx, class... Ts>
class _variant_impl<std::index_sequence<Idx...>, Ts...>
{
  static constexpr std::size_t _max_size = _maximum({sizeof(Ts)...});
  static_assert(_max_size != 0);
  std::size_t _index_{_npos};
  alignas(Ts...) unsigned char _storage_[_max_size];

  template <std::size_t Ny>
  using _at = _m_index<Ny, Ts...>;

  USTDEX_API void _destroy() noexcept
  {
    if (_index_ != _npos)
    {
      // make this local in case destroying the sub-object destroys *this
      const auto index = ustdex::_exchange(_index_, _npos);
      ((Idx == index ? std::destroy_at(static_cast<_at<Idx>*>(_ptr())) : void(0)), ...);
    }
  }

public:
  USTDEX_IMMOVABLE(_variant_impl);

  USTDEX_API _variant_impl() noexcept {}

  USTDEX_API ~_variant_impl()
  {
    _destroy();
  }

  USTDEX_TRIVIAL_API void* _ptr() noexcept
  {
    return _storage_;
  }

  USTDEX_TRIVIAL_API std::size_t _index() const noexcept
  {
    return _index_;
  }

  template <class Ty, class... As>
  USTDEX_API Ty& _emplace(As&&... _as) //
    noexcept(_nothrow_constructible<Ty, As...>)
  {
    constexpr std::size_t _new_index = ustdex::_index_of<Ty, Ts...>();
    static_assert(_new_index != _npos, "Type not in variant");

    _destroy();
    Ty* _value = ::new (_ptr()) Ty{static_cast<As&&>(_as)...};
    _index_    = _new_index;
    return *std::launder(_value);
  }

  template <std::size_t Ny, class... As>
  USTDEX_API _at<Ny>& _emplace_at(As&&... _as) //
    noexcept(_nothrow_constructible<_at<Ny>, As...>)
  {
    static_assert(Ny < sizeof...(Ts), "variant index is too large");

    _destroy();
    _at<Ny>* _value = ::new (_ptr()) _at<Ny>{static_cast<As&&>(_as)...};
    _index_         = Ny;
    return *std::launder(_value);
  }

  template <class Fn, class... As>
  USTDEX_API auto _emplace_from(Fn&& _fn, As&&... _as) //
    noexcept(_nothrow_callable<Fn, As...>) -> _call_result_t<Fn, As...>&
  {
    using _result_t                  = _call_result_t<Fn, As...>;
    constexpr std::size_t _new_index = ustdex::_index_of<_result_t, Ts...>();
    static_assert(_new_index != _npos, "Type not in variant");

    _destroy();
    _result_t* _value = ::new (_ptr()) _result_t(static_cast<Fn&&>(_fn)(static_cast<As&&>(_as)...));
    _index_           = _new_index;
    return *std::launder(_value);
  }

  template <class Fn, class Self, class... As>
  USTDEX_API static void _visit(Fn&& _fn, Self&& _self, As&&... _as) //
    noexcept((_nothrow_callable<Fn, As..., _copy_cvref_t<Self, Ts>> && ...))
  {
    // make this local in case destroying the sub-object destroys *this
    const auto index = _self._index_;
    USTDEX_ASSERT(index != _npos, "");
    ((Idx == index ? static_cast<Fn&&>(_fn)(static_cast<As&&>(_as)..., static_cast<Self&&>(_self).template _get<Idx>())
                   : void()),
     ...);
  }

  template <std::size_t Ny>
  USTDEX_API _at<Ny>&& _get() && noexcept
  {
    USTDEX_ASSERT(Ny == _index_, "");
    return static_cast<_at<Ny>&&>(*static_cast<_at<Ny>*>(_ptr()));
  }

  template <std::size_t Ny>
  USTDEX_API _at<Ny>& _get() & noexcept
  {
    USTDEX_ASSERT(Ny == _index_, "");
    return *static_cast<_at<Ny>*>(_ptr());
  }

  template <std::size_t Ny>
  USTDEX_API const _at<Ny>& _get() const& noexcept
  {
    USTDEX_ASSERT(Ny == _index_, "");
    return *static_cast<const _at<Ny>*>(_ptr());
  }
};

#if USTDEX_MSVC()
template <class... Ts>
struct _mk_variant_
{
  using _indices_t = std::make_index_sequence<sizeof...(Ts)>;
  using type       = _variant_impl<_indices_t, Ts...>;
};

template <class... Ts>
using _variant = typename _mk_variant_<Ts...>::type;
#else
template <class... Ts>
using _variant = _variant_impl<std::make_index_sequence<sizeof...(Ts)>, Ts...>;
#endif

template <class... Ts>
using _decayed_variant = _variant<USTDEX_DECAY(Ts)...>;
} // namespace ustdex

#include "epilogue.hpp"

#endif
