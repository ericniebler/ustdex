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

#ifndef USTDEX_ASYNC_DETAIL_START_ON
#define USTDEX_ASYNC_DETAIL_START_ON

#include "completion_signatures.hpp"
#include "cpos.hpp"
#include "queries.hpp"
#include "rcvr_ref.hpp"
#include "rcvr_with_env.hpp"

#include "prologue.hpp"

namespace ustdex
{
template <class Sch>
struct _sch_env_t
{
  Sch _sch_;

  Sch query(get_scheduler_t) const noexcept
  {
    return _sch_;
  }
};

inline constexpr struct start_on_t
{
private:
  template <class Rcvr, class Sch, class CvSndr>
  struct USTDEX_TYPE_VISIBILITY_DEFAULT _opstate_t
  {
    using operation_state_concept = operation_state_t;
    using _env_t                  = env_of_t<Rcvr>;

    USTDEX_API _opstate_t(Sch _sch, Rcvr _rcvr, CvSndr&& _sndr)
        : _env_rcvr_{static_cast<Rcvr&&>(_rcvr), {_sch}}
        , _opstate1_{connect(schedule(_env_rcvr_._env_._sch_), _rcvr_ref{*this})}
        , _opstate2_{connect(static_cast<CvSndr&&>(_sndr), _rcvr_ref{_env_rcvr_})}
    {}

    USTDEX_IMMOVABLE(_opstate_t);

    USTDEX_API void start() noexcept
    {
      ustdex::start(_opstate1_);
    }

    USTDEX_API void set_value() noexcept
    {
      ustdex::start(_opstate2_);
    }

    template <class Error>
    USTDEX_API void set_error(Error&& _error) noexcept
    {
      ustdex::set_error(static_cast<Rcvr&&>(_env_rcvr_._rcvr()), static_cast<Error&&>(_error));
    }

    USTDEX_API void set_stopped() noexcept
    {
      ustdex::set_stopped(static_cast<Rcvr&&>(_env_rcvr_._rcvr()));
    }

    USTDEX_API auto get_env() const noexcept -> _env_t
    {
      return ustdex::get_env(_env_rcvr_._rcvr());
    }

    _rcvr_with_env_t<Rcvr, _sch_env_t<Sch>> _env_rcvr_;
    connect_result_t<schedule_result_t<Sch>, _rcvr_ref<_opstate_t, _env_t>> _opstate1_;
    connect_result_t<CvSndr, _rcvr_ref<_rcvr_with_env_t<Rcvr, _sch_env_t<Sch>>>> _opstate2_;
  };

  template <class Sch, class Sndr>
  struct USTDEX_TYPE_VISIBILITY_DEFAULT _sndr_t;

public:
  template <class Sch, class Sndr>
  USTDEX_API auto operator()(Sch _sch, Sndr _sndr) const noexcept //
    -> _sndr_t<Sch, Sndr>;
} starts_on{};

template <class Sch, class Sndr>
struct USTDEX_TYPE_VISIBILITY_DEFAULT start_on_t::_sndr_t
{
  using sender_concept = sender_t;
  USTDEX_NO_UNIQUE_ADDRESS start_on_t _tag_;
  Sch _sch_;
  Sndr _sndr_;

  template <class Env>
  using _env_t = env<_sch_env_t<Sch>, FWD_ENV_T<Env>>;

  template <class Self, class... Env>
  USTDEX_API static constexpr auto get_completion_signatures()
  {
    using _sch_sndr   = schedule_result_t<Sch>;
    using _child_sndr = _copy_cvref_t<Self, Sndr>;
    USTDEX_LET_COMPLETIONS(auto(_sndr_completions) = ustdex::get_completion_signatures<_child_sndr, _env_t<Env>...>())
    {
      USTDEX_LET_COMPLETIONS(auto(_sch_completions) = ustdex::get_completion_signatures<_sch_sndr, FWD_ENV_T<Env>...>())
      {
        return _sndr_completions + transform_completion_signatures(_sch_completions, _swallow_transform());
      }
    }
  }

  template <class Rcvr>
  USTDEX_API auto connect(Rcvr _rcvr) && -> _opstate_t<Rcvr, Sch, Sndr>
  {
    return _opstate_t<Rcvr, Sch, Sndr>{_sch_, static_cast<Rcvr&&>(_rcvr), static_cast<Sndr&&>(_sndr_)};
  }

  template <class Rcvr>
  USTDEX_API auto connect(Rcvr _rcvr) const& -> _opstate_t<Rcvr, Sch, const Sndr&>
  {
    return _opstate_t<Rcvr, Sch, const Sndr&>{_sch_, static_cast<Rcvr&&>(_rcvr), _sndr_};
  }

  USTDEX_API env_of_t<Sndr> get_env() const noexcept
  {
    return ustdex::get_env(_sndr_);
  }
};

template <class Sch, class Sndr>
USTDEX_API auto start_on_t::operator()(Sch _sch, Sndr _sndr) const noexcept -> start_on_t::_sndr_t<Sch, Sndr>
{
  return _sndr_t<Sch, Sndr>{{}, _sch, _sndr};
}
} // namespace ustdex

#include "epilogue.hpp"

#endif
