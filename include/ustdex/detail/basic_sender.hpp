#pragma once

#include "completion_signatures.hpp"
#include "config.hpp"
#include "cpos.hpp"
#include "tuple.hpp"
#include "utility.hpp"

#include "prologue.hpp"

namespace ustdex
{
	template <class _Tag>
	struct __sexpr_impl;

	template <class _Sexpr, class _Receiver>
	struct __op_state;

	template <class _Receiver, class _Sexpr, std::size_t _Idx>
	struct __rcvr;

	namespace detail
	{
		struct __ {};

		template <class _Tp>
		using __t = typename _Tp::__t;

		template <class...>
		inline constexpr bool __mnever = false;

		//! Metafunction selects the first of two type arguments.
		template <class _Tp, class _Up>
		using __mfirst = _Tp;

		//! Metafunction selects the second of two type arguments.
		template <class _Tp, class _Up>
		using __msecond = _Up;

		// A type that describes a sender's metadata
		template <class _Tag, class _Data, class... _Child>
		struct __desc
		{
			using __tag = _Tag;
			using __data = _Data;
			using __children = _m_list<_Child...>;

			template <class _Fn>
			using call = _m_call<_Fn, _Tag, _Data, _Child...>;
		};

		template <class _Sender>
		using __impl_of = decltype((std::declval<_Sender>().__impl_));

		// Accessor for the "data" field of a sender
		struct __get_data
		{
			template <class _Data, typename... _Child>
			//USTDEX_ATTR_ALWAYS_INLINE
				auto operator()(_ignore, _Data&& __data, _Child&&...) const noexcept -> _Data&&
			{
				return static_cast<_Data&&>(__data);
			}
		};

		// A function object that is to senders what std::apply is to tuples:
		struct __sexpr_apply_t
		{
			template <class _Sender, class _ApplyFn>
			USTDEX_ATTR_ALWAYS_INLINE
				auto operator()(_Sender&& __sndr, _ApplyFn&& __fun) const
				noexcept(noexcept(__sndr
					.apply(static_cast<_Sender&&>(__sndr), static_cast<_ApplyFn&&>(__fun))))
				-> decltype(__sndr
					.apply(static_cast<_Sender&&>(__sndr), static_cast<_ApplyFn&&>(__fun)))
			{
				return __sndr.apply(static_cast<_Sender&&>(__sndr), static_cast<_ApplyFn&&>(__fun));
			}
		};

		template <class _Fn>
		struct __sexpr_uncurry_fn
		{
			template <class _Tag, class _Data, class... _Child>
			constexpr auto operator()(_Tag, _Data&&, _Child&&...) const noexcept
				-> _m_call<_Fn, _Tag, _Data, _Child...>;
		};

		template <class _CvrefSender, class _Fn>
		using __sexpr_uncurry = _call_result_t<__sexpr_apply_t, _CvrefSender, __sexpr_uncurry_fn<_Fn>>;

		template <class _Sender>
		using __desc_of = __sexpr_uncurry<_Sender, _m_quote<__desc>>;

		template <class _Sexpr, class _Receiver>
		struct __connect_fn
		{
			template <std::size_t _Idx>
			using __receiver_t = __t<__rcvr<_Receiver, _Sexpr, _Idx>>;

			__op_state<_Sexpr, _Receiver>* __op_;

			struct __impl
			{
				__op_state<_Sexpr, _Receiver>* __op_;

				template <std::size_t... _Is, class... _Child>
				auto operator()(std::index_sequence<_Is...>, _Child&&... __child) const
					noexcept((_nothrow_callable<connect_t, _Child, __receiver_t<_Is>> && ...))
					-> _tupl_for<connect_result_t<_Child, __receiver_t<_Is>>...>
				{
					return _tupl{ ustdex::connect(static_cast<_Child&&>(__child), __receiver_t<_Is>{__op_})... };
				}
			};

