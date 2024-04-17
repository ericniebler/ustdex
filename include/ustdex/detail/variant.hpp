/*
 * Copyright (c) 2024 NVIDIA Corporation
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
#pragma once

#include "meta.hpp"
#include "type_traits.hpp"
#include "utility.hpp"

#include <cstddef>
#include <new>
#include <type_traits>

namespace ustdex {
  namespace _detail {
    template <class... Ts, std::size_t... Idx>
    USTDEX_HOST_DEVICE inline void
      _destroy(_mindices<Idx...>, std::size_t idx, void *pv) noexcept {
      ((Idx == idx ? static_cast<Ts *>(pv)->~Ts() : void()), ...);
    }
  } // namespace _detail

  USTDEX_DEVICE constexpr std::size_t _variant_npos = static_cast<std::size_t>(-1);

  template <class Idx, class... Ts>
  class _variant_impl;

  template <std::size_t... Idx, class... Ts>
  class _variant_impl<std::index_sequence<Idx...>, Ts...> : _immovable {
    static constexpr std::size_t _max_size = _max({sizeof(Ts)...});
    std::size_t _index{_variant_npos};
    alignas(Ts...) unsigned char _storage[_max_size];

    USTDEX_HOST_DEVICE void _destroy() noexcept {
      if (_index != _variant_npos) {
        ((Idx == _index ? get<Idx>().~Ts() : void(0)), ...);
      }
    }

    template <std::size_t Ny>
    using _at = _m_at_c<Ny, Ts...>;

   public:
    USTDEX_HOST_DEVICE _variant_impl() noexcept {
    }

    USTDEX_HOST_DEVICE ~_variant_impl() {
      _destroy();
    }

    USTDEX_HOST_DEVICE USTDEX_INLINE void *_get_ptr() noexcept {
      return _storage;
    }

    USTDEX_HOST_DEVICE USTDEX_INLINE std::size_t index() const noexcept {
      return _index;
    }

    template <class Ty, class... As>
    USTDEX_HOST_DEVICE Ty &emplace(As &&...as) //
      noexcept(_nothrow_constructible<Ty, As...>) {
      constexpr std::size_t _new_index = _index_of<Ty, Ts...>();
      static_assert(_new_index != _variant_npos, "Type not in variant");

      _destroy();
      ::new (_storage) Ty{static_cast<As &&>(as)...};
      _index = _new_index;
      return *reinterpret_cast<Ty *>(_storage);
    }

    template <std::size_t Ny, class... As>
    USTDEX_HOST_DEVICE _at<Ny> &emplace(As &&...as) //
      noexcept(_nothrow_constructible<_at<Ny>, As...>) {
      static_assert(Ny < sizeof...(Ts), "variant index is too large");

      _destroy();
      ::new (_storage) _at<Ny>{static_cast<As &&>(as)...};
      _index = Ny;
      return *reinterpret_cast<_at<Ny> *>(_storage);
    }

    template <class Fn, class... As>
    USTDEX_HOST_DEVICE auto emplace_from(Fn &&fn, As &&...as) //
      noexcept(_nothrow_callable<Fn, As...>) -> _call_result_t<Fn, As...> & {
      using _result_t = _call_result_t<Fn, As...>;
      constexpr std::size_t _new_index = _index_of<_result_t, Ts...>();
      static_assert(_new_index != _variant_npos, "Type not in variant");

      _destroy();
      ::new (_storage) _result_t(static_cast<Fn &&>(fn)(static_cast<As &&>(as)...));
      _index = _new_index;
      return *reinterpret_cast<_result_t *>(_storage);
    }

    template <class Fn, class... As>
    USTDEX_HOST_DEVICE void visit(Fn &&fn, As &&...as) & //
      noexcept((_nothrow_callable<Fn, As..., Ts &> && ...)) {
      USTDEX_ASSERT(_index != _variant_npos);
      ((Idx == _index ? static_cast<Fn &&>(fn)(static_cast<As &&>(as)..., get<Idx>())
                      : void()),
       ...);
    }

    template <std::size_t Ny>
    USTDEX_HOST_DEVICE decltype(auto) get() noexcept {
      USTDEX_ASSERT(Ny == _index);
      return *reinterpret_cast<_at<Ny> *>(_storage);
    }

    template <std::size_t Ny>
    USTDEX_HOST_DEVICE decltype(auto) get() const noexcept {
      USTDEX_ASSERT(Ny == _index);
      return *reinterpret_cast<const _at<Ny> *>(_storage);
    }
  };

  template <class... Ts>
  using _variant = _variant_impl<std::make_index_sequence<sizeof...(Ts)>, Ts...>;

  template <class... Ts>
  using _decayed_variant = _variant<_decay_t<Ts>...>;
} // namespace ustdex
