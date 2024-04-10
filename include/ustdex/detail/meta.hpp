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

namespace ustdex {
  template <class...>
  using _mvoid = void;

  template <class... Ts>
  struct _mlist {
    template <class Fn, class... Us>
    using _f = typename Fn::template _f<Ts..., Us...>;
  };

  template <auto Val>
  struct _mvalue {
    static constexpr auto value = Val;
  };

  using _mtrue = _mvalue<true>;
  using _mfalse = _mvalue<false>;

  struct _mexception_base {
    constexpr friend bool _stdex_uncaught_exception(void *) noexcept {
      return true;
    }
  };

  template <class... What>
  struct _mexception : _mexception_base {
    template <class...>
    using _f = _mexception;
  };

  constexpr bool _stdex_uncaught_exception(...) noexcept {
    return false;
  }

  template <class... Ts>
  inline constexpr bool _muncaught_exception =
    _stdex_uncaught_exception(static_cast<_mlist<Ts...> *>(nullptr));

  template <template <class...> class Fn, class... Ts>
  using _minvoke_q = Fn<Ts...>;

  template <class Fn, class... Ts>
  using _minvoke = _minvoke_q<Fn::template _f, Ts...>;

  template <template <class...> class Fn, class List, class Enable = void>
  inline constexpr bool _mvalid_ = false;

  template <template <class...> class Fn, class... Ts>
  inline constexpr bool _mvalid_<Fn, _mlist<Ts...>, _mvoid<Fn<Ts...>>> = true;

  template <template <class...> class Fn, class... Ts>
  inline constexpr bool _mvalid_q = _mvalid_<Fn, _mlist<Ts...>>;

  template <class Fn, class... Ts>
  inline constexpr bool _mvalid = _mvalid_<Fn::template _f, _mlist<Ts...>>;

  struct _midentity {
    template <class Ty>
    using _f = Ty;
  };

  template <class Ty>
  struct _malways {
    template <class...>
    using _f = Ty;
  };

  template <bool>
  struct _mif_ {
    template <class Then, class...>
    using _f = Then;
  };

  template <>
  struct _mif_<false> {
    template <class Then, class Else>
    using _f = Else;
  };

  template <bool If, class Then = void, class... Else>
  using _mif = typename _mif_<If>::template _f<Then, Else...>;

  template <bool>
  struct _mtry_ {
    template <template <class...> class Fn, class... Ts>
    using _g = Fn<Ts...>;

    template <class Fn, class... Ts>
    using _f = typename Fn::template _f<Ts...>;
  };

  template <>
  struct _mtry_<true> {
    template <template <class...> class Fn, class... Ts>
    using _g = _mexception<struct unknown>;

    template <class Fn, class... Ts>
    using _f = _mexception<struct unknown>;
  };

  template <class Fn, class... Ts>
  using _mtry_invoke =
    typename _mtry_<_muncaught_exception<Ts...>>::template _f<Fn, Ts...>;

  template <template <class...> class Fn, class... Ts>
  using _mtry_invoke_q =
    typename _mtry_<_muncaught_exception<Ts...>>::template _g<Fn, Ts...>;

  template <template <class...> class Fn, class... Default>
  struct _mquote;

  template <template <class...> class Fn>
  struct _mquote<Fn> {
    template <class... Ts>
    using _f = Fn<Ts...>;
  };

  template <template <class...> class Fn, class Default>
  struct _mquote<Fn, Default> {
    template <class... Ts>
    using _f =
      typename _mif<_mvalid_q<Fn, Ts...>, _mquote<Fn>, _malways<Default>>::template _f<
        Ts...>;
  };

  template <template <class...> class Fn, class... Default>
  struct _mtry_quote;

  template <template <class...> class Fn>
  struct _mtry_quote<Fn> {
    template <class... Ts>
    using _f = typename _mtry_<_muncaught_exception<Ts...>>::template _g<Fn, Ts...>;
  };

