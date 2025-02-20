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

#include <cassert>
#include <type_traits> // IWYU pragma: keep

#define USTDEX_PP_STRINGIZE(...) #__VA_ARGS__

// Arch detections:
// Arm 64-bit
#if (defined(__aarch64__) || defined(_M_ARM64) || defined(_M_ARM64EC) /*emulation*/)
#  define USTDEX_ARCH_ARM64_() 1
#else
#  define USTDEX_ARCH_ARM64_() 0
#endif

// X86 64-bit

// _M_X64 is defined even if we are compiling in Arm64 emulation mode
#if (defined(_M_X64) && !defined(_M_ARM64EC)) || defined(__amd64__) || defined(__x86_64__)
#  define USTDEX_ARCH_X86_64_() 1
#else
#  define USTDEX_ARCH_X86_64_() 0
#endif

#define USTDEX_ARCH(...) USTDEX_ARCH_##__VA_ARGS__##_()

// Compiler detections:
#if defined(__NVCC__)
#  define USTDEX_NVCC() 1
#elif defined(__NVCOMPILER)
#  define USTDEX_NVHPC() 1
#elif defined(__EDG__)
#  define USTDEX_EDG() 1
#endif

#if defined(_clang__)
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

#if USTDEX_MSVC()
#  define USTDEX_PRAGMA_PUSH_MSVC()      __pragma(warning(push))
#  define USTDEX_PRAGMA_POP_MSVC()       __pragma(warning(pop))
#  define USTDEX_PRAGMA_IGNORE_MSVC(...) __pragma(warning(disable : __VA_ARGS__))
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

#ifndef USTDEX_PRAGMA_PUSH_MSVC
#  define USTDEX_PRAGMA_PUSH_MSVC()
#  define USTDEX_PRAGMA_POP_MSVC()
#  define USTDEX_PRAGMA_IGNORE_MSVC(...)
#endif

#define USTDEX_PRAGMA_PUSH() \
  USTDEX_PRAGMA_PUSH_EDG()   \
  USTDEX_PRAGMA_PUSH_GNU()   \
  USTDEX_PRAGMA_PUSH_MSVC()

#define USTDEX_PRAGMA_POP() \
  USTDEX_PRAGMA_POP_MSVC()  \
  USTDEX_PRAGMA_POP_GNU()   \
  USTDEX_PRAGMA_POP_EDG()

#ifdef USTDEX_CUDA
#  if USTDEX_CUDA
#    undef USTDEX_CUDA
#    define USTDEX_CUDA() 1
#  else
#    undef USTDEX_CUDA
#    define USTDEX_CUDA() 0
#  endif
#else
#  ifdef __CUDACC__
#    define USTDEX_CUDA() 1
#  else
#    define USTDEX_CUDA() 0
#  endif
#endif

#if defined(__CUDA_ARCH__)
#  define USTDEX_HOST_ONLY() 0
#else
#  define USTDEX_HOST_ONLY() 1
#endif

#if USTDEX_CUDA()
#  define USTDEX_HOST            __host__
#  define USTDEX_DEVICE          __device__
#  define USTDEX_HOST_DEVICE     __host__ __device__
#  define USTDEX_DEVICE_CONSTANT __constant__
#else
#  define USTDEX_HOST
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

#if USTDEX_HAS_ATTRIBUTE(_exclude_from_explicit_instantiation__)
#  define USTDEX_ATTR_EXCLUDE_FROM_EXPLICIT_INSTANTIATION __attribute__((_exclude_from_explicit_instantiation__))
#else
#  define USTDEX_ATTR_EXCLUDE_FROM_EXPLICIT_INSTANTIATION
#endif

// The nodebug attribute flattens aliases down to the actual type rather typename meow<T>::type
#if USTDEX_CLANG()
#  define USTDEX_ATTR_NODEBUG_ALIAS USTDEX_ATTR_NODEBUG
#else
#  define USTDEX_ATTR_NODEBUG_ALIAS
#endif

#if USTDEX_MSVC() // || USTDEX_NVRTC()
#  define USTDEX_ATTR_VISIBILITY_HIDDEN
#else
#  define USTDEX_ATTR_VISIBILITY_HIDDEN __attribute__((__visibility__("hidden")))
#endif

#if USTDEX_MSVC()
#  define USTDEX_FORCEINLINE __forceinline
#else
#  define USTDEX_FORCEINLINE __inline__ __attribute__((__always_inline__))
#endif

// - `USTDEX_API` declares the function host/device and hides the symbol from
//   the ABI
// - `USTDEX_TRIVIAL_API` does the same while also forcing inlining and hiding
//   the function from debuggers
#define USTDEX_API        USTDEX_HOST_DEVICE USTDEX_ATTR_VISIBILITY_HIDDEN USTDEX_ATTR_EXCLUDE_FROM_EXPLICIT_INSTANTIATION
#define USTDEX_HOST_API   USTDEX_HOST USTDEX_ATTR_VISIBILITY_HIDDEN USTDEX_ATTR_EXCLUDE_FROM_EXPLICIT_INSTANTIATION
#define USTDEX_DEVICE_API USTDEX_DEVICE USTDEX_ATTR_VISIBILITY_HIDDEN USTDEX_ATTR_EXCLUDE_FROM_EXPLICIT_INSTANTIATION

