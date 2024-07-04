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

#include "completion_signatures.hpp"
#include "cpos.hpp"
#include "queries.hpp"
#include "receiver_with_env.hpp"
#include "tuple.hpp"
#include "utility.hpp"
#include "variant.hpp"

// Must be the last include
#include "prologue.hpp"

namespace USTDEX_NAMESPACE
{
template <class Sch>
struct _sch_env_t
{
  Sch _sch;

  Sch query(get_scheduler_t) const noexcept
  {
    return _sch;
  }
};

USTDEX_DEVICE_CONSTANT constexpr struct start_on_t
{
#ifndef __CUDACC__

private:
#endif

  template <class Rcvr, class Sch, class CvSndr>
  struct _opstate_t
  {
    USTDEX_HOST_DEVICE friend env_of_t<Rcvr> get_env(const _opstate_t* self) noexcept
    {
      return ustdex::get_env(self->_env_rcvr.rcvr());
    }

    using operation_state_concept = operation_state_t;

    using completion_signatures = //
      transform_completion_signatures<
        completion_signatures_of_t<CvSndr, _receiver_with_env_t<Rcvr, _sch_env_t<Sch>>*>,
        transform_completion_signatures<completion_signatures_of_t<schedule_result_t<Sch>, _opstate_t*>,
                                        ustdex::completion_signatures<>,
                                        _malways<ustdex::completion_signatures<>>::_f>>;

    _receiver_with_env_t<Rcvr, _sch_env_t<Sch>> _env_rcvr;
    connect_result_t<schedule_result_t<Sch>, _opstate_t*> _opstate1;
    connect_result_t<CvSndr, _receiver_with_env_t<Rcvr, _sch_env_t<Sch>>*> _opstate2;

    USTDEX_HOST_DEVICE _opstate_t(Sch sch, Rcvr rcvr, CvSndr&& sndr)
        : _env_rcvr{static_cast<Rcvr&&>(rcvr), {sch}}
        , _opstate1{connect(schedule(_env_rcvr._env._sch), this)}
        , _opstate2{connect(static_cast<CvSndr&&>(sndr), &_env_rcvr)}
    {}

    USTDEX_IMMOVABLE(_opstate_t);

    USTDEX_HOST_DEVICE void start() noexcept
    {
      ustdex::start(_opstate1);
    }

    USTDEX_HOST_DEVICE void set_value() noexcept
    {
      ustdex::start(_opstate2);
    }

    template <class Error>
    USTDEX_HOST_DEVICE void set_error(Error&& error) noexcept
    {
      ustdex::set_error(static_cast<Rcvr&&>(_env_rcvr.rcvr()), static_cast<Error&&>(error));
    }

    USTDEX_HOST_DEVICE void set_stopped() noexcept
    {
      ustdex::set_stopped(static_cast<Rcvr&&>(_env_rcvr.rcvr()));
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
struct start_on_t::_sndr_t
{
  using sender_concept = sender_t;
  USTDEX_NO_UNIQUE_ADDRESS start_on_t _tag;
  Sch _sch;
  Sndr _sndr;

  template <class Rcvr>
  USTDEX_HOST_DEVICE auto connect(Rcvr rcvr) && -> _opstate_t<Rcvr, Sch, Sndr>
  {
    return _opstate_t<Rcvr, Sch, Sndr>{_sch, static_cast<Rcvr&&>(rcvr), static_cast<Sndr&&>(_sndr)};
  }

  template <class Rcvr>
  USTDEX_HOST_DEVICE auto connect(Rcvr rcvr) const& -> _opstate_t<Rcvr, Sch, const Sndr&>
  {
    return _opstate_t<Rcvr, Sch, const Sndr&>{_sch, static_cast<Rcvr&&>(rcvr), _sndr};
  }

  USTDEX_HOST_DEVICE env_of_t<Sndr> get_env() const noexcept
  {
    return ustdex::get_env(_sndr);
  }
};

template <class Sch, class Sndr>
USTDEX_HOST_DEVICE auto start_on_t::operator()(Sch sch, Sndr sndr) const noexcept -> start_on_t::_sndr_t<Sch, Sndr>
{
  return _sndr_t<Sch, Sndr>{{}, sch, sndr};
}
} // namespace USTDEX_NAMESPACE

#include "epilogue.hpp"
