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

#ifndef USTDEX_ASYNC_DETAIL_LET_VALUE
#define USTDEX_ASYNC_DETAIL_LET_VALUE

#include "completion_signatures.hpp"
#include "concepts.hpp"
#include "config.hpp"
#include "cpos.hpp"
#include "exception.hpp"
#include "rcvr_ref.hpp"
#include "tuple.hpp"
#include "type_traits.hpp"
#include "variant.hpp"

#include "prologue.hpp"

namespace ustdex
{
// Declare types to use for diagnostics:
struct FUNCTION_MUST_RETURN_A_SENDER;
struct FUNCTION_RETURN_TYPE_IS_NOT_A_SENDER_IN_THE_CURRENT_ENVIRONMENT;

// Forward-declate the let_* algorithm tag types:
struct let_value_t;
struct let_error_t;
struct let_stopped_t;

// Map from a disposition to the corresponding tag types:
namespace detail
{
template <_disposition_t, class Void = void>
extern _m_undefined<Void> _let_tag;
template <class Void>
extern _fn_t<let_value_t>* _let_tag<_value, Void>;
template <class Void>
extern _fn_t<let_error_t>* _let_tag<_error, Void>;
template <class Void>
extern _fn_t<let_stopped_t>* _let_tag<_stopped, Void>;
} // namespace detail

template <_disposition_t Disposition>
struct _let
{
private:
  using LetTag = decltype(detail::_let_tag<Disposition>());
  using SetTag = decltype(detail::_set_tag<Disposition>());

  template <class...>
  using _empty_tuple = _tuple<>;

  /// @brief Computes the type of a variant of tuples to hold the results of
  /// the predecessor sender.
  template <class CvSndr, class Env>
  using _results =
    _gather_completion_signatures<completion_signatures_of_t<CvSndr, Env>, SetTag, _decayed_tuple, _variant>;

  template <class Fn, class Rcvr>
  struct _opstate_fn
  {
    template <class... As>
    using call = connect_result_t<_call_result_t<Fn, USTDEX_DECAY(As) & ...>, _rcvr_ref<Rcvr>>;
  };

  /// @brief Computes the type of a variant of operation states to hold
  /// the second operation state.
  template <class CvSndr, class Fn, class Rcvr>
  using _opstate2_t =
    _gather_completion_signatures<completion_signatures_of_t<CvSndr, FWD_ENV_T<env_of_t<Rcvr>>>,
                                  SetTag,
                                  _opstate_fn<Fn, Rcvr>::template call,
                                  _variant>;

  /// @brief The `let_(value|error|stopped)` operation state.
  /// @tparam CvSndr The cvref-qualified predecessor sender type.
  /// @tparam Fn The function to be called when the predecessor sender
  /// completes.
  /// @tparam Rcvr The receiver connected to the `let_(value|error|stopped)`
  /// sender.
  template <class Rcvr, class CvSndr, class Fn>
  struct USTDEX_TYPE_VISIBILITY_DEFAULT _opstate_t
  {
    using operation_state_concept = operation_state_t;
    using _env_t                  = FWD_ENV_T<env_of_t<Rcvr>>;

    // Compute the type of the variant of operation states
    using _opstate_variant_t = _opstate2_t<CvSndr, Fn, Rcvr>;

    USTDEX_API _opstate_t(CvSndr&& _sndr, Fn _fn, Rcvr _rcvr) noexcept(
      _nothrow_decay_copyable<Fn, Rcvr> && _nothrow_connectable<CvSndr, _opstate_t*>)
        : _rcvr_(static_cast<Rcvr&&>(_rcvr))
        , _fn_(static_cast<Fn&&>(_fn))
        , _opstate1_(ustdex::connect(static_cast<CvSndr&&>(_sndr), _rcvr_ref{*this}))
    {}

    USTDEX_API void start() noexcept
    {
      ustdex::start(_opstate1_);
    }

