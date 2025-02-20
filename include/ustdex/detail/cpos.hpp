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

#ifndef USTDEX_ASYNC_DETAIL_CPOS
#define USTDEX_ASYNC_DETAIL_CPOS

#include "config.hpp"
#include "env.hpp"
#include "meta.hpp"
#include "type_traits.hpp"
#include "utility.hpp"

#include "prologue.hpp"

namespace ustdex
{
struct USTDEX_TYPE_VISIBILITY_DEFAULT dependent_sender_error;

template <class... Sigs>
struct USTDEX_TYPE_VISIBILITY_DEFAULT completion_signatures;

struct USTDEX_TYPE_VISIBILITY_DEFAULT receiver_t
{};

struct USTDEX_TYPE_VISIBILITY_DEFAULT operation_state_t
{};

struct USTDEX_TYPE_VISIBILITY_DEFAULT sender_t
{};

struct USTDEX_TYPE_VISIBILITY_DEFAULT scheduler_t
{};

template <class Ty>
using _sender_concept_t = typename std::remove_reference_t<Ty>::sender_concept;

template <class Ty>
using _receiver_concept_t = typename std::remove_reference_t<Ty>::receiver_concept;

template <class Ty>
using _scheduler_concept_t = typename std::remove_reference_t<Ty>::scheduler_concept;

template <class Ty>
inline constexpr bool _is_sender = _m_callable_q<_sender_concept_t, Ty>;

template <class Ty>
inline constexpr bool _is_receiver = _m_callable_q<_receiver_concept_t, Ty>;

template <class Ty>
inline constexpr bool _is_scheduler = _m_callable_q<_scheduler_concept_t, Ty>;

// handy enumerations for keeping type names readable
enum _disposition_t
{
  _value,
  _error,
  _stopped
};

// make the completion tags equality comparable
template <_disposition_t Disposition>
struct _completion_tag
{
  template <_disposition_t OtherDisposition>
  USTDEX_TRIVIAL_API constexpr auto operator==(_completion_tag<OtherDisposition>) const noexcept -> bool
  {
    return Disposition == OtherDisposition;
  }

  template <_disposition_t OtherDisposition>
  USTDEX_TRIVIAL_API constexpr auto operator!=(_completion_tag<OtherDisposition>) const noexcept -> bool
  {
    return Disposition != OtherDisposition;
  }

