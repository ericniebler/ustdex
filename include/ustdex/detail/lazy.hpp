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

#include "config.hpp"
#include "type_traits.hpp"

#include <new>

namespace ustdex {
  /// @brief A lazy type that can be used to delay the construction of a type.
  template <class Ty>
  struct _lazy {
    _lazy() noexcept {
    }

    ~_lazy() {
    }

    template <class... Ts>
    Ty &construct(Ts &&...ts) noexcept(_nothrow_constructible<Ty, Ts...>) {
      ::new (static_cast<void *>(std::addressof(value))) Ty(static_cast<Ts &&>(ts)...);
      return value;
    }

    void destroy() noexcept {
      value.~Ty();
    }

    union {
      Ty value;
    };
  };

  namespace _detail {
    template <std::size_t Idx, std::size_t Size, std::size_t Align>
    struct _lazy_box_ {
      static_assert(Size != 0);
      alignas(Align) unsigned char _data[Size];
    };

    template <std::size_t Idx, class Ty>
    using _lazy_box = _lazy_box_<Idx, sizeof(Ty), alignof(Ty)>;
  } // namespace _detail

  template <class Idx, class... Ts>
  struct _lazy_tupl;

  template <>
  struct _lazy_tupl<std::index_sequence<>> {
    template <class Fn, class Self, class... Us>
    USTDEX_INLINE USTDEX_HOST_DEVICE static auto apply(Fn &&fn, Self &&self, Us &&...us) //
      noexcept(_nothrow_callable<Fn, Us...>) -> _call_result_t<Fn, Us...> {
      return static_cast<Fn &&>(fn)(static_cast<Us &&>(us)...);
    }
  };

  template <std::size_t... Idx, class... Ts>
  struct _lazy_tupl<std::index_sequence<Idx...>, Ts...> : _detail::_lazy_box<Idx, Ts>... {
    template <std::size_t Ny>
    using _at = _m_at_c<Ny, Ts...>;

    USTDEX_HOST_DEVICE USTDEX_INLINE _lazy_tupl() noexcept {
    }

    USTDEX_HOST_DEVICE ~_lazy_tupl() {
      ((_engaged[Idx] ? _get<Idx, Ts>()->~Ts() : void(0)), ...);
    }

    template <std::size_t Ny, class Ty>
    USTDEX_HOST_DEVICE USTDEX_INLINE Ty *_get() noexcept {
      return reinterpret_cast<Ty *>(this->_detail::_lazy_box<Ny, Ty>::_data);
    }

    template <std::size_t Ny, class... Us>
    USTDEX_HOST_DEVICE USTDEX_INLINE void _emplace(Us &&...us) //
      noexcept(_nothrow_constructible<_at<Ny>, Us...>) {
      using Ty = _at<Ny>;
      ::new (static_cast<void *>(_get<Ny, Ty>())) Ty{static_cast<Us &&>(us)...};
      _engaged[Ny] = true;
    }

    template <class Fn, class Self, class... Us>
    USTDEX_INLINE USTDEX_HOST_DEVICE static auto apply(Fn &&fn, Self &&self, Us &&...us) //
      noexcept(_nothrow_callable<Fn, Us..., _copy_cvref_t<Self, Ts>...>)
        -> _call_result_t<Fn, Us..., _copy_cvref_t<Self, Ts>...> {
      return static_cast<Fn &&>(fn)(
        static_cast<Us &&>(us)...,
        static_cast<_copy_cvref_t<Self, Ts> &&>(*self.template _get<Idx, Ts>())...);
    }

    bool _engaged[sizeof...(Ts)] = {};
  };

  template <class... Ts>
  using _lazy_tuple = _lazy_tupl<std::make_index_sequence<sizeof...(Ts)>, Ts...>;

  template <class... Ts>
  using _decayed_lazy_tuple = _lazy_tuple<_decay_t<Ts>...>;

} // namespace ustdex
