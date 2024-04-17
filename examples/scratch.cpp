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

#include "ustdex/ustdex.hpp"

#include <cstdio>
#include <iostream>
#include <string>

using namespace ustdex;

struct sink {
  using receiver_concept = receiver_t;

  void set_value() noexcept {
  }

  void set_value(int a) noexcept {
    std::printf("%d\n", a);
  }

  template <class... As>
  void set_value(As &&...) noexcept {
    std::puts("In sink::set_value(auto&&...)");
  }

  void set_error(std::exception_ptr) noexcept {
  }

  void set_stopped() noexcept {
  }
};

template <class>
[[deprecated]]
void print() {
}

int main() {
  // thread_context ctx;
  // auto sch = ctx.get_scheduler();

  // auto work = just(1, 2, 3) //
  //           | then([](int a, int b, int c) {
  //               std::printf("%d %d %d\n", a, b, c);
  //               return a + b + c;
  //             });
  // auto s = start_on(sch, std::move(work));
  // std::puts("Hello, world!");
  // sync_wait(s);

  // auto s3 = just(42) | let_value([](int a) {
  //             std::puts("here");
  //             return just(a + 1);
  //           });
  // sync_wait(s3);

  // auto [sch2] = sync_wait(read_env(get_scheduler)).value();

  auto s4 = just(42) | then([](int) {}) | upon_error([](auto) { /*return 42;*/ });
  auto s5 = when_all(std::move(s4), just(42, 43), just(+"hello"));
  using X = completion_signatures_of_t<decltype(s5)>;
  //print<X>();
  auto [i, j, k] = sync_wait(std::move(s5)).value();
  // std::cout << i << ' ' << j << ' ' << k << '\n';

  // auto s1 = when_all(just(42), just(43, 44), just(45, 46, 47)); //
  // auto s2 = when_all(s1, s1, s1, s1, s1, s1);
  // auto s3 = when_all(s2, s2, s2, s2, s2, s2, s2);
  // print<decltype(s3)>();
  //(void) sync_wait(s3);
}
