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

#ifndef USTDEX_DETAIL_EXCEPTION
#define USTDEX_DETAIL_EXCEPTION

#include "config.hpp"       // IWYU pragma: export
#include "preprocessor.hpp" // IWYU pragma: export

#include <exception>        // IWYU pragma: export

// The following macros are used to conditionally compile exception handling code. They
// are used in the same way as `try` and `catch`, but they allow for different behavior
// based on whether exceptions are enabled or not, and whether the code is being compiled
// for device or not.
//
// Usage:
//   USTDEX_TRY
//   {
//     can_throw();               // Code that may throw an exception
//   }
//   USTDEX_CATCH (cuda_error& e)  // Handle CUDA exceptions
//   {
//     printf("CUDA error: %s\n", e.what());
//   }
//   USTDEX_CATCH_ALL              // Handle any other exceptions
//   {
//     printf("unknown error\n");
//   }
#if USTDEX_NO_STDCPP_EXCEPTIONS()
#  define USTDEX_TRY               if constexpr (true) {
#  define USTDEX_CATCH(...)        } else if constexpr (__VA_ARGS__ = ::USTDEX::__catch_any_lvalue; false) {
#  define USTDEX_CATCH_ALL         } else if constexpr (true) {} else
#  define USTDEX_THROW(...)        ::USTDEX::__terminate()
#  define USTDEX_CATCH_FALLTHROUGH } else {}
#else
#  define USTDEX_TRY               try
#  define USTDEX_CATCH             catch
#  define USTDEX_CATCH_ALL         catch(...)
#  define USTDEX_THROW(...)        throw __VA_ARGS__
#  define USTDEX_CATCH_FALLTHROUGH
#endif

#if USTDEX_HOST_ONLY()
// This is the default behavior for host code, and for nvc++
#  define USTDEX_NOEXCEPT_EXPR(...) noexcept(__VA_ARGS__)
#else
// Treat everything as no-throw in device code
#  define USTDEX_NOEXCEPT_EXPR(...) true
#endif

#endif
