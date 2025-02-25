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

#ifndef USTDEX_ASYNC_DETAIL_SYNC_WAIT
#define USTDEX_ASYNC_DETAIL_SYNC_WAIT

// run_loop isn't supported on-device yet, so neither can sync_wait be.
#if !defined(__CUDA_ARCH__)

#  include "exception.hpp"
#  include "meta.hpp"
#  include "run_loop.hpp"
#  include "utility.hpp"
#  include "write_env.hpp"

#  include <optional>
#  include <system_error>
#  include <tuple> // IWYU pragma: keep
#  include <type_traits>

#  include "prologue.hpp"

namespace ustdex
{
/// @brief Function object type for synchronously waiting for the result of a
/// sender.
struct sync_wait_t
{
private:
  struct _env_t
  {
    run_loop* _loop_;

    USTDEX_API auto query(get_scheduler_t) const noexcept
    {
      return _loop_->get_scheduler();
    }

    USTDEX_API auto query(get_delegation_scheduler_t) const noexcept
    {
      return _loop_->get_scheduler();
    }
  };

  template <class Values>
  struct _state_t
  {
    struct USTDEX_TYPE_VISIBILITY_DEFAULT _rcvr_t
    {
      using receiver_concept = receiver_t;
      _state_t* _state_;

      template <class... As>
      USTDEX_API void set_value(As&&... _as) noexcept
      {
        USTDEX_TRY(         //
          ({                //
            _state_->_values_->emplace(static_cast<As&&>(_as)...);
          }),               //
          USTDEX_CATCH(...) //
          ({                //
            _state_->_eptr_ = ::std::current_exception();
          })                //
        )
        _state_->_loop_.finish();
      }

      template <class Error>
      USTDEX_API void set_error(Error _err) noexcept
      {
        if constexpr (std::is_same_v<Error, ::std::exception_ptr>)
        {
          _state_->_eptr_ = static_cast<Error&&>(_err);
        }
        else if constexpr (std::is_same_v<Error, ::std::error_code>)
        {
          _state_->_eptr_ = ::std::make_exception_ptr(::std::system_error(_err));
        }
        else
        {
          _state_->_eptr_ = ::std::make_exception_ptr(static_cast<Error&&>(_err));
        }
        _state_->_loop_.finish();
      }

      USTDEX_API void set_stopped() noexcept
      {
        _state_->_loop_.finish();
      }

      _env_t get_env() const noexcept
      {
        return _env_t{&_state_->_loop_};
      }
    };

    std::optional<Values>* _values_;
    ::std::exception_ptr _eptr_;
    run_loop _loop_;
  };

  template <class Type>
  struct __always_false : std::false_type
  {};

  template <class Diagnostic>
  struct _bad_sync_wait
  {
    static_assert(__always_false<Diagnostic>(), "sync_wait cannot compute the completions of the sender passed to it.");
    static _bad_sync_wait _result();

    const _bad_sync_wait& value() const;
    const _bad_sync_wait& operator*() const;

    int i{}; // so that structured bindings kinda work
  };

public:
  // clang-format off
    /// @brief Synchronously wait for the result of a sender, blocking the
    ///         current thread.
    ///
    /// `sync_wait` connects and starts the given sender, and then drives a
    ///         `run_loop` instance until the sender completes. Additional work
    ///         can be delegated to the `run_loop` by scheduling work on the
    ///         scheduler returned by calling `get_delegation_scheduler` on the
    ///         receiver's environment.
    ///
    /// @pre The sender must have a exactly one value completion signature. That
    ///         is, it can only complete successfully in one way, with a single
    ///         set of values.
    ///
    /// @retval success Returns an engaged `::std::optional` containing the result
    ///         values in a `::std::tuple`.
    /// @retval canceled Returns an empty `::std::optional`.
    /// @retval error Throws the error.
    ///
    /// @throws ::std::rethrow_exception(error) if the error has type
    ///         `::std::exception_ptr`.
    /// @throws ::std::system_error(error) if the error has type
    ///         `::std::error_code`.
    /// @throws error otherwise
  // clang-format on
  template <class Sndr>
  auto operator()(Sndr&& _sndr) const
  {
    using _completions = completion_signatures_of_t<Sndr, _env_t>;

    if constexpr (!_valid_completion_signatures<_completions>)
    {
      return _bad_sync_wait<_completions>::_result();
    }
    else
    {
      using _values = _value_types<_completions, std::tuple, _identity_t>;
      std::optional<_values> _result{};
      _state_t<_values> _state{&_result, {}, {}};

      // Launch the sender with a continuation that will fill in a variant
      using _rcvr   = typename _state_t<_values>::_rcvr_t;
      auto _opstate = ustdex::connect(static_cast<Sndr&&>(_sndr), _rcvr{&_state});
      ustdex::start(_opstate);

      // Wait for the variant to be filled in, and process any work that
      // may be delegated to this thread.
      _state._loop_.run();

      if (_state._eptr_)
      {
        ::std::rethrow_exception(_state._eptr_);
      }

      return _result; // uses NRVO to "return" the result
    }
  }

  template <class Sndr, class Env>
  auto operator()(Sndr&& _sndr, Env&& _env) const
  {
    return (*this)(ustdex::write_env(static_cast<Sndr&&>(_sndr), static_cast<Env&&>(_env)));
  }
};

inline constexpr sync_wait_t sync_wait{};
} // namespace ustdex

#  include "epilogue.hpp"

#endif // !defined(__CUDA_ARCH__)

#endif
