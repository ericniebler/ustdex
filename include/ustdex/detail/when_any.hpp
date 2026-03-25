/*
 * SPDX-FileCopyrightText: Copyright (c) 2025 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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

#ifndef USTDEX_ASYNC_DETAIL_WHEN_ANY
#define USTDEX_ASYNC_DETAIL_WHEN_ANY

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
#include <utility>

#include "prologue.hpp"

namespace ustdex
{

	namespace _when_any 
	{
		template <class Rcvr>
		struct visitor_fn
		{
			Rcvr _rcvr;

			template <std::size_t... I, class Tag, class... As>
			void operator()(_tupl<std::index_sequence<I...>, Tag, As...> result) noexcept
			{
				result.apply(
					[this](Tag tag, As&... args) noexcept
					{
						tag(static_cast<Rcvr&&>(_rcvr), static_cast<As&&>(args)...);
					},
					result);
			}
		};

	}

	struct USTDEX_TYPE_VISIBILITY_DEFAULT when_any_t
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
		template <class Parent, class Child, class... Env>
		USTDEX_API static constexpr auto _child_completions();

		/// The receivers connected to the when_all's sub-operations expose this as
		/// their environment. Its `get_stop_token` query returns the token from
		/// when_all's stop source. All other queries are forwarded to the outer
		/// receiver's environment.
		template <class StateZip>
		struct USTDEX_TYPE_VISIBILITY_DEFAULT _env_t
		{
			using _state_t = _unzip<StateZip>;
			using _rcvr_t = _rcvr_from_state_t<_state_t>;

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

		template <class StateZip>
		struct USTDEX_TYPE_VISIBILITY_DEFAULT _rcvr_t
		{
			using receiver_concept = receiver_t;
			using _state_t = _unzip<StateZip>;

			_state_t& _state_;

			template <class... Ts>
			USTDEX_TRIVIAL_API void set_value(Ts&&... _ts) noexcept
			{
				_state_.notify(ustdex::set_value, static_cast<Ts&&>(_ts)...);
			}

			template <class Error>
			USTDEX_TRIVIAL_API void set_error(Error&& _error) noexcept
			{
				_state_.notify(ustdex::set_error, static_cast<Error&&>(_error));
			}

			USTDEX_API void set_stopped() noexcept
			{
				_state_.notify(ustdex::set_stopped);
			}

			USTDEX_API auto get_env() const noexcept -> _env_t<StateZip>
			{
				return { _state_ };
			}
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
			using _env_t = when_any_t::_env_t<_zip<_state_t>>;
			using _sndr_t = when_any_t::_sndr_t<Sndrs...>;
			using _cv_sndr_t = _m_call1<CvFn, _sndr_t>;

			using _completions_t = completion_signatures_of_t<_sndr_t, _env_t>;

			using _result_t = typename _completions_t::template _transform_q<_decayed_tuple, _variant>;
			using _stop_tok_t = stop_token_of_t<env_of_t<Rcvr>>;
			using _stop_callback_t = stop_callback_for_t<_stop_tok_t, _on_stop_request>;

			USTDEX_API explicit _state_t(Rcvr _rcvr, std::size_t _count)
				: _rcvr_{ static_cast<Rcvr&&>(_rcvr) }
				, _count_{ _count }
				, _stop_source_{}
				, _stop_token_{ _stop_source_.get_token() }
				, _result_{}
				, _on_stop_{}
			{}

			template <class Tag, class... Args>
			void notify(Tag tag, Args&&... args) noexcept
			{
				using result_t = _decayed_tuple<Tag, Args...>;
				bool expect = false;
				if (_emplaced_.compare_exchange_strong(expect, true, std::memory_order_relaxed, std::memory_order_relaxed))
				{
					// This emplacement can happen only once
					if constexpr ((_nothrow_decay_copyable<Args> && ...))
					{
						_result_.template _emplace<result_t>(tag, static_cast<Args&&>(args)...);
					}
					else
					{
						try
						{
							_result_.template _emplace<result_t>(tag, static_cast<Args&&>(args)...);
						}
						catch (...)
						{
							using error_t = _tuple<set_error_t, std::exception_ptr>;
							_result_.template _emplace<error_t>(set_error, std::current_exception());
						}
					}
					// stop pending operations
					_stop_source_.request_stop();
				}
				// make __result_ emplacement visible when __count_ goes from one to zero
				// This relies on the fact that each sender will call notify() at most once
				if (_count_.fetch_sub(1, std::memory_order_acq_rel) == 1)
				{
					_on_stop_.destroy();
					auto stop_token = get_stop_token(get_env(_rcvr_));
					if (stop_token.stop_requested())
					{
						ustdex::set_stopped(static_cast<Rcvr&&>(_rcvr_));
						return;
					}
					_result_._visit(_when_any::visitor_fn<Rcvr>{static_cast<Rcvr&&>(_rcvr_)}, static_cast<_result_t&&>(_result_));
				}
			}

			Rcvr _rcvr_;
			std::atomic<std::size_t> _count_;
			inplace_stop_source _stop_source_;
			inplace_stop_token _stop_token_;
			// USTDEX_NO_UNIQUE_ADDRESS // gcc doesn't like this
			_result_t _result_;
			_lazy<_stop_callback_t> _on_stop_;
			std::atomic<bool> _emplaced_{ false };
		};

		/// The operation state for when_all
		template <class, class, class>
		struct USTDEX_TYPE_VISIBILITY_DEFAULT _opstate_t;

		template <class Rcvr, class CvFn, class... Sndrs>
		struct USTDEX_TYPE_VISIBILITY_DEFAULT _opstate_t<Rcvr, CvFn, _tuple<Sndrs...>>
		{
			using operation_state_concept = operation_state_t;
			using _sndrs_t = _m_call<CvFn, _tuple<Sndrs...>>;
			using _state_t = when_any_t::_state_t<Rcvr, CvFn, _tuple<Sndrs...>>;

			// This function object is used to connect all the sub-operations with
			// receivers, each of which knows which elements in the values tuple it
			// is responsible for setting.
			struct _connect_subs_fn
			{
				template <class... CvSndrs>
				USTDEX_API auto operator()(_state_t& _state, CvSndrs&&... _sndrs_) const
				{
					using _state_ref_t = _zip<_state_t>;
					return _tupl{
					  ustdex::connect(static_cast<CvSndrs&&>(_sndrs_), _rcvr_t<_state_ref_t>{_state})... };
				}
			};

			// This is a tuple of operation states for the sub-operations.
			using _sub_opstates_t = _apply_result_t<_connect_subs_fn, _sndrs_t, _state_t&>;

			_state_t _state_;
			_sub_opstates_t _sub_ops_;

			/// Initialize the data member, connect all the sub-operations and
			/// save the resulting operation states in _sub_ops_.
			USTDEX_API _opstate_t(_sndrs_t&& _sndrs_, Rcvr _rcvr)
				: _state_{ static_cast<Rcvr&&>(_rcvr), sizeof...(Sndrs) }
				, _sub_ops_{ _sndrs_.apply(_connect_subs_fn(), static_cast<_sndrs_t&&>(_sndrs_), _state_) }
			{}

			USTDEX_IMMOVABLE(_opstate_t);

			/// Start all the sub-operations.
			USTDEX_API void start() & noexcept
			{
				// register stop callback:
				_state_._on_stop_.construct(
					get_stop_token(ustdex::get_env(_state_._rcvr_)), _on_stop_request{ _state_._stop_source_ });

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
		USTDEX_API auto operator()(Sndrs... _sndrs) const->_sndr_t<Sndrs...>;
	};

	template <class Parent, class Child, class... Env>
	USTDEX_API constexpr auto when_any_t::_child_completions()
	{
		using _env_t = prop<get_stop_token_t, inplace_stop_token>;
		USTDEX_LET_COMPLETIONS(auto(_completions) = ustdex::get_child_completion_signatures<Parent, Child, env<_env_t, FWD_ENV_T<Env>...>>())
		{
			if constexpr (_completions.count(set_value) > 1)
			{
				return invalid_completion_signature<WHERE(IN_ALGORITHM, when_any_t),
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

	USTDEX_PRAGMA_POP()

		// The sender for when_all
		template <class... Sndrs>
	struct USTDEX_TYPE_VISIBILITY_DEFAULT when_any_t::_sndr_t
	{
		using sender_concept = sender_t;
		using _sndrs_t = _tuple<Sndrs...>;

		USTDEX_NO_UNIQUE_ADDRESS when_any_t _tag_;
		USTDEX_NO_UNIQUE_ADDRESS _ignore _ignore1_;
		_sndrs_t _sndrs_;

		template <class Self, class... Env>
		USTDEX_API static constexpr auto get_completion_signatures()
		{

			auto cs = concat_completion_signatures(
				_child_completions<Self, Sndrs, Env...>()..., completion_signatures<set_stopped_t()>{});

			using Completions = decltype(cs);

			constexpr bool _all_nothrow_decay_copyable = _value_types<Completions, _nothrow_decay_copyable_t, std::conjunction>::value;
			return cs + _eptr_completion_if<!_all_nothrow_decay_copyable>();
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
	USTDEX_API auto when_any_t::operator()(Sndrs... _sndrs) const -> _sndr_t<Sndrs...>
	{
		// If the incoming senders are non-dependent, we can check the completion
		// signatures of the composed sender immediately.
		if constexpr (((!dependent_sender<Sndrs>) && ...))
		{
			using _completions = completion_signatures_of_t<_sndr_t<Sndrs...>>;
			static_assert(_valid_completion_signatures<_completions>);
		}
		return _sndr_t<Sndrs...>{{}, {}, { static_cast<Sndrs&&>(_sndrs)... }};
	}

	inline constexpr when_any_t when_any{};

} // namespace ustdex

#include "epilogue.hpp"

#endif
