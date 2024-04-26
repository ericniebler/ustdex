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
#include "exception.hpp"
#include "meta.hpp"
#include "tuple.hpp"
#include "utility.hpp"

#include <type_traits>

namespace ustdex {
  // Forward-declate the let_* algorithm tag types:
  struct then_t;
  struct upon_error_t;
  struct upon_stopped_t;

  // Map from a disposition to the corresponding tag types:
  namespace _detail {
    template <_disposition_t, class Void = void>
    extern _undefined<Void> _upon_tag;
    template <class Void>
    extern _fn_t<then_t> *_upon_tag<_value, Void>;
    template <class Void>
    extern _fn_t<upon_error_t> *_upon_tag<_error, Void>;
    template <class Void>
    extern _fn_t<upon_stopped_t> *_upon_tag<_stopped, Void>;
  } // namespace _detail

  namespace _upon {
    template <bool IsVoid, bool Nothrow>
    struct _completion_fn { // non-void, potentially throwing case
      template <class Result>
      using _f =
        completion_signatures<set_value_t(Result), set_error_t(std::exception_ptr)>;
    };

    template <>
    struct _completion_fn<true, false> { // void, potentially throwing case
      template <class>
      using _f = completion_signatures<set_value_t(), set_error_t(std::exception_ptr)>;
    };

    template <>
    struct _completion_fn<false, true> { // non-void, non-throwing case
      template <class Result>
      using _f = completion_signatures<set_value_t(Result)>;
    };

    template <>
    struct _completion_fn<true, true> { // void, non-throwing case
      template <class>
      using _f = completion_signatures<set_value_t()>;
    };

    template <class Result, bool Nothrow>
    using _completion_ =
      _minvoke1<_completion_fn<USTDEX_IS_SAME(Result, void), Nothrow>, Result>;

    template <class Fn, class... Ts>
    using _completion =
      _completion_<_call_result_t<Fn, Ts...>, _nothrow_callable<Fn, Ts...>>;
  } // namespace _upon

  template <_disposition_t Disposition>
  struct _upon_t {
#ifndef __CUDACC__
   private:
#endif
    using UponTag = decltype(_detail::_upon_tag<Disposition>());
    using SetTag = decltype(_detail::_set_tag<Disposition>());

    template <class Fn, class... Ts>
    using _error_not_callable = //
      ERROR<                    //
        WHERE(IN_ALGORITHM, UponTag),
        WHAT(FUNCTION_IS_NOT_CALLABLE),
        WITH_FUNCTION(Fn),
        WITH_ARGUMENTS(Ts...)>;

    template <class Fn>
    struct _transform_completion {
      template <class... Ts>
      using _f =
        _minvoke<_mtry_quote<_upon::_completion, _error_not_callable<Fn, Ts...>>, Fn, Ts...>;
    };

    template <class Rcvr, class Sndr, class Fn>
    struct _opstate_t {
      using operation_state_concept = operation_state_t;
      Rcvr _rcvr;
      Fn _fn;
      connect_result_t<Sndr, _opstate_t *> _opstate;

      USTDEX_HOST_DEVICE _opstate_t(Sndr &&sndr, Rcvr rcvr, Fn fn)
        : _rcvr{static_cast<Rcvr &&>(rcvr)}
        , _fn{static_cast<Fn &&>(fn)}
        , _opstate{connect(static_cast<Sndr &&>(sndr), this)} {
      }

      USTDEX_IMMOVABLE(_opstate_t);

