//===----------------------------------------------------------------------===//
//
// Part of CUDA Experimental in CUDA C++ Core Libraries,
// under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// SPDX-FileCopyrightText: Copyright (c) 2024 NVIDIA CORPORATION & AFFILIATES.
//
//===----------------------------------------------------------------------===//
#pragma once

#include "config.hpp"
#include "cpos.hpp"
#include "env.hpp"
#include "exception.hpp"
#include "receiver_with_env.hpp"
#include "queries.hpp"
#include "utility.hpp"

// Must be the last include
#include "prologue.hpp"

namespace USTDEX_NAMESPACE
{
struct write_env_t
{
#ifndef __CUDACC__

private:
#endif
  template <class Rcvr, class Sndr, class Env>
  struct _opstate_t
  {
    using operation_state_concept = operation_state_t;
    using completion_signatures   = completion_signatures_of_t<Sndr, _receiver_with_env_t<Rcvr, Env>*>;

    _receiver_with_env_t<Rcvr, Env> _env_rcvr;
    connect_result_t<Sndr, _receiver_with_env_t<Rcvr, Env>*> _op;

    USTDEX_HOST_DEVICE explicit _opstate_t(Sndr&& sndr, Env env, Rcvr rcvr)
        : _env_rcvr(static_cast<Env&&>(env), static_cast<Rcvr&&>(rcvr))
        , _op(ustdex::connect(static_cast<Sndr&&>(sndr), &_env_rcvr))
    {}

    USTDEX_IMMOVABLE(_opstate_t);

    USTDEX_HOST_DEVICE void start() noexcept
    {
      ustdex::start(_op);
    }
  };

  template <class Sndr, class Env>
  struct _sndr_t;

public:
  /// @brief Wraps one sender in another that modifies the execution
  /// environment by merging in the environment specified.
  template <class Sndr, class Env>
  USTDEX_HOST_DEVICE USTDEX_INLINE constexpr auto operator()(Sndr, Env) const //
    -> _sndr_t<Sndr, Env>;
};

template <class Sndr, class Env>
struct write_env_t::_sndr_t
{
  using sender_concept = sender_t;
  USTDEX_NO_UNIQUE_ADDRESS write_env_t _tag;
  Env _env;
  Sndr _sndr;

  template <class Rcvr>
  USTDEX_HOST_DEVICE auto connect(Rcvr rcvr) && -> _opstate_t<Rcvr, Sndr, Env>
  {
    return _opstate_t<Rcvr, Sndr, Env>{static_cast<Sndr&&>(_sndr), static_cast<Env&&>(_env), static_cast<Rcvr&&>(rcvr)};
  }

  template <class Rcvr>
  USTDEX_HOST_DEVICE auto connect(Rcvr rcvr) const& //
    -> _opstate_t<Rcvr, const Sndr&, Env>
  {
    return _opstate_t<Rcvr, const Sndr&, Env>{_sndr, _env, static_cast<Rcvr&&>(rcvr)};
  }

  USTDEX_HOST_DEVICE env_of_t<Sndr> get_env() const noexcept
  {
    return ustdex::get_env(_sndr);
  }
};

template <class Sndr, class Env>
USTDEX_HOST_DEVICE USTDEX_INLINE constexpr auto write_env_t::operator()(Sndr sndr, Env env) const //
  -> write_env_t::_sndr_t<Sndr, Env>
{
  return write_env_t::_sndr_t<Sndr, Env>{{}, static_cast<Env&&>(env), static_cast<Sndr&&>(sndr)};
}

USTDEX_DEVICE_CONSTANT constexpr write_env_t write_env{};

} // namespace USTDEX_NAMESPACE

#include "epilogue.hpp"