    template <class Tag, class... As>
    USTDEX_API void _complete(Tag, As&&... _as) noexcept
    {
      if constexpr (Tag() == SetTag())
      {
        USTDEX_TRY( //
          ({        //
            // Store the results so the lvalue refs we pass to the function
            // will be valid for the duration of the async op.
            auto& _tupl = _result_.template _emplace<_decayed_tuple<As...>>(static_cast<As&&>(_as)...);
            // Call the function with the results and connect the resulting
            // sender, storing the operation state in _opstate2_.
            auto& _next_op = _opstate2_._emplace_from(
              ustdex::connect, _tupl.apply(static_cast<Fn&&>(_fn_), _tupl), ustdex::_rcvr_ref{_rcvr_});
            ustdex::start(_next_op);
          }),
          USTDEX_CATCH(...) //
          ({                //
            ustdex::set_error(static_cast<Rcvr&&>(_rcvr_), ::std::current_exception());
          })                //
        )
      }
      else
      {
        // Forward the completion to the receiver unchanged.
        Tag()(static_cast<Rcvr&&>(_rcvr_), static_cast<As&&>(_as)...);
      }
    }

    template <class... As>
    USTDEX_TRIVIAL_API void set_value(As&&... _as) noexcept
    {
      _complete(set_value_t(), static_cast<As&&>(_as)...);
    }

    template <class Error>
    USTDEX_TRIVIAL_API void set_error(Error&& _error) noexcept
    {
      _complete(set_error_t(), static_cast<Error&&>(_error));
    }

    USTDEX_TRIVIAL_API void set_stopped() noexcept
    {
      _complete(set_stopped_t());
    }

    USTDEX_API auto get_env() const noexcept -> _env_t
    {
      return ustdex::get_env(_rcvr_);
    }

    Rcvr _rcvr_;
    Fn _fn_;
    _results<CvSndr, _env_t> _result_;
    connect_result_t<CvSndr, _rcvr_ref<_opstate_t, _env_t>> _opstate1_;
    _opstate_variant_t _opstate2_;
  };

  template <class Fn, class... Env>
  struct _transform_args_fn
  {
    template <class... Ts>
    USTDEX_API constexpr auto operator()() const
    {
      if constexpr (!_decay_copyable<Ts...>)
      {
        return invalid_completion_signature<WHERE(IN_ALGORITHM, LetTag),
                                            WHAT(ARGUMENTS_ARE_NOT_DECAY_COPYABLE),
                                            WITH_ARGUMENTS(Ts...)>();
      }
      else if constexpr (!_callable<Fn, std::decay_t<Ts>&...>)
      {
        return invalid_completion_signature<WHERE(IN_ALGORITHM, LetTag),
                                            WHAT(FUNCTION_IS_NOT_CALLABLE),
                                            WITH_FUNCTION(Fn),
                                            WITH_ARGUMENTS(std::decay_t<Ts> & ...)>();
      }
      else if constexpr (!sender<_call_result_t<Fn, std::decay_t<Ts>&...>>)
      {
        return invalid_completion_signature<WHERE(IN_ALGORITHM, LetTag),
                                            WHAT(FUNCTION_MUST_RETURN_A_SENDER),
                                            WITH_FUNCTION(Fn),
                                            WITH_ARGUMENTS(std::decay_t<Ts> & ...)>();
      }
      else if constexpr (!sender_in<_call_result_t<Fn, std::decay_t<Ts>&...>, Env...>)
      {
        return invalid_completion_signature<WHERE(IN_ALGORITHM, LetTag),
                                            WHAT(FUNCTION_RETURN_TYPE_IS_NOT_A_SENDER_IN_THE_CURRENT_ENVIRONMENT),
                                            WITH_FUNCTION(Fn),
                                            WITH_ARGUMENTS(std::decay_t<Ts> & ...),
                                            WITH_ENVIRONMENT(Env...)>();
      }
      else
      {
        using Sndr = _call_result_t<Fn, std::decay_t<Ts>&...>;
        // The function is callable with the arguments and returns a sender, but we
        // do not know whether connect will throw.
        return concat_completion_signatures(get_completion_signatures<Sndr, Env...>(), _eptr_completion());
      }
    }
  };

