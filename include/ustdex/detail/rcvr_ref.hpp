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

#ifndef USTDEX_ASYNC_DETAIL_RCVR_REF
#define USTDEX_ASYNC_DETAIL_RCVR_REF

#include "cpos.hpp"

//
#include "prologue.hpp"

namespace ustdex
{
template <class _Rcvr, class _Env = env_of_t<_Rcvr>>
struct _rcvr_ref
{
  using receiver_concept = receiver_t;
  _Rcvr& _rcvr_;

  template <class... _As>
  USTDEX_TRIVIAL_API void set_value(_As&&... _as) noexcept
  {
    static_cast<_Rcvr&&>(_rcvr_).set_value(static_cast<_As&&>(_as)...);
  }

  template <class _Error>
  USTDEX_TRIVIAL_API void set_error(_Error&& _err) noexcept
  {
    static_cast<_Rcvr&&>(_rcvr_).set_error(static_cast<_Error&&>(_err));
  }

  USTDEX_TRIVIAL_API void set_stopped() noexcept
  {
    static_cast<_Rcvr&&>(_rcvr_).set_stopped();
  }

  USTDEX_API auto get_env() const noexcept -> _Env
  {
    return ustdex::get_env(_rcvr_);
  }
};

template <class _Rcvr>
_rcvr_ref(_Rcvr&) -> _rcvr_ref<_Rcvr>;
} // namespace ustdex

#include "epilogue.hpp"

#endif
