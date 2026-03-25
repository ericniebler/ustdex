#pragma once

#include "config.hpp"
#include "meta.hpp"
#include "completion_signatures.hpp"
#include "binder_closure.hpp"
#include "lazy.hpp"
#include "cpos.hpp"

#include "prologue.hpp"

namespace ustdex
{

    namespace _stop_with
    {
        template<bool Nothrow>
        struct _completion_fn_
        {
            //non-throwing case
            template<typename... Ts>
			using call = completion_signatures<set_value_t(Ts...)>;
        };

        template<>
        struct _completion_fn_<false>
        {
            //potentially throwing case
            template<typename... Ts>
            using call = completion_signatures<set_value_t(Ts...), set_error_t(::std::exception_ptr)>;
        };

        template <bool Nothrow, class... Ts>
        using _completion_ = _m_call<_completion_fn_<Nothrow>, Ts...>;

        template <class Fn, class... Ts>
        using _completion = _completion_<_nothrow_callable<Fn, Ts...>, Ts...>;
    }

    struct stop_with_t
    {
    private:
        template <class Rcvr, class CvSndr, class Fn>
        struct USTDEX_TYPE_VISIBILITY_DEFAULT _opstate_t
        {
            using operation_state_concept = operation_state_t;

			using _env_t = env_of_t<Rcvr>;

            USTDEX_API _opstate_t(CvSndr&& _sndr, Rcvr _rcvr, Fn _fn)
                : _rcvr_{ static_cast<Rcvr&&>(_rcvr) }
                , _fn_{ static_cast<Fn&&>(_fn) }
                , _opstate_{ ustdex::connect(static_cast<CvSndr&&>(_sndr), _rcvr_ref{*this}) }
            {}

            USTDEX_IMMOVABLE(_opstate_t);

            USTDEX_API void start() & noexcept
            {
                ustdex::start(_opstate_);
            }

            template <class... Ts>
            USTDEX_API void set_value(Ts&&... _ts) noexcept
            {
                if (static_cast<Fn&&>(_fn_)(static_cast<const Ts&>(_ts)...))
                {
                    ustdex::set_stopped(static_cast<Rcvr&&>(_rcvr_));
                }
                else
                {
                    if constexpr (_nothrow_callable<Fn, Ts...>)
                    {
                        ustdex::set_value(static_cast<Rcvr&&>(_rcvr_), static_cast<Ts&&>(_ts)...); //
                    }
                    else
                    {
                        USTDEX_TRY(                                //
                            ({                                       //
                              ustdex::set_value(static_cast<Rcvr&&>(_rcvr_), static_cast<Ts&&>(_ts)...); //
                                }),                                      //
                            USTDEX_CATCH(...)                        //
                            ({                                       //
                              ustdex::set_error(static_cast<Rcvr&&>(_rcvr_), ::std::current_exception());
                                })                                       //
                        )
                    }
                }
            }

            template <class Error>
            USTDEX_API void set_error(Error&& _error) noexcept
            {
                ustdex::set_error(static_cast<Rcvr&&>(_rcvr_), static_cast<Error&&>(_error));
            }

            USTDEX_API void set_stopped() noexcept
            {
                ustdex::set_stopped(static_cast<Rcvr&&>(_rcvr_));
            }

            USTDEX_API auto get_env() const noexcept -> _env_t
            {
                return  ustdex::get_env(_rcvr_);
            }

            Rcvr _rcvr_;
            Fn _fn_;
            connect_result_t<CvSndr, _rcvr_ref<_opstate_t, _env_t>> _opstate_;
        };

        template <class Fn>
        struct _transform_args_fn
        {
            template <class... Ts>
            USTDEX_API constexpr auto operator()() const
            {
                if constexpr (_callable<Fn, Ts...> && std::is_same_v<_call_result_t<Fn, const Ts&...>, bool>)
                {
                    return _stop_with::_completion<Fn, Ts...>();
                }
                else
                {
                    return invalid_completion_signature<WHERE(IN_ALGORITHM, stop_with_t),
                        WHAT(FUNCTION_IS_NOT_CALLABLE),
                        WITH_FUNCTION(Fn),
                        WITH_ARGUMENTS(Ts...)>();
                }
            }
        };

        template <class Sndr, class Fn>
        struct USTDEX_TYPE_VISIBILITY_DEFAULT _sndr_t
        {
            using sender_concept = sender_t;
            Sndr _sndr_;
            Fn _fn_;

            template <class Self, class... Env>
            USTDEX_API static constexpr auto get_completion_signatures()
            {
                USTDEX_LET_COMPLETIONS(auto(_child_completions) = get_child_completion_signatures<Self, Sndr, Env...>())
                {
                    return transform_completion_signatures(_child_completions, _transform_args_fn<Fn>{}, {}, {}, completion_signatures<set_stopped_t()>{});
                }
            }

            template <class Rcvr>
            USTDEX_API auto connect(Rcvr _rcvr) &&                                         //
                noexcept(_nothrow_constructible<_opstate_t<Rcvr, Sndr, Fn>, Sndr, Rcvr, Fn>) //
                -> _opstate_t<Rcvr, Sndr, Fn>
            {
                return _opstate_t<Rcvr, Sndr, Fn>{
                    static_cast<Sndr&&>(_sndr_), static_cast<Rcvr&&>(_rcvr), static_cast<Fn&&>(_fn_)};
            }

            template <class Rcvr>
            USTDEX_API auto connect(Rcvr _rcvr) const& //
                noexcept(_nothrow_constructible<_opstate_t<Rcvr, const Sndr&, Fn>,
                    const Sndr&,
                    Rcvr,
                    const Fn&>) //
                -> _opstate_t<Rcvr, const Sndr&, Fn>
            {
                return _opstate_t<Rcvr, const Sndr&, Fn>{_sndr_, static_cast<Rcvr&&>(_rcvr), _fn_};
            }

            USTDEX_API env_of_t<Sndr> get_env() const noexcept
            {
                return ustdex::get_env(_sndr_);
            }
        };

    public:
        template <class Sndr, class Fn>
        USTDEX_TRIVIAL_API auto operator()(Sndr _sndr, Fn _fn) const noexcept //
            -> _sndr_t<Sndr, Fn>
        {
            // If the incoming sender is non-dependent, we can check the completion
            // signatures of the composed sender immediately.
            if constexpr (!dependent_sender<Sndr>)
            {
                using _completions = completion_signatures_of_t<_sndr_t<Sndr, Fn>>;
                static_assert(_valid_completion_signatures<_completions>);
            }
            return _sndr_t<Sndr, Fn>{static_cast<Sndr&&>(_sndr), static_cast<Fn&&>(_fn)};
        }

        template <class Fn>
        USTDEX_TRIVIAL_API auto operator()(Fn _fn) const noexcept -> binder_closure<stop_with_t, Fn>
        {
            return binder_closure<stop_with_t, Fn>{static_cast<Fn&&>(_fn)};
        }
    };


    inline constexpr stop_with_t stop_with{};
}

#include "epilogue.hpp"