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
#include "queries.hpp"
#include "type_traits.hpp"
#include "utility.hpp"

namespace ustdex {
  struct empty_env { };

  USTDEX_DEVICE constexpr struct get_env_t {
    template <class Ty>
    USTDEX_INLINE USTDEX_HOST_DEVICE auto operator()(Ty &&ty) const noexcept //
      -> decltype(ty.get_env()) {
      static_assert(noexcept(ty.get_env()));
      return ty.get_env();
    }

    template <class Self = get_env_t, template <class...> class Op, class Rcvr, class... Ts>
    USTDEX_INLINE USTDEX_HOST_DEVICE auto operator()(Op<Rcvr, Ts...> *ty) const noexcept //
      -> _call_result_t<Self, Rcvr> {
      static_assert(noexcept(ty->get_env()));
      return ty->get_env();
    }

    USTDEX_INLINE USTDEX_HOST_DEVICE empty_env operator()(_ignore_t) const noexcept {
      return {};
    }
  } get_env{};

  template <class Ty, class Query>
  inline constexpr bool _queryable = _mvalid_q<_query_result_t, Query, Ty>;

  template <class Ty, class Query>
  using _nothrow_queryable_ = _mif<noexcept(DECLVAL(Ty).query(Query()))>;

  template <class Ty, class Query>
  inline constexpr bool _nothrow_queryable = _mvalid_q<_nothrow_queryable_, Ty, Query>;

  template <class Ty>
  using env_of_t = decltype(get_env(DECLVAL(Ty)));

  template <class Env1, class Env2>
  struct _env_pair_t : _immovable {
    Env1 _env;
    Env2 (*const _get_env2)(const _env_pair_t *) noexcept;

    template <class Query>
    USTDEX_HOST_DEVICE auto query(Query) const          //
      noexcept(_nothrow_queryable<const Env1 &, Query>) //
      -> _query_result_t<Query, const Env1 &> {
      return _env.query(Query{});
    }

    // The volatile here is to create an ordering between the two `query`
    // overloads. The non-volatile overload will be prefered when both
    // environments have the query.
    template <class Query>
    USTDEX_HOST_DEVICE auto query(Query) const volatile //
      noexcept(_nothrow_queryable<Env2, Query>)         //
      -> _query_result_t<Query, Env2> {
      return _get_env2(this).query(Query{});
    }
  };

  template <class Env, class Rcvr>
  struct _env_rcvr_t : _env_pair_t<Env, env_of_t<Rcvr>> {
    using _env_pair = _env_pair_t<Env, env_of_t<Rcvr>>;
    Rcvr _rcvr;

    static env_of_t<Rcvr> _get_env2(const _env_pair *self) noexcept {
      return ustdex::get_env(static_cast<const _env_rcvr_t *>(self)->_rcvr);
    }

    USTDEX_HOST_DEVICE _env_rcvr_t(Env env, Rcvr rcvr)
      : _env_pair{{}, static_cast<Env &&>(env), &_get_env2}
      , _rcvr(static_cast<Rcvr &&>(rcvr)) {
    }
  };

} // namespace ustdex
