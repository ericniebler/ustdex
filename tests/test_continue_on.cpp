/*
 * Copyright (c) 2022 Lucian Radu Teodorescu
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

#include <ustdex/ustdex.hpp>

#include "../tests/common/checked_receiver.hpp"
#include "../tests/common/error_scheduler.hpp"
#include "../tests/common/impulse_scheduler.hpp"
#include "../tests/common/inline_scheduler.hpp"
#include "../tests/common/stopped_scheduler.hpp"
#include "../tests/common/utility.hpp"

#include <catch2/catch_all.hpp>

#include <stdexcept>
#include <string>

namespace ex = ustdex;

namespace {

  // TEST_CASE("continue_on returns a sender", "[adaptors][continue_on]") {
  //   auto snd = ex::continue_on(ex::just(13), inline_scheduler{});
  //   static_assert(ex::sender<decltype(snd)>);
  //   (void) snd;
  // }

  // TEST_CASE("continue_on with environment returns a sender", "[adaptors][continue_on]") {
  //   auto snd = ex::continue_on(ex::just(13), inline_scheduler{});
  //   static_assert(ex::sender_in<decltype(snd), env<>>);
  //   (void) snd;
  // }

  TEST_CASE("continue_on simple example", "[adaptors][continue_on]") {
    auto snd = ex::continue_on(ex::just(13), inline_scheduler{});
    auto op = ex::connect(std::move(snd), checked_value_receiver{13});
    ex::start(op);
    // The receiver checks if we receive the right value
  }

  TEST_CASE("continue_on can be piped", "[adaptors][continue_on]") {
    // Just continue_on a value to the impulse scheduler
    bool called{false};
    auto sched = impulse_scheduler{};
    auto snd = ex::just(13)           //
             | ex::continue_on(sched) //
             | ex::then([&](int val) { called = true; return val; });
    // Start the operation
    auto op = ex::connect(std::move(snd), checked_value_receiver{13});
    ex::start(op);

    // The value will be available when the scheduler will execute the next operation
    REQUIRE(!called);
    sched.start_next();
    REQUIRE(called);
  }

  TEST_CASE("continue_on calls the receiver when the scheduler dictates", "[adaptors][continue_on]") {
    bool called{false};
    impulse_scheduler sched;
    auto snd = ex::then(ex::continue_on(ex::just(13), sched), [&](int val) { called = true; return val; });
    auto op = ex::connect(snd, checked_value_receiver{13});
    ex::start(op);
    // Up until this point, the scheduler didn't start any task; no effect expected
    CHECK(!called);

    // Tell the scheduler to start executing one task
    sched.start_next();
    CHECK(called);
  }

  TEST_CASE("continue_on calls the given sender when the scheduler dictates", "[adaptors][continue_on]") {
    int counter{0};
    auto snd_base = ex::just() //
                  | ex::then([&]() -> int {
                      ++counter;
                      return 19;
                    });

    impulse_scheduler sched;
    auto snd = ex::then(ex::continue_on(std::move(snd_base), sched), [&](int val) { ++counter; return val; });
    auto op = ex::connect(std::move(snd), checked_value_receiver{19});
    ex::start(op);
    // The sender is started, even if the scheduler hasn't yet triggered
    CHECK(counter == 1);
    // ... but didn't send the value to the receiver yet

    // Tell the scheduler to start executing one task
    sched.start_next();

    // Now the base sender is called, and a value is sent to the receiver
    CHECK(counter == 2);
  }

  TEST_CASE("continue_on works when changing threads", "[adaptors][continue_on]") {
    ex::thread_context thread;
    bool called{false};

    {
      // lunch some work on the thread pool
      auto snd = ex::continue_on(ex::just(), thread.get_scheduler()) //
               | ex::then([&] { called = true; });
      ex::start_detached(std::move(snd));
    }

    thread.join();

    // the work should be executed
    REQUIRE(called);
  }

  TEST_CASE("continue_on can be called with rvalue ref scheduler", "[adaptors][continue_on]") {
    auto snd = ex::continue_on(ex::just(13), inline_scheduler{});
    auto op = ex::connect(std::move(snd), checked_value_receiver{13});
    ex::start(op);
    // The receiver checks if we receive the right value
  }

  TEST_CASE("continue_on can be called with const ref scheduler", "[adaptors][continue_on]") {
    const inline_scheduler sched;
    auto snd = ex::continue_on(ex::just(13), sched);
    auto op = ex::connect(std::move(snd), checked_value_receiver{13});
    ex::start(op);
    // The receiver checks if we receive the right value
  }

  TEST_CASE("continue_on can be called with ref scheduler", "[adaptors][continue_on]") {
    inline_scheduler sched;
    auto snd = ex::continue_on(ex::just(13), sched);
    auto op = ex::connect(std::move(snd), checked_value_receiver{13});
    ex::start(op);
    // The receiver checks if we receive the right value
  }

  TEST_CASE("continue_on forwards set_error calls", "[adaptors][continue_on]") {
    auto eptr = std::make_exception_ptr(std::runtime_error{"error"});
    error_scheduler<std::exception_ptr> sched{eptr};
    auto snd = ex::continue_on(ex::just(13), sched);
    auto op = ex::connect(std::move(snd), checked_error_receiver{eptr});
    ex::start(op);
    // The receiver checks if we receive an error
  }

  TEST_CASE("continue_on forwards set_error calls of other types", "[adaptors][continue_on]") {
    error_scheduler<std::string> sched{std::string{"error"}};
    auto snd = ex::continue_on(ex::just(13), sched);
    auto op = ex::connect(std::move(snd), checked_error_receiver{std::string{"error"}});
    ex::start(op);
    // The receiver checks if we receive an error
  }

  TEST_CASE("continue_on forwards set_stopped calls", "[adaptors][continue_on]") {
    stopped_scheduler sched{};
    auto snd = ex::continue_on(ex::just(13), sched);
    auto op = ex::connect(std::move(snd), checked_stopped_receiver{});
    ex::start(op);
    // The receiver checks if we receive the stopped signal
  }

  TEST_CASE(
    "continue_on has the values_type corresponding to the given values",
    "[adaptors][continue_on]") {
    inline_scheduler sched{};

    check_value_types<types<int>>(ex::continue_on(ex::just(1), sched));
    check_value_types<types<int, double>>(ex::continue_on(ex::just(3, 0.14), sched));
    check_value_types<types<int, double, std::string>>(
      ex::continue_on(ex::just(3, 0.14, std::string{"pi"}), sched));
  }

  TEST_CASE("continue_on keeps error_types from scheduler's sender", "[adaptors][continue_on]") {
    inline_scheduler sched1{};
    error_scheduler<std::exception_ptr> sched2{std::make_exception_ptr(std::runtime_error{"error"})};
    error_scheduler<int> sched3{43};

    check_error_types<>(ex::continue_on(ex::just(1), sched1));
    check_error_types<std::exception_ptr>(ex::continue_on(ex::just(2), sched2));
    check_error_types<int>(ex::continue_on(ex::just(3), sched3));
  }

  TEST_CASE(
    "continue_on sends an exception_ptr if value types are potentially throwing when copied",
    "[adaptors][continue_on]") {
    inline_scheduler sched{};

    check_error_types<std::exception_ptr>(
      ex::continue_on(ex::just(potentially_throwing{}), sched));
  }

  TEST_CASE("continue_on keeps sends_stopped from scheduler's sender", "[adaptors][continue_on]") {
    inline_scheduler sched1{};
    error_scheduler<std::exception_ptr> sched2{std::make_exception_ptr(std::runtime_error{"error"})};
    stopped_scheduler sched3{};

    check_sends_stopped<false>(ex::continue_on(ex::just(1), sched1));
    check_sends_stopped<true>(ex::continue_on(ex::just(2), sched2));
    check_sends_stopped<true>(ex::continue_on(ex::just(3), sched3));
  }

  // struct test_domain_A {
  //   template <ex::sender_expr_for<ex::continue_on_t> Sender, class Env>
  //   auto transform_sender(Sender&&, Env&&) const {
  //     return ex::just(std::string("hello"));
  //   }
  // };

  // struct test_domain_B {
  //   template <ex::sender_expr_for<ex::continue_on_t> Sender, class Env>
  //   auto transform_sender(Sender&&, Env&&) const {
  //     return ex::just(std::string("goodbye"));
  //   }
  // };

  // TEST_CASE(
  //   "continue_on late customization is passed on the destination scheduler",
  //   "[adaptors][continue_on]") {
  //   // The customization will return a different value
  //   ex::scheduler auto sched_A = basic_inline_scheduler<test_domain_A>{};
  //   ex::scheduler auto sched_B = basic_inline_scheduler<test_domain_B>{};
  //   auto snd = ex::on(sched_A, ex::just() | ex::continue_on(sched_B));
  //   std::string res;
  //   auto op = ex::connect(std::move(snd), expect_value_receiver_ex{res});
  //   ex::start(op);
  //   REQUIRE(res == "goodbye");
  // }
} // namespace
