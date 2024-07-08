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

#include "config.hpp"
#include "env.hpp"
#include "meta.hpp"
#include "type_traits.hpp"
#include "utility.hpp"

// This must be the last #include
#include "prologue.hpp"

namespace USTDEX_NAMESPACE
{
struct receiver_t
{};

struct operation_state_t
{};

struct sender_t
{};

struct scheduler_t
{};

template <class Ty>
using _sender_concept_t = typename USTDEX_REMOVE_REFERENCE(Ty)::sender_concept;

template <class Ty>
using _receiver_concept_t = typename USTDEX_REMOVE_REFERENCE(Ty)::receiver_concept;

template <class Ty>
using _scheduler_concept_t = typename USTDEX_REMOVE_REFERENCE(Ty)::scheduler_concept;

template <class Ty>
inline constexpr bool _is_sender = _mvalid_q<_sender_concept_t, Ty>;

template <class Ty>
inline constexpr bool _is_receiver = _mvalid_q<_receiver_concept_t, Ty>;

template <class Ty>
inline constexpr bool _is_scheduler = _mvalid_q<_scheduler_concept_t, Ty>;

USTDEX_DEVICE_CONSTANT constexpr struct set_value_t
{
  template <class Rcvr, class... Ts>
  USTDEX_INLINE USTDEX_HOST_DEVICE auto operator()(Rcvr&& rcvr, Ts&&... ts) const noexcept
    -> decltype(static_cast<Rcvr&&>(rcvr).set_value(static_cast<Ts&&>(ts)...))
  {
    static_assert(USTDEX_IS_SAME(decltype(static_cast<Rcvr&&>(rcvr).set_value(static_cast<Ts&&>(ts)...)), void));
    static_assert(noexcept(static_cast<Rcvr&&>(rcvr).set_value(static_cast<Ts&&>(ts)...)));
    static_cast<Rcvr&&>(rcvr).set_value(static_cast<Ts&&>(ts)...);
  }

  template <class Rcvr, class... Ts>
  USTDEX_INLINE USTDEX_HOST_DEVICE auto operator()(Rcvr* rcvr, Ts&&... ts) const noexcept
    -> decltype(static_cast<Rcvr&&>(*rcvr).set_value(static_cast<Ts&&>(ts)...))
  {
    static_assert(USTDEX_IS_SAME(decltype(static_cast<Rcvr&&>(*rcvr).set_value(static_cast<Ts&&>(ts)...)), void));
    static_assert(noexcept(static_cast<Rcvr&&>(*rcvr).set_value(static_cast<Ts&&>(ts)...)));
    static_cast<Rcvr&&>(*rcvr).set_value(static_cast<Ts&&>(ts)...);
  }
} set_value{};

USTDEX_DEVICE_CONSTANT constexpr struct set_error_t
{
  template <class Rcvr, class E>
  USTDEX_INLINE USTDEX_HOST_DEVICE auto operator()(Rcvr&& rcvr, E&& e) const noexcept
    -> decltype(static_cast<Rcvr&&>(rcvr).set_error(static_cast<E&&>(e)))
  {
    static_assert(USTDEX_IS_SAME(decltype(static_cast<Rcvr&&>(rcvr).set_error(static_cast<E&&>(e))), void));
    static_assert(noexcept(static_cast<Rcvr&&>(rcvr).set_error(static_cast<E&&>(e))));
    static_cast<Rcvr&&>(rcvr).set_error(static_cast<E&&>(e));
  }

  template <class Rcvr, class E>
  USTDEX_INLINE USTDEX_HOST_DEVICE auto operator()(Rcvr* rcvr, E&& e) const noexcept
    -> decltype(static_cast<Rcvr&&>(*rcvr).set_error(static_cast<E&&>(e)))
  {
    static_assert(USTDEX_IS_SAME(decltype(static_cast<Rcvr&&>(*rcvr).set_error(static_cast<E&&>(e))), void));
    static_assert(noexcept(static_cast<Rcvr&&>(*rcvr).set_error(static_cast<E&&>(e))));
    static_cast<Rcvr&&>(*rcvr).set_error(static_cast<E&&>(e));
  }
} set_error{};

USTDEX_DEVICE_CONSTANT constexpr struct set_stopped_t
{
  template <class Rcvr>
  USTDEX_INLINE USTDEX_HOST_DEVICE auto operator()(Rcvr&& rcvr) const noexcept
    -> decltype(static_cast<Rcvr&&>(rcvr).set_stopped())
  {
    static_assert(USTDEX_IS_SAME(decltype(static_cast<Rcvr&&>(rcvr).set_stopped()), void));
    static_assert(noexcept(static_cast<Rcvr&&>(rcvr).set_stopped()));
    static_cast<Rcvr&&>(rcvr).set_stopped();
  }

  template <class Rcvr>
  USTDEX_INLINE USTDEX_HOST_DEVICE auto operator()(Rcvr* rcvr) const noexcept
    -> decltype(static_cast<Rcvr&&>(*rcvr).set_stopped())
  {
    static_assert(USTDEX_IS_SAME(decltype(static_cast<Rcvr&&>(*rcvr).set_stopped()), void));
    static_assert(noexcept(static_cast<Rcvr&&>(*rcvr).set_stopped()));
    static_cast<Rcvr&&>(*rcvr).set_stopped();
  }
} set_stopped{};

USTDEX_DEVICE_CONSTANT constexpr struct start_t
{
  template <class OpState>
  USTDEX_INLINE USTDEX_HOST_DEVICE auto operator()(OpState& opstate) const noexcept -> decltype(opstate.start())
  {
    static_assert(!_is_error<typename OpState::completion_signatures>);
    static_assert(USTDEX_IS_SAME(decltype(opstate.start()), void));
    static_assert(noexcept(opstate.start()));
    opstate.start();
  }
} start{};

USTDEX_DEVICE_CONSTANT constexpr struct connect_t
{
  template <class Sndr, class Rcvr>
  USTDEX_INLINE USTDEX_HOST_DEVICE auto operator()(Sndr&& sndr, Rcvr&& rcvr) const
    noexcept(noexcept(static_cast<Sndr&&>(sndr).connect(static_cast<Rcvr&&>(rcvr))))
      -> decltype(static_cast<Sndr&&>(sndr).connect(static_cast<Rcvr&&>(rcvr)))
  {
    // using opstate_t     = decltype(static_cast<Sndr&&>(sndr).connect(static_cast<Rcvr&&>(rcvr)));
    // using completions_t = typename opstate_t::completion_signatures;
    // static_assert(_is_completion_signatures<completions_t>);

    return static_cast<Sndr&&>(sndr).connect(static_cast<Rcvr&&>(rcvr));
  }
} connect{};

USTDEX_DEVICE_CONSTANT constexpr struct schedule_t
{
  template <class Sch>
  USTDEX_INLINE USTDEX_HOST_DEVICE auto operator()(Sch&& sch) const noexcept
    -> decltype(static_cast<Sch&&>(sch).schedule())
  {
    static_assert(noexcept(static_cast<Sch&&>(sch).schedule()));
    return static_cast<Sch&&>(sch).schedule();
  }
} schedule{};

struct receiver_archetype
{
  using receiver_concept = receiver_t;

