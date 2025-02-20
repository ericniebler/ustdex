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

#ifndef USTDEX_ASYNC_DETAIL_JUST
#define USTDEX_ASYNC_DETAIL_JUST

#include "completion_signatures.hpp"
#include "config.hpp"
#include "cpos.hpp"
#include "tuple.hpp"
#include "utility.hpp"

#include "prologue.hpp"

namespace ustdex
{
// Forward declarations of the just* tag types:
struct just_t;
struct just_error_t;
struct just_stopped_t;

// Map from a disposition to the corresponding tag types:
namespace detail
{
template <_disposition_t, class Void = void>
extern _m_undefined<Void> _just_tag;
template <class Void>
extern _fn_t<just_t>* _just_tag<_value, Void>;
template <class Void>
extern _fn_t<just_error_t>* _just_tag<_error, Void>;
template <class Void>
extern _fn_t<just_stopped_t>* _just_tag<_stopped, Void>;
} // namespace detail

template <_disposition_t Disposition>
struct _just
{
private:
  using JustTag = decltype(detail::_just_tag<Disposition>());
  using SetTag  = decltype(detail::_set_tag<Disposition>());

  template <class Rcvr, class... Ts>
  struct USTDEX_TYPE_VISIBILITY_DEFAULT _opstate_t
  {
    using operation_state_concept = operation_state_t;

    Rcvr _rcvr_;
    _tuple<Ts...> _values_;

    struct _complete_fn
    {
      _opstate_t* _self_;

      USTDEX_API void operator()(Ts&... _ts) const noexcept
      {
        SetTag()(static_cast<Rcvr&&>(_self_->_rcvr_), static_cast<Ts&&>(_ts)...);
      }
    };

    USTDEX_API void start() & noexcept
    {
      _values_.apply(_complete_fn{this}, _values_);
    }
  };

  template <class... Ts>
  struct USTDEX_TYPE_VISIBILITY_DEFAULT _sndr_t
  {
    using sender_concept = sender_t;

    USTDEX_NO_UNIQUE_ADDRESS JustTag _tag_;
    _tuple<Ts...> _values_;

    template <class Self, class... Env>
    USTDEX_API static constexpr auto get_completion_signatures() noexcept
    {
      return ustdex::completion_signatures<SetTag(Ts...)>();
    }

    template <class Rcvr>
    USTDEX_API _opstate_t<Rcvr, Ts...> connect(Rcvr _rcvr) && //
      noexcept(_nothrow_decay_copyable<Rcvr, Ts...>)
    {
      return _opstate_t<Rcvr, Ts...>{static_cast<Rcvr&&>(_rcvr), static_cast<_tuple<Ts...>&&>(_values_)};
    }

    template <class Rcvr>
    USTDEX_API _opstate_t<Rcvr, Ts...> connect(Rcvr _rcvr) const& //
      noexcept(_nothrow_decay_copyable<Rcvr, Ts const&...>)
    {
      return _opstate_t<Rcvr, Ts...>{static_cast<Rcvr&&>(_rcvr), _values_};
    }
  };

public:
  template <class... Ts>
  USTDEX_TRIVIAL_API auto operator()(Ts... _ts) const noexcept
  {
    return _sndr_t<Ts...>{JustTag{}, {{static_cast<Ts&&>(_ts)}...}};
  }
};

inline constexpr struct just_t : _just<_value>
{
} just{};

inline constexpr struct just_error_t : _just<_error>
{
} just_error{};

inline constexpr struct just_stopped_t : _just<_stopped>
{
} just_stopped{};
} // namespace ustdex

#include "epilogue.hpp"

#endif
