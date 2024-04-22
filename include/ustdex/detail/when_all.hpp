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

#include "completion_signatures.hpp"
#include "cpos.hpp"
#include "lazy.hpp"
#include "stop_token.hpp"
#include "tuple.hpp"
#include "type_traits.hpp"
#include "utility.hpp"
#include "variant.hpp"

#include <atomic>

USTDEX_PRAGMA_PUSH()
USTDEX_PRAGMA_IGNORE_EDG(expr_has_no_effect)
USTDEX_PRAGMA_IGNORE_GNU("-Wunused-value")

namespace ustdex {
  // Forward declare the when_all tag type:
  struct when_all_t;

  // Some mechanics for computing a when_all sender's completion signatures:
  namespace _when_all {
    template <class>
    struct _env_t;

    template <class, std::size_t>
    struct _rcvr_t;

    template <class, class, class>
    struct _opstate_t;

    struct SENDER_HAS_TOO_MANY_SUCCESS_COMPLETIONS;

    using _tombstone =
      ERROR<WHERE(IN_ALGORITHM, when_all_t), WHAT(SENDER_HAS_TOO_MANY_SUCCESS_COMPLETIONS)>;

    // Use this to short-circuit the computation of whether all values and
    // errors are nothrow decay-copyable.
    template <class Bool>
    struct _all_nothrow_decay_copyable {
      static_assert(USTDEX_IS_SAME(Bool, _mtrue));
      template <class... Ts>
      using _f = _mbool<_nothrow_decay_copyable<Ts...>>;
    };

    template <>
    struct _all_nothrow_decay_copyable<_mfalse> {
      template <class... Ts>
      using _f = _mfalse;
    };

    //////////////////////////////////////////////////////////////////////////////////////
    // This type is used to compute the completion signatures contributed by one of
    // when_all's child senders. It tracks the completions, whether decay-copying the
    // values and errors can throw, and also which of the when_all's value result
    // datums this sender is responsible for setting.
    //
    // Leave this undefined:
    template <class NothrowVals, class NothrowErrors, class Offsets, class... Sigs>
    struct _completion_metadata;

    //////////////////////////////////////////////////////////////////////////////////////
    // Convert the metadata type into a completion signatures type by adding a
    // set_error_t(exception_ptr) completion if decay-copying any of the values
    // or errors is possibly throwing, and then removing duplicate completion
    // signatures.
    template <class>
    struct _reduce_completions;

    template <class ValsOK, class ErrsOK, class Offsets, class... Sigs>
    struct _reduce_completions<_completion_metadata<ValsOK, ErrsOK, Offsets, Sigs...> &> {
      using type = _mpair< //
        _concat_completion_signatures<
          completion_signatures<Sigs..., set_error_t(std::exception_ptr)>>,
        Offsets>;
    };

    template <class Offsets, class... Sigs>
    struct _reduce_completions<_completion_metadata<_mtrue, _mtrue, Offsets, Sigs...> &> {
      using type =
        _mpair<_concat_completion_signatures<completion_signatures<Sigs...>>, Offsets>;
    };

    template <class Ty>
    using _reduce_completions_t = _t<_reduce_completions<Ty>>;

    //////////////////////////////////////////////////////////////////////////////////////
    // _append_completion
    //
    // We use a set of partial specialization of the _append_completion variable
    // template to append the metadata from a single completion signature into a
    // metadata struct, and we use a fold expression to append all N completion
    // signatures.
    template <class Metadata, class Sig>
    extern _undefined<Metadata> _append_completion;

    template <class ValsOK, class ErrsOK, class... Sigs, class Tag, class... As>
    extern _completion_metadata<
      ValsOK,
      _minvoke<_all_nothrow_decay_copyable<ErrsOK>, As...>,
      _moffsets<>,
      Sigs...,
      Tag(_decay_t<As>...)>
      &_append_completion<
        _completion_metadata<ValsOK, ErrsOK, _moffsets<>, Sigs...>,
        Tag(As...)>;

    // This overload is selected when we see the first set_value_t completion
    // signature.
    template <class ValsOK, class ErrsOK, class... Sigs, class... As>
    extern _completion_metadata<
      _minvoke<_all_nothrow_decay_copyable<ValsOK>, As...>,
      ErrsOK,
      _moffsets<>,
      set_value_t(_decay_t<As>...), // Insert the value signature at the front
      Sigs...>
      &_append_completion<
        _completion_metadata<ValsOK, ErrsOK, _moffsets<>, Sigs...>,
        set_value_t(As...)>;

