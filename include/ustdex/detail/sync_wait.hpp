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

// run_loop isn't supported on-device yet, so neither can sync_wait be.
#ifndef USTDEX_CUDA

#  include "exception.hpp"
#  include "meta.hpp"
#  include "run_loop.hpp"
#  include "utility.hpp"

#  include <optional>
#  include <system_error>
#  include <tuple>

namespace ustdex {
  /// @brief Function object type for synchronously waiting for the result of a
  /// sender.
  struct sync_wait_t {
#  ifndef __CUDACC__
   private:
#  endif
    struct _env_t {
      run_loop* _loop;

      USTDEX_HOST_DEVICE auto query(get_scheduler_t) const noexcept {
        return _loop->get_scheduler();
      }

      USTDEX_HOST_DEVICE auto query(get_delegatee_scheduler_t) const noexcept {
        return _loop->get_scheduler();
      }
    };

    template <class Sndr>
    using _values_of = value_types_of_t<Sndr, _env_t, std::tuple, _midentity::_f>;

    struct _state_t {
      std::exception_ptr _eptr;
      run_loop _loop;
    };

    template <class Values>
    struct _rcvr_t {
      using receiver_concept = receiver_t;
      std::optional<Values>* _values;
      _state_t* _state;

      template <class... As>
      USTDEX_HOST_DEVICE void set_value(As&&... _as) noexcept {
        USTDEX_TRY(
          ({ _values->emplace(static_cast<As&&>(_as)...); }), //
          USTDEX_CATCH(...)({ _state->_eptr = std::current_exception(); }))
        _state->_loop.finish();
      }

      template <class Error>
      USTDEX_HOST_DEVICE void set_error(Error _err) noexcept {
        if constexpr (USTDEX_IS_SAME(Error, std::exception_ptr))
          _state->_eptr = static_cast<Error&&>(_err);
        else if constexpr (USTDEX_IS_SAME(Error, std::error_code))
          _state->_eptr = std::make_exception_ptr(std::system_error(_err));
        else
          _state->_eptr = std::make_exception_ptr(static_cast<Error&&>(_err));
        _state->_loop.finish();
      }

      USTDEX_HOST_DEVICE void set_stopped() noexcept {
        _state->_loop.finish();
      }

      _env_t get_env() const noexcept {
        return _env_t{&_state->_loop};
      }
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
    /// @retval success Returns an engaged `std::optional` containing the result
    ///         values in a `std::tuple`.
    /// @retval canceled Returns an empty `std::optional`.
    /// @retval error Throws the error.
    ///
    /// @throws std::rethrow_exception(error) if the error has type
    ///         `std::exception_ptr`.
    /// @throws std::system_error(error) if the error has type
    ///         `std::error_code`.
    /// @throws error otherwise
    // clang-format on
    template <class Sndr>
    std::optional<_values_of<Sndr>> operator()(Sndr&& sndr) const {
      using values_t = _values_of<Sndr>;
      std::optional<values_t> result;
      _state_t state{};

      // Launch the sender with a continuation that will fill in a variant
      auto opstate =
        connect(static_cast<Sndr&&>(sndr), _rcvr_t<values_t>{&result, &state});
      start(opstate);

      // Wait for the variant to be filled in, and process any work that
      // may be delegated to this thread.
      state._loop.run();

      if (state._eptr)
        std::rethrow_exception(state._eptr);

      return result; // uses NRVO to "return" the result
    }
  };

  inline constexpr sync_wait_t sync_wait{};
} // namespace ustdex

#endif // USTDEX_CUDA