  template <class... Ts>
  void set_value(Ts&&...) noexcept;

  template <class Error>
  void set_error(Error&&) noexcept;

  void set_stopped() noexcept;

  env<> get_env() const noexcept;
};

template <class Sndr, class Rcvr>
using connect_result_t = decltype(connect(DECLVAL(Sndr), DECLVAL(Rcvr)));

template <class Sndr, class Rcvr = receiver_archetype>
using completion_signatures_of_t = typename connect_result_t<Sndr, Rcvr>::completion_signatures;

template <class Sch>
using schedule_result_t = decltype(schedule(DECLVAL(Sch)));

template <class Sndr, class Rcvr>
inline constexpr bool _nothrow_connectable = noexcept(connect(DECLVAL(Sndr), DECLVAL(Rcvr)));

// handy enumerations for keeping type names readable
enum _disposition_t
{
  _value,
  _error,
  _stopped
};

namespace _detail
{
template <_disposition_t, class Void = void>
extern _undefined<Void> _set_tag;
template <class Void>
extern _fn_t<set_value_t>* _set_tag<_value, Void>;
template <class Void>
extern _fn_t<set_error_t>* _set_tag<_error, Void>;
template <class Void>
extern _fn_t<set_stopped_t>* _set_tag<_stopped, Void>;
} // namespace _detail
} // namespace USTDEX_NAMESPACE

#include "epilogue.hpp"
