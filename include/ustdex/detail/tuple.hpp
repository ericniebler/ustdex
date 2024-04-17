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
    [[no_unique_address]] Ty _value;
  };

  template <class Idx, class... Ts>
  struct _tupl;

  template <std::size_t... Idx, class... Ts>
  struct _tupl<std::index_sequence<Idx...>, Ts...> : _box<Idx, Ts>... {
    template <class Fn, class... Us>
    USTDEX_INLINE USTDEX_HOST_DEVICE auto apply(Fn &&fn, Us &&...us) & //
      noexcept(_nothrow_callable<Fn, Us..., Ts &...>)
        -> _call_result_t<Fn, Us..., Ts &...> {
      return static_cast<Fn &&>(fn)(
        static_cast<Us &&>(us)..., this->_box<Idx, Ts>::_value...);
    }
  };

  template <class... Ts>
  USTDEX_HOST_DEVICE _tupl(Ts...) //
    ->_tupl<std::make_index_sequence<sizeof...(Ts)>, Ts...>;

  template <class... Ts>
  using _tuple = _tupl<std::make_index_sequence<sizeof...(Ts)>, Ts...>;

  template <class First, class Second>
  struct _pair {
    First first;
    Second second;
  };

  template <class... Ts>
  using _decayed_tuple = _tuple<_decay_t<Ts>...>;
} // namespace ustdex