  /// @brief The `let_(value|error|stopped)` sender.
  /// @tparam Sndr The predecessor sender.
  /// @tparam Fn The function to be called when the predecessor sender
  /// completes.
  template <class Sndr, class Fn>
  struct USTDEX_TYPE_VISIBILITY_DEFAULT _sndr_t
  {
    using sender_concept = sender_t;
    USTDEX_NO_UNIQUE_ADDRESS LetTag _tag_;
    Fn _fn_;
    Sndr _sndr_;

    template <class Self, class... Env>
    USTDEX_API static constexpr auto get_completion_signatures()
    {
      USTDEX_LET_COMPLETIONS(auto(_child_completions) = get_child_completion_signatures<Self, Sndr, Env...>())
      {
        if constexpr (Disposition == _disposition_t::_value)
        {
          return transform_completion_signatures(_child_completions, _transform_args_fn<Fn>{});
        }
        else if constexpr (Disposition == _disposition_t::_error)
        {
          return transform_completion_signatures(_child_completions, {}, _transform_args_fn<Fn>{});
        }
        else
        {
          return transform_completion_signatures(_child_completions, {}, {}, _transform_args_fn<Fn>{});
        }
      }
    }

    template <class Rcvr>
    USTDEX_API auto connect(Rcvr _rcvr) && noexcept(_nothrow_constructible<_opstate_t<Rcvr, Sndr, Fn>, Sndr, Fn, Rcvr>)
      -> _opstate_t<Rcvr, Sndr, Fn>
    {
      return _opstate_t<Rcvr, Sndr, Fn>(
        static_cast<Sndr&&>(_sndr_), static_cast<Fn&&>(_fn_), static_cast<Rcvr&&>(_rcvr));
    }

    template <class Rcvr>
    USTDEX_API auto connect(Rcvr _rcvr) const& noexcept( //
      _nothrow_constructible<_opstate_t<Rcvr, const Sndr&, Fn>,
                             const Sndr&,
                             const Fn&,
                             Rcvr>) //
      -> _opstate_t<Rcvr, const Sndr&, Fn>
    {
      return _opstate_t<Rcvr, const Sndr&, Fn>(_sndr_, _fn_, static_cast<Rcvr&&>(_rcvr));
    }

    USTDEX_API env_of_t<Sndr> get_env() const noexcept
    {
      return ustdex::get_env(_sndr_);
    }
  };

  template <class Fn>
  struct _closure_t
  {
    using LetTag = decltype(detail::_let_tag<Disposition>());
    Fn _fn_;

    template <class Sndr>
    USTDEX_TRIVIAL_API auto operator()(Sndr _sndr) const //
      -> _call_result_t<LetTag, Sndr, Fn>
    {
      return LetTag()(static_cast<Sndr&&>(_sndr), _fn_);
    }

    template <class Sndr>
    USTDEX_TRIVIAL_API friend auto operator|(Sndr _sndr, const _closure_t& _self) //
      -> _call_result_t<LetTag, Sndr, Fn>
    {
      return LetTag()(static_cast<Sndr&&>(_sndr), _self._fn_);
    }
  };

public:
  template <class Sndr, class Fn>
  USTDEX_API _sndr_t<Sndr, Fn> operator()(Sndr _sndr, Fn _fn) const
  {
    // If the incoming sender is non-dependent, we can check the completion
    // signatures of the composed sender immediately.
    if constexpr (!dependent_sender<Sndr>)
    {
      using _completions = completion_signatures_of_t<_sndr_t<Sndr, Fn>>;
      static_assert(_valid_completion_signatures<_completions>);
    }
    return _sndr_t<Sndr, Fn>{{}, static_cast<Fn&&>(_fn), static_cast<Sndr&&>(_sndr)};
  }

  template <class Fn>
  USTDEX_TRIVIAL_API auto operator()(Fn _fn) const noexcept
  {
    return _closure_t<Fn>{static_cast<Fn&&>(_fn)};
  }
};

inline constexpr struct let_value_t : _let<_value>
{
} let_value{};

inline constexpr struct let_error_t : _let<_error>
{
} let_error{};

inline constexpr struct let_stopped_t : _let<_stopped>
{
} let_stopped{};
} // namespace ustdex

#include "epilogue.hpp"

#endif
