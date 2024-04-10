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
#include "fwd_rcvr.hpp"
#include "tuple.hpp"
#include "variant.hpp"

namespace ustdex {
  template <class LetTag, class SetTag>
  struct _let {
   private:
    template <class, class...>
    using _empty_tuple = _tuple_for<>;

    /// @brief Computes the type of a variant of tuples to hold the results of
    /// the predecessor sender.
    template <class CvSndr, class Env>
    using _results = _impl::_gather_completion_signatures<
      completion_signatures_of_t<CvSndr, Env>,
      SetTag,
      _decayed_tuple,
      _empty_tuple,
      _variant>;

    template <class Fn, class Rcvr>
    struct _let_fn_opstate {
      template <class... As>
      using _f = connect_result_t<_call_result_t<Fn, _decay_t<As> &...>, Rcvr *>;
    };

    /// @brief Computes the type of a variant of operation states to hold
    /// the second operation state.
    template <class CvSndr, class Fn, class Rcvr>
    using _opstate2_t = _impl::_gather_completion_signatures<
      completion_signatures_of_t<CvSndr, Rcvr>,
      SetTag,
      _let_fn_opstate<Fn, Rcvr>::template _f,
      _empty_tuple,
      _variant>;

    /// @brief The `let_(value|error|stopped)` operation state.
    /// @tparam CvSndr The cvref-qualified predecessor sender type.
    /// @tparam Fn The function to be called when the predecessor sender
    /// completes.
    /// @tparam Rcvr The receiver connected to the `let_(value|error|stopped)`
    /// sender.
    template <class CvSndr, class Fn, class Rcvr>
    struct _opstate_t {
      using operation_state_concept = operation_state_t;
      Rcvr _rcvr;
      Fn _fn;
      _results<CvSndr, env_of_t<Rcvr>> _result;
      connect_result_t<CvSndr, _opstate_t *> _opstate1;
      _opstate2_t<CvSndr, Fn, Rcvr> _opstate2;

      USTDEX_HOST_DEVICE _opstate_t(CvSndr &&sndr, Fn fn, Rcvr rcvr)
        : _rcvr(static_cast<Rcvr &&>(rcvr))
        , _fn(static_cast<Fn &&>(fn))
        , _opstate1(ustdex::connect(static_cast<CvSndr &&>(sndr), this)) {
      }

      USTDEX_HOST_DEVICE
      void start() noexcept {
        ustdex::start(_opstate1);
      }

      template <class Tag, class... As>
      USTDEX_HOST_DEVICE
      void _complete(Tag, As &&...as) noexcept {
        if constexpr (USTDEX_IS_SAME(Tag, SetTag)) {
          // Store the results so the lvalue refs we pass to the function
          // will be valid for the duration of the async op.
          auto &tupl =
            _result.template emplace<_decayed_tuple<As...>>(static_cast<As &&>(as)...);
          // Call the function with the results and connect the resulting
          // sender, storing the operation state in _opstate2.
          auto &nextop = _opstate2.emplace_from(
            ustdex::connect, _apply(static_cast<Fn &&>(_fn), tupl), &_rcvr);
          ustdex::start(nextop);
        } else {
          // Forward the completion to the receiver unchanged.
          Tag()(static_cast<Rcvr &&>(_rcvr), static_cast<As &&>(as)...);
        }
      }

      template <class... As>
      USTDEX_HOST_DEVICE USTDEX_INLINE void set_value(As &&...as) noexcept {
        _complete(set_value_t(), static_cast<As &&>(as)...);
      }

      template <class Error>
      USTDEX_HOST_DEVICE USTDEX_INLINE void set_error(Error &&error) noexcept {
        _complete(set_error_t(), static_cast<Error &&>(error));
      }

      USTDEX_HOST_DEVICE USTDEX_INLINE void set_stopped() noexcept {
        _complete(set_stopped_t());
      }

      auto get_env() const noexcept {
        return ustdex::get_env(_rcvr);
      }
    };

    template <class Fn, class... Env>
    struct _completions_fn {
      template <class... As>
      using _f =
        completion_signatures_of_t<_call_result_t<Fn, _decay_t<As> &...>, Env...>;
    };

    /// @brief Computes the completion signatures of the
    /// `let_(value|error|stopped)` sender.
    template <class CvSndr, class Fn, class... Env>
    using _completions = _impl::_gather_completion_signatures<
      completion_signatures_of_t<CvSndr, Env...>,
      SetTag,
      _completions_fn<Fn, Env...>::template _f,
      _impl::_default_completions,
      _impl::_concat_completion_signatures>;

    /// @brief The `let_(value|error|stopped)` sender.
    /// @tparam Sndr The predecessor sender.
    /// @tparam Fn The function to be called when the predecessor sender
    /// completes.
    template <class Sndr, class Fn>
    struct _sndr_t {
      using sender_concept = sender_t;
      [[no_unique_address]] LetTag _tag;
      Fn _fn;
      Sndr _sndr;

      template <class... Env>
      auto get_completion_signatures(const Env &...) && //
        -> _completions<Sndr, Fn, Env...> {
        return {};
      }

      template <class... Env>
      auto get_completion_signatures(const Env &...) const & //
        -> _completions<const Sndr &, Fn, Env...> {
        return {};
      }

      template <class Rcvr>
      auto connect(Rcvr &&rcvr) && {
        return _opstate_t<Sndr, Fn, Rcvr>{
          static_cast<Sndr &&>(_sndr), _fn, static_cast<Rcvr &&>(rcvr)};
      }

      template <class Rcvr>
      auto connect(Rcvr &&rcvr) const & {
        return _opstate_t<const Sndr &, Fn, Rcvr>{_sndr, _fn, static_cast<Rcvr &&>(rcvr)};
      }

      decltype(auto) get_env() const noexcept {
        return ustdex::get_env(_sndr);
      }
    };

   public:
    template <class Sndr, class Fn>
    _sndr_t<Sndr, Fn> operator()(Sndr sndr, Fn fn) const {
      return _sndr_t<Sndr, Fn>{{}, static_cast<Fn &&>(fn), static_cast<Sndr &&>(sndr)};
    }
  };

  inline constexpr struct let_value_t : _let<let_value_t, set_value_t> {
  } let_value{};

  inline constexpr struct let_error_t : _let<let_error_t, set_error_t> {
  } let_error{};

  inline constexpr struct let_stopped_t : _let<let_stopped_t, set_stopped_t> {
  } let_stopped{};
} // namespace ustdex
