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

#include "config.hpp"
#include "env.hpp"
#include "meta.hpp"
#include "type_traits.hpp"
#include "utility.hpp"

namespace ustdex {
  struct receiver_t { };

  struct operation_state_t { };

  struct sender_t { };

  struct scheduler_t { };

  template <class Ty>
  using _sender_concept = typename USTDEX_REMOVE_REFERENCE(Ty)::sender_concept;

  template <class Ty>
  using _receiver_concept = typename USTDEX_REMOVE_REFERENCE(Ty)::receiver_concept;

  template <class Ty>
  using _scheduler_concept = typename USTDEX_REMOVE_REFERENCE(Ty)::scheduler_concept;

  template <class Ty>
  USTDEX_DEVICE constexpr bool _is_sender = _mvalid_q<_sender_concept, Ty>;

  template <class Ty>
  USTDEX_DEVICE constexpr bool _is_receiver = _mvalid_q<_receiver_concept, Ty>;

  template <class Ty>
  USTDEX_DEVICE constexpr bool _is_scheduler = _mvalid_q<_scheduler_concept, Ty>;

  USTDEX_DEVICE constexpr struct set_value_t {
    template <class Rcvr, class... Ts>
    USTDEX_INLINE USTDEX_HOST_DEVICE
    auto
      operator()(Rcvr &&rcvr, Ts &&...ts) const noexcept
      -> decltype(static_cast<Rcvr &&>(rcvr).set_value(static_cast<Ts &&>(ts)...)) {
      static_assert(USTDEX_IS_SAME(
        decltype(static_cast<Rcvr &&>(rcvr).set_value(static_cast<Ts &&>(ts)...)), void));
      static_assert(
        noexcept(static_cast<Rcvr &&>(rcvr).set_value(static_cast<Ts &&>(ts)...)));
      static_cast<Rcvr &&>(rcvr).set_value(static_cast<Ts &&>(ts)...);
    }

    template <class Rcvr, class... Ts>
    USTDEX_INLINE USTDEX_HOST_DEVICE
    auto
      operator()(Rcvr *rcvr, Ts &&...ts) const noexcept
      -> decltype(static_cast<Rcvr &&>(*rcvr).set_value(static_cast<Ts &&>(ts)...)) {
      static_assert(USTDEX_IS_SAME(
        decltype(static_cast<Rcvr &&>(*rcvr).set_value(static_cast<Ts &&>(ts)...)),
        void));
      static_assert(
        noexcept(static_cast<Rcvr &&>(*rcvr).set_value(static_cast<Ts &&>(ts)...)));
      static_cast<Rcvr &&>(*rcvr).set_value(static_cast<Ts &&>(ts)...);
    }
  } set_value{};

  USTDEX_DEVICE constexpr struct set_error_t {
    template <class Rcvr, class E>
    USTDEX_INLINE USTDEX_HOST_DEVICE
    auto
      operator()(Rcvr &&rcvr, E &&e) const noexcept
      -> decltype(static_cast<Rcvr &&>(rcvr).set_error(static_cast<E &&>(e))) {
      static_assert(USTDEX_IS_SAME(
        decltype(static_cast<Rcvr &&>(rcvr).set_error(static_cast<E &&>(e))), void));
      static_assert(noexcept(static_cast<Rcvr &&>(rcvr).set_error(static_cast<E &&>(e))));
      static_cast<Rcvr &&>(rcvr).set_error(static_cast<E &&>(e));
    }

    template <class Rcvr, class E>
    USTDEX_INLINE USTDEX_HOST_DEVICE
    auto
      operator()(Rcvr *rcvr, E &&e) const noexcept
      -> decltype(static_cast<Rcvr &&>(*rcvr).set_error(static_cast<E &&>(e))) {
      static_assert(USTDEX_IS_SAME(
        decltype(static_cast<Rcvr &&>(*rcvr).set_error(static_cast<E &&>(e))), void));
      static_assert(noexcept(static_cast<Rcvr &&>(*rcvr).set_error(static_cast<E &&>(e))));
      static_cast<Rcvr &&>(*rcvr).set_error(static_cast<E &&>(e));
    }
  } set_error{};

