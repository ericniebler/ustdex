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

#include <cstdio>
#include <iostream>

#include "ustdex/ustdex.hpp"

using namespace ustdex;

struct sink
{
  using receiver_concept = receiver_t;

  void set_value() noexcept {}

  void set_value(int a) noexcept
  {
    std::printf("%d\n", a);
  }

  template <class... As>
  void set_value(As&&...) noexcept
  {
    std::puts("In sink::set_value(auto&&...)");
  }

  void set_error(std::exception_ptr) noexcept {}

  void set_stopped() noexcept {}
};

template <class>
[[deprecated]] void print()
{}

static_assert(dependent_sender<decltype(read_env(_empty()))>);

int main()
{
  thread_context ctx;
  auto sch  = ctx.get_scheduler();

  auto work = just(1, 2, 3) //
            | then([](int a, int b, int c) {
                std::printf("%d %d %d\n", a, b, c);
                return a + b + c;
              });
  auto s = start_on(sch, std::move(work));
  static_assert(!dependent_sender<decltype(s)>);
  std::puts("Hello, world!");
  sync_wait(s);

  auto s3 = just(42) | let_value([](int a) {
              std::puts("here");
              return just(a + 1);
            });
  sync_wait(s3);

  auto [sch2]   = sync_wait(read_env(get_scheduler)).value();

  auto [i1, i2] = sync_wait(when_all(just(42), just(43))).value();
  std::cout << i1 << ' ' << i2 << '\n';

  auto s4        = just(42) | then([](int) {}) | upon_error([](auto) { /*return 42;*/ });
  auto s5        = when_all(std::move(s4), just(42, 43), just(+"hello"));
  auto [i, j, k] = sync_wait(std::move(s5)).value();
  std::cout << i << ' ' << j << ' ' << k << '\n';

  auto s6 = sequence(just(42) | then([](int) {
                       std::cout << "sequence sender 1\n";
                     }),
                     just(42) | then([](int) {
                       std::cout << "sequence sender 2\n";
                     }));
  sync_wait(std::move(s6));

  auto s7 =
    just(42)
    | conditional(
      [](int i) {
        return i % 2 == 0;
      },
      then([](int) {
        std::cout << "even\n";
      }),
      then([](int) {
        std::cout << "odd\n";
      }));
  sync_wait(std::move(s7));
}
