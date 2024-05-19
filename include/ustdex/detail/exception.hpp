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
