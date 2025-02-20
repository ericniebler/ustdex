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

#ifndef USTDEX_ASYNC_DETAIL_SEQUENCE
#define USTDEX_ASYNC_DETAIL_SEQUENCE

#include "completion_signatures.hpp"
#include "cpos.hpp"
#include "exception.hpp"
#include "lazy.hpp"
#include "rcvr_ref.hpp"
#include "variant.hpp"

#include "prologue.hpp"

namespace ustdex
{
struct _seq
{
  template <class Rcvr, class Sndr1, class Sndr2>
  struct _args
  {
    using _rcvr_t  = Rcvr;
    using _sndr1_t = Sndr1;
    using _sndr2_t = Sndr2;
  };

  template <class Zip>
  struct USTDEX_TYPE_VISIBILITY_DEFAULT _opstate
  {
    using operation_state_concept = operation_state_t;

    using _args_t  = _unzip<Zip>; // _unzip<Zip> is _args<Rcvr, Sndr1, Sndr2>
    using _rcvr_t  = typename _args_t::_rcvr_t;
    using _sndr1_t = typename _args_t::_sndr1_t;
    using _sndr2_t = typename _args_t::_sndr2_t;

    USTDEX_API friend env_of_t<_rcvr_t> get_env(const _opstate* _self) noexcept
    {
      return ustdex::get_env(_self->_rcvr_);
    }

    _rcvr_t _rcvr_;
    connect_result_t<_sndr1_t, _opstate*> _opstate1_;
    connect_result_t<_sndr2_t, _rcvr_ref_t<_rcvr_t&>> _opstate2_;

    USTDEX_API _opstate(_sndr1_t&& _sndr1, _sndr2_t&& _sndr2, _rcvr_t&& _rcvr)
        : _rcvr_(static_cast<_rcvr_t&&>(_rcvr))
        , _opstate1_(ustdex::connect(static_cast<_sndr1_t&&>(_sndr1), this))
        , _opstate2_(ustdex::connect(static_cast<_sndr2_t&&>(_sndr2), _rcvr_ref(_rcvr_)))
    {}

    USTDEX_API void start() noexcept
    {
      ustdex::start(_opstate1_);
    }

    template <class... Values>
    USTDEX_API void set_value(Values&&...) && noexcept
    {
      ustdex::start(_opstate2_);
    }

    template <class Error>
    USTDEX_API void set_error(Error&& _error) && noexcept
    {
      ustdex::set_error(static_cast<_rcvr_t&&>(_rcvr_), static_cast<Error&&>(_error));
    }

    USTDEX_API void set_stopped() && noexcept
    {
      ustdex::set_stopped(static_cast<_rcvr_t&&>(_rcvr_));
    }
  };

  template <class Sndr1, class Sndr2>
  struct USTDEX_TYPE_VISIBILITY_DEFAULT _sndr_t;

  template <class Sndr1, class Sndr2>
  USTDEX_API auto operator()(Sndr1 _sndr1, Sndr2 _sndr2) const -> _sndr_t<Sndr1, Sndr2>;
};

template <class Sndr1, class Sndr2>
struct USTDEX_TYPE_VISIBILITY_DEFAULT _seq::_sndr_t
{
  using sender_concept = sender_t;
  using _sndr1_t       = Sndr1;
  using _sndr2_t       = Sndr2;

  template <class Self, class... Env>
  USTDEX_API static constexpr auto get_completion_signatures()
  {
    USTDEX_LET_COMPLETIONS(auto(_completions1) = get_child_completion_signatures<Self, Sndr1, Env...>())
    {
      USTDEX_LET_COMPLETIONS(auto(_completions2) = get_child_completion_signatures<Self, Sndr2, Env...>())
      {
        // ignore the first sender's value completions
        return _completions2 + transform_completion_signatures(_completions1, _swallow_transform());
      }
    }
  }

  template <class Rcvr>
  USTDEX_API auto connect(Rcvr _rcvr) &&
  {
    using _opstate_t = _opstate<_zip<_args<Rcvr, Sndr1, Sndr2>>>;
    return _opstate_t{static_cast<Sndr1&&>(_sndr1_), static_cast<Sndr2>(_sndr2_), static_cast<Rcvr&&>(_rcvr)};
  }

  template <class Rcvr>
  USTDEX_API auto connect(Rcvr _rcvr) const&
  {
    using _opstate_t = _opstate<_zip<_args<Rcvr, const Sndr1&, const Sndr2&>>>;
    return _opstate_t{_sndr1_, _sndr2_, static_cast<Rcvr&&>(_rcvr)};
  }

  USTDEX_API env_of_t<Sndr2> get_env() const noexcept
  {
    return ustdex::get_env(_sndr2_);
  }

  USTDEX_NO_UNIQUE_ADDRESS _seq _tag_;
  USTDEX_NO_UNIQUE_ADDRESS _ignore _ign_;
  _sndr1_t _sndr1_;
  _sndr2_t _sndr2_;
};

template <class Sndr1, class Sndr2>
USTDEX_API auto _seq::operator()(Sndr1 _sndr1, Sndr2 _sndr2) const -> _sndr_t<Sndr1, Sndr2>
{
  return _sndr_t<Sndr1, Sndr2>{{}, {}, static_cast<Sndr1&&>(_sndr1), static_cast<Sndr2&&>(_sndr2)};
}

using sequence_t = _seq;
inline constexpr sequence_t sequence{};
} // namespace ustdex

#include "epilogue.hpp"

#endif
