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
  USTDEX_DEVICE constexpr struct start_on_t {
#ifndef __CUDACC__
   private:
#endif
    template <class Sch, class Rcvr>
    struct _rcvr_t {
      using receiver_concept = receiver_t;
      _pair<Sch, Rcvr> *_fs;

      struct _env_t {
        _pair<Sch, Rcvr> *_fs;

        Sch query(get_scheduler_t) const noexcept {
          return _fs->first;
        }

        template <class Query>
        auto query(Query) const noexcept -> _query_result_t<Query, env_of_t<Rcvr>> {
          return _fs->second.query(Query{});
        }
      };

      template <class... As>
      USTDEX_HOST_DEVICE void set_value(As &&...as) noexcept {
        ustdex::set_value(static_cast<Rcvr &&>(_fs->second), static_cast<As &&>(as)...);
      }

      template <class Error>
      USTDEX_HOST_DEVICE void set_error(Error &&error) noexcept {
        ustdex::set_error(static_cast<Rcvr &&>(_fs->second), static_cast<Error &&>(error));
      }

      USTDEX_HOST_DEVICE void set_stopped() noexcept {
        ustdex::set_stopped(static_cast<Rcvr &&>(_fs->second));
      }

      USTDEX_HOST_DEVICE _env_t get_env() const noexcept {
        return {_fs};
      }
    };

    template <class Sch, class Sndr, class Rcvr>
    struct _opstate_t : _immovable {
      using operation_state_concept = operation_state_t;
      _pair<Sch, Rcvr> _fs;
      connect_result_t<schedule_result_t<Sch>, _opstate_t *> _opstate1;
      connect_result_t<Sndr, _rcvr_t<Sch, Rcvr>> _opstate2;

      USTDEX_HOST_DEVICE _opstate_t(Sch sch, Rcvr rcvr, Sndr &&sndr)
        : _fs{sch, rcvr}
        , _opstate1{connect(schedule(_fs.first), this)}
        , _opstate2{connect(static_cast<Sndr &&>(sndr), _rcvr_t<Sch, Rcvr>{&_fs})} {
      }

      USTDEX_HOST_DEVICE void start() noexcept {
        ustdex::start(_opstate1);
      }

      USTDEX_HOST_DEVICE void set_value() noexcept {
        ustdex::start(_opstate2);
      }

      template <class Error>
      USTDEX_HOST_DEVICE void set_error(Error &&error) noexcept {
        ustdex::set_error(static_cast<Rcvr &&>(_fs.second), static_cast<Error &&>(error));
      }

      USTDEX_HOST_DEVICE void set_stopped() noexcept {
        ustdex::set_stopped(static_cast<Rcvr &&>(_fs.second));
      }

      USTDEX_HOST_DEVICE decltype(auto) get_env() const noexcept {
        return ustdex::get_env(_fs.second);
      }
    };

    template <class Sch, class Sndr>
    struct _sndr_t;

   public:
    template <class Sch, class Sndr>
    USTDEX_HOST_DEVICE auto operator()(Sch sch, Sndr sndr) const noexcept //
      -> _sndr_t<Sch, Sndr>;
  } start_on{};

  template <class Sch, class Sndr>
  struct start_on_t::_sndr_t {
    using sender_concept = sender_t;
    USTDEX_NO_UNIQUE_ADDRESS start_on_t _tag;
    Sch _sch;
    Sndr _sndr;

    template <class CvSndr, class... Env>
    using _completions = transform_completion_signatures<
      completion_signatures_of_t<CvSndr, Env...>,
      transform_completion_signatures<
        completion_signatures_of_t<schedule_result_t<Sch>, Env...>,
        completion_signatures<>,
        _malways<completion_signatures<>>::_f>>;

    template <class... Env>
    auto get_completion_signatures(const Env &...) && //
      -> _completions<Sndr, Env...>;

    template <class... Env>
    auto get_completion_signatures(const Env &...) const & //
      -> _completions<const Sndr &, Env...>;

    template <class Rcvr>
    USTDEX_HOST_DEVICE _opstate_t<Sch, Sndr, Rcvr> connect(Rcvr rcvr) && noexcept {
      return _opstate_t<Sch, Sndr, Rcvr>{
        _sch, static_cast<Rcvr &&>(rcvr), static_cast<Sndr &&>(_sndr)};
    }

    template <class Rcvr>
    USTDEX_HOST_DEVICE _opstate_t<Sch, const Sndr &, Rcvr>
      connect(Rcvr rcvr) const & noexcept {
      return _opstate_t<Sch, const Sndr &, Rcvr>{_sch, static_cast<Rcvr &&>(rcvr), _sndr};
    }

    USTDEX_HOST_DEVICE decltype(auto) get_env() const noexcept {
      return ustdex::get_env(_sndr);
    }
  };

  template <class Sch, class Sndr>
  USTDEX_HOST_DEVICE auto start_on_t::operator()(Sch sch, Sndr sndr) const noexcept
    -> start_on_t::_sndr_t<Sch, Sndr> {
    return _sndr_t<Sch, Sndr>{{}, sch, sndr};
  }
} // namespace ustdex