  static constexpr _disposition_t _disposition = Disposition;
};

inline constexpr struct set_value_t : _completion_tag<_value>
{
  template <class Rcvr, class... Ts>
  USTDEX_TRIVIAL_API auto operator()(Rcvr&& _rcvr, Ts&&... _ts) const noexcept
    -> decltype(static_cast<Rcvr&&>(_rcvr).set_value(static_cast<Ts&&>(_ts)...))
  {
    static_assert(std::is_same_v<decltype(static_cast<Rcvr&&>(_rcvr).set_value(static_cast<Ts&&>(_ts)...)), void>);
    static_assert(noexcept(static_cast<Rcvr&&>(_rcvr).set_value(static_cast<Ts&&>(_ts)...)));
    static_cast<Rcvr&&>(_rcvr).set_value(static_cast<Ts&&>(_ts)...);
  }

  template <class Rcvr, class... Ts>
  USTDEX_TRIVIAL_API auto operator()(Rcvr* _rcvr, Ts&&... _ts) const noexcept
    -> decltype(static_cast<Rcvr&&>(*_rcvr).set_value(static_cast<Ts&&>(_ts)...))
  {
    static_assert(std::is_same_v<decltype(static_cast<Rcvr&&>(*_rcvr).set_value(static_cast<Ts&&>(_ts)...)), void>);
    static_assert(noexcept(static_cast<Rcvr&&>(*_rcvr).set_value(static_cast<Ts&&>(_ts)...)));
    static_cast<Rcvr&&>(*_rcvr).set_value(static_cast<Ts&&>(_ts)...);
  }
} set_value{};

inline constexpr struct set_error_t : _completion_tag<_error>
{
  template <class Rcvr, class Ey>
  USTDEX_TRIVIAL_API auto operator()(Rcvr&& _rcvr, Ey&& _e) const noexcept
    -> decltype(static_cast<Rcvr&&>(_rcvr).set_error(static_cast<Ey&&>(_e)))
  {
    static_assert(std::is_same_v<decltype(static_cast<Rcvr&&>(_rcvr).set_error(static_cast<Ey&&>(_e))), void>);
    static_assert(noexcept(static_cast<Rcvr&&>(_rcvr).set_error(static_cast<Ey&&>(_e))));
    static_cast<Rcvr&&>(_rcvr).set_error(static_cast<Ey&&>(_e));
  }

  template <class Rcvr, class Ey>
  USTDEX_TRIVIAL_API auto operator()(Rcvr* _rcvr, Ey&& _e) const noexcept
    -> decltype(static_cast<Rcvr&&>(*_rcvr).set_error(static_cast<Ey&&>(_e)))
  {
    static_assert(std::is_same_v<decltype(static_cast<Rcvr&&>(*_rcvr).set_error(static_cast<Ey&&>(_e))), void>);
    static_assert(noexcept(static_cast<Rcvr&&>(*_rcvr).set_error(static_cast<Ey&&>(_e))));
    static_cast<Rcvr&&>(*_rcvr).set_error(static_cast<Ey&&>(_e));
  }
} set_error{};

inline constexpr struct set_stopped_t : _completion_tag<_stopped>
{
  template <class Rcvr>
  USTDEX_TRIVIAL_API auto operator()(Rcvr&& _rcvr) const noexcept -> decltype(static_cast<Rcvr&&>(_rcvr).set_stopped())
  {
    static_assert(std::is_same_v<decltype(static_cast<Rcvr&&>(_rcvr).set_stopped()), void>);
    static_assert(noexcept(static_cast<Rcvr&&>(_rcvr).set_stopped()));
    static_cast<Rcvr&&>(_rcvr).set_stopped();
  }

  template <class Rcvr>
  USTDEX_TRIVIAL_API auto operator()(Rcvr* _rcvr) const noexcept -> decltype(static_cast<Rcvr&&>(*_rcvr).set_stopped())
  {
    static_assert(std::is_same_v<decltype(static_cast<Rcvr&&>(*_rcvr).set_stopped()), void>);
    static_assert(noexcept(static_cast<Rcvr&&>(*_rcvr).set_stopped()));
    static_cast<Rcvr&&>(*_rcvr).set_stopped();
  }
} set_stopped{};

inline constexpr struct start_t
{
  template <class OpState>
  USTDEX_TRIVIAL_API auto operator()(OpState& _opstate) const noexcept -> decltype(_opstate.start())
  {
    // static_assert(!_m_is_error<typename OpState::completion_signatures>);
    static_assert(std::is_same_v<decltype(_opstate.start()), void>);
    static_assert(noexcept(_opstate.start()));
    _opstate.start();
  }
} start{};

// get_completion_signatures
template <class Sndr>
USTDEX_TRIVIAL_API USTDEX_CONSTEVAL auto get_completion_signatures();

template <class Sndr, class Env>
USTDEX_TRIVIAL_API USTDEX_CONSTEVAL auto get_completion_signatures();

// connect
inline constexpr struct connect_t
{
  template <class Sndr, class Rcvr>
  USTDEX_TRIVIAL_API auto operator()(Sndr&& _sndr, Rcvr&& _rcvr) const
    noexcept(noexcept(static_cast<Sndr&&>(_sndr).connect(static_cast<Rcvr&&>(_rcvr))))
      -> decltype(static_cast<Sndr&&>(_sndr).connect(static_cast<Rcvr&&>(_rcvr)))
  {
    return static_cast<Sndr&&>(_sndr).connect(static_cast<Rcvr&&>(_rcvr));
  }
} connect{};

inline constexpr struct schedule_t
{
  template <class Sch>
  USTDEX_TRIVIAL_API auto operator()(Sch&& _sch) const noexcept -> decltype(static_cast<Sch&&>(_sch).schedule())
  {
    static_assert(noexcept(static_cast<Sch&&>(_sch).schedule()));
    return static_cast<Sch&&>(_sch).schedule();
  }
} schedule{};

template <class Sndr, class Rcvr>
using connect_result_t = decltype(connect(declval<Sndr>(), declval<Rcvr>()));

template <class Sndr, class... Env>
using completion_signatures_of_t = decltype(ustdex::get_completion_signatures<Sndr, Env...>());

template <class Sch>
using schedule_result_t = decltype(schedule(declval<Sch>()));

template <class Sndr, class Rcvr>
inline constexpr bool _nothrow_connectable = noexcept(connect(declval<Sndr>(), declval<Rcvr>()));

namespace detail
{
template <_disposition_t, class Void = void>
extern _m_undefined<Void> _set_tag;
template <class Void>
extern _fn_t<set_value_t>* _set_tag<_value, Void>;
template <class Void>
extern _fn_t<set_error_t>* _set_tag<_error, Void>;
template <class Void>
extern _fn_t<set_stopped_t>* _set_tag<_stopped, Void>;
} // namespace detail
} // namespace ustdex

#include "epilogue.hpp"

#endif
