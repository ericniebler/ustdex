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

#include "completion_signatures.hpp"
#include "config.hpp"
#include "cpos.hpp"
#include "utility.hpp"

// This must be the last #include
#include "prologue.hpp"

namespace USTDEX_NAMESPACE
{
template <class Data, class Rcvr>
struct state
{
  Data data;
  Rcvr receiver;
};

struct receiver_defaults
{
  using receiver_concept = ustdex::receiver_t;

  template <class Rcvr, class... Args>
  USTDEX_HOST_DEVICE USTDEX_INLINE static auto set_value(_ignore, Rcvr& rcvr, Args&&... args) noexcept
    -> ustdex::completion_signatures<ustdex::set_value_t(Args...)>
  {
    ustdex::set_value(std::move(rcvr), (Args&&) args...);
    return {};
  }

  template <class Rcvr, class Error>
  USTDEX_HOST_DEVICE USTDEX_INLINE static auto set_error(_ignore, Rcvr& rcvr, Error&& err) noexcept
    -> ustdex::completion_signatures<ustdex::set_error_t(Error)>
  {
    ustdex::set_error(std::move(rcvr), (Error&&) err);
    return {};
  }

  template <class Rcvr>
  USTDEX_HOST_DEVICE USTDEX_INLINE static auto set_stopped(_ignore, Rcvr& rcvr) noexcept
    -> ustdex::completion_signatures<ustdex::set_stopped_t()>
  {
    ustdex::set_stopped(std::move(rcvr));
    return {};
  }

  template <class Rcvr>
  USTDEX_HOST_DEVICE USTDEX_INLINE static decltype(auto) get_env(_ignore, const Rcvr& rcvr) noexcept
  {
    return ustdex::get_env(rcvr);
  }
};

template <class Data, class Rcvr>
struct basic_receiver
{
  using receiver_concept = ustdex::receiver_t;
  using _rcvr_t          = typename Data::receiver_tag;
  state<Data, Rcvr>& state_;

  template <class... Args>
  USTDEX_HOST_DEVICE USTDEX_INLINE void set_value(Args&&... args) noexcept
  {
    _rcvr_t::set_value(state_.data, state_.receiver, (Args&&) args...);
  }

  template <class Error>
  USTDEX_HOST_DEVICE USTDEX_INLINE void set_error(Error&& err) noexcept
  {
    _rcvr_t::set_error(state_.data, state_.receiver, (Error&&) err);
  }

  USTDEX_HOST_DEVICE USTDEX_INLINE void set_stopped() noexcept
  {
    _rcvr_t::set_stopped(state_.data, state_.receiver);
  }

  USTDEX_HOST_DEVICE USTDEX_INLINE decltype(auto) get_env() const noexcept
  {
    return _rcvr_t::get_env(state_.data, state_.receiver);
  }
};

template <class Rcvr>
inline constexpr bool has_no_environment = USTDEX_IS_SAME(Rcvr, receiver_archetype);

template <bool HasStopped, class Data, class Rcvr>
struct _mk_completions
{
  using _rcvr_t = typename Data::receiver_tag;

  template <class... Args>
  using _set_value_t =
    decltype(+*_rcvr_t::set_value(_declval<Data&>(), _declval<receiver_archetype&>(), _declval<Args>()...));

  template <class Error>
  using _set_error_t =
    decltype(+*_rcvr_t::set_error(_declval<Data&>(), _declval<receiver_archetype&>(), _declval<Error>()));

  using _set_stopped_t = ustdex::completion_signatures<>;
};

template <class Data, class Rcvr>
struct _mk_completions<true, Data, Rcvr> : _mk_completions<false, Data, Rcvr>
{
  using _rcvr_t = typename Data::receiver_tag;

