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
#pragma once

#include "config.hpp"

#include <atomic>

#if USTDEX_CUDA()
#  include <cuda/std/atomic>
#  define USTDEX_CUDA_NS cuda::
#else
#  define USTDEX_CUDA_NS
#endif

namespace ustdex::ustd
{
template <class Ty>
using atomic = USTDEX_CUDA_NS std::atomic<Ty>;

using USTDEX_CUDA_NS std::memory_order;
using USTDEX_CUDA_NS std::memory_order_relaxed;
using USTDEX_CUDA_NS std::memory_order_consume;
using USTDEX_CUDA_NS std::memory_order_acquire;
using USTDEX_CUDA_NS std::memory_order_release;
using USTDEX_CUDA_NS std::memory_order_acq_rel;
using USTDEX_CUDA_NS std::memory_order_seq_cst;
} // namespace ustdex::ustd
