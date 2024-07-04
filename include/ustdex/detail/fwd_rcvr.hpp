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

#include "config.hpp"
#include "cpos.hpp"
#include "env.hpp"

namespace ustdex
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
} // namespace ustdex
