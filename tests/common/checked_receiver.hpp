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

#include "utility.hpp"

#include "ustdex/ustdex.hpp"

// Catch header in its own header block
#include "catch2.hpp" // IWYU pragma: keep

namespace
{
template <class... Values>
struct checked_value_receiver
{
  using receiver_concept = ustdex::receiver_t;

  template <class... As>
  HOST_DEVICE void set_value(As... as) && noexcept
  {
    if constexpr (USTDEX_IS_SAME(ustdex::_m_list<Values...>, ustdex::_m_list<As...>))
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
  HOST_DEVICE void set_error(Error) && noexcept
  {
    FAIL("expected a value completion; got an error");
  }

  HOST_DEVICE void set_stopped() && noexcept
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
  HOST_DEVICE void set_value(As...) && noexcept
  {
    FAIL("expected an error completion; got a value");
  }

  template <class Ty>
  HOST_DEVICE void set_error(Ty ty) && noexcept
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

  HOST_DEVICE void set_stopped() && noexcept
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
  HOST_DEVICE void set_value(As...) && noexcept
  {
    FAIL("expected a stopped completion; got a value");
  }

  template <class Ty>
  HOST_DEVICE void set_error(Ty) && noexcept
  {
    FAIL("expected an stopped completion; got an error");
  }

  HOST_DEVICE void set_stopped() && noexcept {}
};

} // namespace
