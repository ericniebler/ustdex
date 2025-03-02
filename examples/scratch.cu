/*
 * Copyright (c) 2025 NVIDIA Corporation
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
#include <string>

using namespace ustdex;

struct sink {
  using receiver_concept = receiver_t;

  USTDEX_HOST_DEVICE void set_value() noexcept {
  }

  USTDEX_HOST_DEVICE void set_value(int a) noexcept {
    std::printf("%d\n", a);
  }

  template <class... As>
  USTDEX_HOST_DEVICE void set_value(As &&...) noexcept {
    std::printf("%s\n", "In sink::set_value(auto&&...)");
  }

  USTDEX_HOST_DEVICE void set_error([[maybe_unused]] const std::exception_ptr& eptr) noexcept {
    std::printf("Error\n");
  }

  USTDEX_HOST_DEVICE void set_stopped() noexcept {
  }
};

struct _inline_scheduler {
  using scheduler_concept = scheduler_t;

  template <class Rcvr>
  struct _opstate_t {
    using operation_state_concept = operation_state_t;
    using completion_signatures = ustdex::completion_signatures<set_value_t()>;
    Rcvr rcvr;

    USTDEX_HOST_DEVICE void start() noexcept {
      ustdex::set_value(static_cast<Rcvr&&>(rcvr));
    }
  };

  struct _sndr_t {
    using sender_concept = sender_t;
    using completion_signatures = ustdex::completion_signatures<set_value_t()>;

    template <class Rcvr>
    USTDEX_HOST_DEVICE auto connect(Rcvr rcvr) const noexcept {
      return _opstate_t<Rcvr>{rcvr};
    }
  };

  USTDEX_HOST_DEVICE _sndr_t schedule() noexcept {
    return {};
  }
};

template <class>
[[deprecated]]
void print() {
}

USTDEX_HOST_DEVICE void _main() {
  auto s = start_on(
    _inline_scheduler(),
    then(
      just(1, 2, 3),
      []USTDEX_HOST_DEVICE(int a, int b, int c) {
        std::printf("%d %d %d\n", a, b, c);
        return a + b + c;
      }
    )
  );

  auto o = connect(s, sink{});
  start(o);

  constexpr auto just_plus_one = [](int a) { return just(a + 1); };
  auto s3 = let_value(just(42), just_plus_one);
  auto o3 = connect(s3, sink{});
  start(o3);

  auto s4 = just(42) | then([](int){}) | upon_error([](auto){ /*return 42;*/ });
  auto s5 = when_all(std::move(s4), just(42, 42), just(+""));
  auto o5 = connect(std::move(s5), sink{});
  o5.start();
  // using X = completion_signatures_of_t<decltype(s5)>;
  // print<X>();
}

int main() {
  _main();
}