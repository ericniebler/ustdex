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

#include <cassert>
#include <type_traits>

// Needed by the warning suppression macros
#define USTDEX_STRINGIZE(_ARG) #_ARG

// Compiler detections:
#if defined(__NVCC__)
#  define USTDEX_NVCC() 1
#elif defined(__NVCOMPILER)
#  define USTDEX_NVHPC() 1
#elif defined(__EDG__)
#  define USTDEX_EDG() 1
#elif defined(__clang__)
#  define USTDEX_CLANG() 1
#  if defined(_MSC_VER)
#    define USTDEX_CLANG_CL() 1
#  endif
#  if defined(__apple_build_version__)
#    define USTDEX_APPLE_CLANG() 1
#  endif
#elif defined(__GNUC__)
#  define USTDEX_GCC() 1
#elif defined(_MSC_VER)
#  define USTDEX_MSVC() 1
#endif

#ifndef USTDEX_NVCC
#  define USTDEX_NVCC() 0
#endif
#ifndef USTDEX_NVHPC
#  define USTDEX_NVHPC() 0
#endif
#ifndef USTDEX_EDG
#  define USTDEX_EDG() 0
#endif
#ifndef USTDEX_CLANG
#  define USTDEX_CLANG() 0
#endif
#ifndef USTDEX_CLANG_CL
#  define USTDEX_CLANG_CL() 0
#endif
#ifndef USTDEX_APPLE_CLANG
#  define USTDEX_APPLE_CLANG() 0
#endif
#ifndef USTDEX_GCC
#  define USTDEX_GCC() 0
#endif
#ifndef USTDEX_MSVC
#  define USTDEX_MSVC() 0
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////
// warning push/pop portability macros
#if USTDEX_NVCC()
#  define USTDEX_PRAGMA_PUSH() _Pragma("nv_diagnostic push")
#  define USTDEX_PRAGMA_POP()  _Pragma("nv_diagnostic pop")
#  define USTDEX_PRAGMA_IGNORE_EDG(...)                                                  \
    _Pragma(USTDEX_STRINGIZE(nv_diag_suppress __VA_ARGS__))
#elif USTDEX_NVHPC() || USTDEX_EDG()
#  define USTDEX_PRAGMA_PUSH()                                                           \
    _Pragma("diagnostic push") USTDEX_PRAGMA_IGNORE_EDG(invalid_error_number)
#  define USTDEX_PRAGMA_POP() _Pragma("diagnostic pop")
#  define USTDEX_PRAGMA_IGNORE_EDG(...)                                                  \
    _Pragma(USTDEX_STRINGIZE(diag_suppress __VA_ARGS__))
#elif USTDEX_CLANG() || USTDEX_GCC()
#  define USTDEX_PRAGMA_PUSH()                                                           \
    _Pragma("GCC diagnostic push") USTDEX_PRAGMA_IGNORE_GNU("-Wpragmas")                 \
      USTDEX_PRAGMA_IGNORE_GNU("-Wunknown-pragmas")                                      \
        USTDEX_PRAGMA_IGNORE_GNU("-Wunknown-warning-option")                             \
          USTDEX_PRAGMA_IGNORE_GNU("-Wunknown-attributes")                               \
            USTDEX_PRAGMA_IGNORE_GNU("-Wattributes")
#  define USTDEX_PRAGMA_POP() _Pragma("GCC diagnostic pop")
#  define USTDEX_PRAGMA_IGNORE_GNU(...)                                                  \
    _Pragma(USTDEX_STRINGIZE(GCC diagnostic ignored __VA_ARGS__))
#else
#  define USTDEX_PRAGMA_PUSH()
#  define USTDEX_PRAGMA_POP()
#endif

#ifndef USTDEX_PRAGMA_IGNORE_GNU
#  define USTDEX_PRAGMA_IGNORE_GNU(...)
#endif
#ifndef USTDEX_PRAGMA_IGNORE_EDG
#  define USTDEX_PRAGMA_IGNORE_EDG(...)
#endif

#ifdef __CUDACC__
#  define USTDEX_DEVICE      __device__
#  define USTDEX_HOST_DEVICE __host__ __device__
#else
#  define USTDEX_DEVICE
#  define USTDEX_HOST_DEVICE
#endif

