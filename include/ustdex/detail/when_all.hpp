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

#ifndef USTDEX_ASYNC_DETAIL_WHEN_ALL
#define USTDEX_ASYNC_DETAIL_WHEN_ALL

#include "completion_signatures.hpp"
#include "concepts.hpp"
#include "config.hpp"
#include "cpos.hpp"
#include "env.hpp"
#include "exception.hpp"
#include "exclusive_scan.hpp"
#include "lazy.hpp"
#include "meta.hpp"
#include "stop_token.hpp"
#include "tuple.hpp"
#include "type_traits.hpp"
#include "utility.hpp"
#include "variant.hpp"

#include <array>

#include "prologue.hpp"

namespace ustdex
{
struct USTDEX_TYPE_VISIBILITY_DEFAULT when_all_t
{
private:
  template <class... Sndrs>
  struct USTDEX_TYPE_VISIBILITY_DEFAULT _sndr_t;

  // Extract the first template parameter of the _state_t specialization.
  // The first template parameter is the receiver type.
  template <class State>
  using _rcvr_from_state_t = _m_apply<_m_at<0>, State>;

  // Returns the completion signatures of a child sender. Throws an exception if
  // the child sender has more than one set_value completion signature.
  template <class Child, class... Env>
  USTDEX_API static constexpr auto _child_completions();

  // Merges the completion signatures of the child senders into a single set of
  // completion signatures for the when_all sender.
  template <class... Completions>
  USTDEX_API static constexpr auto _merge_completions(Completions...);

  /// The receivers connected to the when_all's sub-operations expose this as
  /// their environment. Its `get_stop_token` query returns the token from
  /// when_all's stop source. All other queries are forwarded to the outer
  /// receiver's environment.
  template <class StateZip>
  struct USTDEX_TYPE_VISIBILITY_DEFAULT _env_t
  {
    using _state_t = _unzip<StateZip>;
    using _rcvr_t  = _rcvr_from_state_t<_state_t>;

    _state_t& _state_;

    USTDEX_API inplace_stop_token query(get_stop_token_t) const noexcept
    {
      return _state_._stop_token_;
    }

    // TODO: only forward the "forwarding" queries
    template <class Tag>
    USTDEX_API auto query(Tag) const noexcept -> _query_result_t<Tag, env_of_t<_rcvr_t>>
    {
      return ustdex::get_env(_state_._rcvr_).query(Tag());
    }
  };

  template <class StateZip, std::size_t Index>
  struct USTDEX_TYPE_VISIBILITY_DEFAULT _rcvr_t
  {
    using receiver_concept = receiver_t;
    using _state_t         = _unzip<StateZip>;

    _state_t& _state_;

    template <class... Ts>
    USTDEX_TRIVIAL_API void set_value(Ts&&... _ts) noexcept
    {
      constexpr std::index_sequence_for<Ts...>* idx = nullptr;
      _state_.template _set_value<Index>(idx, static_cast<Ts&&>(_ts)...);
      _state_._arrive();
    }

    template <class Error>
    USTDEX_TRIVIAL_API void set_error(Error&& _error) noexcept
    {
      _state_._set_error(static_cast<Error&&>(_error));
      _state_._arrive();
    }

    USTDEX_API void set_stopped() noexcept
    {
      _state_._set_stopped();
      _state_._arrive();
    }

    USTDEX_API auto get_env() const noexcept -> _env_t<StateZip>
    {
      return {_state_};
    }
  };

  enum _estate_t : int
  {
    _started,
    _error,
    _stopped
  };

  /// @brief The data stored in the operation state and referred to
  /// by the receiver.
  /// @tparam Rcvr The receiver connected to the when_all sender.
  /// @tparam CvFn A metafunction to apply cv- and ref-qualifiers to the senders
  /// @tparam Sndrs A tuple of the when_all sender's child senders.
  template <class Rcvr, class CvFn, class Sndrs>
  struct _state_t;

  template <class Rcvr, class CvFn, class Idx, class... Sndrs>
  struct _state_t<Rcvr, CvFn, _tupl<Idx, Sndrs...>>
  {
    using _env_t     = when_all_t::_env_t<_zip<_state_t>>;
    using _sndr_t    = when_all_t::_sndr_t<Sndrs...>;
    using _cv_sndr_t = _m_call1<CvFn, _sndr_t>;

    static constexpr auto _completions_and_offsets =
      _sndr_t::template _get_completions_and_offsets<_cv_sndr_t, _env_t>();

