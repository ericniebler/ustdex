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

#include "type_traits.hpp"
#include "utility.hpp"

#include <cstddef>
#include <new>
#include <type_traits>

namespace ustdex {
  namespace _detail {
    USTDEX_HOST_DEVICE
    inline void _noop(unsigned char *) noexcept {
    }

    template <class Ty>
    USTDEX_HOST_DEVICE
    inline void _destroy(unsigned char *storage) noexcept {
      reinterpret_cast<Ty *>(storage)->~Ty();
    }

    using _destroy_fn_t = void (*)(unsigned char *) noexcept;

    template <class... Ts>
    USTDEX_DEVICE constexpr _destroy_fn_t _destroy_fns[] = {&_noop, &_destroy<Ts>...};
  } // namespace _detail

  USTDEX_DEVICE constexpr std::size_t _variant_npos = static_cast<std::size_t>(-1);

  template <class... Ts>
  struct _variant;

  template <class Head, class... Tail>
  class _variant<Head, Tail...> : _immovable {
    static constexpr std::size_t _max_size = _max({sizeof(Head), sizeof(Tail)...});
    std::size_t _index{_variant_npos};
    alignas(Head) alignas(Tail...) unsigned char _storage[_max_size];

    USTDEX_HOST_DEVICE
    void _destroy() noexcept {
      _detail::_destroy_fns<Head, Tail...>[_index + 1](_storage);
      _index = _variant_npos;
    }

   public:
    USTDEX_HOST_DEVICE _variant()
      : _index(_variant_npos) {
    }

    USTDEX_HOST_DEVICE ~_variant() {
      _destroy();
    }

    USTDEX_HOST_DEVICE
    void *_get_ptr() noexcept {
      return _storage;
    }

    USTDEX_HOST_DEVICE
    std::size_t index() const noexcept {
      return _index;
    }

    template <class Ty, class... As>
    USTDEX_HOST_DEVICE
    Ty &emplace(As &&...as) noexcept(noexcept(Ty{static_cast<As &&>(as)...})) {
      constexpr std::size_t _new_index = _index_of<Ty, Head, Tail...>();
      static_assert(_new_index != _variant_npos, "Type not in variant");

      _destroy();
      ::new (_storage) Ty{static_cast<As &&>(as)...};
      _index = _new_index;
      return *reinterpret_cast<Ty *>(_storage);
    }

    template <class Fn, class... As>
    USTDEX_HOST_DEVICE
    auto emplace_from(Fn &&fn, As &&...as) noexcept(_nothrow_callable<Fn, As...>)
      -> _call_result_t<Fn, As...> & {
      using _result_t = _call_result_t<Fn, As...>;
      constexpr std::size_t _new_index = _index_of<_result_t, Head, Tail...>();
      static_assert(_new_index != _variant_npos, "Type not in variant");

      _destroy();
      ::new (_storage) _result_t(static_cast<Fn &&>(fn)(static_cast<As &&>(as)...));
      _index = _new_index;
      return *reinterpret_cast<_result_t *>(_storage);
    }

    template <std::size_t Idx>
    USTDEX_HOST_DEVICE
    decltype(auto) get() noexcept {
      USTDEX_ASSERT(Idx == _index);
      using Ty = _m_at_c<Idx, Head, Tail...>;
      return *reinterpret_cast<Ty *>(_storage);
    }

    template <std::size_t Idx>
    USTDEX_HOST_DEVICE
    decltype(auto) get() const noexcept {
      USTDEX_ASSERT(Idx == _index);
      using Ty = _m_at_c<Idx, Head, Tail...>;
      return *reinterpret_cast<const Ty *>(_storage);
    }
  };
} // namespace ustdex
