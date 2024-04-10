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
#include "cpos.hpp"
#include "tuple.hpp"
#include "utility.hpp"

#include <exception>
#include <type_traits>

namespace ustdex {
  template <class Fn, class... As>
  struct _not_callable_with { };

  template <class Fn>
  struct _transform_completion {
    template <class... Ts>
    using _impl = _mif<
      _nothrow_callable<Fn, Ts...>,
      completion_signatures<set_value_t(_call_result_t<Fn, Ts...>)>,
      completion_signatures<
        set_value_t(_call_result_t<Fn, Ts...>),
        set_error_t(std::exception_ptr)>>;

    template <class... Ts>
    using _f =
      _minvoke<_mtry_quote<_impl, _mexception<_not_callable_with<Fn, Ts...>>>, Ts...>;
  };

  template <class UponTag, class SetTag>
  class _upon {
    template <class Sndr, class Rcvr, class Fn>
    struct _opstate_t : _immovable {
      using operation_state_concept = operation_state_t;
      Rcvr _rcvr;
      Fn _fn;
      connect_result_t<Sndr, _opstate_t *> _opstate;

      _opstate_t(Sndr &&sndr, Rcvr rcvr, Fn fn)
        : _rcvr{static_cast<Rcvr &&>(rcvr)}
        , _fn{static_cast<Fn &&>(fn)}
        , _opstate{connect(static_cast<Sndr &&>(sndr), this)} {
      }

      USTDEX_HOST_DEVICE
      void start() & noexcept {
        ustdex::start(_opstate);
      }

      template <bool CanThrow = false, class... Ts>
      USTDEX_HOST_DEVICE USTDEX_INLINE void _set(Ts &&...ts) noexcept(!CanThrow) {
        if constexpr (CanThrow || _nothrow_callable<Fn, Ts...>) {
          if constexpr (std::is_void_v<_call_result_t<Fn, Ts...>>) {
            static_cast<Fn &&>(_fn)(static_cast<Ts &&>(ts)...);
            ustdex::set_value(static_cast<Rcvr &&>(_rcvr));
          } else {
            ustdex::set_value(
              static_cast<Rcvr &&>(_rcvr),
              static_cast<Fn &&>(_fn)(static_cast<Ts &&>(ts)...));
          }
        } else
          try {
            _set<true>(static_cast<Ts &&>(ts)...);
          } catch (...) {
            ustdex::set_error(static_cast<Rcvr &&>(_rcvr), std::current_exception());
          }
      }

      template <class Tag, class... Ts>
      USTDEX_HOST_DEVICE USTDEX_INLINE void _complete(Tag, Ts &&...ts) noexcept {
        if constexpr (USTDEX_IS_SAME(Tag, SetTag)) {
          _set(static_cast<Ts &&>(ts)...);
        } else {
          Tag()(static_cast<Rcvr &&>(_rcvr), static_cast<Ts &&>(ts)...);
        }
      }

      template <class... Ts>
      USTDEX_HOST_DEVICE
      void set_value(Ts &&...ts) noexcept {
        _complete(set_value_t(), static_cast<Ts &&>(ts)...);
      }

      template <class Error>
      USTDEX_HOST_DEVICE
      void set_error(Error &&error) noexcept {
        _complete(set_error_t(), static_cast<Error &&>(error));
      }

      USTDEX_HOST_DEVICE
      void set_stopped() noexcept {
        _complete(set_stopped_t());
      }
    };

    template <class CvSndr, class Fn, class... Env>
    using _completions = _impl::_gather_completion_signatures<
      completion_signatures_of_t<CvSndr, Env...>,
      SetTag,
      _transform_completion<Fn>::template _f,
      _impl::_default_completions,
      _impl::_concat_completion_signatures>;

    template <class Fn, class Sndr>
    struct _sndr_t {
      using sender_concept = sender_t;
      [[no_unique_address]] UponTag _tag;
      Fn _fn;
      Sndr _sndr;

      template <class... Env>
      auto get_completion_signatures(const Env &...) && //
        -> _completions<Sndr, Fn, Env...>;

      template <class... Env>
      auto get_completion_signatures(const Env &...) const & //
        -> _completions<const Sndr &, Fn, Env...>;

      template <class Rcvr>
      USTDEX_HOST_DEVICE
      _opstate_t<Sndr, Rcvr, Fn> connect(Rcvr rcvr) && noexcept(
        _nothrow_constructible<_opstate_t<Sndr, Rcvr, Fn>, Sndr, Rcvr, Fn>) {
        return _opstate_t<Sndr, Rcvr, Fn>{
          static_cast<Sndr &&>(_sndr),
          static_cast<Rcvr &&>(rcvr),
          static_cast<Fn &&>(_fn)};
      }

      template <class Rcvr>
      USTDEX_HOST_DEVICE
      _opstate_t<const Sndr &, Rcvr, Fn>
        connect(Rcvr rcvr) const & noexcept(_nothrow_constructible<
                                            _opstate_t<const Sndr &, Rcvr, Fn>,
                                            const Sndr &,
                                            Rcvr,
                                            const Fn &>) {
        return _opstate_t<const Sndr &, Rcvr, Fn>{_sndr, static_cast<Rcvr &&>(rcvr), _fn};
      }

      USTDEX_HOST_DEVICE
      decltype(auto) get_env() const noexcept {
        return ustdex::get_env(_sndr);
      }
    };

   public:
    template <class Sndr, class Fn>
    USTDEX_INLINE USTDEX_HOST_DEVICE
    auto
      operator()(Sndr sndr, Fn fn) const noexcept {
      return _sndr_t<Fn, Sndr>{{}, static_cast<Fn &&>(fn), static_cast<Sndr &&>(sndr)};
    }
  };

  inline constexpr struct then_t : _upon<then_t, set_value_t> {
  } then{};

  inline constexpr struct upon_error_t : _upon<upon_error_t, set_error_t> {
  } upon_error{};

  inline constexpr struct upon_stopped_t : _upon<upon_stopped_t, set_stopped_t> {
  } upon_stopped{};
} // namespace ustdex
