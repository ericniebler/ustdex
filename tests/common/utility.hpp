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
#include "ustdex/detail/exception.hpp"
#include "ustdex/detail/sync_wait.hpp"

#include <catch2/catch_all.hpp>

//! A move-only type
struct movable {
  movable(int value)
    : value_(value) {
  }

  movable(movable&&) = default;

  friend bool operator==(const movable& a, const movable& b) noexcept {
    return a.value_ == b.value_;
  }

  friend bool operator!=(const movable& a, const movable& b) noexcept {
    return a.value_ != b.value_;
  }

  int value() {
    return value_;
  } // silence warning of unused private field
 private:
  int value_;
};

//! A type with potentially throwing move/copy constructors
struct potentially_throwing {
  potentially_throwing() = default;

  potentially_throwing(potentially_throwing&&) noexcept(false) {
  }

  potentially_throwing(const potentially_throwing&) noexcept(false) {
  }

  potentially_throwing& operator=(potentially_throwing&&) noexcept(false) {
    return *this;
  }

  potentially_throwing& operator=(const potentially_throwing&) noexcept(false) {
    return *this;
  }
};

template <class Sndr, class... Values>
void check_values(Sndr&& sndr, const Values&... values) noexcept {
  USTDEX_TRY(
    ({
      auto opt = ustdex::sync_wait(static_cast<Sndr&&>(sndr));
      if (!opt) {
        FAIL("Expected value completion; got stopped instead.");
      } else {
        auto&& vals = *opt;
        CHECK(vals == std::tie(values...));
      }
    }),
    USTDEX_CATCH(...)({ //
      FAIL("Expected value completion; got error instead.");
    }))
}

template <class... Ts>
using types = ustdex::_mlist<Ts...>;

template <class... Values, class Sndr>
void check_value_types(Sndr&& sndr) noexcept {
  using actual_t =
    ustdex::value_types_of_t<Sndr, ustdex::empty_env, types, ustdex::_mmake_set>;
  using expected_t = ustdex::_mmake_set<Values...>;

  static_assert(
    ustdex::_mset_eq<expected_t, actual_t>,
    "value_types_of_t does not match expected types");
}

template <class... Errors, class Sndr>
void check_error_types(Sndr&& sndr) noexcept {
  using actual_t = ustdex::error_types_of_t<Sndr, ustdex::empty_env, ustdex::_mmake_set>;
  using expected_t = ustdex::_mmake_set<Errors...>;

  static_assert(
    ustdex::_mset_eq<expected_t, actual_t>,
    "error_types_of_t does not match expected types");
}

template <bool SendsStopped, class Sndr>
void check_sends_stopped(Sndr&& sndr) noexcept {
  static_assert(
    ustdex::sends_stopped<Sndr> == SendsStopped,
    "sends_stopped does not match expected value");
}
