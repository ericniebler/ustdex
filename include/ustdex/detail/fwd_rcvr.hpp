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
#include "env.hpp"

namespace USTDEX_NAMESPACE
{
template <class Rcvr>
struct _fwd_rcvr : Rcvr
{
  USTDEX_HOST_DEVICE decltype(auto) get_env() const noexcept
  {
    // TODO: only forward the "forwarding" queries:
    return ustdex::get_env(static_cast<Rcvr const&>(*this));
  }
};

template <class Rcvr>
struct _fwd_rcvr<Rcvr*>
{
  using receiver_concept = receiver_t;
  Rcvr* _rcvr;

  template <class... As>
  USTDEX_HOST_DEVICE USTDEX_INLINE void set_value(As&&... as) noexcept
  {
    ustdex::set_value(_rcvr);
  }

  template <class Error>
  USTDEX_HOST_DEVICE USTDEX_INLINE void set_error(Error&& error) noexcept
  {
    ustdex::set_error(_rcvr, static_cast<Error&&>(error));
  }

  USTDEX_HOST_DEVICE USTDEX_INLINE void set_stopped() noexcept
  {
    ustdex::set_stopped(_rcvr);
  }

  USTDEX_HOST_DEVICE decltype(auto) get_env() const noexcept
  {
    // TODO: only forward the "forwarding" queries:
    return ustdex::get_env(_rcvr);
  }
};
} // namespace USTDEX_NAMESPACE
