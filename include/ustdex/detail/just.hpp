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

#include "completion_signatures.hpp"
#include "config.hpp"
#include "cpos.hpp"
#include "tuple.hpp"
#include "utility.hpp"

namespace ustdex {
  // Forward declarations of the just* tag types:
  struct just_t;
  struct just_error_t;
  struct just_stopped_t;

  // Map from a disposition to the corresponding tag types:
  namespace _detail {
    template <_disposition_t, class Void = void>
    extern _undefined<Void> _just_tag;
    template <class Void>
    extern _fn_t<just_t> *_just_tag<_value, Void>;
    template <class Void>
    extern _fn_t<just_error_t> *_just_tag<_error, Void>;
    template <class Void>
    extern _fn_t<just_stopped_t> *_just_tag<_stopped, Void>;
  } // namespace _detail

  template <_disposition_t Disposition>
  struct _just {
#ifndef __CUDACC__
   private:
#endif

    using JustTag = decltype(_detail::_just_tag<Disposition>());
    using SetTag = decltype(_detail::_set_tag<Disposition>());

    template <class Rcvr, class... Ts>
    struct opstate_t {
      using operation_state_concept = operation_state_t;
      Rcvr _rcvr;
      _tuple_for<Ts...> _values;

      struct _complete_fn {
        opstate_t *_self;

        USTDEX_HOST_DEVICE void operator()(Ts &...ts) const noexcept {
          SetTag()(static_cast<Rcvr &&>(_self->_rcvr), static_cast<Ts &&>(ts)...);
        }
      };

      USTDEX_HOST_DEVICE void start() & noexcept {
        _apply(_complete_fn{this}, _values);
      }
    };

    template <class... Ts>
    struct sndr_t {
      using sender_concept = sender_t;
      [[no_unique_address]] JustTag _tag;
      _tuple_for<Ts...> _values;

      auto get_completion_signatures(_ignore = {}) const
        -> completion_signatures<SetTag(Ts...)>;

      template <class Rcvr>
      USTDEX_HOST_DEVICE opstate_t<Rcvr, Ts...> connect(Rcvr rcvr) && //
        noexcept(_nothrow_decay_copyable<Rcvr, Ts...>) {
        return opstate_t<Rcvr, Ts...>{
          static_cast<Rcvr &&>(rcvr), static_cast<_tuple_for<Ts...> &&>(_values)};
      }

      template <class Rcvr>
      USTDEX_HOST_DEVICE opstate_t<Rcvr, Ts...> connect(Rcvr rcvr) const & //
        noexcept(_nothrow_decay_copyable<Rcvr, Ts const &...>) {
        return opstate_t<Rcvr, Ts...>{static_cast<Rcvr &&>(rcvr), _values};
      }
    };

   public:
    template <class... Ts>
    USTDEX_INLINE USTDEX_HOST_DEVICE auto operator()(Ts... ts) const noexcept {
      return sndr_t<Ts...>{{}, {static_cast<Ts &&>(ts)...}};
    }
  };

  USTDEX_DEVICE constexpr struct just_t : _just<_value> {
  } just{};

  USTDEX_DEVICE constexpr struct just_error_t : _just<_error> {
  } just_error{};

  USTDEX_DEVICE constexpr struct just_stopped_t : _just<_stopped> {
  } just_stopped{};
} // namespace ustdex
