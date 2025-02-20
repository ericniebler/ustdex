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
#include "meta.hpp"

//
#include "prologue.hpp"

namespace ustdex
{
template <class Rcvr>
constexpr Rcvr* _rcvr_ref(Rcvr& _rcvr) noexcept
{
  return &_rcvr;
}

template <class Rcvr>
constexpr Rcvr* _rcvr_ref(Rcvr* _rcvr) noexcept
{
  return _rcvr;
}

template <class Rcvr>
using _rcvr_ref_t = decltype(ustdex::_rcvr_ref(declval<Rcvr>()));

} // namespace ustdex

#include "epilogue.hpp"

#endif
