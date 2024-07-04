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

#include "config.hpp"
#include "cpos.hpp"

namespace ustdex
{
struct start_detached_t
{
#ifndef __CUDACC__

private:
#endif
  struct _opstate_base_t : _immovable
  {};

  struct _rcvr_t
  {
    using receiver_concept = receiver_t;

    _opstate_base_t* _opstate;
    void (*_destroy)(_opstate_base_t*) noexcept;

    template <class... As>
    void set_value(As&&...) && noexcept
    {
      _destroy(_opstate);
    }

    template <class Error>
    void set_error(Error&&) && noexcept
    {
      std::terminate();
    }

    void set_stopped() && noexcept
    {
      _destroy(_opstate);
    }
  };

  template <class Sndr>
  struct _opstate_t : _opstate_base_t
  {
    using operation_state_concept = operation_state_t;
    using completion_signatures   = ustdex::completion_signatures_of_t<Sndr, _rcvr_t>;
    connect_result_t<Sndr, _rcvr_t> _op;

    static void _destroy(_opstate_base_t* ptr) noexcept
    {
      delete static_cast<_opstate_t*>(ptr);
    }

    USTDEX_HOST_DEVICE explicit _opstate_t(Sndr&& sndr)
        : _op(ustdex::connect(static_cast<Sndr&&>(sndr), _rcvr_t{this, &_destroy}))
    {}

    USTDEX_HOST_DEVICE void start() & noexcept
    {
      ustdex::start(_op);
    }
  };

public:
  /// @brief Eagerly connects and starts a sender and lets it
  /// run detached.
  template <class Sndr>
  USTDEX_HOST_DEVICE USTDEX_INLINE void operator()(Sndr sndr) const
  {
    ustdex::start(*new _opstate_t<Sndr>{static_cast<Sndr&&>(sndr)});
  }
};

USTDEX_DEVICE_CONSTANT constexpr start_detached_t start_detached{};
} // namespace ustdex