    // This overload is selected when we see the second set_value_t completion
    // signature. Senders passed to when_all are only allowed one set_value
    // completion.
    template <class ValsOK, class ErrsOK, class... Sigs, class... As, class... Bs>
    extern _tombstone &_append_completion<
      _completion_metadata<ValsOK, ErrsOK, _moffsets<>, set_value_t(As...), Sigs...>,
      set_value_t(Bs...)>;

    // This overload is selected when we see the second set_value_t completion
    // signature. Senders passed to when_all are only allowed one set_value
    // completion.
    template <class Sig>
    extern _tombstone &_append_completion<_tombstone &, Sig>;

    // We use a fold expression over the bitwise OR operator to append all of the
    // completion signatures from one child sender into a metadata struct.
    template <class Metadata, class Sig>
    auto operator|(Metadata &, Sig *) -> decltype(_append_completion<Metadata, Sig>);

    // The initial value of the fold expression:
    using _inner_fold_init = _completion_metadata<_mtrue, _mtrue, _moffsets<>>;

    template <class... Sigs>
    using _collect_inner = //
      decltype((DECLVAL(_inner_fold_init &) | ... | static_cast<Sigs *>(nullptr)));

    //////////////////////////////////////////////////////////////////////////////////////
    // _merge_metadata
    //
    // After computing a metadata struct for each child sender, all the metadata
    // structs must be merged. We use a set of partial specialization of the
    // _merge_metadata variable template to merge two metadata structs into one,
    // and we use a fold expression to merge all N into one.
    template <class Meta1, class Meta2>
    extern _undefined<Meta1> _merge_metadata;

    // This specialization causes an error to be propagated.
    template <class ValsOK, class ErrsOK, class Offsets, class... LeftSigs, class... What>
    extern ERROR<What...> &_merge_metadata<
      _completion_metadata<ValsOK, ErrsOK, Offsets, LeftSigs...>,
      ERROR<What...>>;

    // This overload is selected with the left and right metadata are both for senders
    // that have no set_value completion signature.
    template <
      class LeftValsOK,
      class LeftErrsOK,
      class Offsets,
      class... LeftSigs,
      class RightValsOK,
      class RightErrsOK,
      class... RightSigs>
    extern _completion_metadata<
      _mtrue,
      _mand<LeftErrsOK, RightErrsOK>,
      _moffsets<>,
      LeftSigs...,
      RightSigs...>
      &_merge_metadata<
        _completion_metadata<LeftValsOK, LeftErrsOK, Offsets, LeftSigs...>,
        _completion_metadata<RightValsOK, RightErrsOK, _moffsets<>, RightSigs...>>;

    // The following two specializations are selected when one of the metadata
    // structs is for a sender with no value completions. In that case, the
    // when_all can never complete successfully, so drop the other set_value
    // completion signature.
    template <
      class LeftValsOK,
      class LeftErrsOK,
      class Offsets,
      class... As,
      class... LeftSigs,
      class RightValsOK,
      class RightErrsOK,
      class... RightSigs>
    extern _completion_metadata<
      _mtrue, // There will be no value completion, so values need not be copied.
      _mand<LeftErrsOK, RightErrsOK>,
      _moffsets<>,
      LeftSigs...,
      RightSigs...>
      &_merge_metadata<
        _completion_metadata<LeftValsOK, LeftErrsOK, Offsets, set_value_t(As...), LeftSigs...>,
        _completion_metadata<RightValsOK, RightErrsOK, _moffsets<>, RightSigs...>>;

    template <
      class LeftValsOK,
      class LeftErrsOK,
      class Offsets,
      class... LeftSigs,
      class RightValsOK,
      class RightErrsOK,
      class... As,
      class... RightSigs>
    extern _completion_metadata<
      _mtrue, // There will be no value completion, so values need not be copied.
      _mand<LeftErrsOK, RightErrsOK>,
      _moffsets<>,
      LeftSigs...,
      RightSigs...>
      &_merge_metadata<
        _completion_metadata<LeftValsOK, LeftErrsOK, Offsets, LeftSigs...>,
        _completion_metadata<
          RightValsOK,
          RightErrsOK,
          _moffsets<>,
          set_value_t(As...),
          RightSigs...>>;