// USTDEX_TRIVIAL_API force-inlines a function, marks its visibility as hidden, and causes debuggers to skip it.
// This is useful for trivial internal functions that do dispatching or other plumbing work. It is particularly
// useful in the definition of customization point objects.
#define USTDEX_TRIVIAL_API        USTDEX_API USTDEX_FORCEINLINE USTDEX_ATTR_ARTIFICIAL USTDEX_ATTR_NODEBUG
#define USTDEX_TRIVIAL_HOST_API   USTDEX_HOST_API USTDEX_FORCEINLINE USTDEX_ATTR_ARTIFICIAL USTDEX_ATTR_NODEBUG
#define USTDEX_TRIVIAL_DEVICE_API USTDEX_DEVICE_API USTDEX_FORCEINLINE USTDEX_ATTR_ARTIFICIAL USTDEX_ATTR_NODEBUG

#if USTDEX_HAS_BUILTIN(__is_same)
#  define USTDEX_IS_SAME(...) __is_same(__VA_ARGS__)
#elif USTDEX_HAS_BUILTIN(__is_same_as)
#  define USTDEX_IS_SAME(...) __is_same_as(__VA_ARGS__)
#else
#  define USTDEX_IS_SAME(...) ustdex::_is_same<__VA_ARGS__>

namespace ustdex
{
template <class T, class U>
inline constexpr bool _is_same = false;
template <class T>
inline constexpr bool _is_same<T, T> = true;
} // namespace ustdex
#endif

#if USTDEX_HAS_BUILTIN(_decay)
#  define USTDEX_DECAY(...) _identity_t<_decay(__VA_ARGS__)>
#else
#  define USTDEX_DECAY(...) std::decay_t<__VA_ARGS__>
#endif

#if USTDEX_HAS_BUILTIN(__remove_reference)
namespace ustdex
{
template <class Ty>
using _remove_reference_t = __remove_reference(Ty);
} // namespace ustdex

#  define USTDEX_REMOVE_REFERENCE(...) ustdex::_remove_reference_t<__VA_ARGS__>
#elif USTDEX_HAS_BUILTIN(__remove_reference_t)
namespace ustdex
{
template <class Ty>
using _remove_reference_t = __remove_reference_t(Ty);
} // namespace ustdex

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
#  define USTDEX_ASSERT(X, Y) assert(X)
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

#if defined(__cpp_consteval) && __cpp_consteval >= 201811L
#  define USTDEX_CONSTEVAL consteval
#else
#  define USTDEX_CONSTEVAL constexpr
#endif

#if USTDEX_MSVC()
#  define USTDEX_VISIBILITY_DEFAULT _declspec(dllimport)
// #elif USTDEX_NVRTC()
// #  define USTDEX_VISIBILITY_DEFAULT
#else
#  define USTDEX_VISIBILITY_DEFAULT __attribute__((__visibility__("default")))
#endif

#if USTDEX_MSVC() // || USTDEX_NVRTC()
#  define USTDEX_TYPE_VISIBILITY_DEFAULT
#elif USTDEX_HAS_ATTRIBUTE(_m_visibility__)
#  define USTDEX_TYPE_VISIBILITY_DEFAULT __attribute__((__visibility__("default")))
#else
#  define USTDEX_TYPE_VISIBILITY_DEFAULT USTDEX_VISIBILITY_DEFAULT
#endif

#if __has_include(<nv/target>)
#  include <nv/target>
#  define USTDEX_IS_HOST        NV_IS_HOST
#  define USTDEX_IS_DEVICE      NV_IS_DEVICE
#  define USTDEX_IF_TARGET      NV_IF_TARGET
#  define USTDEX_IF_ELSE_TARGET NV_IF_ELSE_TARGET
#else
#  define USTDEX_IS_HOST   1
#  define USTDEX_IS_DEVICE 0
#  define USTDEX_IF_TARGET_0(Y)
#  define USTDEX_IF_TARGET_1(Y)          USTDEX_PP_EXPAND Y
#  define USTDEX_IF_TARGET(X, Y)         USTDEX_PP_CAT(USTDEX_IF_TARGET_, X)(Y)
#  define USTDEX_IF_ELSE_TARGET_0(Y, Z)  USTDEX_PP_EXPAND Z
#  define USTDEX_IF_ELSE_TARGET_1(Y, Z)  USTDEX_PP_EXPAND Y
#  define USTDEX_IF_ELSE_TARGET(X, Y, Z) USTDEX_PP_CAT(USTDEX_IF_ELSE_TARGET_, X)(Y, Z)
#endif

#define USTDEX_TYPEID typeid

namespace ustdex
{
}
