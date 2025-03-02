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

#ifndef USTDEX_ASYNC_DETAIL_THREAD_CONTEXT
#define USTDEX_ASYNC_DETAIL_THREAD_CONTEXT

#include "config.hpp"

#if !defined(__CUDA_ARCH__)

#  include "run_loop.hpp"

#  include <thread>

#  include "prologue.hpp"

namespace ustdex
{
struct USTDEX_TYPE_VISIBILITY_DEFAULT thread_context
{
  thread_context() noexcept
      : _thrd_{[this] {
        _loop_.run();
      }}
  {}

  ~thread_context() noexcept
  {
    join();
  }

  void join() noexcept
  {
    if (_thrd_.joinable())
    {
      _loop_.finish();
      _thrd_.join();
    }
  }

  auto get_scheduler()
  {
    return _loop_.get_scheduler();
  }

private:
  run_loop _loop_;
  ::std::thread _thrd_;
};
} // namespace ustdex

#  include "epilogue.hpp"

#endif // !defined(__CUDA_ARCH__)

#endif
