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

// warning #20012-D: __device__ annotation is ignored on a
// function("inplace_stop_source") that is explicitly defaulted on its first
// declaration
USTDEX_PRAGMA_PUSH()
USTDEX_PRAGMA_IGNORE_EDG(20012)

namespace ustdex {
  struct empty_env { };

  namespace _adl {
    template <class Ty>
    auto get_env(Ty *ty) noexcept -> decltype(ty->get_env()) {
      static_assert(noexcept(ty->get_env()));
      return ty->get_env();
    }

    struct _get_env_t {
      template <class Ty>
      auto operator()(Ty *ty) const noexcept -> decltype(get_env(ty)) {
        static_assert(noexcept(get_env(ty)));
        return get_env(ty);
      }
    };
  } // namespace _adl

  struct get_env_t {
    template <class Ty>
    USTDEX_INLINE USTDEX_HOST_DEVICE auto operator()(Ty &&ty) const noexcept //
      -> decltype(ty.get_env()) {
      static_assert(noexcept(ty.get_env()));
      return ty.get_env();
    }

    template <class Ty>
    USTDEX_INLINE USTDEX_HOST_DEVICE auto operator()(Ty *ty) const noexcept //
      -> _call_result_t<_adl::_get_env_t, Ty *> {
      return _adl::_get_env_t()(ty);
    }

    USTDEX_INLINE USTDEX_HOST_DEVICE empty_env operator()(_ignore_t) const noexcept {
      return {};
    }
  };

  namespace _region {
    USTDEX_DEVICE constexpr get_env_t get_env{};
  }
  using namespace _region;

  template <class Ty, class Query>
  inline constexpr bool _queryable = _mvalid_q<_query_result_t, Query, Ty>;

#if defined(__CUDA_ARCH__)
  template <class Ty, class Query>
  inline constexpr bool _nothrow_queryable = true;
#else
  template <class Ty, class Query>
  using _nothrow_queryable_ = _mif<noexcept(DECLVAL(Ty).query(Query()))>;

  template <class Ty, class Query>
  inline constexpr bool _nothrow_queryable = _mvalid_q<_nothrow_queryable_, Ty, Query>;
#endif

  template <class Ty>
  using env_of_t = decltype(get_env(DECLVAL(Ty)));

  template <class Env1, class Env2>
  struct _joined_env_t {
    Env1 _env1;
    Env2 _env2;

    template <class Query>
    USTDEX_HOST_DEVICE auto query(Query) const          //
      noexcept(_nothrow_queryable<const Env1 &, Query>) //
      -> _query_result_t<Query, const Env1 &> {
      return _env1.query(Query{});
    }

    // The volatile here is to create an ordering between the two `query`
    // overloads. The non-volatile overload will be prefered when both
    // environments have the query.
    template <class Query>
    USTDEX_HOST_DEVICE auto query(Query) const volatile //
      noexcept(_nothrow_queryable<const Env2&, Query>)  //
      -> _query_result_t<Query, const Env2 &> {
      return const_cast<const Env2 &>(_env2).query(Query{});
    }
  };

  template <class Env1, class Env2>
  USTDEX_HOST_DEVICE _joined_env_t(Env1&&, Env2&&) -> _joined_env_t<Env1, Env2>;

  template <class Rcvr, class Env>
  struct _receiver_with_env_t : Rcvr {
    using _env_t = _joined_env_t<const Env &, env_of_t<Rcvr>>;

    USTDEX_HOST_DEVICE auto rcvr() noexcept -> Rcvr & {
      return *this;
    }

    USTDEX_HOST_DEVICE auto rcvr() const noexcept -> const Rcvr & {
      return *this;
    }

    USTDEX_HOST_DEVICE auto get_env() const noexcept -> _env_t {
      return _joined_env_t{_env, ustdex::get_env(static_cast<const Rcvr &>(*this))};
    }

    Env _env;
  };
} // namespace ustdex

USTDEX_PRAGMA_POP()
