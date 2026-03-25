#pragma once

#include "config.hpp"
#include "meta.hpp"
#include "completion_signatures.hpp"
#include "binder_closure.hpp"
#include "lazy.hpp"
#include "stop_token.hpp"
#include "cpos.hpp"

#include "prologue.hpp"

namespace ustdex
{

	struct stop_when_t
	{
    private:
        template <class Rcvr, class CvSndr, class Token>
        struct USTDEX_TYPE_VISIBILITY_DEFAULT _opstate_t
        {
            using operation_state_concept = operation_state_t;
            using _env_t = env<env_of_t<Rcvr>, prop<get_stop_token_t, Token>>;

			using _stop_token_of_rcvr = stop_token_of_t<env_of_t<Rcvr>>;
			using _stop_callback_t1 = stop_callback_for_t<_stop_token_of_rcvr, _on_stop_request>;
			using _stop_callback_t2 = stop_callback_for_t<Token, _on_stop_request>;

            USTDEX_API _opstate_t(CvSndr&& _sndr, Rcvr _rcvr, Token _token)
                : _rcvr_{ static_cast<Rcvr&&>(_rcvr) }
                , _token_{ static_cast<Token&&>(_token) }
                , _opstate_{ ustdex::connect(static_cast<CvSndr&&>(_sndr), _rcvr_ref{*this}) }
            {}

            USTDEX_IMMOVABLE(_opstate_t);

            USTDEX_API void start() & noexcept
            {
                _on_stop1_.construct(ustdex::get_stop_token(_rcvr_), _on_stop_request{_stop_source_});
                _on_stop2_.construct(_token_, _on_stop_request{ _stop_source_ });
                ustdex::start(_opstate_);
            }

            template <class... Ts>
            USTDEX_API void set_value(Ts&&... _ts) noexcept
            {
				_on_stop1_.destroy();
				_on_stop2_.destroy();
                if (_stop_source_.stop_requested())
                {
                    ustdex::set_stopped(static_cast<Rcvr&&>(_rcvr_));
                }
                else
                {
                    ustdex::set_value(static_cast<Rcvr&&>(_rcvr_), static_cast<Ts&&>(_ts)...);
                }
            }

            template <class Error>
            USTDEX_API void set_error(Error&& _error) noexcept
            {
                _on_stop1_.destroy();
                _on_stop2_.destroy();
                ustdex::set_error(static_cast<Rcvr&&>(_rcvr_), static_cast<Error&&>(_error));
            }

            USTDEX_API void set_stopped() noexcept
            {
                _on_stop1_.destroy();
                _on_stop2_.destroy();
                ustdex::set_stopped(static_cast<Rcvr&&>(_rcvr_));
            }

            USTDEX_API auto get_env() const noexcept -> _env_t
            {
                return  _env_t{ ustdex::get_env(_rcvr_), prop{get_stop_token, _token_} };
            }

            Rcvr _rcvr_;
            Token _token_;
			inplace_stop_source _stop_source_;
            _lazy<_stop_callback_t1> _on_stop1_;
            _lazy<_stop_callback_t2> _on_stop2_;
            connect_result_t<CvSndr, _rcvr_ref<_opstate_t, _env_t>> _opstate_;
        };

        template <class Sndr, class Token>
        struct USTDEX_TYPE_VISIBILITY_DEFAULT _sndr_t
        {
            using sender_concept = sender_t;
            Sndr _sndr_;
            Token _token_;

            template <class Self, class... Env>
            USTDEX_API static constexpr auto get_completion_signatures()
            {
                USTDEX_LET_COMPLETIONS(auto(_child_completions) = get_child_completion_signatures<Self, Sndr, Env...>())
                {
                    return concat_completion_signatures(_child_completions, completion_signatures<set_stopped_t()>{});
                }
            }

            template <class Rcvr>
            USTDEX_API auto connect(Rcvr _rcvr) &&                                         //
                noexcept(_nothrow_constructible<_opstate_t<Rcvr, Sndr, Token>, Sndr, Rcvr, Token>) //
                -> _opstate_t<Rcvr, Sndr, Token>
            {
                return _opstate_t<Rcvr, Sndr, Token>{
                    static_cast<Sndr&&>(_sndr_), static_cast<Rcvr&&>(_rcvr), static_cast<Token&&>(_token_)};
            }

            template <class Rcvr>
            USTDEX_API auto connect(Rcvr _rcvr) const& //
                noexcept(_nothrow_constructible<_opstate_t<Rcvr, const Sndr&, Token>,
                    const Sndr&,
                    Rcvr,
                    const Token&>) //
                -> _opstate_t<Rcvr, const Sndr&, Token>
            {
                return _opstate_t<Rcvr, const Sndr&, Token>{_sndr_, static_cast<Rcvr&&>(_rcvr), _token_};
            }

            USTDEX_API env_of_t<Sndr> get_env() const noexcept
            {
                return ustdex::get_env(_sndr_);
            }
        };

    public:
        template <class Sndr, class Token>
        USTDEX_TRIVIAL_API auto operator()(Sndr _sndr, Token _token) const noexcept //
            -> _sndr_t<Sndr, Token>
        {
            // If the incoming sender is non-dependent, we can check the completion
            // signatures of the composed sender immediately.
            if constexpr (!dependent_sender<Sndr>)
            {
                using _completions = completion_signatures_of_t<_sndr_t<Sndr, Token>>;
                static_assert(_valid_completion_signatures<_completions>);
            }
            return _sndr_t<Sndr, Token>{static_cast<Sndr&&>(_sndr), static_cast<Token&&>(_token)};
        }

        template <class Token>
        USTDEX_TRIVIAL_API auto operator()(Token _token) const noexcept -> binder_closure<stop_when_t, Token>
        {
            return binder_closure<stop_when_t, Token>{static_cast<Token&&>(_token)};
        }
	};


	inline constexpr stop_when_t stop_when{};
}

#include "epilogue.hpp"