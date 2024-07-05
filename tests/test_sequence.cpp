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

// Include this first
#include <ustdex/ustdex.hpp>

// Then include the test helpers
#include "common/catch2.hpp"
#include "common/checked_receiver.hpp"
#include "common/error_scheduler.hpp"
#include "common/impulse_scheduler.hpp"
#include "common/inline_scheduler.hpp"
#include "common/stopped_scheduler.hpp"
#include "common/utility.hpp"

namespace ex = USTDEX_NAMESPACE;

namespace
{
TEST_CASE("simple use of sequence executes both child operations", "[adaptors][sequence]")
{
  bool flag1{false};
  bool flag2{false};

  auto sndr1 = ex::sequence(
    ex::just() | ex::then([&] {
      flag1 = true;
    }),
    ex::just() | ex::then([&] {
      flag2 = true;
    }));

  check_value_types<types<>>(sndr1);
  check_error_types<std::exception_ptr>(sndr1);
  check_sends_stopped<false>(sndr1);

  auto op = ex::connect(std::move(sndr1), checked_value_receiver<>{});
  ex::start(op);

  CHECK(flag1);
  CHECK(flag2);
}

} // namespace