    using _completions_t   = decltype(_completions_and_offsets.first);
    using _values_t        = _value_types<_completions_t, _lazy_tuple, _m_self_or<_nil>::call>;
    using _errors_t        = _error_types<_completions_t, _variant>;
    using _stop_tok_t      = stop_token_of_t<env_of_t<Rcvr>>;
    using _stop_callback_t = stop_callback_for_t<_stop_tok_t, _on_stop_request>;

    USTDEX_API explicit _state_t(Rcvr _rcvr, std::size_t _count)
        : _rcvr_{static_cast<Rcvr&&>(_rcvr)}
        , _count_{_count}
        , _stop_source_{}
        , _stop_token_{_stop_source_.get_token()}
        , _state_{_started}
        , _errors_{}
        , _values_{}
        , _on_stop_{}
    {}

    template <std::size_t Index, std::size_t... Jdx, class... Ts>
    USTDEX_API void _set_value(std::index_sequence<Jdx...>*, [[maybe_unused]] Ts&&... _ts) noexcept
    {
      if constexpr (!std::is_same_v<_values_t, _nil>)
      {
        constexpr std::size_t Offset = _completions_and_offsets.second[Index];
        if constexpr (_nothrow_decay_copyable<Ts...>)
        {
          (_values_.template _emplace<Jdx + Offset>(static_cast<Ts&&>(_ts)), ...);
        }
        else
        {
          USTDEX_TRY( //
            ({ //
              (_values_.template _emplace<Jdx + Offset>(static_cast<Ts&&>(_ts)), ...);
            }),
            USTDEX_CATCH(...) //
            ({ //
              _set_error(::std::current_exception());
            }) //
          )
        }
      }
    }

    template <class Error>
    USTDEX_API void _set_error(Error&& _err) noexcept
    {
      // TODO: Use weaker memory orders
      if (_error != _state_.exchange(_error))
      {
        _stop_source_.request_stop();
        // We won the race, free to write the error into the operation state
        // without worry.
        if constexpr (_nothrow_decay_copyable<Error>)
        {
          _errors_.template _emplace<USTDEX_DECAY(Error)>(static_cast<Error&&>(_err));
        }
        else
        {
          USTDEX_TRY( //
            ({ //
              _errors_.template _emplace<USTDEX_DECAY(Error)>(static_cast<Error&&>(_err));
            }),
            USTDEX_CATCH(...) //
            ({ //
              _errors_.template _emplace<::std::exception_ptr>(::std::current_exception());
            }) //
          )
        }
      }
    }

    USTDEX_API void _set_stopped() noexcept
    {
      std::underlying_type_t<_estate_t> _expected = _started;
      // Transition to the "stopped" state if and only if we're in the
      // "started" state. (If this fails, it's because we're in an
      // error state, which trumps cancellation.)
      if (_state_.compare_exchange_strong(_expected, static_cast<std::underlying_type_t<_estate_t>>(_stopped)))
      {
        _stop_source_.request_stop();
      }
    }

    USTDEX_API void _arrive() noexcept
    {
      if (0 == --_count_)
      {
        _complete();
      }
    }

    USTDEX_API void _complete() noexcept
    {
      // Stop callback is no longer needed. Destroy it.
      _on_stop_.destroy();
      // All child operations have completed and arrived at the barrier.
      switch (_state_.load(std::memory_order_relaxed))
      {
        case _started:
          if constexpr (!std::is_same_v<_values_t, _nil>)
          {
            // All child operations completed successfully:
            _values_.apply(ustdex::set_value, static_cast<_values_t&&>(_values_), static_cast<Rcvr&&>(_rcvr_));
          }
          break;
        case _error:
          // One or more child operations completed with an error:
          _errors_._visit(ustdex::set_error, static_cast<_errors_t&&>(_errors_), static_cast<Rcvr&&>(_rcvr_));
          break;
        case _stopped:
          ustdex::set_stopped(static_cast<Rcvr&&>(_rcvr_));
          break;
        default:;
      }
    }

    Rcvr _rcvr_;
    std::atomic<std::size_t> _count_;
    inplace_stop_source _stop_source_;
    inplace_stop_token _stop_token_;
    std::atomic<std::underlying_type_t<_estate_t>> _state_;
    _errors_t _errors_;
    // USTDEX_NO_UNIQUE_ADDRESS // gcc doesn't like this
    _values_t _values_;
    _lazy<_stop_callback_t> _on_stop_;
  };

