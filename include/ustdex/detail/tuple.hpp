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

#include <utility>

#include "config.hpp"
#include "type_traits.hpp"

namespace ustdex {
  template <class Ty, std::size_t Idx>
  struct _box {
    [[no_unique_address]] Ty _value;
  };

  template <class Idx, class... Ts>
  struct _tuple;

  template <std::size_t... Idx, class... Ts>
  struct _tuple<std::index_sequence<Idx...>, Ts...> : _box<Ts, Idx>... {
    template <class Fn, class Tup>
    using _nothrow = _mvalue<_nothrow_callable<Fn, _copy_cvref_t<Tup, Ts>...>>;

    template <class Fn, class Tup>
    USTDEX_INLINE USTDEX_HOST_DEVICE
    constexpr friend auto _apply(Fn &&fn, Tup &&tup) noexcept(_v<_nothrow<Fn, Tup>>)
      //noexcept(DECLVAL(Fn)(DECLVAL(_copy_cvref_t<Tup, Ts>)...)))
      -> decltype(DECLVAL(Fn)(DECLVAL(_copy_cvref_t<Tup, Ts>)...)) {
      return static_cast<Fn &&>(fn)(
        static_cast<_copy_cvref_t<Tup, _box<Ts, Idx>> &&>(tup)._value...);
    }
  };

  template <class... Ts>
  USTDEX_HOST_DEVICE _tuple(Ts...) -> _tuple<std::index_sequence_for<Ts...>, Ts...>;

  template <class... Ts>
  using _tuple_for = _tuple<std::index_sequence_for<Ts...>, Ts...>;

  template <std::size_t Idx, class Ty>
  USTDEX_HOST_DEVICE
  constexpr Ty &&_get(_box<Ty, Idx> &&_self) noexcept {
    return static_cast<Ty &&>(_self._value);
  }

  template <std::size_t Idx, class Ty>
  USTDEX_HOST_DEVICE
  constexpr Ty &_get(_box<Ty, Idx> &_self) noexcept {
    return _self._value;
  }

  template <std::size_t Idx, class Ty>
  USTDEX_HOST_DEVICE
  constexpr const Ty &_get(const _box<Ty, Idx> &_self) noexcept {
    return _self._value;
  }

  template <class First, class Second>
  struct pair {
    First first;
    Second second;
  };

  template <class... Ts>
  using _decayed_tuple = _tuple_for<_decay_t<Ts>...>;
} // namespace ustdex
