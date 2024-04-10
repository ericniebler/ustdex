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

#include "completion_signatures.hpp"
#include "cpos.hpp"
#include "queries.hpp"
#include "tuple.hpp"
#include "utility.hpp"
#include "variant.hpp"

namespace ustdex {
  inline constexpr struct start_on_t {
   private:
    template <class Sch, class Rcvr>
    struct _rcvr_t {
      using receiver_concept = receiver_t;
      pair<Sch, Rcvr> *_pair;

      struct _env_t {
        pair<Sch, Rcvr> *_pair;

        Sch query(get_scheduler_t) const noexcept {
          return _pair->first;
        }

        template <class Query>
        auto query(Query) const noexcept -> _query_result_t<env_of_t<Rcvr>, Query> {
          return _pair->second.query(Query{});
        }
      };

      template <class... As>
      USTDEX_HOST_DEVICE
      void set_value(As &&...as) noexcept {
        ustdex::set_value(static_cast<Rcvr &&>(_pair->second), static_cast<As &&>(as)...);
      }

      template <class Error>
      USTDEX_HOST_DEVICE
      void set_error(Error &&error) noexcept {
        ustdex::set_error(
          static_cast<Rcvr &&>(_pair->second), static_cast<Error &&>(error));
      }

      USTDEX_HOST_DEVICE
      void set_stopped() noexcept {
        ustdex::set_stopped(static_cast<Rcvr &&>(_pair->second));
      }

      USTDEX_HOST_DEVICE
      _env_t get_env() const noexcept {
        return {_pair};
      }
    };

    template <class Sch, class Sndr, class Rcvr>
    struct _opstate_t : _immovable {
      using operation_state_concept = operation_state_t;
      pair<Sch, Rcvr> _pair;
      connect_result_t<schedule_result_t<Sch>, _opstate_t *> _opstate1;
      connect_result_t<Sndr, _rcvr_t<Sch, Rcvr>> _opstate2;

      _opstate_t(Sch sch, Rcvr rcvr, Sndr &&sndr)
        : _pair{sch, rcvr}
        , _opstate1{connect(schedule(_pair.first), this)}
        , _opstate2{connect(static_cast<Sndr &&>(sndr), _rcvr_t<Sch, Rcvr>{&_pair})} {
      }

      USTDEX_HOST_DEVICE
      void start() noexcept {
        ustdex::start(_opstate1);
      }

      USTDEX_HOST_DEVICE
      void set_value() noexcept {
        ustdex::start(_opstate2);
      }

      template <class Error>
      USTDEX_HOST_DEVICE
      void set_error(Error &&error) noexcept {
        ustdex::set_error(
          static_cast<Rcvr &&>(_pair.second), static_cast<Error &&>(error));
      }

      USTDEX_HOST_DEVICE
      void set_stopped() noexcept {
        ustdex::set_stopped(static_cast<Rcvr &&>(_pair.second));
      }

      USTDEX_HOST_DEVICE
      decltype(auto) get_env() const noexcept {
        return ustdex::get_env(_pair.second);
      }
    };

    template <class...>
    using _eat_value_completions_t = completion_signatures<>;

    template <class Sch, class Sndr, class Tag = start_on_t>
    struct _sndr_t {
      using sender_concept = sender_t;
      [[no_unique_address]] Tag _tag;
      Sch _sch;
      Sndr _sndr;

      template <class CvSndr, class... Env>
      using _completions = transform_completion_signatures<
        completion_signatures_of_t<CvSndr, Env...>,
        transform_completion_signatures<
          completion_signatures_of_t<schedule_result_t<Sch>, Env...>,
          completion_signatures<>,
          _eat_value_completions_t>>;

      template <class... Env>
      auto get_completion_signatures(
        const Env &...) && -> completion_signatures_of_t<Sndr, Env...>;

      template <class... Env>
      auto get_completion_signatures(
        const Env &...) const & -> completion_signatures_of_t<const Sndr &, Env...>;

      template <class Rcvr>
      USTDEX_HOST_DEVICE
      _opstate_t<Sch, Sndr, Rcvr> connect(Rcvr rcvr) && noexcept {
        return _opstate_t<Sch, Sndr, Rcvr>{
          _sch, static_cast<Rcvr &&>(rcvr), static_cast<Sndr &&>(_sndr)};
      }

      template <class Rcvr>
      USTDEX_HOST_DEVICE
      _opstate_t<Sch, const Sndr &, Rcvr> connect(Rcvr rcvr) const & noexcept {
        return _opstate_t<Sch, const Sndr &, Rcvr>{
          _sch, static_cast<Rcvr &&>(rcvr), _sndr};
      }

      USTDEX_HOST_DEVICE
      decltype(auto) get_env() const noexcept {
        return ustdex::get_env(_sndr);
      }
    };

   public:
    template <class Sch, class Sndr>
    USTDEX_HOST_DEVICE
    _sndr_t<Sch, Sndr>
      operator()(Sch sch, Sndr sndr) const noexcept {
      return _sndr_t<Sch, Sndr>{{}, sch, sndr};
    }
  } start_on{};
} // namespace ustdex
