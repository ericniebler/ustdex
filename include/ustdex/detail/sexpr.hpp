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

#include "env.hpp"
#include "tuple.hpp"
#include "type_traits.hpp"
#include "utility.hpp"

namespace ustdex {
  struct _sexpr_defaults {
    static inline constexpr auto _get_attrs = //
      [](_ignore, const auto &..._child) noexcept -> decltype(auto) {
      if constexpr (sizeof...(_child) == 1) {
        return ustdex::get_env(_child...); // BUGBUG: should be only the forwarding queries
      } else {
        return empty_env();
      }
    };
  };

  template <class Tag>
  struct _sexpr_traits : _sexpr_defaults { };

  template <class Tag, class DescFn>
  struct _sexpr : _call_result_t<DescFn> { };

  template <class... Ts>
  inline constexpr auto _desc_v = [] {
    return _tuple_for<Ts...>{};
  };

  template <class Tag, class Data, class... Children>
  _sexpr(Tag, Data, Children...)
    -> _sexpr<Tag, decltype(_desc_v<Tag, Data, Children...>)>;

} // namespace ustdex
