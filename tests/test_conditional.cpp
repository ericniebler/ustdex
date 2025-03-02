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

// Include this first
#include <ustdex/ustdex.hpp>

// Then include the test helpers
#include "common/catch2.hpp" // IWYU pragma: keep
#include "common/checked_receiver.hpp"
#include "common/utility.hpp"

namespace ex = ustdex;

namespace
{
TEST_CASE("simple use of conditional runs exactly one of the two closures", "[adaptors][conditional]")
{
  for (int i = 42; i < 44; ++i)
  {
    bool even{false};
    bool odd{false};

    auto sndr1 =
      ex::just(i)
      | ex::conditional(
        [](int i) {
          return i % 2 == 0;
        },
        ex::then([&](int) {
          even = true;
        }),
        ex::then([&](int) {
          odd = true;
        }));

    check_value_types<_m_list<>>(sndr1);
    check_error_types<std::exception_ptr>(sndr1);
    check_sends_stopped<false>(sndr1);

    auto op = ex::connect(std::move(sndr1), checked_value_receiver<>{});
    ex::start(op);

    CHECK(even == (i % 2 == 0));
    CHECK(odd == (i % 2 == 1));
  }
}

} // namespace
