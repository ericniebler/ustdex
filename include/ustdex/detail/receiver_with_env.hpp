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

#include "cpos.hpp"
#include "env.hpp"

// Must be the last include
#include "prologue.hpp"

namespace USTDEX_NAMESPACE
{
template <class Rcvr, class Env>
struct _receiver_with_env_t : Rcvr
{
  using _env_t = _receiver_with_env_t const&;

  USTDEX_INLINE USTDEX_HOST_DEVICE auto rcvr() noexcept -> Rcvr&
  {
    return *this;
  }

  USTDEX_INLINE USTDEX_HOST_DEVICE auto rcvr() const noexcept -> const Rcvr&
  {
    return *this;
  }

  USTDEX_INLINE USTDEX_HOST_DEVICE auto get_env() const noexcept -> _env_t
  {
    return _env_t{*this};
  }

  template <class Query>
  USTDEX_INLINE USTDEX_HOST_DEVICE constexpr decltype(auto) _get_1st(Query) const noexcept
  {
    if constexpr (_queryable<Env, Query>)
    {
      return (_env);
    }
    else if constexpr (_queryable<env_of_t<Rcvr>, Query>)
    {
      return ustdex::get_env(static_cast<const Rcvr&>(*this));
    }
  }

  template <class Query, class Self = _receiver_with_env_t>
  using _1st_env_t = decltype(DECLVAL(const Self&)._get_1st(Query{}));

  template <class Query>
  USTDEX_INLINE USTDEX_HOST_DEVICE constexpr auto query(Query query) const
    noexcept(_nothrow_queryable<_1st_env_t<Query>, Query>) //
    -> _query_result_t<_1st_env_t<Query>, Query>
  {
    return _get_1st(query).query(query);
  }

  Env _env;
};

template <class Rcvr, class Env>
struct _receiver_with_env_t<Rcvr*, Env>
{
  using _env_t = _receiver_with_env_t const&;

  USTDEX_INLINE USTDEX_HOST_DEVICE auto rcvr() const noexcept -> Rcvr*
  {
    return _rcvr;
  }

  template <class... As>
  USTDEX_INLINE USTDEX_HOST_DEVICE void set_value(As&&... as) && noexcept
  {
    ustdex::set_value(_rcvr, static_cast<As&&>(as)...);
  }

  template <class Error>
  USTDEX_INLINE USTDEX_HOST_DEVICE void set_error(Error&& error) && noexcept
  {
    ustdex::set_error(_rcvr, static_cast<Error&&>(error));
  }

  USTDEX_INLINE USTDEX_HOST_DEVICE void set_stopped() && noexcept
  {
    ustdex::set_stopped(_rcvr);
  }

  USTDEX_INLINE USTDEX_HOST_DEVICE auto get_env() const noexcept -> _env_t
  {
    return _env_t{*this};
  }

  template <class Query>
  USTDEX_INLINE USTDEX_HOST_DEVICE constexpr decltype(auto) _get_1st(Query) const noexcept
  {
    if constexpr (_queryable<Env, Query>)
    {
      return (_env);
    }
    else if constexpr (_queryable<env_of_t<Rcvr>, Query>)
    {
      return ustdex::get_env(_rcvr);
    }
  }

  template <class Query, class Self = _receiver_with_env_t>
  using _1st_env_t = decltype(DECLVAL(const Self&)._get_1st(Query{}));

  template <class Query>
  USTDEX_INLINE USTDEX_HOST_DEVICE constexpr auto query(Query query) const
    noexcept(_nothrow_queryable<_1st_env_t<Query>, Query>) //
    -> _query_result_t<_1st_env_t<Query>, Query>
  {
    return _get_1st(query).query(query);
  }

  Rcvr* _rcvr;
  Env _env;
};
} // namespace USTDEX_NAMESPACE

#include "epilogue.hpp"