			template <class... _Child>
			auto operator()(_ignore, _ignore, _Child&&... __child) const
				noexcept(_nothrow_callable<__impl, std::index_sequence_for<_Child...>, _Child...>)
				-> _call_result_t<__impl, std::index_sequence_for<_Child...>, _Child...>
			{
				return __impl{ __op_ }(std::index_sequence_for<_Child...>(), static_cast<_Child&&>(__child)...);
			}

			auto operator()(_ignore, _ignore) const noexcept -> _tuple<>
			{
				return {};
			}
		};

		template <class _Tag, class _Sexpr, class _Receiver>
		using __state_type_t =
			std::decay_t<decltype(__sexpr_impl<_Tag>::get_state(std::declval<_Sexpr>(), std::declval<_Receiver&>()))>;

		template <class _Self, class _Tag, class _Index, class _Sexpr, class _Receiver>
		using __env_type_t = decltype(
			__sexpr_impl<_Tag>::get_env(std::declval<_Index>(), std::declval<__state_type_t<_Tag, _Sexpr, _Receiver>&>(), std::declval<_Receiver&>())
			);

		template <class _Fn>
		struct __drop_front_impl
		{
			template <class... _Rest>
			auto operator()(_ignore, _Rest&&... __rest) const noexcept(
				_nothrow_callable<const _Fn&, _Rest...>) -> _call_result_t<const _Fn&, _Rest...>
			{
				return __fn(static_cast<_Rest&&>(__rest)...);
			};

			_Fn __fn;
		};

		template <class _Fn>
		inline constexpr __drop_front_impl<_Fn> __drop_front(_Fn __fn) noexcept
		{
			return __drop_front_impl<_Fn>{std::move(__fn)};
		};

		template <class _Tag, class... _Captures>
		struct __captures_impl
		{
			template<class _Cvref, class _Fun>
			auto operator()(_Cvref a, _Fun&& __fun) noexcept(_nothrow_callable<_Fun, _Tag, _m_call<_Cvref, _Captures>...>)
				-> _call_result_t<_Fun, _Tag, _m_call<_Cvref, _Captures>...>
			{
				return __captures3_tup.apply([__fun = std::move(__fun)](_Captures&... __captures3) mutable
					-> _call_result_t<_Fun, _Tag, _m_call<_Cvref, _Captures>...>
					{
						return static_cast<_Fun&&>(__fun)(_Tag(), const_cast<_m_call<_Cvref, _Captures>&&>(__captures3)...);
					}, __captures3_tup);
			}
			_tuple<_Captures...> __captures3_tup;
		};

		template <class _Tag, class... _Captures>
		constexpr auto __captures(_Tag, _Captures&&... __captures2)
		{
			return __captures_impl<_Tag, _Captures...>{_tuple<_Captures...>{std::move(__captures2)...}};
		}

		template <class _Tag, class _Data, class... _Child>
		using __captures_t = decltype(__captures(_Tag(), std::declval<_Data>(), std::declval<_Child>()...));

		template <class _Sexpr, class _Receiver>
		struct __op_base : _immovable
		{
			using __tag_t = typename std::decay_t<_Sexpr>::__tag_t;
			using __state_t = __state_type_t<__tag_t, _Sexpr, _Receiver>;

			USTDEX_NO_UNIQUE_ADDRESS
				_Receiver __rcvr_;
			USTDEX_NO_UNIQUE_ADDRESS
				__state_t __state_;

			__op_base(_Sexpr&& __sndr, _Receiver&& __rcvr) noexcept(
				_nothrow_decay_copyable<_Receiver>
				&& _nothrow_callable<decltype(__sexpr_impl<__tag_t>::get_state), _Sexpr, _Receiver&>)
				: __rcvr_(static_cast<_Receiver&&>(__rcvr))
				, __state_(__sexpr_impl<__tag_t>::get_state(static_cast<_Sexpr&&>(__sndr), __rcvr_))
			{
			}

