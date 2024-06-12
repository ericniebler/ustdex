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
#include "tuple.hpp"

#include <functional>

// warning #20012-D: __device__ annotation is ignored on a
// function("inplace_stop_source") that is explicitly defaulted on its first
// declaration
USTDEX_PRAGMA_PUSH()
USTDEX_PRAGMA_IGNORE_EDG(20012)

namespace ustdex {
  template <class Ty>
  extern Ty _unwrap_ref;

  template <class Ty>
  extern Ty &_unwrap_ref<std::reference_wrapper<Ty>>;

  template <class Ty>
  using _unwrap_reference_t = decltype(_unwrap_ref<Ty>);

  template <class Query, class Value>
  struct prop {
    USTDEX_NO_UNIQUE_ADDRESS Query _query;
    USTDEX_NO_UNIQUE_ADDRESS Value _value;

    USTDEX_INLINE USTDEX_HOST_DEVICE constexpr auto
      query() const noexcept -> const Value & {
      return _value;
    }
  };

  template <class... Envs>
  struct env {
    _tuple<Envs...> _envs;

    template <class Query>
    USTDEX_INLINE USTDEX_HOST_DEVICE constexpr decltype(auto)
      _get_1st(Query) const noexcept {
      constexpr bool _flags[] = {_queryable<Envs, Query>..., true};
      constexpr std::size_t _idx = ustdex::_find_pos(_flags, _flags + sizeof...(Envs));
      if constexpr (_idx != _npos) {
        return ustdex::_cget<_idx>(_envs);
      }
    }

    template <class Query, class Env = env>
    using _1st_env_t = decltype(DECLVAL(const Env &)._get_1st(Query{}));

    template <class Query>
    USTDEX_INLINE USTDEX_HOST_DEVICE constexpr auto
      query(Query query) const noexcept(_nothrow_queryable<_1st_env_t<Query>, Query>) //
      -> _query_result_t<_1st_env_t<Query>, Query> {
      return _get_1st(query).query(query);
    }
  };

  // partial specialization for two environments
  template <class Env0, class Env1>
  struct env<Env0, Env1> {
    USTDEX_NO_UNIQUE_ADDRESS Env0 _env0;
    USTDEX_NO_UNIQUE_ADDRESS Env1 _env1;

    template <class Query>
    USTDEX_INLINE USTDEX_HOST_DEVICE constexpr decltype(auto)
      _get_1st(Query) const noexcept {
      if constexpr (_queryable<Env0, Query>) {
        return (_env0);
      } else if constexpr (_queryable<Env1, Query>) {
        return (_env1);
      }
    }

    template <class Query, class Env = env>
    using _1st_env_t = decltype(DECLVAL(const Env &)._get_1st(Query{}));

    template <class Query>
    USTDEX_INLINE USTDEX_HOST_DEVICE constexpr auto
      query(Query query) const noexcept(_nothrow_queryable<_1st_env_t<Query>, Query>) //
      -> _query_result_t<_1st_env_t<Query>, Query> {
      return _get_1st(query).query(query);
    }
  };

  template <class... Envs>
  USTDEX_HOST_DEVICE env(Envs...) -> env<_unwrap_reference_t<Envs>...>;

  using empty_env = env<>;

  namespace _adl {
    template <class Ty>
    USTDEX_INLINE USTDEX_HOST_DEVICE auto get_env(Ty *ty) noexcept //
      -> decltype(ty->get_env()) {
      static_assert(noexcept(ty->get_env()));
      return ty->get_env();
    }

    struct _get_env_t {
      template <class Ty>
      USTDEX_INLINE USTDEX_HOST_DEVICE auto operator()(Ty *ty) const noexcept //
        -> decltype(get_env(ty)) {
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

    USTDEX_INLINE USTDEX_HOST_DEVICE empty_env operator()(_ignore) const noexcept {
      return {};
    }
  };

  namespace _region {
    USTDEX_DEVICE_CONSTANT constexpr get_env_t get_env{};
  } // namespace _region

  using namespace _region;

  template <class Ty>
  using env_of_t = decltype(get_env(DECLVAL(Ty)));
} // namespace ustdex

USTDEX_PRAGMA_POP()
