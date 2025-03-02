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

#pragma once

#include "config.hpp"

#include <algorithm> // IWYU pragma: keep
#include <functional>

namespace ustdex
{
#if __cplusplus >= 202002L
// exclusive_scan is constexpr in C++20
using std::exclusive_scan;
#else
template <class InputIterator, class OutputIterator, class Tp, class BinaryOp>
USTDEX_API constexpr OutputIterator
exclusive_scan(InputIterator first, InputIterator last, OutputIterator result, Tp init, BinaryOp b)
{
  if (first != last)
  {
    Tp tmp(b(init, *first));
    while (true)
    {
      *result = static_cast<Tp&&>(init);
      ++result;
      ++first;
      if (first == last)
      {
        break;
      }
      init = static_cast<Tp&&>(tmp);
      tmp  = b(init, *first);
    }
  }
  return result;
}

template <class InputIterator, class OutputIterator, class Tp>
USTDEX_API constexpr OutputIterator
exclusive_scan(InputIterator first, InputIterator last, OutputIterator result, Tp init)
{
  return ustdex::exclusive_scan(first, last, result, init, std::plus<>());
}
#endif
} // namespace ustdex
