/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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

#ifndef USTDEX_ASYNC_DETAIL_CONCEPTS
#define USTDEX_ASYNC_DETAIL_CONCEPTS

#include "completion_signatures.hpp"
#include "config.hpp"
#include "cpos.hpp"
#include "preprocessor.hpp"

#include "prologue.hpp"

namespace ustdex
{
// Receiver concepts:
template <class Rcvr>
USTDEX_CONCEPT receiver = //
  USTDEX_REQUIRES_EXPR((Rcvr)) //
  ( //
    requires(_is_receiver<std::decay_t<Rcvr>>), //
    requires(std::is_move_constructible_v<std::decay_t<Rcvr>>), //
    requires(std::is_constructible_v<std::decay_t<Rcvr>, Rcvr>), //
    requires(std::is_nothrow_move_constructible_v<std::decay_t<Rcvr>>) //
  );

template <class Rcvr, class Sig>
inline constexpr bool _valid_completion_for = false;

template <class Rcvr, class Tag, class... As>
inline constexpr bool _valid_completion_for<Rcvr, Tag(As...)> = _callable<Tag, Rcvr, As...>;

template <class Rcvr, class Completions>
inline constexpr bool _has_completions = false;

template <class Rcvr, class... Sigs>
inline constexpr bool _has_completions<Rcvr, completion_signatures<Sigs...>> =
  (_valid_completion_for<Rcvr, Sigs> && ...);

template <class Rcvr, class Completions>
USTDEX_CONCEPT receiver_of = //
  USTDEX_REQUIRES_EXPR((Rcvr, Completions)) //
  ( //
    requires(receiver<Rcvr>), //
    requires(_has_completions<std::decay_t<Rcvr>, Completions>) //
  );

// Queryable traits:
template <class Ty>
USTDEX_CONCEPT _queryable = std::is_destructible_v<Ty>;

// Awaitable traits:
template <class>
USTDEX_CONCEPT _is_awaitable = false; // TODO: Implement this concept.

// Sender traits:
template <class Sndr>
USTDEX_API constexpr bool _enable_sender()
{
  if constexpr (_is_sender<Sndr>)
  {
    return true;
  }
  else
  {
    return _is_awaitable<Sndr>;
  }
}

template <class Sndr>
inline constexpr bool enable_sender = _enable_sender<Sndr>();

// Sender concepts:
template <class... Env>
struct _completions_tester
{
  template <class Sndr, bool EnableIfConstexpr = (get_completion_signatures<Sndr, Env...>(), true)>
  USTDEX_API static constexpr auto _is_valid(int) -> bool
  {
    return _valid_completion_signatures<completion_signatures_of_t<Sndr, Env...>>;
  }

  template <class Sndr>
  USTDEX_API static constexpr auto _is_valid(long) -> bool
  {
    return false;
  }
};

template <class Sndr>
USTDEX_CONCEPT sender = //
  USTDEX_REQUIRES_EXPR((Sndr)) //
  ( //
    requires(enable_sender<std::decay_t<Sndr>>), //
    requires(std::is_move_constructible_v<std::decay_t<Sndr>>), //
    requires(std::is_constructible_v<std::decay_t<Sndr>, Sndr>) //
  );

template <class Sndr, class... Env>
USTDEX_CONCEPT sender_in = //
  USTDEX_REQUIRES_EXPR((Sndr, variadic Env)) //
  ( //
    requires(sender<Sndr>), //
    requires(sizeof...(Env) <= 1), //
    requires((_queryable<Env> && ...)), //
    requires(_completions_tester<Env...>::template _is_valid<Sndr>(0)) //
  );

template <class Sndr>
USTDEX_CONCEPT dependent_sender = //
  USTDEX_REQUIRES_EXPR((Sndr)) //
  ( //
    requires(sender<Sndr>), //
    requires(_is_dependent_sender<Sndr>()) //
  );

} // namespace ustdex

#include "epilogue.hpp"

#endif // USTDEX_ASYNC_DETAIL_CONCEPTS
