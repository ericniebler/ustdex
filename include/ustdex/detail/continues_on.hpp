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

#ifndef USTDEX_ASYNC_DETAIL_CONTINUE_ON
#define USTDEX_ASYNC_DETAIL_CONTINUE_ON

#include "completion_signatures.hpp"
#include "cpos.hpp"
#include "exception.hpp"
#include "meta.hpp"
#include "queries.hpp"
#include "rcvr_ref.hpp"
#include "tuple.hpp"
#include "variant.hpp"

#include "prologue.hpp"

namespace ustdex
{
struct USTDEX_TYPE_VISIBILITY_DEFAULT continue_on_t
{
private:
  template <class... As>
  using _set_value_tuple_t = _tuple<set_value_t, USTDEX_DECAY(As)...>;

  template <class Error>
  using _set_error_tuple_t   = _tuple<set_error_t, USTDEX_DECAY(Error)>;

  using _set_stopped_tuple_t = _tuple<set_stopped_t>;

  using _complete_fn         = void (*)(void*) noexcept;

  template <class Rcvr, class Result>
  struct USTDEX_TYPE_VISIBILITY_DEFAULT _rcvr_t
  {
    using receiver_concept = receiver_t;
    Rcvr _rcvr_;
    Result _result_;
    _complete_fn _complete_;

    template <class Tag, class... As>
    USTDEX_API void operator()(Tag, As&... _as) noexcept
    {
      Tag()(static_cast<Rcvr&&>(_rcvr_), static_cast<As&&>(_as)...);
    }

    template <class Tag, class... As>
    USTDEX_API void _set_result(Tag, As&&... _as) noexcept
    {
      using _tupl_t = _tuple<Tag, USTDEX_DECAY(As)...>;
      if constexpr (_nothrow_decay_copyable<As...>)
      {
        _result_.template _emplace<_tupl_t>(Tag(), static_cast<As&&>(_as)...);
      }
      else
      {
        USTDEX_TRY( //
          ({        //
            _result_.template _emplace<_tupl_t>(Tag(), static_cast<As&&>(_as)...);
          }),
          USTDEX_CATCH(...) //
          ({                //
            ustdex::set_error(static_cast<Rcvr&&>(_rcvr_), ::std::current_exception());
          })                //
        )
      }
      _complete_ = +[](void* _ptr) noexcept {
        auto& _self = *static_cast<_rcvr_t*>(_ptr);
        auto& _tupl = *static_cast<_tupl_t*>(_self._result_._ptr());
        _tupl.apply(_self, _tupl);
      };
    }

    USTDEX_API void set_value() noexcept
    {
      _complete_(this);
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

    USTDEX_API env_of_t<Rcvr> get_env() const noexcept
    {
      return ustdex::get_env(_rcvr_);
    }
  };

  template <class Rcvr, class CvSndr, class Sch>
  struct USTDEX_TYPE_VISIBILITY_DEFAULT _opstate_t
  {
    using operation_state_concept = operation_state_t;
    using _env_t                  = FWD_ENV_T<env_of_t<Rcvr>>;

    using _result_t =
      typename completion_signatures_of_t<CvSndr, _env_t>::template _transform_q<_decayed_tuple, _variant>;

    USTDEX_API _opstate_t(CvSndr&& _sndr, Sch _sch, Rcvr _rcvr)
        : _rcvr_{static_cast<Rcvr&&>(_rcvr), {}, nullptr}
        , _opstate1_{ustdex::connect(static_cast<CvSndr&&>(_sndr), _rcvr_ref{*this})}
        , _opstate2_{ustdex::connect(schedule(_sch), _rcvr_ref{_rcvr_})}
    {}

    USTDEX_IMMOVABLE(_opstate_t);

    USTDEX_API void start() noexcept
    {
      ustdex::start(_opstate1_);
    }

    template <class... As>
    USTDEX_API void set_value(As&&... _as) noexcept
    {
      _rcvr_._set_result(set_value_t(), static_cast<As&&>(_as)...);
      ustdex::start(_opstate2_);
    }

    template <class Error>
    USTDEX_API void set_error(Error&& _error) noexcept
    {
      _rcvr_._set_result(set_error_t(), static_cast<Error&&>(_error));
      ustdex::start(_opstate2_);
    }

    USTDEX_API void set_stopped() noexcept
    {
      _rcvr_._set_result(set_stopped_t());
      ustdex::start(_opstate2_);
    }

    USTDEX_API auto get_env() const noexcept -> _env_t
    {
      return ustdex::get_env(_rcvr_._rcvr);
    }

    _rcvr_t<Rcvr, _result_t> _rcvr_;
    connect_result_t<CvSndr, _rcvr_ref<_opstate_t, _env_t>> _opstate1_;
    connect_result_t<schedule_result_t<Sch>, _rcvr_ref<_rcvr_t<Rcvr, _result_t>>> _opstate2_;
  };