  template <template <class...> class Fn, class Default>
  struct _mtry_quote<Fn, Default> {
    template <class... Ts>
    using _f =
      typename _mif<_mvalid_q<Fn, Ts...>, _mtry_quote<Fn>, _malways<Default>>::template _f<
        Ts...>;
  };

  template <class Tp>
  inline constexpr auto _v = Tp::value;

  template <auto Value>
  inline constexpr auto _v<_mvalue<Value>> = Value;

  template <class Tp, Tp Value>
  inline constexpr auto _v<std::integral_constant<Tp, Value>> = Value;

  template <class Fn, class... Ts>
  struct _mbind_front {
    template <class... Us>
    using _f = _minvoke<Fn, Ts..., Us...>;
  };

  template <template <class...> class Fn, class... Ts>
  struct _mbind_front_q {
    template <class... Us>
    using _f = _minvoke_q<Fn, Ts..., Us...>;
  };

  template <class Fn, class... Ts>
  struct _mbind_back {
    template <class... Us>
    using _f = _minvoke<Fn, Us..., Ts...>;
  };

  template <template <class...> class Fn, class... Ts>
  struct _mbind_back_q {
    template <class... Us>
    using _f = _minvoke_q<Fn, Us..., Ts...>;
  };

#if __has_builtin(__type_pack_element)
  template <bool>
  struct _m_at_ {
    template <class Np, class... Ts>
    using _f = __type_pack_element<_v<Np>, Ts...>;
  };

  template <class Np, class... Ts>
  using _m_at = _minvoke<_m_at_<_v<Np> == ~0ul>, Np, Ts...>;

  template <std::size_t Np, class... Ts>
  using _m_at_c = _minvoke<_m_at_<Np == ~0ul>, _mvalue<Np>, Ts...>;
#else
  template <std::size_t Idx>
  struct _mget {
    template <class, class, class, class, class... Ts>
    using _f = _minvoke<_mtry_<sizeof...(Ts) == ~0ull>, _mget<Idx - 4>, Ts...>;
  };

  template <>
  struct _mget<0> {
    template <class T, class...>
    using _f = T;
  };

  template <>
  struct _mget<1> {
    template <class, class T, class...>
    using _f = T;
  };

  template <>
  struct _mget<2> {
    template <class, class, class T, class...>
    using _f = T;
  };

  template <>
  struct _mget<3> {
    template <class, class, class, class T, class...>
    using _f = T;
  };

  template <class Np, class... Ts>
  using _m_at = _minvoke<_mget<_v<Np>>, Ts...>;

  template <std::size_t Np, class... Ts>
  using _m_at_c = _minvoke<_mget<Np>, Ts...>;
#endif

  template <class... Ts>
  struct _mset;

  template <>
  struct _mset<> {
    template <class Fn, class... Us>
    using _f = _minvoke<Fn, Us...>;
  };

  template <class Ty, class... Ts>
  struct _mset<Ty, Ts...>
    : _mlist<Ty>
    , _mset<Ts...> {
    template <class Fn, class... Us>
    using _f = _minvoke<Fn, Ty, Ts..., Us...>;
  };

  template <class Set, class Ty>
  inline constexpr bool _mset_contains = USTDEX_IS_BASE_OF(_mlist<Ty>, Set);

  template <class... Set, class Ty>
  auto operator+(_mset<Set...> &, _mlist<Ty> &)
    -> _mif<_mset_contains<_mset<Set...>, Ty>, _mset<Set...>, _mset<Ty, Set...>> &;

  template <class Set, class... Ts>
  using _mset_insert_ref =
    decltype((std::declval<Set &>() + ... + std::declval<_mlist<Ts> &>()));

  template <class Set, class... Ts>
  using _mset_insert = USTDEX_REMOVE_REFERENCE(_mset_insert_ref<Set, Ts...>);

  template <class Fn>
  struct _munique {
    template <class... Ts>
    using _f = _minvoke<_mset_insert<_mset<>, Ts...>, Fn>;
  };
} // namespace ustdex
