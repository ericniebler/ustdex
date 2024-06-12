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

#include "ustdex/detail/config.hpp"
#include "ustdex/detail/continue_on.hpp"
#include "ustdex/detail/just.hpp"
#include "ustdex/detail/let_value.hpp"
#include "ustdex/detail/start_on.hpp"
#include "ustdex/detail/then.hpp"
#include "ustdex/detail/when_all.hpp"

#include <catch2/catch_all.hpp>

#include "../tests/common/checked_receiver.hpp"
#include "../tests/common/error_scheduler.hpp"
#include "../tests/common/impulse_scheduler.hpp"
#include "../tests/common/stopped_scheduler.hpp"
#include "../tests/common/utility.hpp"

namespace ex = ustdex;

namespace {
  TEST_CASE("when_all simple example", "[when_all]") {
    auto snd = ex::when_all(ex::just(3), ex::just(0.1415));
    auto snd1 = std::move(snd) | ex::then([](int x, double y) { return x + y; });
    auto op = ex::connect(std::move(snd1), checked_value_receiver{3.1415});
    ex::start(op);
  }

  TEST_CASE("when_all returning two values can be waited on", "[when_all]") {
    auto snd = ex::when_all(ex::just(2), ex::just(3));
    check_values(std::move(snd), 2, 3);
  }

  TEST_CASE("when_all with 5 senders", "[when_all]") {
    auto snd =
      ex::when_all(ex::just(2), ex::just(3), ex::just(5), ex::just(7), ex::just(11));
    check_values(std::move(snd), 2, 3, 5, 7, 11);
  }

  TEST_CASE("when_all with just one sender", "[when_all]") {
    auto snd = ex::when_all(ex::just(2));
    check_values(std::move(snd), 2);
  }

  TEST_CASE("when_all with move-only types", "[when_all]") {
    auto snd = ex::when_all(ex::just(movable(2)));
    check_values(std::move(snd), movable(2));
  }

  TEST_CASE("when_all with no senders", "[when_all]") {
    auto snd = ex::when_all();
    check_values(std::move(snd));
  }

  TEST_CASE("when_all when one sender sends void", "[when_all]") {
    auto snd = ex::when_all(ex::just(2), ex::just());
    check_values(std::move(snd), 2);
  }

  TEST_CASE("when_all completes when children complete", "[when_all]") {
    impulse_scheduler sched;
    bool called{false};
    auto snd = ex::when_all(
                 ex::just(11) | ex::continue_on(sched),
                 ex::just(13) | ex::continue_on(sched),
                 ex::just(17) | ex::continue_on(sched))
             | ex::then([&](int a, int b, int c) {
                 called = true;
                 return a + b + c;
               });
    auto op = ex::connect(std::move(snd), checked_value_receiver{41});
    ex::start(op);
    // The when_all scheduler will complete only after 3 impulses
    CHECK_FALSE(called);
    sched.start_next();
    CHECK_FALSE(called);
    sched.start_next();
    CHECK_FALSE(called);
    sched.start_next();
    CHECK(called);
  }

  TEST_CASE("when_all can be used with just_*", "[when_all]") {
    auto snd = ex::when_all(ex::just(2), ex::just_error(42), ex::just_stopped());
    auto op = ex::connect(std::move(snd), checked_error_receiver{42});
    ex::start(op);
  }

  TEST_CASE(
    "when_all terminates with error if one child terminates with error",
    "[when_all]") {
    error_scheduler sched{42};
    auto snd =
      ex::when_all(ex::just(2), ex::just(5) | ex::continue_on(sched), ex::just(7));
    auto op = ex::connect(std::move(snd), checked_error_receiver{42});
    ex::start(op);
  }

  TEST_CASE("when_all terminates with stopped if one child is cancelled", "[when_all]") {
    stopped_scheduler sched;
    auto snd =
      ex::when_all(ex::just(2), ex::just(5) | ex::continue_on(sched), ex::just(7));
    auto op = ex::connect(std::move(snd), checked_stopped_receiver{});
    ex::start(op);
  }

  TEST_CASE("when_all cancels remaining children if error is detected", "[when_all]") {
    impulse_scheduler sched;
    error_scheduler err_sched{42};
    bool called1{false};
    bool called3{false};
    bool cancelled{false};
    auto snd = ex::when_all(
      ex::start_on(sched, ex::just()) | ex::then([&] { called1 = true; }),
      ex::start_on(sched, ex::just(5) | ex::continue_on(err_sched)),
      ex::start_on(sched, ex::just()) | ex::then([&] { called3 = true; })
        | ex::let_stopped([&] {
            cancelled = true;
            return ex::just();
          }));
    auto op = ex::connect(std::move(snd), checked_error_receiver{42});
    ex::start(op);
    // The first child will complete; the third one will be cancelled
    CHECK_FALSE(called1);
    CHECK_FALSE(called3);
    sched.start_next(); // start the first child
    CHECK(called1);
    sched.start_next(); // start the second child; this will generate an error
    CHECK_FALSE(called3);
    sched.start_next(); // start the third child
    CHECK_FALSE(called3);
    CHECK(cancelled);
  }