  /// The operation state for when_all
  template <class, class, class>
  struct USTDEX_TYPE_VISIBILITY_DEFAULT _opstate_t;

  template <class Rcvr, class CvFn, std::size_t... Idx, class... Sndrs>
  struct USTDEX_TYPE_VISIBILITY_DEFAULT _opstate_t<Rcvr, CvFn, _tupl<std::index_sequence<Idx...>, Sndrs...>>
  {
    using operation_state_concept = operation_state_t;
    using _sndrs_t                = _m_call<CvFn, _tuple<Sndrs...>>;
    using _state_t                = when_all_t::_state_t<Rcvr, CvFn, _tuple<Sndrs...>>;

    // This function object is used to connect all the sub-operations with
    // receivers, each of which knows which elements in the values tuple it
    // is responsible for setting.
    struct _connect_subs_fn
    {
      template <class... CvSndrs>
      USTDEX_API auto operator()(_state_t& _state, CvSndrs&&... _sndrs_) const
      {
        using _state_ref_t = _zip<_state_t>;
        // When there are no offsets, the when_all sender has no value
        // completions. All child senders can be connected to receivers
        // of the same type, saving template instantiations.
        [[maybe_unused]] constexpr bool _no_values =
          std::is_same_v<decltype(_state_t::_completions_and_offsets.second), _nil>;
        // The offsets are used to determine which elements in the values
        // tuple each receiver is responsible for setting.
        return _tupl{
          ustdex::connect(static_cast<CvSndrs&&>(_sndrs_), _rcvr_t<_state_ref_t, _no_values ? 0 : Idx>{_state})...};
      }
    };

    // This is a tuple of operation states for the sub-operations.
    using _sub_opstates_t = _apply_result_t<_connect_subs_fn, _sndrs_t, _state_t&>;

    _state_t _state_;
    _sub_opstates_t _sub_ops_;

    /// Initialize the data member, connect all the sub-operations and
    /// save the resulting operation states in _sub_ops_.
    USTDEX_API _opstate_t(_sndrs_t&& _sndrs_, Rcvr _rcvr)
        : _state_{static_cast<Rcvr&&>(_rcvr), sizeof...(Sndrs)}
        , _sub_ops_{_sndrs_.apply(_connect_subs_fn(), static_cast<_sndrs_t&&>(_sndrs_), _state_)}
    {}

    USTDEX_IMMOVABLE(_opstate_t);

    /// Start all the sub-operations.
    USTDEX_API void start() & noexcept
    {
      // register stop callback:
      _state_._on_stop_.construct(
        get_stop_token(ustdex::get_env(_state_._rcvr_)), _on_stop_request{_state_._stop_source_});

      if (_state_._stop_source_.stop_requested())
      {
        // Manually clean up the stop callback. We won't be starting the
        // sub-operations, so they won't complete and clean up for us.
        _state_._on_stop_.destroy();

        // Stop has already been requested. Don't bother starting the child
        // operations.
        ustdex::set_stopped(static_cast<Rcvr&&>(_state_._rcvr_));
      }
      else
      {
        // Start all the sub-operations.
        _sub_ops_.for_each(ustdex::start, _sub_ops_);

        // If there are no sub-operations, we're done.
        if constexpr (sizeof...(Sndrs) == 0)
        {
          _state_._complete();
        }
      }
    }
  };

