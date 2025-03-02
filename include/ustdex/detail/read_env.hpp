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

#ifndef USTDEX_ASYNC_DETAIL_READ_ENV
#define USTDEX_ASYNC_DETAIL_READ_ENV

#include "completion_signatures.hpp"
#include "config.hpp"
#include "cpos.hpp"
#include "env.hpp"
#include "exception.hpp"
#include "queries.hpp"
#include "utility.hpp"

#include "prologue.hpp"

namespace ustdex
{
struct THE_CURRENT_ENVIRONMENT_LACKS_THIS_QUERY;
struct THE_CURRENT_ENVIRONMENT_RETURNED_VOID_FOR_THIS_QUERY;

struct read_env_t
{
private:
  template <class Rcvr, class Query>
  struct USTDEX_TYPE_VISIBILITY_DEFAULT _opstate_t
  {
    using operation_state_concept = operation_state_t;

    Rcvr _rcvr_;

    USTDEX_API explicit _opstate_t(Rcvr _rcvr)
        : _rcvr_(static_cast<Rcvr&&>(_rcvr))
    {}

    USTDEX_IMMOVABLE(_opstate_t);

    USTDEX_API void start() noexcept
    {
      // If the query invocation is noexcept, call it directly. Otherwise,
      // wrap it in a try-catch block and forward the exception to the
      // receiver.
      if constexpr (_nothrow_callable<Query, env_of_t<Rcvr>>)
      {
        // This looks like a use after move, but `set_value` takes its
        // arguments by forwarding reference, so it's safe.
        ustdex::set_value(static_cast<Rcvr&&>(_rcvr_), Query()(ustdex::get_env(_rcvr_)));
      }
      else
      {
        USTDEX_TRY( //
          ({        //
            ustdex::set_value(static_cast<Rcvr&&>(_rcvr_), Query()(ustdex::get_env(_rcvr_)));
          }),
          USTDEX_CATCH(...) //
          ({                //
            ustdex::set_error(static_cast<Rcvr&&>(_rcvr_), ::std::current_exception());
          })                //
        )
      }
    }
  };

  template <class Query>
  struct USTDEX_TYPE_VISIBILITY_DEFAULT _sndr_t;

public:
  /// @brief Returns a sender that, when connected to a receiver and started,
  /// invokes the query with the receiver's environment and forwards the result
  /// to the receiver's `set_value` member.
  template <class Query>
  USTDEX_TRIVIAL_API constexpr _sndr_t<Query> operator()(Query) const noexcept;
};

template <class Query>
struct USTDEX_TYPE_VISIBILITY_DEFAULT read_env_t::_sndr_t
{
  using sender_concept = sender_t;
  USTDEX_NO_UNIQUE_ADDRESS read_env_t _tag;
  USTDEX_NO_UNIQUE_ADDRESS Query _query;

  template <class Self, class Env>
  USTDEX_API static constexpr auto get_completion_signatures()
  {
    if constexpr (!_callable<Query, Env>)
    {
      return invalid_completion_signature<WHERE(IN_ALGORITHM, read_env_t),
                                          WHAT(THE_CURRENT_ENVIRONMENT_LACKS_THIS_QUERY),
                                          WITH_QUERY(Query),
                                          WITH_ENVIRONMENT(Env)>();
    }
    else if constexpr (std::is_void_v<_call_result_t<Query, Env>>)
    {
      return invalid_completion_signature<WHERE(IN_ALGORITHM, read_env_t),
                                          WHAT(THE_CURRENT_ENVIRONMENT_RETURNED_VOID_FOR_THIS_QUERY),
                                          WITH_QUERY(Query),
                                          WITH_ENVIRONMENT(Env)>();
    }
    else if constexpr (_nothrow_callable<Query, Env>)
    {
      return completion_signatures<set_value_t(_call_result_t<Query, Env>)>{};
    }
    else
    {
      return completion_signatures<set_value_t(_call_result_t<Query, Env>), set_error_t(::std::exception_ptr)>{};
    }
  }

  template <class Rcvr>
  USTDEX_API auto connect(Rcvr _rcvr) const noexcept(_nothrow_movable<Rcvr>) -> _opstate_t<Rcvr, Query>
  {
    return _opstate_t<Rcvr, Query>{static_cast<Rcvr&&>(_rcvr)};
  }
};

template <class Query>
USTDEX_TRIVIAL_API constexpr read_env_t::_sndr_t<Query> read_env_t::operator()(Query _query) const noexcept
{
  return _sndr_t<Query>{{}, _query};
}

inline constexpr read_env_t read_env{};

} // namespace ustdex

#include "epilogue.hpp"

#endif
