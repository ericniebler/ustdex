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

#include "config.hpp"
#include "cpos.hpp"

// Must be the last include
#include "prologue.hpp"

namespace USTDEX_NAMESPACE
{
struct start_detached_t
{
#ifndef __CUDACC__

private:
#endif
  struct _opstate_base_t : _immovable
  {};

  struct _rcvr_t
  {
    using receiver_concept = receiver_t;

    _opstate_base_t* _opstate;
    void (*_destroy)(_opstate_base_t*) noexcept;

    template <class... As>
    void set_value(As&&...) && noexcept
    {
      _destroy(_opstate);
    }

    template <class Error>
    void set_error(Error&&) && noexcept
    {
      ::std::terminate();
    }

    void set_stopped() && noexcept
    {
      _destroy(_opstate);
    }
  };

  template <class Sndr>
  struct _opstate_t : _opstate_base_t
  {
    using operation_state_concept = operation_state_t;
    using completion_signatures   = ustdex::completion_signatures_of_t<Sndr, _rcvr_t>;
    connect_result_t<Sndr, _rcvr_t> _op;

    static void _destroy(_opstate_base_t* ptr) noexcept
    {
      delete static_cast<_opstate_t*>(ptr);
    }

    USTDEX_HOST_DEVICE explicit _opstate_t(Sndr&& sndr)
        : _op(ustdex::connect(static_cast<Sndr&&>(sndr), _rcvr_t{this, &_destroy}))
    {}

    USTDEX_HOST_DEVICE void start() & noexcept
    {
      ustdex::start(_op);
    }
  };

public:
  /// @brief Eagerly connects and starts a sender and lets it
  /// run detached.
  template <class Sndr>
  USTDEX_HOST_DEVICE USTDEX_INLINE void operator()(Sndr sndr) const
  {
    ustdex::start(*new _opstate_t<Sndr>{static_cast<Sndr&&>(sndr)});
  }
};

USTDEX_DEVICE_CONSTANT constexpr start_detached_t start_detached{};
} // namespace USTDEX_NAMESPACE

#include "epilogue.hpp"
