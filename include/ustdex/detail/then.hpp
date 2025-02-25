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

#ifndef USTDEX_ASYNC_DETAIL_THEN
#define USTDEX_ASYNC_DETAIL_THEN

#include "completion_signatures.hpp"
#include "concepts.hpp"
#include "cpos.hpp"
#include "exception.hpp"
#include "meta.hpp"
#include "rcvr_ref.hpp"

#include "prologue.hpp"

namespace ustdex
{
// Forward-declate the then and upon_* algorithm tag types:
struct then_t;
struct upon_error_t;
struct upon_stopped_t;

// Map from a disposition to the corresponding tag types:
namespace detail
{
template <_disposition_t, class Void = void>
extern _m_undefined<Void> _upon_tag;
template <class Void>
extern _fn_t<then_t>* _upon_tag<_value, Void>;
template <class Void>
extern _fn_t<upon_error_t>* _upon_tag<_error, Void>;
template <class Void>
extern _fn_t<upon_stopped_t>* _upon_tag<_stopped, Void>;
} // namespace detail

namespace _upon
{
template <bool IsVoid, bool Nothrow>
struct _completion_fn
{ // non-void, potentially throwing case
  template <class Result>
  using call = completion_signatures<set_value_t(Result), set_error_t(::std::exception_ptr)>;
};

template <>
struct _completion_fn<true, false>
{ // void, potentially throwing case
  template <class>
  using call = completion_signatures<set_value_t(), set_error_t(::std::exception_ptr)>;
};

template <>
struct _completion_fn<false, true>
{ // non-void, non-throwing case
  template <class Result>
  using call = completion_signatures<set_value_t(Result)>;
};

template <>
struct _completion_fn<true, true>
{ // void, non-throwing case
  template <class>
  using call = completion_signatures<set_value_t()>;
};

template <class Result, bool Nothrow>
using _completion_ = _m_call1<_completion_fn<std::is_same_v<Result, void>, Nothrow>, Result>;

template <class Fn, class... Ts>
using _completion = _completion_<_call_result_t<Fn, Ts...>, _nothrow_callable<Fn, Ts...>>;
} // namespace _upon

template <_disposition_t Disposition>
struct _upon_t
{
private:
  using UponTag = decltype(detail::_upon_tag<Disposition>());
  using SetTag  = decltype(detail::_set_tag<Disposition>());

  template <class Rcvr, class CvSndr, class Fn>
  struct USTDEX_TYPE_VISIBILITY_DEFAULT _opstate_t
  {
    using operation_state_concept = operation_state_t;
    using _env_t                  = env_of_t<Rcvr>;

    USTDEX_API _opstate_t(CvSndr&& _sndr, Rcvr _rcvr, Fn _fn)
        : _rcvr_{static_cast<Rcvr&&>(_rcvr)}
        , _fn_{static_cast<Fn&&>(_fn)}
        , _opstate_{ustdex::connect(static_cast<CvSndr&&>(_sndr), _rcvr_ref{*this})}
    {}

    USTDEX_IMMOVABLE(_opstate_t);

    USTDEX_API void start() & noexcept
    {
      ustdex::start(_opstate_);
    }

    template <bool CanThrow = false, class... Ts>
    USTDEX_API void _set(Ts&&... _ts) noexcept(!CanThrow)
    {
      if constexpr (CanThrow || _nothrow_callable<Fn, Ts...>)
      {
        if constexpr (std::is_same_v<void, _call_result_t<Fn, Ts...>>)
        {
          static_cast<Fn&&>(_fn_)(static_cast<Ts&&>(_ts)...);
          ustdex::set_value(static_cast<Rcvr&&>(_rcvr_));
        }
        else
        {
          ustdex::set_value(static_cast<Rcvr&&>(_rcvr_), static_cast<Fn&&>(_fn_)(static_cast<Ts&&>(_ts)...));
        }
      }
      else
      {
        USTDEX_TRY( //
          ({ //
            _set<true>(static_cast<Ts&&>(_ts)...); //
          }), //
          USTDEX_CATCH(...) //
          ({ //
            ustdex::set_error(static_cast<Rcvr&&>(_rcvr_), ::std::current_exception());
          }) //
        )
      }
    }

