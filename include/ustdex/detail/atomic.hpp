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

#include <atomic>

#include "config.hpp"

#if USTDEX_CUDA
#  include <cuda/std/atomic>
#  define USTDEX_CUDA_NS cuda::
#else
#  define USTDEX_CUDA_NS
#endif

namespace USTDEX_NAMESPACE::ustd
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
} // namespace USTDEX_NAMESPACE::ustd
