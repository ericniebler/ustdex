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

#ifndef USTDEX_ASYNC_DETAIL_CONDITIONAL
#define USTDEX_ASYNC_DETAIL_CONDITIONAL

#include "completion_signatures.hpp"
#include "concepts.hpp"
#include "config.hpp"
#include "just_from.hpp"
#include "meta.hpp"
#include "type_traits.hpp"
#include "variant.hpp"

#include <exception>

#include "prologue.hpp"

//! \file conditional.cuh
//! This file defines the \c conditional sender. \c conditional is a sender that
//! selects between two continuations based on the result of a predecessor. It
//! accepts a predecessor, a predicate, and two continuations. It passes the
//! result of the predecessor to the predicate. If the predicate returns \c true,
//! the result is passed to the first continuation; otherwise, it is passed to
//! the second continuation.
//!
//! By "continuation", we mean a so-called sender adaptor closure: a unary function
//! that takes a sender and returns a new sender. The expression `then(f)` is an
//! example of a continuation.

namespace ustdex
{
struct FUNCTION_MUST_RETURN_A_BOOLEAN_TESTABLE_VALUE;

struct _cond_t
{
  template <class Pred, class Then, class Else>
  struct _data
  {
    Pred _pred_;
    Then _then_;
    Else _else_;
  };

  template <class... As>
  USTDEX_HOST_DEVICE USTDEX_FORCEINLINE static auto _mk_complete_fn(As&&... _as) noexcept
  {
    return [&](auto _sink) noexcept {
      return _sink(static_cast<As&&>(_as)...);
    };
  }

  template <class... As>
  using _just_from_t = decltype(just_from(_cond_t::_mk_complete_fn(declval<As>()...)));

  template <class Pred, class Then, class Else, class... Env>
  struct _either_sig_fn
  {
    template <class... As>
    USTDEX_API constexpr auto operator()() const
    {
      if constexpr (!_callable<Pred, As&...>)
      {
        return invalid_completion_signature<WHERE(IN_ALGORITHM, _cond_t),
                                            WHAT(FUNCTION_IS_NOT_CALLABLE),
                                            WITH_FUNCTION(Pred),
                                            WITH_ARGUMENTS(As & ...)>();
      }
      else if constexpr (!std::is_convertible_v<_call_result_t<Pred, As&...>, bool>)
      {
        return invalid_completion_signature<WHERE(IN_ALGORITHM, _cond_t),
                                            WHAT(FUNCTION_MUST_RETURN_A_BOOLEAN_TESTABLE_VALUE),
                                            WITH_FUNCTION(Pred),
                                            WITH_ARGUMENTS(As & ...)>();
      }
      else
      {
        return concat_completion_signatures(
          get_completion_signatures<_call_result_t<Then, _just_from_t<As...>>, Env...>(),
          get_completion_signatures<_call_result_t<Else, _just_from_t<As...>>, Env...>());
      }
    }
  };

  template <class Sndr, class Rcvr, class Pred, class Then, class Else>
  struct _opstate
  {
    using operation_state_concept = operation_state_t;
    using _data_t                 = _data<Pred, Then, Else>;
    using _env_t                  = FWD_ENV_T<env_of_t<Rcvr>>;

    template <class... As>
    using _opstate_t = //
      _m_list< //
        connect_result_t<_call_result_t<Then, _just_from_t<As...>>, _rcvr_ref<Rcvr>>,
        connect_result_t<_call_result_t<Else, _just_from_t<As...>>, _rcvr_ref<Rcvr>>>;

    using _next_ops_variant_t = //
      _value_types<completion_signatures_of_t<Sndr, _env_t>, _opstate_t, _m_concat_into_q<_variant>::call>;

    USTDEX_API _opstate(Sndr&& _sndr, Rcvr&& _rcvr, _data_t&& _data)
        : _rcvr_{static_cast<Rcvr&&>(_rcvr)}
        , _data_{static_cast<_data_t&&>(_data)}
        , _op_{ustdex::connect(static_cast<Sndr&&>(_sndr), _rcvr_ref{*this})}
    {}

    USTDEX_API void start() noexcept
    {
      ustdex::start(_op_);
    }

    template <class... As>
    USTDEX_API void set_value(As&&... _as) noexcept
    {
      auto _just = just_from(_cond_t::_mk_complete_fn(static_cast<As&&>(_as)...));
      USTDEX_TRY( //
        ({ //
          if (static_cast<Pred&&>(_data_._pred_)(_as...))
          {
            auto& _op = _ops_._emplace_from(connect, static_cast<Then&&>(_data_._then_)(_just), _rcvr_ref{_rcvr_});
            ustdex::start(_op);
          }
          else
          {
            auto& _op = _ops_._emplace_from(connect, static_cast<Else&&>(_data_._else_)(_just), _rcvr_ref{_rcvr_});
            ustdex::start(_op);
          }
        }),
        USTDEX_CATCH(...) //
        ({ //
          ustdex::set_error(static_cast<Rcvr&&>(_rcvr_), ::std::current_exception());
        }) //
      )
    }

    template <class Error>
    USTDEX_API void set_error(Error&& _error) noexcept
    {
      ustdex::set_error(static_cast<Rcvr&&>(_rcvr_), static_cast<Error&&>(_error));
    }

