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
  struct _env_t
  {
    template <class Query>
    USTDEX_TRIVIAL_API static constexpr decltype(auto) _get_1st(const _env_t& self) noexcept
    {
      if constexpr (_queryable_with<Env, Query>)
      {
        return (self._rcvr_->_env_);
      }
      else if constexpr (_queryable_with<env_of_t<Rcvr>, Query>)
      {
        return ustdex::get_env(static_cast<const Rcvr&>(*self._rcvr_));
      }
    }

    template <class Query>
    using _1st_env_t = decltype(_env_t::_get_1st<Query>(declval<const _env_t&>()));

    template <class Query>
    USTDEX_TRIVIAL_API constexpr auto query(Query) const noexcept(_nothrow_queryable_with<_1st_env_t<Query>, Query>) //
      -> _query_result_t<_1st_env_t<Query>, Query>
    {
      return _env_t::_get_1st<Query>(*this);
    }

    _rcvr_with_env_t const* _rcvr_;
  };

  USTDEX_TRIVIAL_API auto base() && noexcept -> Rcvr&&
  {
    return static_cast<Rcvr&&>(*this);
  }

  USTDEX_TRIVIAL_API auto base() & noexcept -> Rcvr&
  {
    return *this;
  }

  USTDEX_TRIVIAL_API auto base() const& noexcept -> Rcvr const&
  {
    return *this;
  }

  USTDEX_TRIVIAL_API auto get_env() const noexcept -> _env_t
  {
    return _env_t{this};
  }

  Env _env_;
};

template <class Rcvr, class Env>
_rcvr_with_env_t(Rcvr, Env) -> _rcvr_with_env_t<Rcvr, Env>;

} // namespace ustdex

#include "epilogue.hpp"

#endif