  using _set_stopped_t = decltype(+*_rcvr_t::set_stopped(_declval<Data&>(), _declval<receiver_archetype&>()));
};

template <class...>
using _ignore_value_signature = ustdex::completion_signatures<>;

template <class>
using _ignore_error_signature = ustdex::completion_signatures<>;

template <class Completions>
constexpr bool _has_stopped = !USTDEX_IS_SAME(
  ustdex::completion_signatures<>,
  ustdex::transform_completion_signatures<Completions,
                                          ustdex::completion_signatures<>,
                                          _ignore_value_signature,
                                          _ignore_error_signature>);

template <bool PotentiallyThrowing, class Rcvr>
void set_current_exception_if(Rcvr& rcvr) noexcept
{
  if constexpr (PotentiallyThrowing)
  {
    ustdex::set_error(std::move(rcvr), ::std::current_exception());
  }
}

// A generic type that holds the data for an async operation, and
// that provides a `start` method for enqueuing the work.
template <class Sndr, class Data, class Rcvr>
struct basic_opstate
{
  using _rcvr_t        = basic_receiver<Data, Rcvr>;
  using _completions_t = completion_signatures_of_t<Sndr, _rcvr_t>;
  using _traits_t      = _mk_completions<_has_stopped<_completions_t>, Data, Rcvr>;

  using completion_signatures =
    transform_completion_signatures<_completions_t,
                                    ustdex::completion_signatures<>, // TODO
                                    _traits_t::template _set_value_t,
                                    _traits_t::template _set_error_t,
                                    typename _traits_t::_set_stopped_t>;

  USTDEX_HOST_DEVICE basic_opstate(Sndr&& sndr, Data data, Rcvr rcvr)
      : state_{static_cast<Data&&>(data), static_cast<Rcvr&&>(rcvr)}
      , op_(ustdex::connect(static_cast<Sndr&&>(sndr), _rcvr_t{state_}))
  {}

  USTDEX_HOST_DEVICE USTDEX_INLINE void start() noexcept
  {
    ustdex::start(op_);
  }

  state<Data, Rcvr> state_;
  ustdex::connect_result_t<Sndr, _rcvr_t> op_;
};

template <class Sndr, class Rcvr>
USTDEX_HOST_DEVICE USTDEX_INLINE auto _make_opstate(Sndr sndr, Rcvr rcvr)
{
  auto [tag, data, child] = std::move(sndr);
  return basic_opstate(std::move(child), std::move(data), std::move(rcvr));
}

template <class Data, class... Sndrs>
USTDEX_HOST_DEVICE USTDEX_INLINE auto _get_attrs(int, const Data& data, const Sndrs&... sndrs) noexcept
  -> decltype(data.get_attrs(sndrs...))
{
  return data.get_attrs(sndrs...);
}

template <class Data, class... Sndrs>
USTDEX_HOST_DEVICE USTDEX_INLINE auto _get_attrs(long, const Data& data, const Sndrs&... sndrs) noexcept
  -> decltype(ustdex::get_env(sndrs...))
{
  return ustdex::get_env(sndrs...);
}

template <class Data, class... Sndrs>
struct basic_sender;

template <class Data, class Sndr>
struct basic_sender<Data, Sndr>
{
  using sender_concept = ustdex::sender_t;
  using _tag_t         = typename Data::sender_tag;
  using _rcvr_t        = typename Data::receiver_tag;

  [[no_unique_address]] _tag_t _tag_;
  Data data_;
  Sndr sndr_;

  // Connect the sender to the receiver (the continuation) and
  // return the state_type object for this operation.
  template <class Rcvr>
  USTDEX_HOST_DEVICE USTDEX_INLINE auto connect(Rcvr rcvr) &&
  {
    return _make_opstate(std::move(*this), std::move(rcvr));
  }

  template <class Rcvr>
  USTDEX_HOST_DEVICE USTDEX_INLINE auto connect(Rcvr rcvr) const&
  {
    return _make_opstate(*this, std::move(rcvr));
  }

  USTDEX_HOST_DEVICE USTDEX_INLINE decltype(auto) get_env() const noexcept
  {
    return ustdex::_get_attrs(0, data_, sndr_);
  }
};

template <class Data, class... Sndrs>
basic_sender(_ignore, Data, Sndrs...) -> basic_sender<Data, Sndrs...>;

} // namespace USTDEX_NAMESPACE

#include "epilogue.hpp"
