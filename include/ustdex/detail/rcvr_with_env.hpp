/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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

#ifndef USTDEX_ASYNC_DETAIL_RCVR_WITH_ENV
#define USTDEX_ASYNC_DETAIL_RCVR_WITH_ENV

#include "cpos.hpp"
#include "env.hpp"

#include "prologue.hpp"

namespace ustdex
{
template <class Rcvr, class Env>
struct _rcvr_with_env_t : Rcvr
{
  using _env_t = _rcvr_with_env_t const&;

  USTDEX_TRIVIAL_API auto _rcvr() noexcept -> Rcvr&
  {
    return *this;
  }

  USTDEX_TRIVIAL_API auto _rcvr() const noexcept -> const Rcvr&
  {
    return *this;
  }

  USTDEX_TRIVIAL_API auto get_env() const noexcept -> _env_t
  {
    return _env_t{*this};
  }

  template <class Query>
  USTDEX_TRIVIAL_API constexpr decltype(auto) _get_1st(Query) const noexcept
  {
    if constexpr (_queryable_with<Env, Query>)
    {
      return (_env_);
    }
    else if constexpr (_queryable_with<env_of_t<Rcvr>, Query>)
    {
      return ustdex::get_env(static_cast<const Rcvr&>(*this));
    }
  }

  template <class Query, class Self = _rcvr_with_env_t>
  using _1st_env_t = decltype(declval<const Self&>()._get_1st(Query{}));

  template <class Query>
  USTDEX_TRIVIAL_API constexpr auto query(Query _query) const
    noexcept(_nothrow_queryable_with<_1st_env_t<Query>, Query>) //
    -> _query_result_t<_1st_env_t<Query>, Query>
  {
    return _get_1st(_query).query(_query);
  }

  Env _env_;
};

template <class Rcvr, class Env>
struct _rcvr_with_env_t<Rcvr*, Env>
{
  using _env_t = _rcvr_with_env_t const&;

  USTDEX_TRIVIAL_API auto _rcvr() const noexcept -> Rcvr*
  {
    return _rcvr_;
  }

  template <class... As>
  USTDEX_TRIVIAL_API void set_value(As&&... _as) && noexcept
  {
    ustdex::set_value(_rcvr_, static_cast<As&&>(_as)...);
  }

  template <class Error>
  USTDEX_TRIVIAL_API void set_error(Error&& _error) && noexcept
  {
    ustdex::set_error(_rcvr_, static_cast<Error&&>(_error));
  }

  USTDEX_TRIVIAL_API void set_stopped() && noexcept
  {
    ustdex::set_stopped(_rcvr_);
  }

  USTDEX_TRIVIAL_API auto get_env() const noexcept -> _env_t
  {
    return _env_t{*this};
  }

  template <class Query>
  USTDEX_TRIVIAL_API constexpr decltype(auto) _get_1st(Query) const noexcept
  {
    if constexpr (_queryable_with<Env, Query>)
    {
      return (_env_);
    }
    else if constexpr (_queryable_with<env_of_t<Rcvr>, Query>)
    {
      return ustdex::get_env(_rcvr_);
    }
  }

  template <class Query, class Self = _rcvr_with_env_t>
  using _1st_env_t = decltype(declval<const Self&>()._get_1st(Query{}));

  template <class Query>
  USTDEX_TRIVIAL_API constexpr auto query(Query _query) const
    noexcept(_nothrow_queryable_with<_1st_env_t<Query>, Query>) //
    -> _query_result_t<_1st_env_t<Query>, Query>
  {
    return _get_1st(_query).query(_query);
  }

  Rcvr* _rcvr_;
  Env _env_;
};

template <class Rcvr, class Env>
_rcvr_with_env_t(Rcvr, Env) -> _rcvr_with_env_t<Rcvr, Env>;

} // namespace ustdex

#include "epilogue.hpp"

#endif