    // This overload is selected when both metadata structs are for senders with
    // a single value completion. Concatenate the value types.
    template <
      class LeftValsOK,
      class LeftErrsOK,
      auto... Offsets,
      class... As,
      class... LeftSigs,
      class RightValsOK,
      class RightErrsOK,
      class... Bs,
      class... RightSigs>
    extern _completion_metadata<
      _mand<LeftValsOK, RightValsOK>,
      _mand<LeftErrsOK, RightErrsOK>,
      _moffsets<Offsets..., sizeof...(Bs) + (0, ..., Offsets)>,
      set_value_t(As..., Bs...), // Concatenate the value types.
      LeftSigs...,
      RightSigs...>
      &_merge_metadata<
        _completion_metadata<
          LeftValsOK,
          LeftErrsOK,
          _moffsets<Offsets...>,
          set_value_t(As...),
          LeftSigs...>,
        _completion_metadata<
          RightValsOK,
          RightErrsOK,
          _moffsets<>,
          set_value_t(Bs...),
          RightSigs...>>;

    template <class... What, class Other>
    extern ERROR<What...> &_merge_metadata<ERROR<What...> &, Other>;

    // We use a fold expression over the bitwise AND operator to merge all the
    // completion metadata structs from the child senders into a single metadata
    // struct.
    template <class Meta1, class Meta2>
    auto operator&(Meta1 &, Meta2 &) -> decltype(_merge_metadata<Meta1, Meta2>);

    // The initial value for the fold.
    using _outer_fold_init =
      _completion_metadata<_mtrue, _mtrue, _moffsets<0ul>, set_value_t(), set_stopped_t()>;

    template <class... Sigs>
    using _collect_outer = //
      _reduce_completions_t<decltype((DECLVAL(_outer_fold_init &) & ... & DECLVAL(Sigs)))>;

    // TODO: The Env argument to completion_signatures_of_t should be a
    // forwarding env that updates the stop token.
    template <class Sndr, class... Env>
    using _inner_completions_ = //
      _mapply_q<_collect_inner, completion_signatures_of_t<Sndr, Env...>>;

    template <class Sndr, class... Env>
    using _inner_completions = //
      _midentity_or_error_with<_inner_completions_<Sndr, Env...>, WITH_SENDER(Sndr)>;

    enum _state_t {
      _started,
      _error,
      _stopped
    };

    /// @brief The data stored in the operation state and refered to
    /// by the receiver.
    /// @tparam Rcvr The receiver connected to the when_all sender.
    /// @tparam Values A _lazy_tuple of the values.
    /// @tparam Errors A _variant of the possible errors.
    /// @tparam StopToken The stop token from the outer receiver's env.
    template <class Rcvr, class Values, class Errors, class StopToken>
    struct _data_t {
      using stop_callback_t = stop_callback_for_t<StopToken, _on_stop_request>;

      template <std::size_t Offset, std::size_t... Idx, class... Ts>
      USTDEX_HOST_DEVICE void _set_value(_mindices<Idx...>, Ts &&...ts) noexcept {
        if constexpr (!USTDEX_IS_SAME(Values, _nil)) {
          if constexpr (_nothrow_decay_copyable<Ts...>) {
            (_values.template _emplace<Idx + Offset>(static_cast<Ts &&>(ts)), ...);
          } else {
            try {
              (_values.template _emplace<Idx + Offset>(static_cast<Ts &&>(ts)), ...);
            } catch (...) {
              _set_error(std::current_exception());
            }
          }
        }
      }

      template <class Error>
      USTDEX_HOST_DEVICE void _set_error(Error &&_err) noexcept {
        // TODO: Use weaker memory orders
        if (_error != _state.exchange(_error)) {
          _stop_source.request_stop();
          // We won the race, free to write the error into the operation state
          // without worry.
          if constexpr (_nothrow_decay_copyable<Error>) {
            _errors.template emplace<_decay_t<Error>>(static_cast<Error &&>(_err));
          } else {
            try {
              _errors.template emplace<_decay_t<Error>>(static_cast<Error &&>(_err));
            } catch (...) {
              _errors.template emplace<std::exception_ptr>(std::current_exception());
            }
          }
        }
      }

      USTDEX_HOST_DEVICE void _set_stopped() noexcept {
        _state_t expected = _started;
        // Transition to the "stopped" state if and only if we're in the
        // "started" state. (If this fails, it's because we're in an
        // error state, which trumps cancellation.)
        if (_state.compare_exchange_strong(expected, _stopped)) {
          _stop_source.request_stop();
        }
      }

      void _arrive() noexcept {
        if (0 == --_count) {
          _complete();
        }
      }

