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
#include "preprocessor.hpp"

#include <exception>

#if defined(__CUDACC__)
#  include <nv/target>
#  define USTDEX_CATCH(...)
#  define USTDEX_TRY(TRY, CATCH)                                                         \
    NV_IF_TARGET(                                                                        \
      NV_IS_HOST,                                                                        \
      (try { USTDEX_PP_EXPAND TRY } catch (...){USTDEX_PP_EXPAND CATCH}),                \
      ({USTDEX_PP_EXPAND TRY}))
#else
#  define USTDEX_CATCH(...)
#  define USTDEX_TRY(TRY, CATCH)                                                         \
    try {                                                                                \
      USTDEX_PP_EXPAND TRY                                                               \
    } catch (...) {                                                                      \
      USTDEX_PP_EXPAND CATCH                                                             \
    }
#endif

#if defined(__CUDA_ARCH__)
// Treat everything as no-throw in device code
#  define USTDEXEC_NOEXCEPT(...) true
#else
// This is the default behavior for host code, and for nvc++
#  define USTDEXEC_NOEXCEPT(...) noexcept(__VA_ARGS__)
#endif
