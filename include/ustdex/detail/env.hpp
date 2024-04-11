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

namespace ustdex {
  struct empty_env { };

  USTDEX_DEVICE constexpr struct get_env_t {
    template <class Ty>
    USTDEX_INLINE USTDEX_HOST_DEVICE
    auto
      operator()(const Ty &ty) const noexcept -> decltype(ty.get_env()) {
      static_assert(noexcept(ty.get_env()));
      return ty.get_env();
    }

    template <class Ty>
    USTDEX_INLINE USTDEX_HOST_DEVICE
    auto
      operator()(const Ty *ty) const noexcept -> decltype(ty->get_env()) {
      static_assert(noexcept(ty->get_env()));
      return ty->get_env();
    }

    USTDEX_INLINE USTDEX_HOST_DEVICE
    empty_env
      operator()(_ignore) const noexcept {
      return {};
    }
  } get_env{};

  template <class Ty, class Query>
  using _query_result_t = decltype(DECLVAL(Ty).query(Query()));

  template <class Ty, class Query>
  inline constexpr bool _queryable = _mvalid_q<_query_result_t, Ty, Query>;

  template <class Ty, class Query>
  using _nothrow_queryable_ = _mif<noexcept(DECLVAL(Ty).query(Query()))>;

  template <class Ty, class Query>
  inline constexpr bool _nothrow_queryable = _mvalid_q<_nothrow_queryable_, Ty, Query>;

  template <class Ty>
  using env_of_t = decltype(get_env(DECLVAL(Ty)));
} // namespace ustdex