			USTDEX_ATTR_ALWAYS_INLINE auto __state() & noexcept -> __state_t&
			{
				return __state_;
			}

			USTDEX_ATTR_ALWAYS_INLINE auto __state() const& noexcept -> const __state_t&
			{
				return __state_;
			}

			USTDEX_ATTR_ALWAYS_INLINE auto __rcvr() & noexcept -> _Receiver&
			{
				return __rcvr_;
			}

			USTDEX_ATTR_ALWAYS_INLINE auto __rcvr() const& noexcept -> const _Receiver&
			{
				return __rcvr_;
			}
		};

		struct __defaults
		{
			static constexpr struct
			{
				template <class... _Child>
				auto operator()(_ignore, const _Child&... __child) const noexcept -> decltype(auto)
				{
					if constexpr (sizeof...(__child) == 1)
					{
						return ustdex::get_env(__child...);
					}
					else
					{
						return env<>();
					}
				}
			} get_attrs;

			static constexpr struct
			{
				template <class _Receiver>
				auto operator()(_ignore, _ignore, const _Receiver& __rcvr) const noexcept
					-> env_of_t<const _Receiver&>
				{
					return ustdex::get_env(__rcvr);
				};

			} get_env;

			static constexpr struct
			{
				template <class _Sender>
				auto operator()(_Sender&& __sndr, _ignore) const noexcept -> decltype(auto)
				{
					return __sndr.apply(static_cast<_Sender&&>(__sndr), __get_data());
				};

			} get_state;

			static constexpr struct
			{
				//requires __connectable<_Sender, _Receiver>
				template <class _Sender, class _Receiver>
				auto operator()(_Sender&& __sndr, _Receiver __rcvr) const noexcept(
					_nothrow_constructible<__op_state<_Sender, _Receiver>, _Sender, _Receiver>)
					-> __op_state<_Sender, _Receiver>
				{
					return __op_state<_Sender, _Receiver>{
						static_cast<_Sender&&>(__sndr), static_cast<_Receiver&&>(__rcvr)};
				};
			} connect;

			static constexpr struct
			{
				template <class _StartTag = start_t, class... _ChildOps>
				auto operator()(
					_ignore,
					_ignore,
					_ChildOps&... __ops) const noexcept
				{
					(_StartTag()(__ops), ...);
				};
			} start;

			static constexpr struct
			{
				template <class _Index, class _Receiver, class _SetTag, class... _Args>
				auto operator()(
					_Index,
					_ignore,
					_Receiver& __rcvr,
					_SetTag,
					_Args&&... __args) const noexcept
				{
					static_assert(_Index::value == 0, "I don't know how to complete this operation.");
					_SetTag()(std::move(__rcvr), static_cast<_Args&&>(__args)...);
				};
			} complete;

			static constexpr struct
			{
				template <class _Sender, typename... _Args>
				void operator()(_Sender&&, _Args&&...) const noexcept
				{
					static_assert(
						__mnever < tag_of_t<_Sender>>
						"No customization of get_completion_signatures for this sender tag type.");
				};
			} get_completion_signatures;
		};
	}

	using detail::__sexpr_apply_t;
	inline constexpr __sexpr_apply_t __sexpr_apply{};

	template <class _Sender>
	using tag_of_t = typename detail::__desc_of<_Sender>::__tag;

	template <class _Sender>
	using data_of = typename detail::__desc_of<_Sender>::__data;

	template <class _Sender, class _Continuation = _m_quote<_m_list>>
	using children_of = _m_apply<_Continuation, typename detail::__desc_of<_Sender>::__children>;

	template <class _Tag, class _Data, class... _Child>
	using __descriptor_fn_t = _fn_t<detail::__desc<_Tag, _Data, _Child...>>*;

	using __sexpr_defaults = detail::__defaults;