#if defined(__has_attribute)
#  define USTDEX_HAS_ATTRIBUTE(...) __has_attribute(__VA_ARGS__)
#else
#  define USTDEX_HAS_ATTRIBUTE(...) 0
#endif

#if defined(__has_builtin)
#  define USTDEX_HAS_BUILTIN(...) __has_builtin(__VA_ARGS__)
#else
#  define USTDEX_HAS_BUILTIN(...) 0
#endif

#if defined(__has_cpp_attribute)
#  define USTDEX_HAS_CPP_ATTRIBUTE(...) __has_cpp_attribute(__VA_ARGS__)
#else
#  define USTDEX_HAS_CPP_ATTRIBUTE(...) 0
#endif

#if USTDEX_HAS_ATTRIBUTE(__nodebug__)
#  define USTDEX_ATTR_NODEBUG __attribute__((__nodebug__))
#else
#  define USTDEX_ATTR_NODEBUG
#endif

#if USTDEX_HAS_ATTRIBUTE(__artificial__) && !defined(__CUDACC__)
#  define USTDEX_ATTR_ARTIFICIAL __attribute__((__artificial__))
#else
#  define USTDEX_ATTR_ARTIFICIAL
#endif

#if USTDEX_HAS_ATTRIBUTE(__always_inline__)
#  define USTDEX_ATTR_ALWAYS_INLINE __attribute__((__always_inline__))
#else
#  define USTDEX_ATTR_ALWAYS_INLINE
#endif

#define USTDEX_INLINE                                                                    \
  USTDEX_ATTR_ALWAYS_INLINE USTDEX_ATTR_ARTIFICIAL USTDEX_ATTR_NODEBUG inline

#if USTDEX_HAS_BUILTIN(__is_same)
#  define USTDEX_IS_SAME(...) __is_same(__VA_ARGS__)
#elif USTDEX_HAS_BUILTIN(__is_same_as)
#  define USTDEX_IS_SAME(...) __is_same_as(__VA_ARGS__)
#else
#  define USTDEX_IS_SAME(...) ustdex::_is_same<__VA_ARGS__>

namespace ustdex {
  template <class T, class U>
  inline constexpr bool _is_same = false;
  template <class T>
  inline constexpr bool _is_same<T, T> = true;
} // namespace ustdex
#endif

#if USTDEX_HAS_BUILTIN(__remove_reference)
namespace ustdex {
  template <class Ty>
  using _remove_reference_t = __remove_reference(Ty);
} // namespace ustdex

#  define USTDEX_REMOVE_REFERENCE(...) ustdex::_remove_reference_t<__VA_ARGS__>
#elif USTDEX_HAS_BUILTIN(__remove_reference_t)
namespace ustdex {
  template <class Ty>
  using _remove_reference_t = __remove_reference_t(Ty);
} // namespace ustdex

#  define USTDEX_REMOVE_REFERENCE(...) ustdex::_remove_reference_t<__VA_ARGS__>
#else
#  define USTDEX_REMOVE_REFERENCE(...) std::remove_reference_t<__VA_ARGS__>
#endif

#if USTDEX_HAS_BUILTIN(__is_base_of) || (_MSC_VER >= 1914)
#  define USTDEX_IS_BASE_OF(...) __is_base_of(__VA_ARGS__)
#else
#  define USTDEX_IS_BASE_OF(...) std::is_base_of_v<__VA_ARGS__>
#endif

#ifndef USTDEX_ASSERT
#  define USTDEX_ASSERT assert
#endif

#ifdef USTDEX_CUDA
#  if USTDEX_CUDA == 0
#    undef USTDEX_CUDA
#  endif
#endif

#if USTDEX_HAS_CPP_ATTRIBUTE(no_unique_address)
#  define USTDEX_NO_UNIQUE_ADDRESS [[no_unique_address]]
#else
#  define USTDEX_NO_UNIQUE_ADDRESS
#endif

// GCC struggles with guaranteed copy elision
#if USTDEX_GCC()
#  define USTDEX_IMMOVABLE(XP) XP(XP&&)
#else
#  define USTDEX_IMMOVABLE(XP) XP(XP&&) = delete
#endif

#include "exception.hpp"