      USTDEX_HOST_DEVICE void start() & noexcept {
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
        } else {
          USTDEX_TRY(
            ({                                       //
              _set<true>(static_cast<Ts &&>(ts)...); //
            }),                                      //
            USTDEX_CATCH(...)({
              ustdex::set_error(static_cast<Rcvr &&>(_rcvr), std::current_exception());
            }))
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
      USTDEX_HOST_DEVICE void set_value(Ts &&...ts) noexcept {
        _complete(set_value_t(), static_cast<Ts &&>(ts)...);
      }

      template <class Error>
      USTDEX_HOST_DEVICE void set_error(Error &&error) noexcept {
        _complete(set_error_t(), static_cast<Error &&>(error));
      }

      USTDEX_HOST_DEVICE void set_stopped() noexcept {
        _complete(set_stopped_t());
      }

      USTDEX_HOST_DEVICE env_of_t<Rcvr> get_env() const noexcept {
        return ustdex::get_env(_rcvr);
      }
    };

    template <class CvSndr, class Fn, class... Env>
    using _completions = _gather_completion_signatures<
      completion_signatures_of_t<CvSndr, Env...>,
      SetTag,
      _transform_completion<Fn>::template _f,
      _default_completions,
      _concat_completion_signatures>;

    template <class Fn, class Sndr>
    struct _sndr_t {
      using sender_concept = sender_t;
      USTDEX_NO_UNIQUE_ADDRESS UponTag _tag;
      Fn _fn;
      Sndr _sndr;

      template <class... Env>
      auto get_completion_signatures(Env &&...) && //
        -> _completions<Sndr, Fn, Env...>;

      template <class... Env>
      auto get_completion_signatures(Env &&...) const & //
        -> _completions<const Sndr &, Fn, Env...>;

      template <class Rcvr>
      USTDEX_HOST_DEVICE auto connect(Rcvr rcvr) &&                                  //
        noexcept(_nothrow_constructible<_opstate_t<Rcvr, Sndr, Fn>, Sndr, Rcvr, Fn>) //
        -> _opstate_t<Rcvr, Sndr, Fn> {
        return _opstate_t<Rcvr, Sndr, Fn>{
          static_cast<Sndr &&>(_sndr),
          static_cast<Rcvr &&>(rcvr),
          static_cast<Fn &&>(_fn)};
      }

      template <class Rcvr>
      USTDEX_HOST_DEVICE auto connect(Rcvr rcvr) const & //
        noexcept(_nothrow_constructible<
                 _opstate_t<Rcvr, const Sndr &, Fn>,
                 const Sndr &,
                 Rcvr,
                 const Fn &>) //
        -> _opstate_t<Rcvr, const Sndr &, Fn> {
        return _opstate_t<Rcvr, const Sndr &, Fn>{_sndr, static_cast<Rcvr &&>(rcvr), _fn};
      }

      USTDEX_HOST_DEVICE env_of_t<Sndr> get_env() const noexcept {
        return ustdex::get_env(_sndr);
      }
    };

    template <class Fn>
    struct _closure_t {
      using UponTag = decltype(_detail::_upon_tag<Disposition>());
      Fn _fn;

      template <class Sndr>
      USTDEX_HOST_DEVICE USTDEX_INLINE friend auto operator|(Sndr sndr, _closure_t &&_self) {
        return UponTag()(static_cast<Sndr &&>(sndr), static_cast<Fn &&>(_self._fn));
      }
    };

   public:
    template <class Sndr, class Fn>
    USTDEX_INLINE USTDEX_HOST_DEVICE auto operator()(Sndr sndr, Fn fn) const noexcept {
      return _sndr_t<Fn, Sndr>{{}, static_cast<Fn &&>(fn), static_cast<Sndr &&>(sndr)};
    }

    template <class Fn>
    USTDEX_INLINE USTDEX_HOST_DEVICE auto operator()(Fn fn) const noexcept {
      return _closure_t<Fn>{static_cast<Fn &&>(fn)};
    }
  };

  USTDEX_DEVICE constexpr struct then_t : _upon_t<_value> {
  } then{};

  USTDEX_DEVICE constexpr struct upon_error_t : _upon_t<_error> {
  } upon_error{};

  USTDEX_DEVICE constexpr struct upon_stopped_t : _upon_t<_stopped> {
  } upon_stopped{};
} // namespace ustdex
