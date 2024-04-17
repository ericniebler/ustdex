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
#include "meta.hpp"
#include "type_traits.hpp"

#include <initializer_list>

namespace ustdex {
  struct _ignore_t {
    template <class... As>
    constexpr _ignore_t(As &&...) noexcept {};
  };

  template <class...>
  struct _undefined;

  struct _empty { };

  struct [[deprecated]] _deprecated { };

  struct _nil { };

  struct _immovable {
    _immovable() = default;
    _immovable(const _immovable &) = delete;
    _immovable &operator=(const _immovable &) = delete;
  };

  inline constexpr std::size_t _max(std::initializer_list<std::size_t> il) noexcept {
    std::size_t max = 0;
    for (auto i: il) {
      if (i > max) {
        max = i;
      }
    }
    return max;
  }

  template <class Ty, class... Ts>
  inline constexpr std::size_t _index_of() noexcept {
    constexpr bool _same[] = {USTDEX_IS_SAME(Ty, Ts)...};
    for (std::size_t i = 0; i < sizeof...(Ts); ++i) {
      if (_same[i]) {
        return i;
      }
    }
    return static_cast<std::size_t>(-1ul);
  }
} // namespace ustdex
