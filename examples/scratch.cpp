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

#include "ustdex/detail/sexpr.hpp"
#include "ustdex/detail/variant.hpp"
#include "ustdex/ustdex.hpp"

#include <cstdio>
#include <string>

using namespace ustdex;

struct sink {
  using receiver_concept = receiver_t;

  void set_value() noexcept {
  }

  void set_value(int a) noexcept {
    std::printf("%d\n", a);
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
  run_loop loop;

  auto work = just(1, 2, 3) //
            | then([](int a, int b, int c) {
                      std::printf("%d %d %d\n", a, b, c);
                      return a + b + c;
                    });
  auto s = start_on(loop.get_scheduler(), std::move(work));
  auto o = connect(s, sink{});
  start(o);

  std::optional<std::tuple<int>> x = sync_wait(just(42));

  std::puts("Hello, world!");
  loop.finish();
  loop.run();

  // auto s2 = continue_on(just(42), loop.get_scheduler());
  // auto o2 = connect(s2, sink{});
  // start(o2);

  auto s3 = just(42) | let_value([](int a) {
    std::puts("here");
    return just(a + 1);
  });
  auto o3 = connect(s3, sink{});
  start(o3);

  // _variant<int, std::string, _tuple_for<int, int>> v{};
  // v.emplace<_tuple_for<int, int>>(1,2);
  // auto& tup = v.get<2>();
  // _sexpr t{set_value, 1, just()};
  // auto [a,b,c] = t;
}