      void _complete() noexcept {
        // Stop callback is no longer needed. Destroy it.
        _on_stop.destroy();
        // All child operations have completed and arrived at the barrier.
        switch (_state.load(std::memory_order_relaxed)) {
        case _started:
          if constexpr (!USTDEX_IS_SAME(Values, _nil)) {
            // All child operations completed successfully:
            _values.apply(
              ustdex::set_value,
              static_cast<Values &&>(_values),
              static_cast<Rcvr &&>(_rcvr));
          }
          break;
        case _error:
          // One or more child operations completed with an error:
          _errors.visit(
            ustdex::set_error,
            static_cast<Errors &&>(_errors),
            static_cast<Rcvr &&>(_rcvr));
          break;
        case _stopped:
          ustdex::set_stopped(static_cast<Rcvr &&>(_rcvr));
          break;
        default:;
        }
      }

      Rcvr _rcvr;
      std::atomic<std::size_t> _count;
      inplace_stop_source _stop_source;
      inplace_stop_token _stop_token{_stop_source.get_token()};
      std::atomic<_state_t> _state{_started};
      Errors _errors;
      Values _values;
      _lazy<stop_callback_t> _on_stop;
    };

    /// The receivers connected to the when_all's sub-operations expose this as
    /// their environment. Its `get_stop_token` query returns the token from
    /// when_all's stop source. All other queries are forwarded to the outer
    /// receiver's environment.
    template <class Rcvr, class Values, class Errors, class StopToken>
    struct _env_t<_data_t<Rcvr, Values, Errors, StopToken>> {
      _data_t<Rcvr, Values, Errors, StopToken> *_data;

      USTDEX_HOST_DEVICE inplace_stop_token query(get_stop_token_t) const noexcept {
        return _data->_stop_token;
      }

      template <class Tag>
      USTDEX_HOST_DEVICE auto query(Tag) const noexcept //
        -> _query_result_t<Tag, env_of_t<Rcvr>> {
        return ustdex::get_env(_data->_rcvr).query(Tag());
      }
    };

    template <class Rcvr, class Values, class Errors, class StopToken, std::size_t Offset>
    struct _rcvr_t<_data_t<Rcvr, Values, Errors, StopToken>, Offset> {
      using receiver_concept = receiver_t;

      using _data_t = _when_all::_data_t<Rcvr, Values, Errors, StopToken>;
      _data_t *_data;

      template <class... Ts>
      USTDEX_HOST_DEVICE USTDEX_INLINE void set_value(Ts &&...ts) noexcept {
        constexpr auto idx = _mmake_indices<sizeof...(Ts)>();
        _data->template _set_value<Offset>(idx, static_cast<Ts &&>(ts)...);
        _data->_arrive();
      }

      template <class Error>
      USTDEX_HOST_DEVICE USTDEX_INLINE void set_error(Error &&error) noexcept {
        _data->_set_error(static_cast<Error &&>(error));
        _data->_arrive();
      }

      USTDEX_HOST_DEVICE void set_stopped() noexcept {
        _data->_set_stopped();
        _data->_arrive();
      }

      USTDEX_HOST_DEVICE auto get_env() const noexcept -> _env_t<_data_t> {
        return _env_t<_data_t>{_data};
      }
    };

    // This function object is used to connect all the sub-operations with
    // receivers, each of which knows which elements in the values tuple it
    // is responsible for setting.
    struct _connect_subs_fn {
      template <class Data, std::size_t... Offsets, std::size_t... Idx, class... CvSndrs>
      USTDEX_HOST_DEVICE auto operator()(
        Data *data,
        _moffsets<Offsets...> *,
        _mindices<Idx...> y,
        CvSndrs &&...sndrs) const {
        if constexpr (0 != sizeof...(Offsets)) {
          // The offsets are used to determine which elements in the values
          // tuple each receiver is responsible for setting.
          constexpr std::size_t offsets[] = {Offsets...};
          return _tupl{ustdex::connect(
            static_cast<CvSndrs &&>(sndrs), _rcvr_t<Data, offsets[Idx]>{data})...};
        } else {
          // When there are no offsets, the when_all sender has no value
          // completions. All child senders can be connected to receivers
          // of the same type.
          return _tupl{
            ustdex::connect(static_cast<CvSndrs &&>(sndrs), _rcvr_t<Data, 0>{data})...};
        }
      }
    };

    /// The operation state for when_all
    template <class CvFn, std::size_t... Idx, class... Sndrs, class Rcvr>
    struct _opstate_t<CvFn, _tupl<std::index_sequence<Idx...>, Sndrs...>, Rcvr> {
      using operation_state_concept = operation_state_t;
      using _sndrs_t = _minvoke<CvFn, _tuple<Sndrs...>>;
      using _env_t = env_of_t<Rcvr>;
      using _completions_offsets_pair_t = //
        _collect_outer<_inner_completions<_minvoke1<CvFn, Sndrs>, _env_t>...>;
      using _completions_t = _mfirst<_completions_offsets_pair_t>;
      using _indices_t = _mindices<Idx...>;
      using _offsets_t = _msecond<_completions_offsets_pair_t>;
      using _values_t = _value_types<_completions_t, _lazy_tuple, _msingle_or<_nil>::_f>;
      using _errors_t = _error_types<_completions_t, _variant>;
      using _stok_t = stop_token_of_t<_env_t>;
      using _data_t = _when_all::_data_t<Rcvr, _values_t, _errors_t, _stok_t>;

