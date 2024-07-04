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

namespace
{
template <class... Values>
struct checked_value_receiver
{
  using receiver_concept = ustdex::receiver_t;

  template <class... As>
  USTDEX_HOST_DEVICE void set_value(As... as) && noexcept
  {
    if constexpr (USTDEX_IS_SAME(ustdex::_mlist<Values...>, ustdex::_mlist<As...>))
    {
      _values.apply(
        [&](auto const&... vs) {
          CHECK(((vs == as) && ...));
        },
        _values);
    }
    else
    {
      FAIL("expected a value completion; got a different value");
    }
  }

  template <class Error>
  USTDEX_HOST_DEVICE void set_error(Error) && noexcept
  {
    FAIL("expected a value completion; got an error");
  }

  USTDEX_HOST_DEVICE void set_stopped() && noexcept
  {
    FAIL("expected a value completion; got stopped");
  }

  ustdex::_tuple<Values...> _values;
};

template <class... Values>
checked_value_receiver(Values...) -> checked_value_receiver<Values...>;

template <class Error>
struct checked_error_receiver
{
  using receiver_concept = ustdex::receiver_t;

  template <class... As>
  USTDEX_HOST_DEVICE void set_value(As...) && noexcept
  {
    FAIL("expected an error completion; got a value");
  }

  template <class Ty>
  USTDEX_HOST_DEVICE void set_error(Ty ty) && noexcept
  {
    if constexpr (USTDEX_IS_SAME(Error, Ty))
    {
      CHECK(ty == _error);
    }
    else
    {
      FAIL("expected an error completion; got a different error");
    }
  }

  USTDEX_HOST_DEVICE void set_stopped() && noexcept
  {
    FAIL("expected a value completion; got stopped");
  }

  Error _error;
};

template <class Error>
checked_error_receiver(Error) -> checked_error_receiver<Error>;

struct checked_stopped_receiver
{
  using receiver_concept = ustdex::receiver_t;

  template <class... As>
  USTDEX_HOST_DEVICE void set_value(As...) && noexcept
  {
    FAIL("expected a stopped completion; got a value");
  }

  template <class Ty>
  USTDEX_HOST_DEVICE void set_error(Ty) && noexcept
  {
    FAIL("expected an stopped completion; got an error");
  }

  USTDEX_HOST_DEVICE void set_stopped() && noexcept {}
};

} // namespace
