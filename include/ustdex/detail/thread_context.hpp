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

#if USTDEX_HOST_ONLY()

#  include <thread>

#  include "run_loop.hpp"

// Must be the last include
#  include "prologue.hpp"

namespace USTDEX_NAMESPACE
{
struct thread_context
{
  thread_context() noexcept
      : _thread{[this] {
        _loop.run();
      }}
  {}

  ~thread_context() noexcept
  {
    join();
  }

  void join() noexcept
  {
    if (_thread.joinable())
    {
      _loop.finish();
      _thread.join();
    }
  }

  auto get_scheduler()
  {
    return _loop.get_scheduler();
  }

private:
  run_loop _loop;
  ::std::thread _thread;
};
} // namespace USTDEX_NAMESPACE

#  include "epilogue.hpp"

#endif // USTDEX_HOST_ONLY()
