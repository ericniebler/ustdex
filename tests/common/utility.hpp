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

#pragma once

#include "ustdex/ustdex.hpp"

// Catch header in its own header block
#include "catch2.hpp" // IWYU pragma: keep

#if defined(__CUDACC__)
#  define HOST_DEVICE __host__ __device__
#else
#  define HOST_DEVICE
#endif

//! A move-only type
struct movable
{
  HOST_DEVICE movable(int value)
      : value_(value)
  {}

  movable(movable&&) = default;

  HOST_DEVICE friend bool operator==(const movable& a, const movable& b) noexcept
  {
    return a.value_ == b.value_;
  }

  HOST_DEVICE friend bool operator!=(const movable& a, const movable& b) noexcept
  {
    return a.value_ != b.value_;
  }

  HOST_DEVICE int value()
  {
    return value_;
  } // silence warning of unused private field

private:
  int value_;
};

//! A type with potentially throwing move/copy constructors
struct potentially_throwing
{
  potentially_throwing() = default;

  HOST_DEVICE potentially_throwing(potentially_throwing&&) noexcept(false) {}

  HOST_DEVICE potentially_throwing(const potentially_throwing&) noexcept(false) {}

  HOST_DEVICE potentially_throwing& operator=(potentially_throwing&&) noexcept(false)
  {
    return *this;
  }

  HOST_DEVICE potentially_throwing& operator=(const potentially_throwing&) noexcept(false)
  {
    return *this;
  }
};

struct string
{
  string() = default;

  HOST_DEVICE explicit string(char const* c)
  {
    std::size_t len = 0;
    while (c[len++])
      ;
    char* tmp = str = new char[len];
    while ((*tmp++ = *c++))
      ;
  }

  HOST_DEVICE string(string&& other) noexcept
      : str(other.str)
  {
    other.str = nullptr;
  }

  HOST_DEVICE string(const string& other)
      : string(string(other.str))
  {}

  HOST_DEVICE ~string()
  {
    delete[] str;
  }

  HOST_DEVICE friend bool operator==(const string& left, const string& right) noexcept
  {
    char const* l = left.str;
    char const* r = right.str;
    while (*l && *r)
    {
      if (*l++ != *r++)
      {
        return false;
      }
    }
    return *l == *r;
  }

  HOST_DEVICE friend bool operator!=(const string& left, const string& right) noexcept
  {
    return !(left == right);
  }

private:
  char* str{};
};

struct error_code
{
  HOST_DEVICE friend bool operator==(const error_code& left, const error_code& right) noexcept
  {
    return left.ec == right.ec;
  }

  HOST_DEVICE friend bool operator!=(const error_code& left, const error_code& right) noexcept
  {
    return !(left == right);
  }

  std::errc ec;
};

// run_loop isn't supported on-device yet, so neither can sync_wait be.
#if USTDEX_HOST_ONLY()

template <class Sndr, class... Values>
void check_values(Sndr&& sndr, const Values&... values) noexcept
{
  try
  {
    auto opt = ustdex::sync_wait(static_cast<Sndr&&>(sndr));
    if (!opt)
    {
      FAIL("Expected value completion; got stopped instead.");
    }
    else
    {
      auto&& vals = *opt;
      CHECK(vals == std::tie(values...));
    }
  }
  catch (...)
  {
    FAIL("Expected value completion; got error instead.");
  }
}

#else  // USTDEX_HOST_ONLY()

template <class Sndr, class... Values>
void check_values(Sndr&& sndr, const Values&... values) noexcept
{}

#endif // USTDEX_HOST_ONLY()

template <class... Ts>
using _m_list = ustdex::_m_list<Ts...>;

template <class... Values, class Sndr>
HOST_DEVICE void check_value_types(Sndr&& sndr) noexcept
{
  using actual_t = ustdex::value_types_of_t<Sndr, ustdex::env<>, _m_list, ustdex::_m_make_set>;
  static_assert(ustdex::_m_set_eq_v<actual_t, Values...>, "value_types_of_t does not match expected types");
}

template <class... Errors, class Sndr>
HOST_DEVICE void check_error_types(Sndr&& sndr) noexcept
{
  using actual_t = ustdex::error_types_of_t<Sndr, ustdex::env<>, ustdex::_m_make_set>;
  static_assert(ustdex::_m_set_eq_v<actual_t, Errors...>, "error_types_of_t does not match expected types");
}

template <bool SendsStopped, class Sndr>
HOST_DEVICE void check_sends_stopped(Sndr&& sndr) noexcept
{
  static_assert(ustdex::sends_stopped<Sndr> == SendsStopped, "sends_stopped does not match expected value");
}
