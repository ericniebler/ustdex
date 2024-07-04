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
#include "config.hpp"
#include "cpos.hpp"
#include "tuple.hpp"
#include "utility.hpp"

// Must be the last include
#include "prologue.hpp"

namespace USTDEX_NAMESPACE
{
// Forward declarations of the just* tag types:
struct just_t;
struct just_error_t;
struct just_stopped_t;

// Map from a disposition to the corresponding tag types:
namespace _detail
{
template <_disposition_t, class Void = void>
extern _undefined<Void> _just_tag;
template <class Void>
extern _fn_t<just_t>* _just_tag<_value, Void>;
template <class Void>
extern _fn_t<just_error_t>* _just_tag<_error, Void>;
template <class Void>
extern _fn_t<just_stopped_t>* _just_tag<_stopped, Void>;
} // namespace _detail

template <_disposition_t Disposition>
struct _just
{
#ifndef __CUDACC__

private:
#endif

  using JustTag = decltype(_detail::_just_tag<Disposition>());
  using SetTag  = decltype(_detail::_set_tag<Disposition>());

  template <class Rcvr, class... Ts>
  struct opstate_t
  {
    using operation_state_concept = operation_state_t;
    using completion_signatures   = ustdex::completion_signatures<SetTag(Ts...)>;
    Rcvr _rcvr;
    _tuple<Ts...> _values;

    struct _complete_fn
    {
      opstate_t* _self;

      USTDEX_HOST_DEVICE void operator()(Ts&... ts) const noexcept
      {
        SetTag()(static_cast<Rcvr&&>(_self->_rcvr), static_cast<Ts&&>(ts)...);
      }
    };

    USTDEX_HOST_DEVICE void start() & noexcept
    {
      _values.apply(_complete_fn{this}, _values);
    }
  };

  template <class... Ts>
  struct _sndr_t
  {
    using sender_concept        = sender_t;
    using completion_signatures = ustdex::completion_signatures<SetTag(Ts...)>;

    USTDEX_NO_UNIQUE_ADDRESS JustTag _tag;
    _tuple<Ts...> _values;

    template <class Rcvr>
    USTDEX_HOST_DEVICE opstate_t<Rcvr, Ts...> connect(Rcvr rcvr) && //
      noexcept(_nothrow_decay_copyable<Rcvr, Ts...>)
    {
      return opstate_t<Rcvr, Ts...>{static_cast<Rcvr&&>(rcvr), static_cast<_tuple<Ts...>&&>(_values)};
    }

    template <class Rcvr>
    USTDEX_HOST_DEVICE opstate_t<Rcvr, Ts...> connect(Rcvr rcvr) const& //
      noexcept(_nothrow_decay_copyable<Rcvr, Ts const&...>)
    {
      return opstate_t<Rcvr, Ts...>{static_cast<Rcvr&&>(rcvr), _values};
    }
  };

public:
  template <class... Ts>
  USTDEX_INLINE USTDEX_HOST_DEVICE auto operator()(Ts... ts) const noexcept
  {
    return _sndr_t<Ts...>{{}, {static_cast<Ts&&>(ts)...}};
  }
};

USTDEX_DEVICE_CONSTANT constexpr struct just_t : _just<_value>
{
} just{};

USTDEX_DEVICE_CONSTANT constexpr struct just_error_t : _just<_error>
{
} just_error{};

USTDEX_DEVICE_CONSTANT constexpr struct just_stopped_t : _just<_stopped>
{
} just_stopped{};
} // namespace USTDEX_NAMESPACE

#include "epilogue.hpp"
