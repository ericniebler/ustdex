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

#include <cassert>
#include <type_traits>

#include "preprocessor.hpp"

#ifndef USTDEX_NAMESPACE
#  define USTDEX_NAMESPACE ustdex
#endif

namespace USTDEX_NAMESPACE
{
}

// Make ustdex an alias for USTDEX_NAMESPACE if USTDEX_NAMESPACE is not "ustdex"
#define USTDEX_PP_PROBE_NAMESPACE_DETAIL_ustdex USTDEX_PP_PROBE(~, 1)
#define USTDEX_HAS_DEFAULT_NAMESPACE() \
  USTDEX_PP_CHECK(USTDEX_PP_CAT(USTDEX_PP_PROBE_NAMESPACE_DETAIL_, USTDEX_NAMESPACE))

// Compiler detections:
#if defined(__NVCC__)
#  define USTDEX_NVCC() 1
#elif defined(__NVCOMPILER)
#  define USTDEX_NVHPC() 1
#elif defined(__EDG__)
#  define USTDEX_EDG() 1
#endif

#if defined(__clang__)
#  define USTDEX_CLANG() 1
#  if defined(_MSC_VER)
#    define USTDEX_CLANG_CL() 1
#  endif
#  if defined(__apple_build_version__)
#    define USTDEX_APPLE_CLANG() 1
#  endif
#elif defined(__GNUC__)
#  define USTDEX_GCC() 1
#endif

#if defined(_MSC_VER)
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
#  define USTDEX_PRAGMA_PUSH_EDG()      _Pragma("nv_diagnostic push")
#  define USTDEX_PRAGMA_POP_EDG()       _Pragma("nv_diagnostic pop")
#  define USTDEX_PRAGMA_IGNORE_EDG(...) _Pragma(USTDEX_PP_STRINGIZE(nv_diag_suppress __VA_ARGS__))
#elif USTDEX_NVHPC() || USTDEX_EDG()
#  define USTDEX_PRAGMA_PUSH_EDG()      _Pragma("diagnostic push") USTDEX_PRAGMA_IGNORE_EDG(invalid_error_number)
#  define USTDEX_PRAGMA_POP_EDG()       _Pragma("diagnostic pop")
#  define USTDEX_PRAGMA_IGNORE_EDG(...) _Pragma(USTDEX_PP_STRINGIZE(diag_suppress __VA_ARGS__))
#endif

#if USTDEX_CLANG() || USTDEX_GCC()
#  define USTDEX_PRAGMA_PUSH_GNU()                                                                                     \
    _Pragma("GCC diagnostic push") USTDEX_PRAGMA_IGNORE_GNU("-Wpragmas") USTDEX_PRAGMA_IGNORE_GNU("-Wunknown-pragmas") \
      USTDEX_PRAGMA_IGNORE_GNU("-Wunknown-warning-option") USTDEX_PRAGMA_IGNORE_GNU("-Wunknown-attributes")            \
        USTDEX_PRAGMA_IGNORE_GNU("-Wattributes")
#  define USTDEX_PRAGMA_POP_GNU()       _Pragma("GCC diagnostic pop")
#  define USTDEX_PRAGMA_IGNORE_GNU(...) _Pragma(USTDEX_PP_STRINGIZE(GCC diagnostic ignored __VA_ARGS__))
#endif

#ifndef USTDEX_PRAGMA_PUSH_EDG
#  define USTDEX_PRAGMA_PUSH_EDG()
#  define USTDEX_PRAGMA_POP_EDG()
#  define USTDEX_PRAGMA_IGNORE_EDG(...)
#endif

#ifndef USTDEX_PRAGMA_PUSH_GNU
#  define USTDEX_PRAGMA_PUSH_GNU()
#  define USTDEX_PRAGMA_POP_GNU()
#  define USTDEX_PRAGMA_IGNORE_GNU(...)
#endif

#define USTDEX_PRAGMA_PUSH() \
  USTDEX_PRAGMA_PUSH_EDG()   \
  USTDEX_PRAGMA_PUSH_GNU()

#define USTDEX_PRAGMA_POP() \
  USTDEX_PRAGMA_POP_GNU()   \
  USTDEX_PRAGMA_POP_EDG()

#ifndef USTDEX_CUDA
#  ifdef __CUDACC__
#    define USTDEX_CUDA 1
#  else
#    define USTDEX_CUDA 0
#  endif
#endif

#if defined(__CUDA_ARCH__)
#  define USTDEX_HOST_ONLY() 0
#else
#  define USTDEX_HOST_ONLY() 1
#endif

#if USTDEX_CUDA
#  define USTDEX_DEVICE          __device__
#  define USTDEX_HOST_DEVICE     __host__ __device__
#  define USTDEX_DEVICE_CONSTANT __constant__
#else
#  define USTDEX_DEVICE
#  define USTDEX_HOST_DEVICE
#  define USTDEX_DEVICE_CONSTANT inline
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

#define USTDEX_INLINE USTDEX_ATTR_ALWAYS_INLINE USTDEX_ATTR_ARTIFICIAL USTDEX_ATTR_NODEBUG inline

#if USTDEX_HAS_BUILTIN(__is_same)
#  define USTDEX_IS_SAME(...) __is_same(__VA_ARGS__)
#elif USTDEX_HAS_BUILTIN(__is_same_as)
#  define USTDEX_IS_SAME(...) __is_same_as(__VA_ARGS__)
#else
#  define USTDEX_IS_SAME(...) ustdex::_is_same<__VA_ARGS__>

namespace USTDEX_NAMESPACE
{
template <class T, class U>
inline constexpr bool _is_same = false;
template <class T>
inline constexpr bool _is_same<T, T> = true;
} // namespace USTDEX_NAMESPACE
#endif

#if USTDEX_HAS_BUILTIN(__remove_reference)
namespace USTDEX_NAMESPACE
{
template <class Ty>
using _remove_reference_t = __remove_reference(Ty);
} // namespace USTDEX_NAMESPACE

#  define USTDEX_REMOVE_REFERENCE(...) ustdex::_remove_reference_t<__VA_ARGS__>
#elif USTDEX_HAS_BUILTIN(__remove_reference_t)
namespace USTDEX_NAMESPACE
{
template <class Ty>
using _remove_reference_t = __remove_reference_t(Ty);
} // namespace USTDEX_NAMESPACE

#  define USTDEX_REMOVE_REFERENCE(...) ustdex::_remove_reference_t<__VA_ARGS__>
#else
#  define USTDEX_REMOVE_REFERENCE(...) ::std::remove_reference_t<__VA_ARGS__>
#endif

#if USTDEX_HAS_BUILTIN(__is_base_of) || (_MSC_VER >= 1914)
#  define USTDEX_IS_BASE_OF(...) __is_base_of(__VA_ARGS__)
#else
#  define USTDEX_IS_BASE_OF(...) ::std::is_base_of_v<__VA_ARGS__>
#endif

#ifndef USTDEX_ASSERT
#  define USTDEX_ASSERT assert
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
