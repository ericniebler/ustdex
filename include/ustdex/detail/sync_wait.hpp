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

// run_loop isn't supported on-device yet, so neither can sync_wait be.
#if USTDEX_HOST_ONLY()

#  include <optional>
#  include <system_error>
#  include <tuple>

#  include "exception.hpp"
#  include "meta.hpp"
#  include "run_loop.hpp"
#  include "utility.hpp"

// Must be the last include
#  include "prologue.hpp"

namespace USTDEX_NAMESPACE
{
/// @brief Function object type for synchronously waiting for the result of a
/// sender.
struct sync_wait_t
{
#  ifndef __CUDACC__

private:
#  endif
  struct _env_t
  {
    run_loop* _loop;

    USTDEX_HOST_DEVICE auto query(get_scheduler_t) const noexcept
    {
      return _loop->get_scheduler();
    }

    USTDEX_HOST_DEVICE auto query(get_delegatee_scheduler_t) const noexcept
    {
      return _loop->get_scheduler();
    }
  };

  template <class Sndr>
  struct _state_t
  {
    struct _rcvr_t
    {
      using receiver_concept = receiver_t;
      _state_t* _state;

      template <class... As>
      USTDEX_HOST_DEVICE void set_value(As&&... _as) noexcept
      {
        USTDEX_TRY( //
          ({ //
            _state->_values->emplace(static_cast<As&&>(_as)...);
          }), //
          USTDEX_CATCH(...)( //
            { //
              _state->_eptr = ::std::current_exception();
            }))
        _state->_loop.finish();
      }

      template <class Error>
      USTDEX_HOST_DEVICE void set_error(Error _err) noexcept
      {
        if constexpr (USTDEX_IS_SAME(Error, ::std::exception_ptr))
        {
          _state->_eptr = static_cast<Error&&>(_err);
        }
        else if constexpr (USTDEX_IS_SAME(Error, ::std::error_code))
        {
          _state->_eptr = ::std::make_exception_ptr(::std::system_error(_err));
        }
        else
        {
          _state->_eptr = ::std::make_exception_ptr(static_cast<Error&&>(_err));
        }
        _state->_loop.finish();
      }

      USTDEX_HOST_DEVICE void set_stopped() noexcept
      {
        _state->_loop.finish();
      }

      _env_t get_env() const noexcept
      {
        return _env_t{&_state->_loop};
      }
    };

    using _values_t = value_types_of_t<Sndr, _rcvr_t, ::std::tuple, _midentity::_f>;

    ::std::optional<_values_t>* _values;
    ::std::exception_ptr _eptr;
    run_loop _loop;
  };

  struct _invalid_sync_wait
  {
    const _invalid_sync_wait& value() const
    {
      return *this;
    }

    const _invalid_sync_wait& operator*() const
    {
      return *this;
    }

    int i;
  };

public:
  // clang-format off
    /// @brief Synchronously wait for the result of a sender, blocking the
    ///         current thread.
    ///
    /// `sync_wait` connects and starts the given sender, and then drives a
    ///         `run_loop` instance until the sender completes. Additional work
    ///         can be delegated to the `run_loop` by scheduling work on the
    ///         scheduler returned by calling `get_delegatee_scheduler` on the
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
  auto operator()(Sndr&& sndr) const
  {
    using _rcvr_t      = typename _state_t<Sndr>::_rcvr_t;
    using _values_t    = typename _state_t<Sndr>::_values_t;
    using _completions = completion_signatures_of_t<Sndr, _rcvr_t>;
    static_assert(_is_completion_signatures<_completions>);

    if constexpr (!_is_completion_signatures<_completions>)
    {
      return _invalid_sync_wait{0};
    }
    else
    {
      ::std::optional<_values_t> result{};
      _state_t<Sndr> state{&result};

      // Launch the sender with a continuation that will fill in a variant
      auto opstate = connect(static_cast<Sndr&&>(sndr), _rcvr_t{&state});
      start(opstate);

      // Wait for the variant to be filled in, and process any work that
      // may be delegated to this thread.
      state._loop.run();

      if (state._eptr)
      {
        ::std::rethrow_exception(state._eptr);
      }

      return result; // uses NRVO to "return" the result
    }
  }
};

inline constexpr sync_wait_t sync_wait{};
} // namespace USTDEX_NAMESPACE

#  include "epilogue.hpp"

#endif // USTDEX_HOST_ONLY()
