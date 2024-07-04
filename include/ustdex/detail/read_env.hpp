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

#include "config.hpp"
#include "cpos.hpp"
#include "env.hpp"
#include "exception.hpp"
#include "queries.hpp"
#include "utility.hpp"

// This must be the last #include
#include "prologue.hpp"

namespace USTDEX_NAMESPACE
{
struct THE_CURRENT_ENVIRONMENT_LACKS_THIS_QUERY;

struct read_env_t
{
#ifndef __CUDACC__

private:
#endif
  template <class Query, class Env>
  using _error_env_lacks_query = //
    ERROR<WHERE(IN_ALGORITHM, read_env_t),
          WHAT(THE_CURRENT_ENVIRONMENT_LACKS_THIS_QUERY),
          WITH_QUERY(Query),
          WITH_ENVIRONMENT(Env)>;

  struct _completions_fn
  {
    template <class Query, class Env>
    using _f = _mif<_nothrow_callable<Query, Env>,
                    completion_signatures<set_value_t(_call_result_t<Query, Env>)>,
                    completion_signatures<set_value_t(_call_result_t<Query, Env>), set_error_t(::std::exception_ptr)>>;
  };

  template <class Rcvr, class Query>
  struct opstate_t
  {
    using operation_state_concept = operation_state_t;
    using completion_signatures   = //
      _minvoke<_mif<_callable<Query, env_of_t<Rcvr>>, _completions_fn, _error_env_lacks_query<Query, env_of_t<Rcvr>>>,
               Query,
               env_of_t<Rcvr>>;

    Rcvr _rcvr;

    explicit opstate_t(Rcvr rcvr)
        : _rcvr(static_cast<Rcvr&&>(rcvr))
    {}

    USTDEX_IMMOVABLE(opstate_t);

    USTDEX_HOST_DEVICE void start() noexcept
    {
      // If the query invocation is noexcept, call it directly. Otherwise,
      // wrap it in a try-catch block and forward the exception to the
      // receiver.
      if constexpr (_nothrow_callable<Query, env_of_t<Rcvr>>)
      {
        // This looks like a use after move, but `set_value` takes its
        // arguments by forwarding reference, so it's safe.
        ustdex::set_value(static_cast<Rcvr&&>(_rcvr), Query()(ustdex::get_env(_rcvr)));
      }
      else
      {
        USTDEX_TRY( //
          ( //
            { ustdex::set_value(static_cast<Rcvr&&>(_rcvr), Query()(ustdex::get_env(_rcvr))); }),
          USTDEX_CATCH(...)( //
            { //
              ustdex::set_error(static_cast<Rcvr&&>(_rcvr), ::std::current_exception());
            }))
      }
    }
  };

  template <class Query>
  struct _sndr_t;

public:
  /// @brief Returns a sender that, when connected to a receiver and started,
  /// invokes the query with the receiver's environment and forwards the result
  /// to the receiver's `set_value` member.
  template <class Query>
  USTDEX_HOST_DEVICE USTDEX_INLINE constexpr _sndr_t<Query> operator()(Query) const noexcept;
};

template <class Query>
struct read_env_t::_sndr_t
{
  using sender_concept = sender_t;
  USTDEX_NO_UNIQUE_ADDRESS read_env_t _tag;
  USTDEX_NO_UNIQUE_ADDRESS Query _query;

  template <class Rcvr>
  USTDEX_HOST_DEVICE auto connect(Rcvr rcvr) const noexcept(_nothrow_movable<Rcvr>) -> opstate_t<Rcvr, Query>
  {
    return opstate_t<Rcvr, Query>{static_cast<Rcvr&&>(rcvr)};
  }
};

template <class Query>
USTDEX_HOST_DEVICE USTDEX_INLINE constexpr read_env_t::_sndr_t<Query> read_env_t::operator()(Query query) const noexcept
{
  return _sndr_t<Query>{{}, query};
}

USTDEX_DEVICE_CONSTANT constexpr read_env_t read_env{};

} // namespace USTDEX_NAMESPACE

#include "epilogue.hpp"
