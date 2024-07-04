//===----------------------------------------------------------------------===//
//
// Part of CUDA Experimental in CUDA C++ Core Libraries,
// under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// SPDX-FileCopyrightText: Copyright (c) 2024 NVIDIA CORPORATION & AFFILIATES.
//
//===----------------------------------------------------------------------===//
#pragma once

#include "env.hpp"
#include "tuple.hpp"
#include "type_traits.hpp"
#include "utility.hpp"

namespace USTDEX_NAMESPACE
{
struct _sexpr_defaults
{
  static inline constexpr auto _get_attrs = //
    [](_ignore, const auto&... _child) noexcept -> decltype(auto) {
    if constexpr (sizeof...(_child) == 1)
    {
      return ustdex::get_env(_child...); // BUGBUG: should be only the forwarding queries
    }
    else
    {
      return env();
    }
  };
};

template <class Tag>
struct _sexpr_traits : _sexpr_defaults
{};

template <class Tag, class DescFn>
struct _sexpr : _call_result_t<DescFn>
{};

template <class... Ts>
inline constexpr auto _desc_v = [] {
  return _tuple<Ts...>{};
};

template <class Tag, class Data, class... Children>
_sexpr(Tag, Data, Children...) -> _sexpr<Tag, decltype(_desc_v<Tag, Data, Children...>)>;

} // namespace USTDEX_NAMESPACE
