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

#include "run_loop.hpp"

#include <thread>

namespace ustdex {
  struct thread_context {
    thread_context() noexcept
      : _thread{[this] {
        _loop.run();
      }} {
    }

    ~thread_context() noexcept {
      join();
    }

    void join() noexcept {
      _loop.finish();
      _thread.join();
    }

    auto get_scheduler() {
      return _loop.get_scheduler();
    }

   private:
    std::thread _thread;
    run_loop _loop;
  };
} // namespace ustdex