      // This is a _tuple of operation states for the sub-operations.
      using _sub_opstates_t =
        _apply_result_t<_connect_subs_fn, _sndrs_t, _data_t *, _offsets_t *, _indices_t>;

      _data_t _data;
      _sub_opstates_t _sub_ops;

      /// Initialize the data member, connect all the sub-operations and
      /// save the resulting operation states in _sub_ops.
      USTDEX_HOST_DEVICE _opstate_t(_sndrs_t &&sndrs, Rcvr rcvr)
        : _data{static_cast<Rcvr &&>(rcvr), sizeof...(Sndrs)}
        , _sub_ops{sndrs.apply(
            _connect_subs_fn(),
            static_cast<_sndrs_t &&>(sndrs),
            &_data,
            static_cast<_offsets_t *>(nullptr),
            _indices_t())} {
      }

      USTDEX_IMMOVABLE(_opstate_t);

      /// Start all the sub-operations.
      USTDEX_HOST_DEVICE void start() & noexcept {
        // register stop callback:
        _data._on_stop.construct(
          get_stop_token(ustdex::get_env(_data._rcvr)),
          _on_stop_request{_data._stop_source});

        if (_data._stop_source.stop_requested()) {
          // Manually clean up the stop callback. We won't be starting the
          // sub-operations, so they won't complete and clean up for us.
          _data._on_stop.destroy();

          // Stop has already been requested. Don't bother starting the child
          // operations.
          ustdex::set_stopped(static_cast<Rcvr &&>(_data._rcvr));
        } else {
          // Start all the sub-operations.
          _sub_ops.for_each(ustdex::start, _sub_ops);

          // If there are no sub-operations, we're done.
          if constexpr (sizeof...(Sndrs) == 0) {
            _data._complete();
          }
        }
      }
    };

    template <class... Sndrs>
    struct _sndr_t;
  } // namespace _when_all

  struct when_all_t {
    template <class... Sndrs>
    USTDEX_HOST_DEVICE _when_all::_sndr_t<Sndrs...> operator()(Sndrs... sndrs) const;
  };

  // The sender for when_all
  template <class... Sndrs>
  struct _when_all::_sndr_t {
    using sender_concept = sender_t;
    using _sndrs_t = _tuple<Sndrs...>;

    template <class CvFn, class... Env>
    using _completions = //
      _minvoke_q<        //
        _mfirst,
        _when_all::_collect_outer<
          _when_all::_inner_completions<_minvoke1<CvFn, Sndrs>, Env...>...>>;

    USTDEX_NO_UNIQUE_ADDRESS when_all_t _tag;
    USTDEX_NO_UNIQUE_ADDRESS _ignore_t _ignore1;
    _sndrs_t _sndrs;

    template <class... Env>
    auto get_completion_signatures(const Env &...) && //
      -> _completions<_cp, Env...>;

    template <class... Env>
    auto get_completion_signatures(const Env &...) const & //
      -> _completions<_cpclr, Env...>;

    template <class Rcvr>
    USTDEX_HOST_DEVICE auto connect(Rcvr rcvr) && -> _opstate_t<_cp, _sndrs_t, Rcvr> {
      return _opstate_t<_cp, _sndrs_t, Rcvr>(
        static_cast<_sndrs_t &&>(_sndrs), static_cast<Rcvr &&>(rcvr));
    }

    template <class Rcvr>
    USTDEX_HOST_DEVICE auto connect(Rcvr rcvr) const & //
      -> _opstate_t<_cpclr, _sndrs_t, Rcvr> {
      return _opstate_t<_cpclr, _sndrs_t, Rcvr>(_sndrs, static_cast<Rcvr &&>(rcvr));
    }
  };

  template <class... Sndrs>
  USTDEX_HOST_DEVICE _when_all::_sndr_t<Sndrs...>
    when_all_t::operator()(Sndrs... sndrs) const {
    return _when_all::_sndr_t<Sndrs...>{{}, {}, {static_cast<Sndrs &&>(sndrs)...}};
  }

  USTDEX_DEVICE constexpr when_all_t when_all{};

} // namespace ustdex

USTDEX_PRAGMA_POP()
