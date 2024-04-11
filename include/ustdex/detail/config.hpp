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

#ifdef __CUDACC__
#  define USTDEX_DEVICE __device__
#  define USTDEX_HOST_DEVICE __host__ __device__
#  define USTDEX_TRY
#  define USTDEX_CATCH(...) if constexpr (false)
#else
#  define USTDEX_DEVICE
#  define USTDEX_HOST_DEVICE
#  define USTDEX_TRY try
#  define USTDEX_CATCH catch
#endif

#if __has_attribute(__nodebug__)
#  define USTDEX_ATTR_NODEBUG __attribute__((__nodebug__))
#else
#  define USTDEX_ATTR_NODEBUG
#endif

#if __has_attribute(__artificial__) && !defined(__CUDACC__)
#  define USTDEX_ATTR_ARTIFICIAL __attribute__((__artificial__))
#else
#  define USTDEX_ATTR_ARTIFICIAL
#endif

#if __has_attribute(__always_inline__)
#  define USTDEX_ATTR_ALWAYS_INLINE __attribute__((__always_inline__))
#else
#  define USTDEX_ATTR_ALWAYS_INLINE
#endif

#  define USTDEX_INLINE                                                                  \
    USTDEX_ATTR_ALWAYS_INLINE USTDEX_ATTR_ARTIFICIAL USTDEX_ATTR_NODEBUG inline

#if __has_builtin(__is_same)
#  define USTDEX_IS_SAME(...) __is_same(__VA_ARGS__)
#elif __has_builtin(__is_same_as)
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

#if __has_builtin(__remove_reference)
namespace ustdex {
  template <class Ty>
  using _remove_reference_t = __remove_reference(Ty);
} // namespace ustdex
#  define USTDEX_REMOVE_REFERENCE(...) ustdex::_remove_reference_t<__VA_ARGS__>
#elif __has_builtin(__remove_reference_t)
namespace ustdex {
  template <class Ty>
  using _remove_reference_t = __remove_reference_t(Ty);
} // namespace ustdex
#  define USTDEX_REMOVE_REFERENCE(...) ustdex::_remove_reference_t<__VA_ARGS__>
#else
#  define USTDEX_REMOVE_REFERENCE(...) std::remove_reference_t<__VA_ARGS__>
#endif

#if __has_builtin(__is_base_of)
#  define USTDEX_IS_BASE_OF(...) __is_base_of(__VA_ARGS__)
#else
#  define USTDEX_IS_BASE_OF(...) std::is_base_of_v<__VA_ARGS__>
#endif

#ifndef USTDEX_ASSERT
#  define USTDEX_ASSERT assert
#endif
