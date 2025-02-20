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

#ifndef USTDEX_ASYNC_DETAIL_EXCEPTION
#define USTDEX_ASYNC_DETAIL_EXCEPTION

#include "config.hpp" // IWYU pragma: export
#include "preprocessor.hpp" // IWYU pragma: export

#include <exception> // IWYU pragma: export

#if USTDEX_CUDA()
#  define USTDEX_CATCH(...)
#  define USTDEX_TRY(TRY, CATCH) \
    USTDEX_IF_ELSE_TARGET(       \
      USTDEX_IS_HOST, (try { USTDEX_PP_EXPAND TRY } catch (...){USTDEX_PP_EXPAND CATCH}), ({USTDEX_PP_EXPAND TRY}))
#else
#  define USTDEX_CATCH(...)
#  define USTDEX_TRY(TRY, CATCH) USTDEX_PP_EXPAND(try { USTDEX_PP_EXPAND TRY } catch (...){USTDEX_PP_EXPAND CATCH})
#endif

#if USTDEX_HOST_ONLY()
// This is the default behavior for host code, and for nvc++
#  define USTDEX_NOEXCEPT_EXPR(...) noexcept(__VA_ARGS__)
#else
// Treat everything as no-throw in device code
#  define USTDEX_NOEXCEPT_EXPR(...) true
#endif

#endif
