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
#include "exception.hpp"
#include "lazy.hpp"
#include "rcvr_ref.hpp"
#include "variant.hpp"

// This must be the last #include
#include "prologue.hpp"

namespace USTDEX_NAMESPACE
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

  template <class Ref>
  struct _opstate
  {
    using operation_state_concept = operation_state_t;

    using _args_t  = _deref_t<Ref>; // _deref_t<Ref> is _args<Rcvr, Sndr1, Sndr2>
    using _rcvr_t  = typename _args_t::_rcvr_t;
    using _sndr1_t = typename _args_t::_sndr1_t;
    using _sndr2_t = typename _args_t::_sndr2_t;

    using completion_signatures = //
      transform_completion_signatures_of< //
        _sndr1_t,
        _opstate*,
        completion_signatures_of_t<_sndr2_t, _rcvr_ref_t<_rcvr_t&>>,
        _malways<completion_signatures<>>::_f>; // swallow the first sender's value completions

    USTDEX_HOST_DEVICE friend env_of_t<_rcvr_t> get_env(const _opstate* self) noexcept
    {
      return ustdex::get_env(self->_rcvr);
    }

    _rcvr_t _rcvr;
    connect_result_t<_sndr1_t, _opstate*> _op1;
    connect_result_t<_sndr2_t, _rcvr_ref_t<_rcvr_t&>> _op2;

    USTDEX_HOST_DEVICE _opstate(_sndr1_t&& sndr1, _sndr2_t&& sndr2, _rcvr_t&& rcvr)
        : _rcvr(static_cast<_rcvr_t&&>(rcvr))
        , _op1(ustdex::connect(static_cast<_sndr1_t&&>(sndr1), this))
        , _op2(ustdex::connect(static_cast<_sndr2_t&&>(sndr2), _rcvr_ref(_rcvr)))
    {}

    USTDEX_HOST_DEVICE void start() noexcept
    {
      ustdex::start(_op1);
    }

    template <class... Values>
    USTDEX_HOST_DEVICE void set_value(Values&&...) && noexcept
    {
      ustdex::start(_op2);
    }

    template <class Error>
    USTDEX_HOST_DEVICE void set_error(Error&& err) && noexcept
    {
      ustdex::set_error(static_cast<_rcvr_t&&>(_rcvr), static_cast<Error&&>(err));
    }

    USTDEX_HOST_DEVICE void set_stopped() && noexcept
    {
      ustdex::set_stopped(static_cast<_rcvr_t&&>(_rcvr));
    }
  };

  template <class Sndr1, class Sndr2>
  struct _sndr;

  template <class Sndr1, class Sndr2>
  auto operator()(Sndr1 sndr1, Sndr2 sndr2) const;
};

template <class Sndr1, class Sndr2>
struct _seq::_sndr
{
  using sender_concept = sender_t;
  using _sndr1_t       = Sndr1;
  using _sndr2_t       = Sndr2;

  template <class Rcvr>
  USTDEX_HOST_DEVICE auto connect(Rcvr rcvr) &&
  {
    using _opstate_t = _opstate<_ref_t<_args<Rcvr, Sndr1, Sndr2>>>;
    return _opstate_t{static_cast<Sndr1&&>(_sndr1), static_cast<Sndr2>(_sndr2), static_cast<Rcvr&&>(rcvr)};
  }

  template <class Rcvr>
  USTDEX_HOST_DEVICE auto connect(Rcvr rcvr) const&
  {
    using _opstate_t = _opstate<_ref_t<_args<Rcvr, const Sndr1&, const Sndr2&>>>;
    return _opstate_t{_sndr1, _sndr2, static_cast<Rcvr&&>(rcvr)};
  }

  USTDEX_HOST_DEVICE env_of_t<Sndr2> get_env() const noexcept
  {
    return ustdex::get_env(_sndr2);
  }

  USTDEX_NO_UNIQUE_ADDRESS _seq _tag;
  USTDEX_NO_UNIQUE_ADDRESS _ignore _ign;
  _sndr1_t _sndr1;
  _sndr2_t _sndr2;
};

template <class Sndr1, class Sndr2>
auto _seq::operator()(Sndr1 sndr1, Sndr2 sndr2) const
{
  return _sndr<Sndr1, Sndr2>{{}, {}, static_cast<Sndr1&&>(sndr1), static_cast<Sndr2&&>(sndr2)};
}

using sequence_t = _seq;
inline constexpr sequence_t sequence{};
} // namespace USTDEX_NAMESPACE

#include "epilogue.hpp"
