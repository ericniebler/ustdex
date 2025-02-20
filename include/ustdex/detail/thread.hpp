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

#ifndef USTDEX_ASYNC_DETAIL_THREAD
#define USTDEX_ASYNC_DETAIL_THREAD

#include "config.hpp"

#include <thread>

#if USTDEX_CUDA()
#  include <nv/target>
#  define USTDEX_FOR_HOST_OR_DEVICE(FOR_HOST, FOR_DEVICE) NV_IF_TARGET(NV_IS_HOST, FOR_HOST, FOR_DEVICE)
#else
#  define USTDEX_FOR_HOST_OR_DEVICE(FOR_HOST, FOR_DEVICE) {USTDEX_PP_EXPAND FOR_HOST}
#endif

namespace ustdex
{
#if USTDEX_NVHPC()
struct _thread_id
{
  union
  {
    ::std::thread::id _host_;
    int _device_;
  };

  USTDEX_API _thread_id() noexcept
      : _host_()
  {}
  USTDEX_API _thread_id(::std::thread::id __host) noexcept
      : _host_(__host)
  {}
  USTDEX_API _thread_id(int _device) noexcept
      : _device_(_device)
  {}

  USTDEX_API friend bool operator==(const _thread_id& _self, const _thread_id& _other) noexcept
  {
    USTDEX_IF_ELSE_TARGET(
      USTDEX_IS_HOST, (return _self._host_ == _other._host_;), (return _self._device_ == _other._device_;))
  }

  USTDEX_API friend bool operator!=(const _thread_id& _self, const _thread_id& _other) noexcept
  {
    return !(_self == _other);
  }
};
#elif USTDEX_CUDA()
using _thread_id = int;
#else
using _thread_id = ::std::thread::id;
#endif

inline USTDEX_API _thread_id _this_thread_id() noexcept
{
  USTDEX_IF_ELSE_TARGET(USTDEX_IS_HOST,
                        (return ::std::this_thread::get_id();),
                        (return static_cast<int>(threadIdx.x + blockIdx.x * blockDim.x);))
}

inline USTDEX_API void _this_thread_yield() noexcept
{
  USTDEX_IF_ELSE_TARGET(USTDEX_IS_HOST, (::std::this_thread::yield();), (void();))
}
} // namespace ustdex

#endif
