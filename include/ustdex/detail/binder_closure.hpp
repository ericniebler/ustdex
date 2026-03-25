#ifndef USTDEX_ASYNC_DETAIL_BINDER_CLOSURE
#define USTDEX_ASYNC_DETAIL_BINDER_CLOSURE

#include "config.hpp"
#include "tuple.hpp"

#include "prologue.hpp"

namespace ustdex
{

	template<class bind_t, typename... Args>
	struct binder_closure
	{
		_tuple<Args...> _args_;

		binder_closure(Args... args) noexcept : _args_({ static_cast<Args&&>(args)... })
		{}

		template<class Sndr, std::size_t... I>
		static auto make(Sndr&& sndr, _tuple<Args...> _args, std::index_sequence<I...>) noexcept ->_call_result_t<bind_t, Sndr, Args...>
		{
			return bind_t()(static_cast<Sndr&&>(sndr), static_cast<Args&&>(const_cast<Args&>(_cget<I, Args>(_args)))...);
		}

		template <class Sndr>
		USTDEX_TRIVIAL_API auto operator()(Sndr _sndr) noexcept ->_call_result_t<bind_t, Sndr, Args...>
		{
			return this->make<Sndr>(static_cast<Sndr&&>(_sndr), static_cast<_tuple<Args...>&&>(_args_), std::make_index_sequence<sizeof...(Args)>());
		}

		template <class Sndr>
		USTDEX_TRIVIAL_API friend auto operator|(Sndr sndr, binder_closure self) noexcept ->_call_result_t<bind_t, Sndr, Args...>
		{
			return self.make(static_cast<Sndr&&>(sndr), static_cast<_tuple<Args...>&&>(self._args_), std::make_index_sequence<sizeof...(Args)>());
		}
	};

}

#include "epilogue.hpp"

#endif // USTDEX_ASYNC_DETAIL_BINDER_CLOSURE