  template <class... Ts>
  using _decay_all = _m_list<std::decay_t<Ts>...>;

public:
  template <class... Sndrs>
  USTDEX_API auto operator()(Sndrs... _sndrs) const -> _sndr_t<Sndrs...>;
};

template <class Child, class... Env>
USTDEX_API constexpr auto when_all_t::_child_completions()
{
  using _env_t = prop<get_stop_token_t, inplace_stop_token>;
  USTDEX_LET_COMPLETIONS(auto(_completions) = get_completion_signatures<Child, env<_env_t, FWD_ENV_T<Env>>...>())
  {
    if constexpr (_completions.count(set_value) > 1)
    {
      return invalid_completion_signature<WHERE(IN_ALGORITHM, when_all_t),
                                          WHAT(SENDER_HAS_TOO_MANY_SUCCESS_COMPLETIONS),
                                          WITH_SENDER(Child)>();
    }
    else
    {
      return _completions;
    }
  }
}

USTDEX_PRAGMA_PUSH()
USTDEX_PRAGMA_IGNORE_GNU("-Wunused-value")

template <class... Completions>
USTDEX_API constexpr auto when_all_t::_merge_completions(Completions... _cs)
{
  // Use USTDEX_LET_COMPLETIONS to ensure all completions are valid:
  USTDEX_LET_COMPLETIONS(auto(_tmp) = (completion_signatures{}, ..., _cs)) // NB: uses overloaded comma operator
  {
    auto _non_value_completions = concat_completion_signatures(
      completion_signatures<set_stopped_t()>(),
      transform_completion_signatures(_cs, _swallow_transform(), _decay_transform<set_error_t>())...);

    if constexpr (((0 == _cs.count(set_value)) || ...))
    {
      // at least one child sender has no value completions at all, so the
      // when_all will never complete with set_value. return just the error and
      // stopped completions.
      return _pair{_non_value_completions, _nil{}};
    }
    else
    {
      using _m_list_size_fn                                    = _m_bind_front<_m_indirect_q<_m_apply>, _m_size>;
      std::array<std::size_t, sizeof...(Completions)> _offsets = {
        _value_types<Completions, _m_list, _m_list_size_fn::call>::value...};
      (void) ustdex::exclusive_scan(_offsets.begin(), _offsets.end(), _offsets.begin(), 0ul);

      // All child senders have exactly one value completion signature, each of
      // which may have multiple arguments. Concatenate all the arguments into a
      // single set_value_t completion signature.
      using _values_t = _m_call< //
        _m_concat_into<_m_compose<_m_quote<completion_signatures>, _m_quote_f<set_value_t>>>, //
        _value_types<Completions, _decay_all, _identity_t>...>;
      // Add the value completion to the error and stopped completions.
      auto _local = _non_value_completions + _values_t();
      // Check if any of the values or errors are not nothrow decay-copyable.
      constexpr bool _all_nothrow_decay_copyable =
        (_value_types<Completions, _nothrow_decay_copyable_t, _identity_t>::value && ...);
      return _pair{_local + _eptr_completion_if<!_all_nothrow_decay_copyable>(), _offsets};
    }
  }
}

USTDEX_PRAGMA_POP()

// The sender for when_all
template <class... Sndrs>
struct USTDEX_TYPE_VISIBILITY_DEFAULT when_all_t::_sndr_t
{
  using sender_concept = sender_t;
  using _sndrs_t       = _tuple<Sndrs...>;

  USTDEX_NO_UNIQUE_ADDRESS when_all_t _tag_;
  USTDEX_NO_UNIQUE_ADDRESS _ignore _ignore1_;
  _sndrs_t _sndrs_;

  template <class Self, class... Env>
  USTDEX_API static constexpr auto _get_completions_and_offsets()
  {
    return _merge_completions(_child_completions<_copy_cvref_t<Self, Sndrs>, Env...>()...);
  }

  template <class Self, class... Env>
  USTDEX_API static constexpr auto get_completion_signatures()
  {
    return _get_completions_and_offsets<Self, Env...>().first;
  }

  template <class Rcvr>
  USTDEX_API auto connect(Rcvr _rcvr) && -> _opstate_t<Rcvr, _cp, _sndrs_t>
  {
    return _opstate_t<Rcvr, _cp, _sndrs_t>(static_cast<_sndrs_t&&>(_sndrs_), static_cast<Rcvr&&>(_rcvr));
  }

  template <class Rcvr>
  USTDEX_API auto connect(Rcvr _rcvr) const& -> _opstate_t<Rcvr, _cpclr, _sndrs_t>
  {
    return _opstate_t<Rcvr, _cpclr, _sndrs_t>(_sndrs_, static_cast<Rcvr&&>(_rcvr));
  }
};

template <class... Sndrs>
USTDEX_API auto when_all_t::operator()(Sndrs... _sndrs) const -> _sndr_t<Sndrs...>
{
  // If the incoming senders are non-dependent, we can check the completion
  // signatures of the composed sender immediately.
  if constexpr (((!dependent_sender<Sndrs>) && ...))
  {
    using _completions = completion_signatures_of_t<_sndr_t<Sndrs...>>;
    static_assert(_valid_completion_signatures<_completions>);
  }
  return _sndr_t<Sndrs...>{{}, {}, {static_cast<Sndrs&&>(_sndrs)...}};
}

inline constexpr when_all_t when_all{};

} // namespace ustdex

#include "epilogue.hpp"

#endif
