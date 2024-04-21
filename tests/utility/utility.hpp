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

template <class Sndr, class... Values>
void check_values(Sndr&& sndr, const Values&... values) noexcept {
  USTDEX_TRY {
    auto opt = ustdex::sync_wait(static_cast<Sndr&&>(sndr));
    if (!opt) {
      FAIL("Expected value completion; got stopped instead.");
    } else {
      auto&& vals = *opt;
      CHECK(vals == std::tie(values...));
    }
  }
  USTDEX_CATCH(...) {
    FAIL("Expected value completion; got error instead.");
  }
}
