//===----------------------------------------------------------------------===//
//
// Part of CUDA Experimental in CUDA C++ Core Libraries,
// under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// SPDX-FileCopyrightText: Copyright (c) 2024 NVIDIA CORPORATION & AFFILIATES.
//
//===----------------------------------------------------------------------===//

#pragma once

#include "ustdex/ustdex.hpp"

// Catch header in its own header block
#include "catch2.hpp"

//! A move-only type
struct movable
{
  USTDEX_HOST_DEVICE movable(int value)
      : value_(value)
  {}

  movable(movable&&) = default;

  USTDEX_HOST_DEVICE friend bool operator==(const movable& a, const movable& b) noexcept
  {
    return a.value_ == b.value_;
  }

  USTDEX_HOST_DEVICE friend bool operator!=(const movable& a, const movable& b) noexcept
  {
    return a.value_ != b.value_;
  }

  USTDEX_HOST_DEVICE int value()
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

  USTDEX_HOST_DEVICE potentially_throwing(potentially_throwing&&) noexcept(false) {}

  USTDEX_HOST_DEVICE potentially_throwing(const potentially_throwing&) noexcept(false) {}

  USTDEX_HOST_DEVICE potentially_throwing& operator=(potentially_throwing&&) noexcept(false)
  {
    return *this;
  }

  USTDEX_HOST_DEVICE potentially_throwing& operator=(const potentially_throwing&) noexcept(false)
  {
    return *this;
  }
};

struct string
{
  string() = default;

  USTDEX_HOST_DEVICE explicit string(char const* c)
  {
    std::size_t len = 0;
    while (c[len++])
      ;
    char* tmp = str = new char[len];
    while ((*tmp++ = *c++))
      ;
  }

  USTDEX_HOST_DEVICE string(string&& other) noexcept
      : str(other.str)
  {
    other.str = nullptr;
  }

  USTDEX_HOST_DEVICE string(const string& other)
      : string(string(other.str))
  {}

  USTDEX_HOST_DEVICE ~string()
  {
    delete[] str;
  }

  USTDEX_HOST_DEVICE friend bool operator==(const string& left, const string& right) noexcept
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

  USTDEX_HOST_DEVICE friend bool operator!=(const string& left, const string& right) noexcept
  {
    return !(left == right);
  }

private:
  char* str{};
};

struct error_code
{
  USTDEX_HOST_DEVICE friend bool operator==(const error_code& left, const error_code& right) noexcept
  {
    return left.ec == right.ec;
  }

  USTDEX_HOST_DEVICE friend bool operator!=(const error_code& left, const error_code& right) noexcept
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

#else // USTDEX_HOST_ONLY()

template <class Sndr, class... Values>
void check_values(Sndr&& sndr, const Values&... values) noexcept
{}

#endif // USTDEX_HOST_ONLY()

template <class... Ts>
using types = ustdex::_mlist<Ts...>;

template <class... Values, class Sndr>
USTDEX_HOST_DEVICE void check_value_types(Sndr&& sndr) noexcept
{
  using actual_t   = ustdex::value_types_of_t<Sndr, ustdex::env<>, types, ustdex::_mmake_set>;
  using expected_t = ustdex::_mmake_set<Values...>;

  static_assert(ustdex::_mset_eq<expected_t, actual_t>, "value_types_of_t does not match expected types");
}

template <class... Errors, class Sndr>
USTDEX_HOST_DEVICE void check_error_types(Sndr&& sndr) noexcept
{
  using actual_t   = ustdex::error_types_of_t<Sndr, ustdex::env<>, ustdex::_mmake_set>;
  using expected_t = ustdex::_mmake_set<Errors...>;

  static_assert(ustdex::_mset_eq<expected_t, actual_t>, "error_types_of_t does not match expected types");
}

template <bool SendsStopped, class Sndr>
USTDEX_HOST_DEVICE void check_sends_stopped(Sndr&& sndr) noexcept
{
  static_assert(ustdex::sends_stopped<Sndr> == SendsStopped, "sends_stopped does not match expected value");
}
