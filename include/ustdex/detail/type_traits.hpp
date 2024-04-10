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

#include "meta.hpp"

namespace ustdex {
  template <class Ty>
  using _fnptr = Ty && (*) () noexcept;
#define DECLVAL(...) _fnptr<__VA_ARGS__>()()

//////////////////////////////////////////////////////////////////////////////////////////////////
// __decay_t: An efficient implementation for std::decay
#if __has_builtin(__decay)

  template <class Ty>
  using _decay_t = __decay(Ty);

  // #elif STDEXEC_NVHPC()

  //   template <class Ty>
  //   using _decay_t = std::decay_t<Ty>;

#else

  struct _decay_object {
    template <class Ty>
    static Ty _g(Ty const &);
    template <class Ty>
    using _f = decltype(_g(DECLVAL(Ty)));
  };

  struct _decay_default {
    template <class Ty>
    static Ty _g(Ty);
    template <class Ty>
    using _f = decltype(_g(DECLVAL(Ty)));
  };

  struct _decay_abominable {
    template <class Ty>
    using _f = Ty;
  };

  struct _decay_void {
    template <class Ty>
    using _f = void;
  };

  template <class Ty>
  extern _decay_object _mdecay;

  template <class Ty, class... Us>
  extern _decay_default _mdecay<Ty(Us...)>;

  template <class Ty, class... Us>
  extern _decay_default _mdecay<Ty(Us...) noexcept>;

  template <class Ty, class... Us>
  extern _decay_default _mdecay<Ty (&)(Us...)>;

  template <class Ty, class... Us>
  extern _decay_default _mdecay<Ty (&)(Us...) noexcept>;

  template <class Ty, class... Us>
  extern _decay_abominable _mdecay<Ty(Us...) const>;

  template <class Ty, class... Us>
  extern _decay_abominable _mdecay<Ty(Us...) const noexcept>;

  template <class Ty, class... Us>
  extern _decay_abominable _mdecay<Ty(Us...) const &>;

  template <class Ty, class... Us>
  extern _decay_abominable _mdecay<Ty(Us...) const & noexcept>;

  template <class Ty, class... Us>
  extern _decay_abominable _mdecay<Ty(Us...) const &&>;

  template <class Ty, class... Us>
  extern _decay_abominable _mdecay<Ty(Us...) const && noexcept>;

  template <class Ty>
  extern _decay_default _mdecay<Ty[]>;

  template <class Ty, std::size_t N>
  extern _decay_default _mdecay<Ty[N]>;

  template <class Ty, std::size_t N>
  extern _decay_default _mdecay<Ty (&)[N]>;

  template <>
  inline _decay_void _mdecay<void>;

  template <>
  inline _decay_void _mdecay<void const>;

  template <class Ty>
  using _decay_t = typename decltype(_mdecay<Ty>)::template _f<Ty>;

#endif

  //////////////////////////////////////////////////////////////////////////////////////////////////
  // _copy_cvref_t: For copying cvref from one type to another
  struct _cp {
    template <class Tp>
    using _f = Tp;
  };

  struct _cpc {
    template <class Tp>
    using _f = const Tp;
  };

  struct _cplr {
    template <class Tp>
    using _f = Tp &;
  };

  struct _cprr {
    template <class Tp>
    using _f = Tp &&;
  };

  struct _cpclr {
    template <class Tp>
    using _f = const Tp &;
  };

  struct _cpcrr {
    template <class Tp>
    using _f = const Tp &&;
  };

  template <class>
  extern _cp _cpcvr;
  template <class Tp>
  extern _cpc _cpcvr<const Tp>;
  template <class Tp>
  extern _cplr _cpcvr<Tp &>;
  template <class Tp>
  extern _cprr _cpcvr<Tp &&>;
  template <class Tp>
  extern _cpclr _cpcvr<const Tp &>;
  template <class Tp>
  extern _cpcrr _cpcvr<const Tp &&>;
  template <class Tp>
  using _copy_cvref_fn = decltype(_cpcvr<Tp>);

  template <class From, class To>
  using _copy_cvref_t = typename _copy_cvref_fn<From>::template _f<To>;

  template <class Fn, class... As>
  using _call_result_t = decltype(DECLVAL(Fn)(DECLVAL(As)...));

  template <class Fn, class... As>
  inline constexpr bool _callable = _mvalid_q<_call_result_t, Fn, As...>;

  template <class Fn, class... As>
  using _nothrow_callable_ = _mif<noexcept(DECLVAL(Fn)(DECLVAL(As)...))>;

  template <class Fn, class... As>
  inline constexpr bool _nothrow_callable = _mvalid_q<_nothrow_callable_, Fn, As...>;

  template <class Ty, class... As>
  using _nothrow_constructible_ = _mif<noexcept(Ty{DECLVAL(As)...})>;

  template <class Ty, class... As>
  inline constexpr bool _nothrow_constructible =
    _mvalid_q<_nothrow_constructible_, Ty, As...>;
} // namespace ustdex