  USTDEX_DEVICE constexpr struct set_stopped_t {
    template <class Rcvr>
    USTDEX_INLINE USTDEX_HOST_DEVICE
    auto
      operator()(Rcvr &&rcvr) const noexcept
      -> decltype(static_cast<Rcvr &&>(rcvr).set_stopped()) {
      static_assert(
        USTDEX_IS_SAME(decltype(static_cast<Rcvr &&>(rcvr).set_stopped()), void));
      static_assert(noexcept(static_cast<Rcvr &&>(rcvr).set_stopped()));
      static_cast<Rcvr &&>(rcvr).set_stopped();
    }

    template <class Rcvr>
    USTDEX_INLINE USTDEX_HOST_DEVICE
    auto
      operator()(Rcvr *rcvr) const noexcept
      -> decltype(static_cast<Rcvr &&>(*rcvr).set_stopped()) {
      static_assert(
        USTDEX_IS_SAME(decltype(static_cast<Rcvr &&>(*rcvr).set_stopped()), void));
      static_assert(noexcept(static_cast<Rcvr &&>(*rcvr).set_stopped()));
      static_cast<Rcvr &&>(*rcvr).set_stopped();
    }
  } set_stopped{};

  USTDEX_DEVICE constexpr struct start_t {
    template <class OpState>
    USTDEX_INLINE USTDEX_HOST_DEVICE
    auto
      operator()(OpState &opstate) const noexcept -> decltype(opstate.start()) {
      static_assert(USTDEX_IS_SAME(decltype(opstate.start()), void));
      static_assert(noexcept(opstate.start()));
      opstate.start();
    }
  } start{};

  USTDEX_DEVICE constexpr struct get_completion_signatures_t {
    template <class Sndr>
    USTDEX_INLINE USTDEX_HOST_DEVICE
    auto
      operator()(Sndr &&sndr) const noexcept
      -> decltype(static_cast<Sndr &&>(sndr).get_completion_signatures()) {
      return {};
    }

    template <class Sndr, class E>
    USTDEX_INLINE USTDEX_HOST_DEVICE
    auto
      operator()(Sndr &&sndr, E &&e) const noexcept
      -> decltype(static_cast<Sndr &&>(sndr).get_completion_signatures(
        static_cast<E &&>(e))) {
      return {};
    }
  } get_completion_signatures{};

  USTDEX_DEVICE constexpr struct connect_t {
    template <class Sndr, class Rcvr>
    USTDEX_INLINE USTDEX_HOST_DEVICE
    auto
      operator()(Sndr &&sndr, Rcvr &&rcvr) const
      noexcept(noexcept(static_cast<Sndr &&>(sndr).connect(static_cast<Rcvr &&>(rcvr))))
        -> decltype(static_cast<Sndr &&>(sndr).connect(static_cast<Rcvr &&>(rcvr))) {
      return static_cast<Sndr &&>(sndr).connect(static_cast<Rcvr &&>(rcvr));
    }
  } connect{};

  USTDEX_DEVICE constexpr struct schedule_t {
    template <class Sch>
    USTDEX_INLINE USTDEX_HOST_DEVICE
    auto
      operator()(Sch &&sch) const noexcept
      -> decltype(static_cast<Sch &&>(sch).schedule()) {
      static_assert(noexcept(static_cast<Sch &&>(sch).schedule()));
      return static_cast<Sch &&>(sch).schedule();
    }
  } schedule{};

  template <class Sndr, class... Env>
  using completion_signatures_of_t =
    decltype(get_completion_signatures(DECLVAL(Sndr), DECLVAL(Env)...));

  template <class Sndr, class Rcvr>
  using connect_result_t = decltype(connect(DECLVAL(Sndr), DECLVAL(Rcvr)));

  template <class Sch>
  using schedule_result_t = decltype(schedule(DECLVAL(Sch)));

  template <class Sndr, class Rcvr>
  USTDEX_DEVICE constexpr bool _nothrow_connectable =
    noexcept(connect(DECLVAL(Sndr), DECLVAL(Rcvr)));

  // handy enumerations for keeping type names readable
  enum _disposition_t {
    _value,
    _error,
    _stopped
  };

  namespace _detail {
    template <_disposition_t, class Void = void>
    extern _undefined<Void> _set_tag;
    template <class Void>
    extern set_value_t _set_tag<_value, Void>;
    template <class Void>
    extern set_error_t _set_tag<_error, Void>;
    template <class Void>
    extern set_stopped_t _set_tag<_stopped, Void>;
  } // namespace _detail
} // namespace ustdex
