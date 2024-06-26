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

#include <utility>

namespace ustdex {
  template <std::size_t Idx, class Ty>
  struct _box {
    // Too many compiler bugs with [[no_unique_address]] to use it here.
    // E.g., https://github.com/llvm/llvm-project/issues/88077
    // USTDEX_NO_UNIQUE_ADDRESS
    Ty _value;
  };

  template <std::size_t Idx, class Ty>
  USTDEX_INLINE USTDEX_HOST_DEVICE constexpr auto
    _cget(_box<Idx, Ty> const &box) noexcept -> Ty const & {
    return box._value;
  }

  template <class Idx, class... Ts>
  struct _tupl;

  template <std::size_t... Idx, class... Ts>
  struct _tupl<std::index_sequence<Idx...>, Ts...> : _box<Idx, Ts>... {
    template <class Fn, class Self, class... Us>
    USTDEX_INLINE USTDEX_HOST_DEVICE static auto
      apply(Fn &&fn, Self &&self, Us &&...us) //
      noexcept(noexcept(static_cast<Fn &&>(fn)(
        static_cast<Us &&>(us)...,
        static_cast<Self &&>(self)._box<Idx, Ts>::_value...)))
        -> decltype(static_cast<Fn &&>(fn)(
          static_cast<Us &&>(us)...,
          static_cast<Self &&>(self)._box<Idx, Ts>::_value...)) {
      return static_cast<Fn &&>(fn)(
        static_cast<Us &&>(us)..., static_cast<Self &&>(self)._box<Idx, Ts>::_value...);
    }

    template <class Fn, class Self, class... Us>
    USTDEX_INLINE USTDEX_HOST_DEVICE static auto
      for_each(Fn &&fn, Self &&self, Us &&...us) //
      noexcept((_nothrow_callable<Fn, Us..., _copy_cvref_t<Self, Ts>> && ...))
        -> _mif<(_callable<Fn, Us..., _copy_cvref_t<Self, Ts>> && ...)> {
      return (
        static_cast<Fn &&>(fn)(
          static_cast<Us &&>(us)..., static_cast<Self &&>(self)._box<Idx, Ts>::_value),
        ...);
    }
  };

  template <class... Ts>
  USTDEX_HOST_DEVICE _tupl(Ts...) //
    ->_tupl<std::make_index_sequence<sizeof...(Ts)>, Ts...>;

  template <class... Ts>
  using _tuple = _tupl<std::make_index_sequence<sizeof...(Ts)>, Ts...>;

  template <class Fn, class Tupl, class... Us>
  using _apply_result_t =
    decltype(DECLVAL(Tupl).apply(DECLVAL(Fn), DECLVAL(Tupl), DECLVAL(Us)...));

  template <class First, class Second>
  struct _pair {
    First first;
    Second second;
  };

  template <class... Ts>
  using _decayed_tuple = _tuple<_decay_t<Ts>...>;
} // namespace ustdex
