/*
 * SPDX-FileCopyrightText: Copyright (c) 2025 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
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

#ifndef USTDEX_ASYNC_DETAIL_ENV
#define USTDEX_ASYNC_DETAIL_ENV

#include "meta.hpp"
#include "queries.hpp"
#include "tuple.hpp"
#include "utility.hpp"

#include <functional>

#include "prologue.hpp"

USTDEX_PRAGMA_PUSH()

// Suppress the warning: "definition of implicit copy constructor for 'env<>' is
// deprecated because it has a user-declared copy assignment operator". We need to
// suppress this warning rather than fix the code because defaulting or defining
// the copy constructor would prevent aggregate initialization, which these types
// depend on.
USTDEX_PRAGMA_IGNORE_GNU("-Wunknown-warning-option")
USTDEX_PRAGMA_IGNORE_GNU("-Wdeprecated-copy")

// warning #20012-D: __device__ annotation is ignored on a
// function("inplace_stop_source") that is explicitly defaulted on its first
// declaration
USTDEX_PRAGMA_IGNORE_EDG(20012)

namespace ustdex
{
template <class Ty>
extern Ty _unwrap_ref;

template <class Ty>
extern Ty& _unwrap_ref<::std::reference_wrapper<Ty>>;

template <class Ty>
extern Ty& _unwrap_ref<std::reference_wrapper<Ty>>;

template <class Ty>
using _unwrap_reference_t = decltype(_unwrap_ref<Ty>);

template <class Query, class Value>
struct USTDEX_TYPE_VISIBILITY_DEFAULT prop
{
  USTDEX_NO_UNIQUE_ADDRESS Query _query;
  USTDEX_NO_UNIQUE_ADDRESS Value _value;

  USTDEX_TRIVIAL_API constexpr auto query(Query) const noexcept -> const Value&
  {
    return _value;
  }

  prop& operator=(const prop&) = delete;
};

template <class Query, class Value>
prop(Query, Value) -> prop<Query, Value>;

template <class... Envs>
struct USTDEX_TYPE_VISIBILITY_DEFAULT env
{
  _tuple<Envs...> _envs_;

  template <class Query>
  USTDEX_TRIVIAL_API static constexpr decltype(auto) _get_1st(const env& _self) noexcept
  {
    constexpr bool _flags[]    = {_queryable_with<Envs, Query>..., false};
    constexpr std::size_t _idx = ustdex::_find_pos(_flags, _flags + sizeof...(Envs));
    if constexpr (_idx != _npos)
    {
      return ustdex::_cget<_idx>(_self._envs_);
    }
  }

  template <class Query>
  using _1st_env_t = decltype(env::_get_1st<Query>(declval<const env&>()));

  template <class Query>
  USTDEX_TRIVIAL_API constexpr auto query(Query _query) const
    noexcept(_nothrow_queryable_with<_1st_env_t<Query>, Query>) //
    -> _query_result_t<_1st_env_t<Query>, Query>
  {
    return env::_get_1st<Query>(*this).query(_query);
  }

  env& operator=(const env&) = delete;
};

// partial specialization for two environments
template <class Env0, class Env1>
struct USTDEX_TYPE_VISIBILITY_DEFAULT env<Env0, Env1>
{
  USTDEX_NO_UNIQUE_ADDRESS Env0 _env0_;
  USTDEX_NO_UNIQUE_ADDRESS Env1 _env1_;

  template <class Query>
  USTDEX_TRIVIAL_API static constexpr decltype(auto) _get_1st(const env& _self) noexcept
  {
    if constexpr (_queryable_with<Env0, Query>)
    {
      return (_self._env0_);
    }
    else
    {
      return (_self._env1_);
    }
  }

  template <class Query>
  using _1st_env_t = decltype(env::_get_1st<Query>(declval<const env&>()));

  template <class Query>
  USTDEX_TRIVIAL_API constexpr auto query(Query _query) const
    noexcept(_nothrow_queryable_with<_1st_env_t<Query>, Query>) //
    -> _query_result_t<_1st_env_t<Query>, Query>
  {
    return env::_get_1st<Query>(*this).query(_query);
  }

  env& operator=(const env&) = delete;
};

template <class... Envs>
env(Envs...) -> env<_unwrap_reference_t<Envs>...>;

using empty_env [[deprecated("use ustdex::env<> instead")]] = env<>;

struct get_env_t
{
  template <class Ty>
  USTDEX_TRIVIAL_API auto operator()(Ty&& _ty) const noexcept -> decltype(_ty.get_env())
  {
    static_assert(noexcept(_ty.get_env()));
    return _ty.get_env();
  }

  USTDEX_TRIVIAL_API env<> operator()(_ignore) const noexcept
  {
    return {};
  }
};

namespace _region
{
inline constexpr get_env_t get_env{};
} // namespace _region

using namespace _region;

template <class Ty>
using env_of_t = decltype(ustdex::get_env(declval<Ty>()));
} // namespace ustdex

USTDEX_PRAGMA_POP()

#include "epilogue.hpp"

#endif