    USTDEX_API void set_stopped() noexcept
    {
      ustdex::set_stopped(static_cast<Rcvr&&>(_rcvr_));
    }

    USTDEX_API auto get_env() const noexcept -> _env_t
    {
      return ustdex::get_env(_rcvr_);
    }

    Rcvr _rcvr_;
    _data_t _data_;
    connect_result_t<Sndr, _rcvr_ref<_opstate, _env_t>> _op_;
    _next_ops_variant_t _ops_;
  };

  template <class Sndr, class Pred, class Then, class Else>
  struct USTDEX_TYPE_VISIBILITY_DEFAULT _sndr_t;

  template <class Pred, class Then, class Else>
  struct _closure
  {
    _cond_t::_data<Pred, Then, Else> _data_;

    template <class Sndr>
    USTDEX_TRIVIAL_API auto _mk_sender(Sndr&& _sndr) //
      -> _sndr_t<Sndr, Pred, Then, Else>;

    template <class Sndr>
    USTDEX_TRIVIAL_API auto operator()(Sndr _sndr) //
      -> _sndr_t<Sndr, Pred, Then, Else>
    {
      return _mk_sender(static_cast<Sndr&&>(_sndr));
    }

    template <class Sndr>
    USTDEX_TRIVIAL_API friend auto operator|(Sndr _sndr, _closure&& _self) //
      -> _sndr_t<Sndr, Pred, Then, Else>
    {
      return _self._mk_sender(static_cast<Sndr&&>(_sndr));
    }
  };

  template <class Sndr, class Pred, class Then, class Else>
  USTDEX_TRIVIAL_API auto operator()(Sndr _sndr, Pred _pred, Then _then, Else _else) const //
    -> _sndr_t<Sndr, Pred, Then, Else>;

  template <class Pred, class Then, class Else>
  USTDEX_TRIVIAL_API auto operator()(Pred _pred, Then _then, Else _else) const
  {
    return _closure<Pred, Then, Else>{
      {static_cast<Pred&&>(_pred), static_cast<Then&&>(_then), static_cast<Else&&>(_else)}};
  }
};

template <class Sndr, class Pred, class Then, class Else>
struct USTDEX_TYPE_VISIBILITY_DEFAULT _cond_t::_sndr_t
{
  using _data_t = _cond_t::_data<Pred, Then, Else>;
  _cond_t _tag_;
  _data_t _data_;
  Sndr _sndr_;

  template <class Self, class... Env>
  USTDEX_API static constexpr auto get_completion_signatures()
  {
    USTDEX_LET_COMPLETIONS(auto(_child_completions) = get_child_completion_signatures<Self, Sndr, Env...>())
    {
      return concat_completion_signatures(
        transform_completion_signatures(_child_completions, _either_sig_fn<Pred, Then, Else, Env...>{}),
        _eptr_completion());
    }
  }

  template <class Rcvr>
  USTDEX_API auto connect(Rcvr _rcvr) && -> _opstate<Sndr, Rcvr, Pred, Then, Else>
  {
    return {static_cast<Sndr&&>(_sndr_), static_cast<Rcvr&&>(_rcvr), static_cast<_data_t&&>(_data_)};
  }

  template <class Rcvr>
  USTDEX_API auto connect(Rcvr _rcvr) const& -> _opstate<Sndr const&, Rcvr, Pred, Then, Else>
  {
    return {_sndr_, static_cast<Rcvr&&>(_rcvr), static_cast<_data_t&&>(_data_)};
  }

  USTDEX_API env_of_t<Sndr> get_env() const noexcept
  {
    return ustdex::get_env(_sndr_);
  }
};

template <class Sndr, class Pred, class Then, class Else>
USTDEX_TRIVIAL_API auto _cond_t::operator()(Sndr _sndr, Pred _pred, Then _then, Else _else) const //
  -> _sndr_t<Sndr, Pred, Then, Else>
{
  if constexpr (!dependent_sender<Sndr>)
  {
    using _completions = completion_signatures_of_t<_sndr_t<Sndr, Pred, Then, Else>>;
    static_assert(_valid_completion_signatures<_completions>);
  }

  return _sndr_t<Sndr, Pred, Then, Else>{
    {},
    {static_cast<Pred&&>(_pred), static_cast<Then&&>(_then), static_cast<Else&&>(_else)},
    static_cast<Sndr&&>(_sndr)};
}

template <class Pred, class Then, class Else>
template <class Sndr>
USTDEX_TRIVIAL_API auto _cond_t::_closure<Pred, Then, Else>::_mk_sender(Sndr&& _sndr) //
  -> _sndr_t<Sndr, Pred, Then, Else>
{
  if constexpr (!dependent_sender<Sndr>)
  {
    using _completions = completion_signatures_of_t<_sndr_t<Sndr, Pred, Then, Else>>;
    static_assert(_valid_completion_signatures<_completions>);
  }

  return _sndr_t<Sndr, Pred, Then, Else>{
    {}, static_cast<_cond_t::_data<Pred, Then, Else>&&>(_data_), static_cast<Sndr&&>(_sndr)};
}

inline constexpr _cond_t conditional{};
} // namespace ustdex

#include "epilogue.hpp"

#endif
