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

#pragma once

#include "ustdex/ustdex.hpp" // IWYU pragma: export

#if !USTDEX_HOST_ONLY()
// disable exceptions in Catch2 when compiling for device:
#  define CATCH_CONFIG_DISABLE_EXCEPTIONS
#endif

#include <catch2/catch_all.hpp> // IWYU pragma: export

// In device code, a few of the Catch2 macros are not supported.
#if !USTDEX_HOST_ONLY()
#  undef CHECK
#  define CHECK(expr)                                                         \
    if (expr)                                                                 \
    {                                                                         \
    }                                                                         \
    else                                                                      \
    {                                                                         \
      printf("Test failed: \"%s\", %s(%d)", #expr, __FILE__, (int) __LINE__); \
      asm("trap;");                                                           \
    }

#  undef FAIL
#  define FAIL(msg)                                                         \
    {                                                                       \
      printf("Test failed: \"%s\", %s(%d)", msg, __FILE__, (int) __LINE__); \
      asm("trap;");                                                         \
    }

#  undef CHECK_FALSE
#  define CHECK_FALSE(expr) CHECK(!(expr))

#  undef REQUIRE
#  define REQUIRE(expr) CHECK(expr)
#endif
