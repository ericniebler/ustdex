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

#ifndef USTDEX_ASYNC_DETAIL_JUST_FROM
#define USTDEX_ASYNC_DETAIL_JUST_FROM

#include "completion_signatures.hpp"
#include "config.hpp"
#include "cpos.hpp"
#include "rcvr_ref.hpp"
#include "tuple.hpp"
#include "utility.hpp"

#include "prologue.hpp"

namespace ustdex
{
// Forward declarations of the just* tag types:
struct just_from_t;
struct just_error_from_t;
struct just_stopped_from_t;

// Map from a disposition to the corresponding tag types:
namespace detail
{
template <_disposition_t, class Void = void>
extern _m_undefined<Void> _just_from_tag;
template <class Void>
extern _fn_t<just_from_t>* _just_from_tag<_value, Void>;
template <class Void>
extern _fn_t<just_error_from_t>* _just_from_tag<_error, Void>;
template <class Void>
extern _fn_t<just_stopped_from_t>* _just_from_tag<_stopped, Void>;
} // namespace detail

struct AN_ERROR_COMPLETION_MUST_HAVE_EXACTLY_ONE_ERROR_ARGUMENT;
struct A_STOPPED_COMPLETION_MUST_HAVE_NO_ARGUMENTS;

template <_disposition_t Disposition>
struct _just_from
{
private:
  using JustTag = decltype(detail::_just_from_tag<Disposition>());
  using SetTag  = decltype(detail::_set_tag<Disposition>());

  using _diag_t = _m_if<SetTag() == set_error,
                        AN_ERROR_COMPLETION_MUST_HAVE_EXACTLY_ONE_ERROR_ARGUMENT,
                        A_STOPPED_COMPLETION_MUST_HAVE_NO_ARGUMENTS>;

  template <class... Ts>
  using _error_t = ERROR<WHERE(IN_ALGORITHM, JustTag), WHAT(_diag_t), WITH_COMPLETION_SIGNATURE<SetTag(Ts...)>>;

  struct _probe_fn
  {
    template <class... Ts>
    auto operator()(Ts&&... _ts) const noexcept
      -> _m_if<_signature_disposition<SetTag(Ts...)> != _invalid_disposition,
               completion_signatures<SetTag(Ts...)>,
               _error_t<Ts...>>;
  };

  template <class Rcvr>
  struct _complete_fn
  {
    Rcvr& _rcvr_;

    template <class... Ts>
    USTDEX_API auto operator()(Ts&&... _ts) const noexcept
    {
      SetTag()(static_cast<Rcvr&&>(_rcvr_), static_cast<Ts&&>(_ts)...);
    }
  };

  template <class Rcvr, class Fn>
  struct _opstate
  {
    using operation_state_concept = operation_state_t;

    Rcvr _rcvr_;
    Fn _fn_;

    USTDEX_API void start() & noexcept
    {
      static_cast<Fn&&>(_fn_)(_complete_fn<Rcvr>{_rcvr_});
    }
  };

  template <class Fn>
  struct USTDEX_TYPE_VISIBILITY_DEFAULT _sndr_t
  {
    using sender_concept = sender_t;

    USTDEX_NO_UNIQUE_ADDRESS JustTag _tag_;
    Fn _fn_;

    template <class Self, class...>
    USTDEX_API static constexpr auto get_completion_signatures() noexcept
    {
      return _call_result_t<Fn, _probe_fn>{};
    }

    template <class Rcvr>
    USTDEX_API _opstate<Rcvr, Fn> connect(Rcvr _rcvr) && //
      noexcept(_nothrow_decay_copyable<Rcvr, Fn>)
    {
      return _opstate<Rcvr, Fn>{static_cast<Rcvr&&>(_rcvr), static_cast<Fn&&>(_fn_)};
    }

    template <class Rcvr>
    USTDEX_API _opstate<Rcvr, Fn> connect(Rcvr _rcvr) const& //
      noexcept(_nothrow_decay_copyable<Rcvr, Fn const&>)
    {
      return _opstate<Rcvr, Fn>{static_cast<Rcvr&&>(_rcvr), _fn_};
    }
  };

public:
  template <class Fn>
  USTDEX_TRIVIAL_API auto operator()(Fn _fn) const noexcept
  {
    using _completions = _call_result_t<Fn, _probe_fn>;
    static_assert(_valid_completion_signatures<_completions>,
                  "The function passed to just_from must return an instance of a specialization of "
                  "completion_signatures<>.");
    return _sndr_t<Fn>{{}, static_cast<Fn&&>(_fn)};
  }
};

inline constexpr struct just_from_t : _just_from<_value>
{
} just_from{};

inline constexpr struct just_error_from_t : _just_from<_error>
{
} just_error_from{};

inline constexpr struct just_stopped_from_t : _just_from<_stopped>
{
} just_stopped_from{};
} // namespace ustdex

#include "epilogue.hpp"

#endif