  template <class Sndr, class Sch>
  struct USTDEX_TYPE_VISIBILITY_DEFAULT _sndr_t;

  template <class Sch>
  struct _closure_t;

public:
  template <class Sndr, class Sch>
  USTDEX_API _sndr_t<Sndr, Sch> operator()(Sndr _sndr, Sch _sch) const noexcept;

  template <class Sch>
  USTDEX_TRIVIAL_API _closure_t<Sch> operator()(Sch _sch) const noexcept;
};

template <class Sch>
struct USTDEX_TYPE_VISIBILITY_DEFAULT continue_on_t::_closure_t
{
  Sch _sch;

  template <class Sndr>
  USTDEX_TRIVIAL_API friend auto operator|(Sndr _sndr, _closure_t&& _self)
  {
    return continue_on_t()(static_cast<Sndr&&>(_sndr), static_cast<Sch&&>(_self._sch));
  }
};

template <class Tag>
struct _decay_args
{
  template <class... Ts>
  USTDEX_TRIVIAL_API constexpr auto operator()() const noexcept
  {
    if constexpr (!_decay_copyable<Ts...>)
    {
      return invalid_completion_signature<WHERE(IN_ALGORITHM, continue_on_t),
                                          WHAT(ARGUMENTS_ARE_NOT_DECAY_COPYABLE),
                                          WITH_ARGUMENTS(Ts...)>();
    }
    else if constexpr (!_nothrow_decay_copyable<Ts...>)
    {
      return completion_signatures<Tag(USTDEX_DECAY(Ts)...), set_error_t(::std::exception_ptr)>{};
    }
    else
    {
      return completion_signatures<Tag(USTDEX_DECAY(Ts)...)>{};
    }
  }
};

template <class Sndr, class Sch>
struct USTDEX_TYPE_VISIBILITY_DEFAULT continue_on_t::_sndr_t
{
  using sender_concept = sender_t;
  USTDEX_NO_UNIQUE_ADDRESS continue_on_t _tag;
  Sch _sch;
  Sndr _sndr;

  struct USTDEX_TYPE_VISIBILITY_DEFAULT _attrs_t
  {
    _sndr_t* _sndr;

    template <class SetTag>
    USTDEX_API auto query(get_completion_scheduler_t<SetTag>) const noexcept
    {
      return _sndr->_sch;
    }

    template <class Query>
    USTDEX_API auto query(Query) const //
      -> _query_result_t<Query, env_of_t<Sndr>>
    {
      return ustdex::get_env(_sndr->_sndr).query(Query{});
    }
  };

  template <class Self, class... Env>
  USTDEX_API static constexpr auto get_completion_signatures()
  {
    USTDEX_LET_COMPLETIONS(auto(_child_completions) = get_child_completion_signatures<Self, Sndr, Env...>())
    {
      USTDEX_LET_COMPLETIONS(
        auto(_sch_completions) = ustdex::get_completion_signatures<schedule_result_t<Sch>, Env...>())
      {
        // The scheduler contributes error and stopped completions.
        return concat_completion_signatures(
          transform_completion_signatures(_sch_completions, _swallow_transform()),
          transform_completion_signatures(_child_completions, _decay_args<set_value_t>{}, _decay_args<set_error_t>{}));
      }
    }
  }

  template <class Rcvr>
  USTDEX_API _opstate_t<Rcvr, Sndr, Sch> connect(Rcvr _rcvr) &&
  {
    return {static_cast<Sndr&&>(_sndr), _sch, static_cast<Rcvr&&>(_rcvr)};
  }

  template <class Rcvr>
  USTDEX_API _opstate_t<Rcvr, const Sndr&, Sch> connect(Rcvr _rcvr) const&
  {
    return {_sndr, _sch, static_cast<Rcvr&&>(_rcvr)};
  }

  USTDEX_API _attrs_t get_env() const noexcept
  {
    return _attrs_t{this};
  }
};

template <class Sndr, class Sch>
USTDEX_API auto continue_on_t::operator()(Sndr _sndr, Sch _sch) const noexcept -> continue_on_t::_sndr_t<Sndr, Sch>
{
  return _sndr_t<Sndr, Sch>{{}, _sch, static_cast<Sndr&&>(_sndr)};
}

template <class Sch>
USTDEX_TRIVIAL_API continue_on_t::_closure_t<Sch> continue_on_t::operator()(Sch _sch) const noexcept
{
  return _closure_t<Sch>{_sch};
}

inline constexpr continue_on_t continues_on{};
} // namespace ustdex

#include "epilogue.hpp"

#endif
