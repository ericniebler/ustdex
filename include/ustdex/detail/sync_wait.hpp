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
  struct sync_wait_t;

  namespace _detail {
    template <bool>
    struct _exactly_one {
      template <class Tuple>
      using _f = Tuple;
    };

    template <>
    struct _exactly_one<false> {
      template <class... Ts>
      static auto _to_sig(std::tuple<Ts...>*) -> set_value_t (*)(Ts...);

      template <class... Sigs>
      static auto _collect(Sigs*...) -> WITH_COMPLETIONS<Sigs...>;

      template <class... Tuples>
      using _f = ERROR<
        WHERE(IN_ALGORITHM, sync_wait_t),
        WHAT(SENDER_HAS_TOO_MANY_SUCCESS_COMPLETIONS),
        decltype(_collect(_to_sig((Tuples*) 0)...))>;
    };
  } // namespace _detail

  namespace _sync_wait {
    template <class>
    static constexpr bool _ok = false;

    template <class... Ts>
    static constexpr bool _ok<std::tuple<Ts...>> = true;
  } // namespace _sync_wait

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

    struct _rcvr_base {
      std::exception_ptr _eptr;
      mutable run_loop _loop;

      _env_t get_env() const noexcept {
        return _env_t{&_loop};
      }
    };

    template <class Tuple>
    struct _rcvr : _rcvr_base {
      using receiver_concept = receiver_t;

      template <class... As>
      USTDEX_HOST_DEVICE void set_value(As&&... _as) noexcept {
        USTDEX_TRY(
          ({ //
            _values->emplace(static_cast<As&&>(_as)...);
          }),                 //
          USTDEX_CATCH(...)({ //
            _eptr = std::current_exception();
          }))
        _loop.finish();
      }

      template <class Error>
      USTDEX_HOST_DEVICE void set_error(Error _err) noexcept {
        if constexpr (USTDEX_IS_SAME(Error, std::exception_ptr))
          _eptr = static_cast<Error&&>(_err);
        else if constexpr (USTDEX_IS_SAME(Error, std::error_code))
          _eptr = std::make_exception_ptr(std::system_error(_err));
        else
          _eptr = std::make_exception_ptr(static_cast<Error&&>(_err));
        _loop.finish();
      }

      USTDEX_HOST_DEVICE void set_stopped() noexcept {
        _loop.finish();
      }

      std::optional<Tuple>* _values;
    };

    template <class... Tuples>
    using _variant_t =
      _mtry_invoke<_detail::_exactly_one<sizeof...(Tuples) == 1>, Tuples...>;

    template <class Sndr, class Rcvr = _rcvr_base>
    using _values_t = value_types_of_t<Sndr, Rcvr*, std::tuple, _variant_t>;

    struct _invalid_sync_wait {
      const _invalid_sync_wait& value() const;
      const _invalid_sync_wait& operator*() const;
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
    auto operator()(Sndr&& sndr) const {
      using _sync_wait::_ok;
      using _vals_t = _values_t<Sndr>;
      static_assert(_ok<_vals_t>);

      // Avoid cascading errors by skipping the sync_wait logic if the sender
      // has too many value completions, or is invalid in some other way.
      if constexpr (!_ok<_vals_t>) {
        return _invalid_sync_wait{0};
      } else {
        static_assert(USTDEX_IS_SAME(_vals_t, _values_t<Sndr, _rcvr<_vals_t>>));

        std::optional<_vals_t> result{};
        _rcvr<_vals_t> rcvr{{}, &result};

        // Launch the sender with a continuation that will fill in a variant
        auto opstate = connect(static_cast<Sndr&&>(sndr), &rcvr);
        start(opstate);

        // Wait for the variant to be filled in, and process any work that
        // may be delegated to this thread.
        rcvr._loop.run();

        if (rcvr._eptr)
          std::rethrow_exception(rcvr._eptr);

        return result; // uses NRVO to "return" the result
      }
    }
  };

  inline constexpr sync_wait_t sync_wait{};
} // namespace ustdex

#endif // USTDEX_CUDA
