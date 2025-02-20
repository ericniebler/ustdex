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

#ifndef USTDEX_ASYNC_DETAIL_FWD_RCVR
#define USTDEX_ASYNC_DETAIL_FWD_RCVR

#include "config.hpp"
#include "cpos.hpp"
#include "env.hpp"

namespace ustdex
{
template <class Rcvr>
struct _fwd_rcvr : Rcvr
{
  USTDEX_API decltype(auto) get_env() const noexcept
  {
    // TODO: only forward the "forwarding" queries:
    return ustdex::get_env(static_cast<Rcvr const&>(*this));
  }
};

template <class Rcvr>
struct _fwd_rcvr<Rcvr*>
{
  using receiver_concept = receiver_t;
  Rcvr* _rcvr_;

  template <class... As>
  USTDEX_TRIVIAL_API void set_value(As&&... _as) noexcept
  {
    ustdex::set_value(_rcvr_);
  }

  template <class Error>
  USTDEX_TRIVIAL_API void set_error(Error&& _error) noexcept
  {
    ustdex::set_error(_rcvr_, static_cast<Error&&>(_error));
  }

  USTDEX_TRIVIAL_API void set_stopped() noexcept
  {
    ustdex::set_stopped(_rcvr_);
  }

  USTDEX_API decltype(auto) get_env() const noexcept
  {
    // TODO: only forward the "forwarding" queries:
    return ustdex::get_env(_rcvr_);
  }
};
} // namespace ustdex

#endif
