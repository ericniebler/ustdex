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

#include "cpos.hpp"
#include "env.hpp"

namespace ustdex {
  template <class Rcvr, class Env>
  struct _receiver_with_env_t : Rcvr {
    using _env_t = _joined_env_t<const Env &, env_of_t<Rcvr>>;

    USTDEX_HOST_DEVICE auto rcvr() noexcept -> Rcvr & {
      return *this;
    }

    USTDEX_HOST_DEVICE auto rcvr() const noexcept -> const Rcvr & {
      return *this;
    }

    USTDEX_HOST_DEVICE auto get_env() const noexcept -> _env_t {
      return _joined_env_t{_env, ustdex::get_env(static_cast<const Rcvr &>(*this))};
    }

    Env _env;
  };

  template <class Rcvr, class Env>
  struct _receiver_with_env_t<Rcvr*, Env> {
    using _env_t = _joined_env_t<const Env &, env_of_t<Rcvr *>>;

    USTDEX_INLINE USTDEX_HOST_DEVICE auto rcvr() const noexcept -> Rcvr * {
      return _rcvr;
    }

    template <class... As>
    USTDEX_INLINE USTDEX_HOST_DEVICE void set_value(As&&... as) && noexcept {
      ustdex::set_value(_rcvr, static_cast<As &&>(as)...);
    }

    template <class Error>
    USTDEX_INLINE USTDEX_HOST_DEVICE void set_error(Error &&error) && noexcept {
      ustdex::set_error(_rcvr, static_cast<Error &&>(error));
    }

    USTDEX_INLINE USTDEX_HOST_DEVICE void set_stopped() && noexcept {
      ustdex::set_stopped(_rcvr);
    }

    USTDEX_HOST_DEVICE auto get_env() const noexcept -> _env_t {
      return _joined_env_t{_env, ustdex::get_env(_rcvr)};
    }

    Rcvr* _rcvr;
    Env _env;
  };
} // namespace ustdex
