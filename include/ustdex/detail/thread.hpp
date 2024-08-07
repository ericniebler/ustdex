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

#include <thread>

#if defined(__CUDACC__)
#  include <nv/target>
#  define USTDEX_FOR_HOST_OR_DEVICE(FOR_HOST, FOR_DEVICE)                                \
    NV_IF_TARGET(NV_IS_HOST, FOR_HOST, FOR_DEVICE)
#else
#  define USTDEX_FOR_HOST_OR_DEVICE(FOR_HOST, FOR_DEVICE)                                \
    { USTDEX_PP_EXPAND FOR_HOST }
#endif

#if __has_include(<xmmintrin.h>)
#  include <xmmintrin.h>
#  define USTDEX_PAUSE() USTDEX_FOR_HOST_OR_DEVICE((_mm_pause();), (void();))
#elif defined(_MSC_VER)
#  include <intrin.h>
#  define USTDEX_PAUSE() USTDEX_FOR_HOST_OR_DEVICE((_mm_pause()), (void()))
#else
#  define USTDEX_PAUSE() __asm__ __volatile__("pause")
#endif

namespace USTDEX_NAMESPACE
{
#if defined(__CUDA_ARCH__)
using _thread_id = int;
#elif USTDEX_NVHPC()
struct _thread_id
{
  union
  {
    ::std::thread::id _host;
    int _device;
  };

  USTDEX_HOST_DEVICE _thread_id() noexcept
      : _host()
  {}
  USTDEX_HOST_DEVICE _thread_id(::std::thread::id host) noexcept
      : _host(host)
  {}
  USTDEX_HOST_DEVICE _thread_id(int device) noexcept
      : _device(device)
  {}

  USTDEX_HOST_DEVICE friend bool operator==(const _thread_id& self, const _thread_id& other) noexcept
  {
    USTDEX_FOR_HOST_OR_DEVICE((return self._host == other._host;), (return self._device == other._device;))
  }

  USTDEX_HOST_DEVICE friend bool operator!=(const _thread_id& self, const _thread_id& other) noexcept
  {
    return !(self == other);
  }
};
#else
using _thread_id = ::std::thread::id;
#endif

inline USTDEX_HOST_DEVICE _thread_id _this_thread_id() noexcept
{
  USTDEX_FOR_HOST_OR_DEVICE((return ::std::this_thread::get_id();),
                            (return static_cast<int>(threadIdx.x + blockIdx.x * blockDim.x);))
}

inline USTDEX_HOST_DEVICE void _this_thread_yield() noexcept
{
  USTDEX_FOR_HOST_OR_DEVICE((::std::this_thread::yield();), (void();))
}
} // namespace USTDEX_NAMESPACE
