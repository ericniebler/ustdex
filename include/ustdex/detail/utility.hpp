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
#include "meta.hpp"
#include "type_traits.hpp"

#include <initializer_list>

namespace ustdex {
  USTDEX_DEVICE_CONSTANT constexpr std::size_t _npos = ~0UL;

  struct _ignore {
    template <class... As>
    constexpr _ignore(As &&...) noexcept {};
  };

  template <class...>
  struct _undefined;

  struct _empty { };

  struct [[deprecated]] _deprecated { };

  struct _nil { };

  struct _immovable {
    _immovable() = default;
    USTDEX_IMMOVABLE(_immovable);
  };

  inline constexpr std::size_t _max(std::initializer_list<std::size_t> il) noexcept {
    std::size_t max = 0;
    for (auto i: il) {
      if (i > max) {
        max = i;
      }
    }
    return max;
  }

  constexpr std::size_t
    _find_pos(bool const *const begin, bool const *const end) noexcept {
    for (bool const *where = begin; where != end; ++where) {
      if (*where) {
        return static_cast<std::size_t>(where - begin);
      }
    }
    return _npos;
  }

  template <class Ty, class... Ts>
  inline constexpr std::size_t _index_of() noexcept {
    constexpr bool _same[] = {USTDEX_IS_SAME(Ty, Ts)...};
    return ustdex::_find_pos(_same, _same + sizeof...(Ts));
  }

  template <class Ty, class Uy = Ty>
  inline constexpr Ty _exchange(Ty &obj, Uy &&new_value) noexcept {
    constexpr bool _nothrow =                        //
      noexcept(Ty(static_cast<Ty &&>(obj))) &&       //
      noexcept(obj = static_cast<Uy &&>(new_value)); //
    static_assert(_nothrow);

    Ty old_value = static_cast<Ty &&>(obj);
    obj = static_cast<Uy &&>(new_value);
    return old_value;
  }

  template <class Ty>
  inline constexpr Ty _decay_copy(Ty &&ty) noexcept(_nothrow_decay_copyable<Ty>) {
    return static_cast<Ty &&>(ty);
  }

  // _ref is for keeping type names short in compiler output.
  // It has the unfortunate side effect of obfuscating the types.
#if USTDEX_NVHPC() || USTDEX_EDG()
  // This version of _ref is for compilers that display the return
  // type of a monomorphic lambda in the compiler output.
  inline constexpr auto _ref = [](auto fn) noexcept {
    return [fn](auto...) noexcept -> decltype(auto) {
      return fn;
    };
  };
#else
  inline constexpr auto _ref = [](auto fn) noexcept {
    return [fn]() noexcept -> decltype(auto) {
      return fn;
    };
  };
#endif

  template <class Ty>
  using _ref_t = decltype(_ref(static_cast<Ty (*)()>(nullptr)));

  template <class Ref>
  using _deref_t = decltype(_declval<Ref>()()());
} // namespace ustdex
