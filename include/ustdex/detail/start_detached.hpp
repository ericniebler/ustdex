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

#ifndef USTDEX_ASYNC_DETAIL_START_DETACHED
#define USTDEX_ASYNC_DETAIL_START_DETACHED

#include "config.hpp"
#include "cpos.hpp"

#include "prologue.hpp"

namespace ustdex
{
struct start_detached_t
{
private:
  struct _opstate_base_t : _immovable
  {};

  struct USTDEX_TYPE_VISIBILITY_DEFAULT _rcvr_t
  {
    using receiver_concept = receiver_t;

    _opstate_base_t* _opstate_;
    void (*_destroy)(_opstate_base_t*) noexcept;

    template <class... As>
    void set_value(As&&...) && noexcept
    {
      _destroy(_opstate_);
    }

    template <class Error>
    void set_error(Error&&) && noexcept
    {
      std::terminate();
    }

    void set_stopped() && noexcept
    {
      _destroy(_opstate_);
    }
  };

  template <class Sndr>
  struct USTDEX_TYPE_VISIBILITY_DEFAULT _opstate_t : _opstate_base_t
  {
    using operation_state_concept = operation_state_t;
    connect_result_t<Sndr, _rcvr_t> _opstate_;

    static void _destroy(_opstate_base_t* _ptr) noexcept
    {
      delete static_cast<_opstate_t*>(_ptr);
    }

    USTDEX_API explicit _opstate_t(Sndr&& _sndr)
        : _opstate_(ustdex::connect(static_cast<Sndr&&>(_sndr), _rcvr_t{this, &_destroy}))
    {}

    USTDEX_API void start() & noexcept
    {
      ustdex::start(_opstate_);
    }
  };

public:
  /// @brief Eagerly connects and starts a sender and lets it
  /// run detached.
  template <class Sndr>
  USTDEX_TRIVIAL_API void operator()(Sndr _sndr) const
  {
    ustdex::start(*new _opstate_t<Sndr>{static_cast<Sndr&&>(_sndr)});
  }
};

inline constexpr start_detached_t start_detached{};
} // namespace ustdex

#include "epilogue.hpp"

#endif