	template <class _Sexpr, class _Receiver>
	struct __op_state : detail::__op_base<_Sexpr, _Receiver>
	{
		using __desc_t = typename std::decay_t<_Sexpr>::__desc_t;
		using __tag_t = typename __desc_t::__tag;
		using __data_t = typename __desc_t::__data;
		using __state_t = typename __op_state::__state_t;
		using __inner_ops_t =
			_call_result_t<detail::__sexpr_apply_t, _Sexpr, detail::__connect_fn<_Sexpr, _Receiver>>;

		__inner_ops_t __inner_ops_;

		__op_state(_Sexpr&& __sexpr, _Receiver __rcvr) noexcept(
			_nothrow_constructible<detail::__op_base<_Sexpr, _Receiver>, _Sexpr, _Receiver>
			&& _nothrow_callable<detail::__sexpr_apply_t, _Sexpr, detail::__connect_fn<_Sexpr, _Receiver>>)
			: __op_state::__op_base{ static_cast<_Sexpr&&>(__sexpr), static_cast<_Receiver&&>(__rcvr) }
			, __inner_ops_(__sexpr_apply(
				static_cast<_Sexpr&&>(__sexpr),
				detail::__connect_fn<_Sexpr, _Receiver>{this}))
		{}

		USTDEX_ATTR_ALWAYS_INLINE void start() & noexcept
		{
			auto&& __rcvr = this->__rcvr();
			__inner_ops_.apply(
				[&](auto&... __ops) noexcept
				{
					__sexpr_impl<__tag_t>::start(this->__state(), __rcvr, __ops...);
				},
				__inner_ops_);
		}

		template <class _Index, class _Tag2, class... _Args>
		USTDEX_ATTR_ALWAYS_INLINE
			void __complete(_Index, _Tag2, _Args&&... __args) noexcept
		{
			using __tag_t = typename __op_state::__tag_t;
			auto&& __rcvr = this->__rcvr();
			__sexpr_impl<__tag_t>::complete(
				_Index(), this->__state(), __rcvr, _Tag2(), static_cast<_Args&&>(__args)...);
		}

		template <class _Index>
		USTDEX_ATTR_ALWAYS_INLINE
			auto __get_env(_Index) const noexcept
			-> detail::__env_type_t<_Index, __tag_t, _Index, _Sexpr, _Receiver>
		{
			const auto& __rcvr = this->__rcvr();
			return __sexpr_impl<__tag_t>::get_env(_Index(), this->__state(), __rcvr);
		}
	};

	template <class _Receiver, class _Sexpr, std::size_t _Idx>
	struct __rcvr
	{
		struct __t
		{
			using receiver_concept = receiver_t;
			using __id = __rcvr;

			using __index = _m_size_t<_Idx>;
			using __parent_op_t = __op_state<_Sexpr, _Receiver>;
			using __tag_t = tag_of_t<_Sexpr>;

			// A pointer to the parent operation state, which contains the one created with
			// this receiver.
			__parent_op_t* __op_;

			template <class... _Args>
			USTDEX_ATTR_ALWAYS_INLINE
				void set_value(_Args&&... __args) noexcept
			{
				__op_->__complete(__index(), ustdex::set_value, static_cast<_Args&&>(__args)...);
			}

			template <class _Error>
			USTDEX_ATTR_ALWAYS_INLINE
				void set_error(_Error&& __err) noexcept
			{
				__op_->__complete(__index(), ustdex::set_error, static_cast<_Error&&>(__err));
			}

			USTDEX_ATTR_ALWAYS_INLINE void set_stopped() noexcept
			{
				__op_->__complete(__index(), ustdex::set_stopped);
			}

			template <typename _Self = __t, typename = std::enable_if_t<std::is_same_v<_Self, __t>>>
			USTDEX_ATTR_ALWAYS_INLINE
				auto get_env() const noexcept
				-> detail::__env_type_t<_Self, __tag_t, __index, _Sexpr, _Receiver>
			{
				return __op_->__get_env(__index());
			}
		};
	};

	template <class _Tag>
	struct __sexpr_impl : detail::__defaults
	{
		using not_specialized = void;
	};

