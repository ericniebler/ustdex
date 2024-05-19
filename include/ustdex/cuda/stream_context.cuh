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

#include "ustdex/detail/config.hpp"

#ifndef USTDEX_CUDA
#  error "This file should only be included in a CUDA compilation unit"
#endif

#if defined(_CUDA_MEMORY_RESOURCE) && !defined(LIBCUDACXX_ENABLE_EXPERIMENTAL_MEMORY_RESOURCE)
#  error "This file requires the experimental memory resource feature to be enabled"
#endif

#if !defined(LIBCUDACXX_ENABLE_EXPERIMENTAL_MEMORY_RESOURCE)
#  define LIBCUDACXX_ENABLE_EXPERIMENTAL_MEMORY_RESOURCE
#endif

// from libcu++:
#include <cuda/memory_resource>
#include <cuda/stream_ref>

// from the CUDA Toolkit:
#include <cuda_runtime.h>


namespace ustdex {

  struct stream_scheduler {};

  enum class stream_priority { low, normal, high };

  struct stream_context {
   public:
    stream_context()
      : _dev_id(_get_device())
      //, _hub(dev_id_, _pinned.get())
    {
    }

    stream_scheduler get_scheduler(stream_priority priority = stream_priority::normal) const {
      // return {context_state_t(
      //   _pinned.get(), _managed.get(), &_stream_pools, &_hub, priority)};
      return {};
    }

   private:
    // resource_storage<pinned_resource> _pinned{};
    // resource_storage<managed_resource> _managed{};
    // stream_pools_t stream_pools_{};

    static int _get_device() noexcept {
      int dev_id{};
      cudaGetDevice(&dev_id);
      return dev_id;
    }

    int _dev_id{};
    // queue::task_hub_t hub_;
  };

} // namespace ustdex