  TEST_CASE("when_all cancels remaining children if cancel is detected", "[when_all]") {
    stopped_scheduler stopped_sched;
    impulse_scheduler sched;
    bool called1{false};
    bool called3{false};
    bool cancelled{false};
    auto snd = ex::when_all(
      ex::start_on(sched, ex::just()) | ex::then([&] { called1 = true; }),
      ex::start_on(sched, ex::just(5) | ex::continue_on(stopped_sched)),
      ex::start_on(sched, ex::just()) | ex::then([&] { called3 = true; })
        | ex::let_stopped([&] {
            cancelled = true;
            return ex::just();
          }));
    auto op = ex::connect(std::move(snd), checked_stopped_receiver{});
    ex::start(op);
    // The first child will complete; the third one will be cancelled
    CHECK_FALSE(called1);
    CHECK_FALSE(called3);
    sched.start_next(); // start the first child
    CHECK(called1);
    sched.start_next(); // start the second child; this will call set_stopped
    CHECK_FALSE(called3);
    sched.start_next(); // start the third child
    CHECK_FALSE(called3);
    CHECK(cancelled);
  }

  template <class... Ts>
  struct just_ref {
    using sender_concept = ex::sender_t;
    auto get_completion_signatures(ustdex::_ignore = {}) const
      -> ex::completion_signatures<ex::set_value_t(Ts &...)>;
  };

  TEST_CASE(
    "when_all has the values_type based on the children, decayed and as rvalue "
    "references",
    "[when_all]") {
    check_value_types<types<int>>(ex::when_all(ex::just(13)));
    check_value_types<types<double>>(ex::when_all(ex::just(3.14)));
    check_value_types<types<int, double>>(ex::when_all(ex::just(3, 0.14)));

    check_value_types<types<>>(ex::when_all(ex::just()));

    check_value_types<types<int, double>>(ex::when_all(ex::just(3), ex::just(0.14)));
    check_value_types<types<int, double, int, double>>(
      ex::when_all(ex::just(3), ex::just(0.14), ex::just(1, 0.4142)));

    // if one child returns void, then the value is simply missing
    check_value_types<types<int, double>>(
      ex::when_all(ex::just(3), ex::just(), ex::just(0.14)));

    // if one child has no value completion, the when_all has no value
    // completion
    check_value_types<>(ex::when_all(ex::just(3), ex::just_stopped(), ex::just(0.14)));

    // if children send references, they get decayed
    check_value_types<types<int, double>>(
      ex::when_all(just_ref<int>(), just_ref<double>()));
  }

  struct throwing_copy {
    throwing_copy() = default;

    throwing_copy(throwing_copy const &) {
    }

    throwing_copy(throwing_copy &&) = default;
  };

  TEST_CASE("when_all has the error_types based on the children", "[when_all]") {
    check_error_types<int>(ex::when_all(ex::just_error(13)));

    check_error_types<double>(ex::when_all(ex::just_error(3.14)));

    check_error_types<>(ex::when_all(ex::just()));

    check_error_types<int, double>(ex::when_all(ex::just_error(3), ex::just_error(0.14)));

    check_error_types<int, double, std::string>(ex::when_all(
      ex::just_error(3), ex::just_error(0.14), ex::just_error(std::string{"err"})));

    check_error_types<std::exception_ptr>(ex::when_all(
      ex::just(13), ex::just_error(std::exception_ptr{}), ex::just_stopped()));

    // if the child sends something with a potentially throwing decay-copy,
    // the when_all has an exception_ptr error completion.
    check_error_types<std::exception_ptr>(ex::when_all(just_ref<throwing_copy>()));
  }

  TEST_CASE("when_all has the sends_stopped == true", "[when_all]") {
    check_sends_stopped<true>(ex::when_all(ex::just(13)));
    check_sends_stopped<true>(ex::when_all(ex::just_error(-1)));
    check_sends_stopped<true>(ex::when_all(ex::just_stopped()));

    check_sends_stopped<true>(ex::when_all(ex::just(3), ex::just(0.14)));
    check_sends_stopped<true>(
      ex::when_all(ex::just(3), ex::just_error(-1), ex::just_stopped()));
  }
} // namespace
