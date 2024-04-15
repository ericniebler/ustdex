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

#include "config.hpp"
#include "cpos.hpp"
#include "env.hpp"
#include "queries.hpp"
#include "utility.hpp"

#include <exception>

namespace ustdex {
  struct THE_CURRENT_ENVIRONMENT_LACKS_THIS_QUERY;

  struct read_env_t {
#ifndef __CUDACC__
   private:
#endif
    template <class Query, class Env>
    using _error_env_lacks_query = //
      ERROR<
        WHERE(IN_ALGORITHM, read_env_t),
        WHAT(THE_CURRENT_ENVIRONMENT_LACKS_THIS_QUERY),
        WITH_QUERY(Query),
        WITH_ENVIRONMENT(Env)>;

    struct _completions_fn {
      template <class Query, class Env>
      using _f = _mif<
        _nothrow_callable<Query, Env>,
        completion_signatures<set_value_t(_call_result_t<Query, Env>)>,
        completion_signatures<
          set_value_t(_call_result_t<Query, Env>),
          set_error_t(std::exception_ptr)>>;
    };

    template <class Query, class Rcvr>
    struct opstate_t : _immovable {
      using operation_state_concept = operation_state_t;
      Rcvr _rcvr;

      USTDEX_HOST_DEVICE void start() noexcept {
        // If the query invocation is noexcept, call it directly. Otherwise,
        // wrap it in a try-catch block and forward the exception to the
        // receiver.
        if constexpr (_nothrow_callable<Query, env_of_t<Rcvr>>) {
          // This looks like a use after move, but `set_value` takes its
          // arguments by forwarding reference, so it's safe.
          ustdex::set_value(static_cast<Rcvr &&>(_rcvr), Query()(ustdex::get_env(_rcvr)));
        } else {
          USTDEX_TRY {
            ustdex::set_value(
              static_cast<Rcvr &&>(_rcvr), Query()(ustdex::get_env(_rcvr)));
          }
          USTDEX_CATCH(...) {
            ustdex::set_error(static_cast<Rcvr &&>(_rcvr), std::current_exception());
          }
        }
      }
    };

    template <class Query>
    struct sndr_t;

   public:
    /// @brief Returns a sender that, when connected to a receiver and started,
    /// invokes the query with the receiver's environment and forwards the result
    /// to the receiver's `set_value` member.
    template <class Query>
    USTDEX_HOST_DEVICE USTDEX_INLINE constexpr sndr_t<Query> operator()(Query) const noexcept;
  };

  template <class Query>
  struct read_env_t::sndr_t {
    using sender_concept = sender_t;
    [[no_unique_address]] read_env_t _tag;
    [[no_unique_address]] Query _query;

    template <class Env>
    auto get_completion_signatures(const Env &) const //
      -> _minvoke<
        _mif<_callable<Query, Env>, _completions_fn, _error_env_lacks_query<Query, Env>>,
        Query,
        Env>;

    template <class Rcvr>
    USTDEX_HOST_DEVICE auto connect(Rcvr rcvr) const noexcept(_nothrow_movable<Rcvr>)
      -> opstate_t<Query, Rcvr> {
      return opstate_t<Query, Rcvr>{{}, static_cast<Rcvr &&>(rcvr)};
    }
  };

  template <class Query>
  USTDEX_HOST_DEVICE USTDEX_INLINE constexpr read_env_t::sndr_t<Query>
    read_env_t::operator()(Query query) const noexcept {
    return sndr_t<Query>{{}, query};
  }

  USTDEX_DEVICE constexpr read_env_t read_env{};

} // namespace ustdex
