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
#include "meta.hpp"
#include "queries.hpp"
#include "tuple.hpp"
#include "utility.hpp"
#include "variant.hpp"

namespace ustdex {
  USTDEX_DEVICE constexpr struct continue_on_t {
#ifndef __CUDACC__
   private:
#endif
    template <class... As>
    using _set_value_tuple_t = _tuple_for<set_value_t, _decay_t<As>...>;

    template <class Error>
    using _set_error_tuple_t = _tuple_for<set_error_t, _decay_t<Error>>;

    using _set_stopped_tuple_t = _tuple_for<set_stopped_t>;

    using _complete_fn = void (*)(void *) noexcept;

    template <class Rcvr, class Result>
    struct _rcvr_t {
      using receiver_concept = receiver_t;
      Rcvr _rcvr;
      Result _result;
      _complete_fn _complete;

      template <class Tag, class... As>
      USTDEX_HOST_DEVICE
      void
        operator()(Tag, As &...as) noexcept {
        Tag()(static_cast<Rcvr &&>(_rcvr), static_cast<As &&>(as)...);
      }

      template <class Tag, class... As>
      USTDEX_HOST_DEVICE
      void _set_result(Tag, As &&...as) noexcept {
        using _tupl_t = _tuple_for<Tag, _decay_t<As>...>;
        _result.template emplace<_tupl_t>(Tag(), static_cast<As &&>(as)...);
        _complete = +[](void *ptr) noexcept {
          auto &self = *static_cast<_rcvr_t *>(ptr);
          auto &tupl = *static_cast<_tupl_t *>(self._result._get_ptr());
          _apply(self, tupl);
        };
      }

      USTDEX_HOST_DEVICE
      void set_value() noexcept {
        _complete(this);
      }

      template <class Error>
      USTDEX_HOST_DEVICE
      void set_error(Error &&error) noexcept {
        ustdex::set_error(static_cast<Rcvr &&>(_rcvr), static_cast<Error &&>(error));
      }

      USTDEX_HOST_DEVICE
      void set_stopped() noexcept {
        ustdex::set_stopped(static_cast<Rcvr &&>(_rcvr));
      }

      USTDEX_HOST_DEVICE
      decltype(auto) get_env() const noexcept {
        return ustdex::get_env(_rcvr);
      }
    };

    template <class CvSndr, class Sch, class Rcvr>
    struct _opstate_t : _immovable {
      using operation_state_concept = operation_state_t;
      using _result_t = _transform_completion_signatures<
        completion_signatures_of_t<CvSndr, env_of_t<Rcvr>>,
        _set_value_tuple_t,
        _set_error_tuple_t,
        _set_stopped_tuple_t,
        _variant>;

      _rcvr_t<Rcvr, _result_t> _rcvr;
      connect_result_t<CvSndr, _opstate_t *> _opstate1;
      connect_result_t<schedule_result_t<Sch>, _rcvr_t<Rcvr, _result_t> *> _opstate2;

      USTDEX_HOST_DEVICE _opstate_t(CvSndr &&sndr, Sch sch, Rcvr rcvr)
        : _rcvr{static_cast<Rcvr &&>(rcvr), {}, nullptr}
        , _opstate1{ustdex::connect(static_cast<CvSndr &&>(sndr), this)}
        , _opstate2{ustdex::connect(schedule(sch), &_rcvr)} {
      }

      USTDEX_HOST_DEVICE
      void start() noexcept {
        ustdex::start(_opstate1);
      }

      template <class... As>
      USTDEX_HOST_DEVICE
      void set_value(As &&...as) noexcept {
        _rcvr._set_result(set_value_t(), static_cast<As &&>(as)...);
        ustdex::start(_opstate2);
      }

      template <class Error>
      USTDEX_HOST_DEVICE
      void set_error(Error &&error) noexcept {
        _rcvr._set_result(set_error_t(), static_cast<Error &&>(error));
        ustdex::start(_opstate2);
      }

      USTDEX_HOST_DEVICE
      void set_stopped() noexcept {
        _rcvr._set_result(set_stopped_t());
        ustdex::start(_opstate2);
      }

      USTDEX_HOST_DEVICE
      decltype(auto) get_env() const noexcept {
        return ustdex::get_env(_rcvr._rcvr);
      }
    };

    template <class Sndr, class Sch, class Tag = continue_on_t>
    struct _sndr_t {
      using sender_concept = sender_t;
      [[no_unique_address]] Tag _tag;
      Sch _sch;
      Sndr _sndr;

      struct _attrs_t {
        _sndr_t *_sndr;

        template <class SetTag>
        USTDEX_HOST_DEVICE
        auto query(get_completion_scheduler_t<SetTag>) const noexcept {
          return _sndr->_sch;
        }

        template <class Query>
        USTDEX_HOST_DEVICE
        auto query(Query) const -> _query_result_t<env_of_t<Sndr>, Query> {
          return ustdex::get_env(_sndr->_sndr).query(Query{});
        }
      };

      template <class Rcvr>
      USTDEX_HOST_DEVICE
      _opstate_t<Sndr, Sch, Rcvr> connect(Rcvr rcvr) && {
        return {static_cast<Sndr &&>(_sndr), _sch, static_cast<Rcvr &&>(rcvr)};
      }

      template <class Rcvr>
      USTDEX_HOST_DEVICE
      _opstate_t<const Sndr &, Sch, Rcvr> connect(Rcvr rcvr) const & {
        return {_sndr, _sch, static_cast<Rcvr &&>(rcvr)};
      }

      USTDEX_HOST_DEVICE
      _attrs_t get_env() const noexcept {
        return _attrs_t{this};
      }
    };

    template <class Sch>
    struct _closure_t {
      Sch _sch;

      template <class Sndr>
      USTDEX_HOST_DEVICE USTDEX_INLINE friend auto
        operator|(Sndr sndr, _closure_t &&_self) {
        return _sndr_t<Sndr, Sch>{{}, _self._sch, static_cast<Sndr &&>(sndr)};
      }
    };

   public:
    template <class Sndr, class Sch>
    USTDEX_HOST_DEVICE
    auto
      operator()(Sndr sndr, Sch sch) const noexcept {
      return _sndr_t<Sndr, Sch>{{}, sch, static_cast<Sndr &&>(sndr)};
    }

    template <class Sch>
    USTDEX_HOST_DEVICE USTDEX_INLINE auto
      operator()(Sch sch) const noexcept {
      return _closure_t<Sch>{sch};
    }
  } continue_on{};
} // namespace ustdex