    template <class Tag, class... Ts>
    USTDEX_TRIVIAL_API void _complete(Tag, Ts&&... _ts) noexcept
    {
      if constexpr (std::is_same_v<Tag, SetTag>)
      {
        _set(static_cast<Ts&&>(_ts)...);
      }
      else
      {
        Tag()(static_cast<Rcvr&&>(_rcvr_), static_cast<Ts&&>(_ts)...);
      }
    }

    template <class... Ts>
    USTDEX_API void set_value(Ts&&... _ts) noexcept
    {
      _complete(set_value_t(), static_cast<Ts&&>(_ts)...);
    }

    template <class Error>
    USTDEX_API void set_error(Error&& _error) noexcept
    {
      _complete(set_error_t(), static_cast<Error&&>(_error));
    }

    USTDEX_API void set_stopped() noexcept
    {
      _complete(set_stopped_t());
    }

    USTDEX_API auto get_env() const noexcept -> _env_t
    {
      return ustdex::get_env(_rcvr_);
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
      if constexpr (_callable<Fn, Ts...>)
      {
        return _upon::_completion<Fn, Ts...>();
      }
      else
      {
        return invalid_completion_signature<WHERE(IN_ALGORITHM, UponTag),
                                            WHAT(FUNCTION_IS_NOT_CALLABLE),
                                            WITH_FUNCTION(Fn),
                                            WITH_ARGUMENTS(Ts...)>();
      }
    }
  };

  template <class Fn, class Sndr>
  struct USTDEX_TYPE_VISIBILITY_DEFAULT _sndr_t
  {
    using sender_concept = sender_t;
    USTDEX_NO_UNIQUE_ADDRESS UponTag _tag_;
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
    USTDEX_API auto connect(Rcvr _rcvr) && //
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

  template <class Fn>
  struct _closure_t
  {
    using UponTag = decltype(detail::_upon_tag<Disposition>());
    Fn _fn_;

    template <class Sndr>
    USTDEX_TRIVIAL_API auto operator()(Sndr _sndr) -> _call_result_t<UponTag, Sndr, Fn>
    {
      return UponTag()(static_cast<Sndr&&>(_sndr), static_cast<Fn&&>(_fn_));
    }

    template <class Sndr>
    USTDEX_TRIVIAL_API friend auto operator|(Sndr _sndr, _closure_t&& _self) //
      -> _call_result_t<UponTag, Sndr, Fn>
    {
      return UponTag()(static_cast<Sndr&&>(_sndr), static_cast<Fn&&>(_self._fn_));
    }
  };

public:
  template <class Sndr, class Fn>
  USTDEX_TRIVIAL_API auto operator()(Sndr _sndr, Fn _fn) const noexcept //
    -> _sndr_t<Fn, Sndr>
  {
    // If the incoming sender is non-dependent, we can check the completion
    // signatures of the composed sender immediately.
    if constexpr (!dependent_sender<Sndr>)
    {
      using _completions = completion_signatures_of_t<_sndr_t<Fn, Sndr>>;
      static_assert(_valid_completion_signatures<_completions>);
    }
    return _sndr_t<Fn, Sndr>{{}, static_cast<Fn&&>(_fn), static_cast<Sndr&&>(_sndr)};
  }

  template <class Fn>
  USTDEX_TRIVIAL_API auto operator()(Fn _fn) const noexcept
  {
    return _closure_t<Fn>{static_cast<Fn&&>(_fn)};
  }
};

inline constexpr struct then_t : _upon_t<_value>
{
} then{};

inline constexpr struct upon_error_t : _upon_t<_error>
{
} upon_error{};

inline constexpr struct upon_stopped_t : _upon_t<_stopped>
{
} upon_stopped{};
} // namespace ustdex

#include "epilogue.hpp"

#endif
