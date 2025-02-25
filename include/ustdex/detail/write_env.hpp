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

#ifndef USTDEX_ASYNC_DETAIL_WRITE_ENV
#define USTDEX_ASYNC_DETAIL_WRITE_ENV

#include "completion_signatures.hpp"
#include "config.hpp"
#include "cpos.hpp"
#include "env.hpp"
#include "exception.hpp"
#include "rcvr_ref.hpp"
#include "rcvr_with_env.hpp"

#include "prologue.hpp"

namespace ustdex
{
struct write_env_t
{
private:
  template <class Rcvr, class Sndr, class Env>
  struct USTDEX_TYPE_VISIBILITY_DEFAULT _opstate_t
  {
    using operation_state_concept = operation_state_t;

    USTDEX_API explicit _opstate_t(Sndr&& _sndr, Env _env, Rcvr _rcvr)
        : _env_rcvr_{static_cast<Rcvr&&>(_rcvr), static_cast<Env&&>(_env)}
        , _opstate_(ustdex::connect(static_cast<Sndr&&>(_sndr), _rcvr_ref{_env_rcvr_}))
    {}

    USTDEX_IMMOVABLE(_opstate_t);

    USTDEX_API void start() noexcept
    {
      ustdex::start(_opstate_);
    }

    _rcvr_with_env_t<Rcvr, Env> _env_rcvr_;
    connect_result_t<Sndr, _rcvr_ref<_rcvr_with_env_t<Rcvr, Env>>> _opstate_;
  };

  template <class Sndr, class Env>
  struct USTDEX_TYPE_VISIBILITY_DEFAULT _sndr_t;

public:
  /// @brief Wraps one sender in another that modifies the execution
  /// environment by merging in the environment specified.
  template <class Sndr, class Env>
  USTDEX_TRIVIAL_API constexpr auto operator()(Sndr, Env) const //
    -> _sndr_t<Sndr, Env>;
};

template <class Sndr, class Env>
struct USTDEX_TYPE_VISIBILITY_DEFAULT write_env_t::_sndr_t
{
  using sender_concept = sender_t;
  USTDEX_NO_UNIQUE_ADDRESS write_env_t _tag_;
  Env _env_;
  Sndr _sndr_;

  template <class Self, class... Env2>
  USTDEX_API static constexpr auto get_completion_signatures()
  {
    using Child = _copy_cvref_t<Self, Sndr>;
    return ustdex::get_completion_signatures<Child, env<const Env&, FWD_ENV_T<Env2>>...>();
  }

  template <class Rcvr>
  USTDEX_API auto connect(Rcvr _rcvr) && -> _opstate_t<Rcvr, Sndr, Env>
  {
    return _opstate_t<Rcvr, Sndr, Env>{
      static_cast<Sndr&&>(_sndr_), static_cast<Env&&>(_env_), static_cast<Rcvr&&>(_rcvr)};
  }

  template <class Rcvr>
  USTDEX_API auto connect(Rcvr _rcvr) const& -> _opstate_t<Rcvr, const Sndr&, Env>
  {
    return _opstate_t<Rcvr, const Sndr&, Env>{_sndr_, _env_, static_cast<Rcvr&&>(_rcvr)};
  }

  USTDEX_API auto get_env() const noexcept -> env_of_t<Sndr>
  {
    return ustdex::get_env(_sndr_);
  }
};

template <class Sndr, class Env>
USTDEX_TRIVIAL_API constexpr auto write_env_t::operator()(Sndr _sndr, Env _env) const //
  -> write_env_t::_sndr_t<Sndr, Env>
{
  return write_env_t::_sndr_t<Sndr, Env>{{}, static_cast<Env&&>(_env), static_cast<Sndr&&>(_sndr)};
}

inline constexpr write_env_t write_env{};

} // namespace ustdex

#include "epilogue.hpp"

#endif