	template <class _Tag>
	using __get_attrs_fn =
		decltype(detail::__drop_front(__sexpr_impl<_Tag>::get_attrs));

	namespace
	{
		//! A struct template to aid in creating senders.
		//! This struct closely resembles P2300's [_`basic-sender`_](https://eel.is/c++draft/exec#snd.expos-24),
		//! but is not an exact implementation.
		//! Note: The struct named `__basic_sender` is just a dummy type and is also not _`basic-sender`_.
		template <typename _DescriptorFn>
		struct __sexpr
		{
			using sender_concept = sender_t;

			using __desc_t = decltype(std::declval<_DescriptorFn>()());
			using __tag_t = typename __desc_t::__tag;
			using __captures_t = _m_call<__desc_t, _m_quote<detail::__captures_t>>;

			mutable __captures_t __impl_;

			template <class _Tag, class _Data, class... _Child>
			explicit __sexpr(_Tag, _Data&& __data, _Child&&... __child)
				: __impl_(
					detail::__captures(
						_Tag(),
						static_cast<_Data&&>(__data),
						static_cast<_Child&&>(__child)...))
			{}

			using __impl = __sexpr_impl<__tag_t>;
			using _Self_t = __sexpr;

			template <class _Self = _Self_t>
			USTDEX_ATTR_ALWAYS_INLINE
				auto get_env() const noexcept
				-> __result_of<__sexpr_apply, const _Self&, __get_attrs_fn<__tag_t>>
			{
				return __sexpr_apply(*this, detail::__drop_front(__impl::get_attrs));
			}

			template <class _Self, class... _Env>
			USTDEX_ATTR_ALWAYS_INLINE
				static constexpr auto get_completion_signatures() noexcept ->
				__result_of<__impl::get_completion_signatures, _Self, _Env...>
			{
				return {};
			}

			// BUGBUG fix receiver constraint here:
			template <class _Receiver>
			USTDEX_ATTR_ALWAYS_INLINE
				auto connect(_Receiver&& __rcvr)
				noexcept(_nothrow_callable<decltype(__impl::connect), _Self_t, _Receiver>) ->
				__result_of<__impl::connect, _Self_t, _Receiver>
			{
				return __impl::connect(
					static_cast<_Self_t&&>(*this), static_cast<_Receiver&&>(__rcvr));
			}

			template <class _Sender, class _ApplyFn>
			USTDEX_ATTR_ALWAYS_INLINE
				static auto apply(_Sender&& __sndr, _ApplyFn&& __fun) noexcept(
					_nothrow_callable<detail::__impl_of<_Sender>, _copy_cvref_fn<_Sender>, _ApplyFn>)
				-> _call_result_t<detail::__impl_of<_Sender>, _copy_cvref_fn<_Sender>, _ApplyFn>
			{
				return static_cast<_Sender&&>(__sndr)
					.__impl_(_copy_cvref_fn<_Sender>(), static_cast<_ApplyFn&&>(__fun));
			}
		};

		template <class _Tag, class _Data, class... _Child>
		__sexpr(_Tag, _Data, _Child...) -> __sexpr<__descriptor_fn_t<_Tag, _Data, _Child...>>;
	} // namespace

	template <class _Tag, class _Data, class... _Child>
	using _sexpr_t = __sexpr<__descriptor_fn_t<_Tag, _Data, _Child...>>;

	namespace detail
	{
		template <class _Tag>
		struct _make_sexpr_t
		{
			template <class _Data = __, class... _Child>
			constexpr auto operator()(_Data _data = {}, _Child... _child) const
			{
				return _sexpr_t<_Tag, _Data, _Child...>{
					_Tag(), static_cast<_Data&&>(_data), static_cast<_Child&&>(_child)...};
			}
		};
	} // namespace __detail

	template <class _Tag>
	inline constexpr detail::_make_sexpr_t<_Tag> _make_sexpr{};
}

#include "epilogue.hpp"