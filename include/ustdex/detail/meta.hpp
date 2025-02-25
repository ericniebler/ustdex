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

#ifndef USTDEX_ASYNC_DETAIL_META
#define USTDEX_ASYNC_DETAIL_META

#include "config.hpp"
#include "preprocessor.hpp"

#include <utility> // IWYU pragma: keep
#if __cpp_lib_three_way_comparison
#  include <compare>
#endif

// Include last
#include "prologue.hpp"

USTDEX_PRAGMA_PUSH()
USTDEX_PRAGMA_IGNORE_GNU("-Wgnu-string-literal-operator-template")
USTDEX_PRAGMA_IGNORE_GNU("-Wnon-template-friend")

namespace ustdex
{
template <class Ret, class... Args>
using _fn_t USTDEX_ATTR_NODEBUG_ALIAS = Ret(Args...);

template <class Ty>
Ty&& declval() noexcept;

template <class Ty>
struct _m_type
{
  using type USTDEX_ATTR_NODEBUG_ALIAS = Ty;
};

template <class...>
struct _m_list;

template <class... Ts>
using _m_list_ptr = _m_list<Ts...>*;

template <class... Ty>
struct _m_undefined;

template <class... Ty>
using _m_undefined_ptr = _m_undefined<Ty...>*;

template <std::size_t Np>
using _m_size_t USTDEX_ATTR_NODEBUG_ALIAS = std::integral_constant<std::size_t, Np>;

struct _m_size
{
  template <class... Ts>
  using call USTDEX_ATTR_NODEBUG_ALIAS = _m_size_t<sizeof...(Ts)>;
};

template <template <class...> class Fn>
struct _m_quote
{
  template <class... Ts>
  using call USTDEX_ATTR_NODEBUG_ALIAS = Fn<Ts...>;
};

template <template <class> class Fn>
struct _m_quote1
{
  template <class Ty>
  using call USTDEX_ATTR_NODEBUG_ALIAS = Fn<Ty>;
};

template <class Fn, class... Ts>
using _m_call USTDEX_ATTR_NODEBUG_ALIAS = typename Fn::template call<Ts...>;

template <class Fn, class Ty>
using _m_call1 USTDEX_ATTR_NODEBUG_ALIAS = typename Fn::template call<Ty>;

template <class Fn, class... Ts>
struct _m_bind_front
{
  template <class... Us>
  using call USTDEX_ATTR_NODEBUG_ALIAS = _m_call<Fn, Ts..., Us...>;
};

template <template <class...> class Fn, class... Ts>
struct _m_bind_front_q
{
  template <class... Us>
  using call USTDEX_ATTR_NODEBUG_ALIAS = Fn<Ts..., Us...>;
};

template <class Fn, class... Ts>
struct _m_bind_back
{
  template <class... Us>
  using call USTDEX_ATTR_NODEBUG_ALIAS = _m_call<Fn, Us..., Ts...>;
};

template <template <class...> class Fn, class... Ts>
struct _m_bind_back_q
{
  template <class... Us>
  using call USTDEX_ATTR_NODEBUG_ALIAS = Fn<Us..., Ts...>;
};

namespace detail
{
template <std::size_t DependentValue>
struct _m_call_indirect_fn
{
  template <template <class...> class _Fn, class... _Ts>
  using call USTDEX_ATTR_NODEBUG_ALIAS = _Fn<_Ts...>;
};
} // namespace detail

// Adds an indirection to a meta-callable to avoid the dreaded "pack expansion
// argument for non-pack parameter" error.
template <class _Fn>
struct _m_indirect
{
  template <class... _Ts>
  using call USTDEX_ATTR_NODEBUG_ALIAS =
    typename detail::_m_call_indirect_fn<sizeof(_m_list<_Ts...>*)>::template call<_Fn::template call, _Ts...>;
};

template <template <class...> class _Fn>
struct _m_indirect_q
{
  template <class... _Ts>
  using call USTDEX_ATTR_NODEBUG_ALIAS =
    typename detail::_m_call_indirect_fn<sizeof(_m_list<_Ts...>*)>::template call<_Fn, _Ts...>;
};

template <bool>
struct _m_if_
{
  template <class Then, class...>
  using call USTDEX_ATTR_NODEBUG_ALIAS = Then;
};

template <>
struct _m_if_<false>
{
  template <class, class Else>
  using call USTDEX_ATTR_NODEBUG_ALIAS = Else;
};

template <bool Cond, class Then = void, class... Else>
using _m_if USTDEX_ATTR_NODEBUG_ALIAS = _m_call<_m_if_<Cond>, Then, Else...>;

#if defined(__cpp_pack_indexing)

USTDEX_PRAGMA_PUSH()
USTDEX_PRAGMA_IGNORE_CLANG("-Wc++26-extensions")

template <size_t _Ip, class... _Ts>
using _m_index = _Ts...[_Ip];

USTDEX_PRAGMA_POP()

#elif USTDEX_HAS_BUILTIN(_type_pack_element)

namespace detail
{
// On some versions of gcc, _type_pack_element cannot be mangled so
// hide it behind a named template.
template <std::size_t _Ip>
struct USTDEX_TYPE_VISIBILITY_DEFAULT _m_index_fn
{
  template <class... _Ts>
  using call USTDEX_ATTR_NODEBUG_ALIAS = _type_pack_element<_Ip, _Ts...>;
};
} // namespace detail

template <std::size_t _Ip, class... _Ts>
using _m_index USTDEX_ATTR_NODEBUG_ALIAS = _m_call<detail::_m_index_fn<_Ip>, _Ts...>;

#else

template <std::size_t _Ip, class... _Ts>
using _m_index USTDEX_ATTR_NODEBUG_ALIAS = std::tuple_element_t<_Ip, std::tuple<_Ts...>>;

#endif

template <std::size_t Ip>
struct _m_at
{
  template <class... Ts>
  using call USTDEX_ATTR_NODEBUG_ALIAS = _m_index<Ip, Ts...>;
};

// The following must be left undefined
template <class...>
struct DIAGNOSTIC;

struct WHERE;

struct IN_ALGORITHM;

struct WHAT;

struct WHY;

struct WITH_FUNCTION;

struct WITH_SENDER;

struct WITH_ARGUMENTS;

struct WITH_QUERY;

struct WITH_ENVIRONMENT;

struct WITH_SIGNATURES;

template <class>
struct WITH_COMPLETION_SIGNATURE;

struct FUNCTION_IS_NOT_CALLABLE;

struct UNKNOWN;

struct SENDER_HAS_TOO_MANY_SUCCESS_COMPLETIONS;

struct ARGUMENTS_ARE_NOT_DECAY_COPYABLE;

template <class... Sigs>
struct WITH_COMPLETIONS
{};

struct _merror_base
{
  // virtual ~_merror_base() = default;

  USTDEX_HOST_DEVICE constexpr friend bool _ustdex_unhandled_error(void*) noexcept
  {
    return true;
  }
};

template <class... What>
struct USTDEX_TYPE_VISIBILITY_DEFAULT ERROR : _merror_base
{
  // The following aliases are to simplify error propagation
  // in the completion signatures meta-programming.
  template <class...>
  using call USTDEX_ATTR_NODEBUG_ALIAS         = ERROR;

  using _partitioned USTDEX_ATTR_NODEBUG_ALIAS = ERROR;

  template <template <class...> class, template <class...> class>
  using _value_types USTDEX_ATTR_NODEBUG_ALIAS = ERROR;

  template <template <class...> class>
  using _error_types USTDEX_ATTR_NODEBUG_ALIAS   = ERROR;

  using _sends_stopped USTDEX_ATTR_NODEBUG_ALIAS = ERROR;

  // The following operator overloads also simplify error propagation.
  USTDEX_HOST_DEVICE ERROR operator+();

  template <class Ty>
  USTDEX_HOST_DEVICE ERROR& operator,(Ty&);

  template <class... With>
  USTDEX_HOST_DEVICE ERROR<What..., With...>& with(ERROR<With...>&);
};

USTDEX_HOST_DEVICE constexpr bool _ustdex_unhandled_error(...) noexcept
{
  return false;
}

template <class Ty>
inline constexpr bool _m_is_error = false;

template <class... What>
inline constexpr bool _m_is_error<ERROR<What...>> = true;

template <class... What>
inline constexpr bool _m_is_error<ERROR<What...>&> = true;

// True if any of the types in Ts... are errors; false otherwise.
template <class... Ts>
inline constexpr bool _m_contains_error =
#if USTDEX_MSVC()
  (_m_is_error<Ts> || ...);
#else
  _ustdex_unhandled_error(static_cast<_m_list<Ts...>*>(nullptr));
#endif

template <class... Ts>
using _m_find_error USTDEX_ATTR_NODEBUG_ALIAS = decltype(+(declval<Ts&>(), ..., declval<ERROR<UNKNOWN>&>()));

template <template <class...> class Fn, class... Ts, class = Fn<Ts...>>
USTDEX_API auto _m_callable_q_(int) -> std::true_type;

template <template <class...> class Fn, class... Ts>
USTDEX_API auto _m_callable_q_(long) -> std::false_type;

template <template <class...> class Fn, class... Ts>
USTDEX_CONCEPT _m_callable_q = decltype(ustdex::_m_callable_q_<Fn, Ts...>(0))::value;

template <bool Error>
struct _m_self_or_error_with_
{
  template <class Ty, class... With>
  using call USTDEX_ATTR_NODEBUG_ALIAS = Ty;
};

template <>
struct _m_self_or_error_with_<true>
{
  template <class Ty, class... With>
  using call USTDEX_ATTR_NODEBUG_ALIAS = decltype(declval<Ty&>().with(declval<ERROR<With...>&>()));
};

template <class Ty, class... With>
using _m_self_or_error_with USTDEX_ATTR_NODEBUG_ALIAS = _m_call<_m_self_or_error_with_<_m_is_error<Ty>>, Ty, With...>;

template <class Ty>
struct _m_always
{
  template <class...>
  using call USTDEX_ATTR_NODEBUG_ALIAS = Ty;
};

template <bool>
struct _m_try_;

template <>
struct _m_try_<false>
{
  template <template <class...> class Fn, class... Ts>
  using _call_q USTDEX_ATTR_NODEBUG_ALIAS = Fn<Ts...>;

  template <class Fn, class... Ts>
  using call USTDEX_ATTR_NODEBUG_ALIAS = typename Fn::template call<Ts...>;
};

template <>
struct _m_try_<true>
{
  template <template <class...> class Fn, class... Ts>
  using _call_q USTDEX_ATTR_NODEBUG_ALIAS = _m_find_error<Ts...>;

  template <class Fn, class... Ts>
  using call USTDEX_ATTR_NODEBUG_ALIAS = _m_find_error<Fn, Ts...>;
};

template <class Fn, class... Ts>
using _m_try_call USTDEX_ATTR_NODEBUG_ALIAS = typename _m_try_<_m_contains_error<Fn, Ts...>>::template call<Fn, Ts...>;

template <template <class...> class Fn, class... Ts>
using _m_try_call_q USTDEX_ATTR_NODEBUG_ALIAS = typename _m_try_<_m_contains_error<Ts...>>::template _call_q<Fn, Ts...>;

// wraps a meta-callable such that if any of the arguments are errors, the
// result is an error.
template <class Fn>
struct _m_try
{
  template <class... Ts>
  using call USTDEX_ATTR_NODEBUG_ALIAS = _m_try_call<Fn, Ts...>;
};

template <template <class...> class Fn, class... Default>
struct _m_try_q;

// equivalent to _m_try<_m_quote<Fn>>
template <template <class...> class Fn>
struct _m_try_q<Fn>
{
  template <class... Ts>
  using call USTDEX_ATTR_NODEBUG_ALIAS = typename _m_try_<_m_contains_error<Ts...>>::template _call_q<Fn, Ts...>;
};

// equivalent to _m_try<_m_quote<Fn, Default>>
template <template <class...> class Fn, class Default>
struct _m_try_q<Fn, Default>
{
  template <class... Ts>
  using call USTDEX_ATTR_NODEBUG_ALIAS =
    typename _m_if<_m_callable_q<Fn, Ts...>, //
                   _m_try_q<Fn>,
                   _m_always<Default>>::template call<Ts...>;
};

template <class Return>
struct _m_quote_f
{
  template <class... Args>
  using call USTDEX_ATTR_NODEBUG_ALIAS = Return(Args...);
};

template <class First, class Second>
struct _m_pair
{
  using first                            = First;
  using second USTDEX_ATTR_NODEBUG_ALIAS = Second;
};

template <class Pair>
using _m_first USTDEX_ATTR_NODEBUG_ALIAS = typename Pair::first;

template <class Pair>
using _m_second USTDEX_ATTR_NODEBUG_ALIAS = typename Pair::second;

template <template <class...> class Second, template <class...> class First>
struct _m_compose_q
{
  template <class... Ts>
  using call USTDEX_ATTR_NODEBUG_ALIAS = Second<First<Ts...>>;
};

template <class Second, class First>
struct _m_compose : _m_compose_q<Second::template call, First::template call>
{};

struct _m_self
{
  template <class Ty>
  using call USTDEX_ATTR_NODEBUG_ALIAS = Ty;
};

struct _m_count
{
  template <class... Ts>
  using call USTDEX_ATTR_NODEBUG_ALIAS = std::integral_constant<std::size_t, sizeof...(Ts)>;
};

template <class...>
using _m_void USTDEX_ATTR_NODEBUG_ALIAS = void;

template <template <class...> class Fn, class List, class = void>
extern _m_undefined<List> _m_apply_q_;

template <template <class...> class Fn, template <class...> class List, class... Ts>
extern _fn_t<Fn<Ts...>>* _m_apply_q_<Fn, List<Ts...>, _m_void<Fn<Ts...>>>;

template <template <class...> class Fn, class Ret, class... As>
extern _fn_t<Fn<Ret, As...>>* _m_apply_q_<Fn, Ret(As...), _m_void<Fn<Ret, As...>>>;

template <template <class...> class Fn, class List>
using _m_apply_q USTDEX_ATTR_NODEBUG_ALIAS = decltype(_m_apply_q_<Fn, List>());

template <class Fn, class List>
using _m_apply USTDEX_ATTR_NODEBUG_ALIAS = decltype(_m_apply_q_<Fn::template call, List>());

template <std::size_t Count>
struct USTDEX_TYPE_VISIBILITY_DEFAULT _m_maybe_concat_fn
{
  using _next_t USTDEX_ATTR_NODEBUG_ALIAS = _m_maybe_concat_fn<(Count < 8 ? 0 : Count - 8)>;

  template <class... Ts,
            template <class...> class A = _m_list,
            class... As,
            template <class...> class B = _m_list,
            class... Bs,
            template <class...> class C = _m_list,
            class... Cs,
            template <class...> class D = _m_list,
            class... Ds,
            template <class...> class E = _m_list,
            class... Es,
            template <class...> class F = _m_list,
            class... Fs,
            template <class...> class G = _m_list,
            class... Gs,
            template <class...> class H = _m_list,
            class... Hs,
            class... Tail>
  USTDEX_API static auto call(
    _m_list_ptr<Ts...>,         // state
    _m_undefined_ptr<A<As...>>, // 1
    _m_undefined_ptr<B<Bs...>>, // 2
    _m_undefined_ptr<C<Cs...>>, // 3
    _m_undefined_ptr<D<Ds...>>, // 4
    _m_undefined_ptr<E<Es...>>, // 5
    _m_undefined_ptr<F<Fs...>>, // 6
    _m_undefined_ptr<G<Gs...>>, // 7
    _m_undefined_ptr<H<Hs...>>, // 8
    Tail*... _tail)             // rest
    -> decltype(_next_t::call(
      _m_list_ptr<Ts..., As..., Bs..., Cs..., Ds..., Es..., Fs..., Gs..., Hs...>{nullptr},
      _tail...,
      _m_undefined_ptr<_m_list<>>{nullptr},
      _m_undefined_ptr<_m_list<>>{nullptr},
      _m_undefined_ptr<_m_list<>>{nullptr},
      _m_undefined_ptr<_m_list<>>{nullptr},
      _m_undefined_ptr<_m_list<>>{nullptr},
      _m_undefined_ptr<_m_list<>>{nullptr},
      _m_undefined_ptr<_m_list<>>{nullptr}));
};

template <>
struct USTDEX_TYPE_VISIBILITY_DEFAULT _m_maybe_concat_fn<0>
{
  template <class... Ts>
  USTDEX_API static auto call(_m_list_ptr<Ts...>, ...) -> _fn_t<_m_list<Ts...>>*;
};

struct USTDEX_TYPE_VISIBILITY_DEFAULT _m_concat
{
  template <class... Lists>
  using call USTDEX_ATTR_NODEBUG_ALIAS = decltype(_m_maybe_concat_fn<sizeof...(Lists)>::call(
    _m_list_ptr<>{nullptr},
    static_cast<_m_undefined_ptr<Lists>>(nullptr)...,
    _m_undefined_ptr<_m_list<>>{nullptr},
    _m_undefined_ptr<_m_list<>>{nullptr},
    _m_undefined_ptr<_m_list<>>{nullptr},
    _m_undefined_ptr<_m_list<>>{nullptr},
    _m_undefined_ptr<_m_list<>>{nullptr},
    _m_undefined_ptr<_m_list<>>{nullptr},
    _m_undefined_ptr<_m_list<>>{nullptr})());
};

template <class Continuation>
struct _m_concat_into
{
  template <class... Args>
  using call USTDEX_ATTR_NODEBUG_ALIAS = _m_apply<Continuation, _m_call<_m_concat, Args...>>;
};

template <template <class...> class Continuation>
struct _m_concat_into_q
{
  template <class... Args>
  using call USTDEX_ATTR_NODEBUG_ALIAS = _m_apply_q<Continuation, _m_call<_m_concat, Args...>>;
};

template <class Ty>
struct _m_self_or
{
  template <class Uy = Ty>
  using call USTDEX_ATTR_NODEBUG_ALIAS = Uy;
};

template <class Ret>
struct _m_quote_function
{
  template <class... Args>
  using call USTDEX_ATTR_NODEBUG_ALIAS = Ret(Args...);
};

template <class _Set, class... _Ts>
inline constexpr bool _m_set_contains_v = (USTDEX_IS_BASE_OF(_m_type<_Ts>, _Set) && ...);

template <class _Set, class... _Ts>
struct _m_set_contains : std::bool_constant<_m_set_contains_v<_Set, _Ts...>>
{};

namespace _set
{
template <class... _Ts>
struct _tupl;

template <>
struct _tupl<>
{
  template <class _Ty>
  using _maybe_insert USTDEX_ATTR_NODEBUG_ALIAS = _tupl<_Ty>;

  USTDEX_API static constexpr std::size_t _size() noexcept
  {
    return 0;
  }
};

template <class _Ty, class... _Ts>
struct _tupl<_Ty, _Ts...>
    : _m_type<_Ty>
    , _tupl<_Ts...>
{
  template <class _Uy>
  using _maybe_insert USTDEX_ATTR_NODEBUG_ALIAS = _m_if<_m_set_contains_v<_tupl, _Uy>, _tupl, _tupl<_Uy, _Ty, _Ts...>>;

  USTDEX_API static constexpr std::size_t _size() noexcept
  {
    return sizeof...(_Ts) + 1;
  }
};

template <bool _Empty>
struct _bulk_insert
{
  template <class _Set, class...>
  using call USTDEX_ATTR_NODEBUG_ALIAS = _Set;
};

template <>
struct _bulk_insert<false>
{
#if USTDEX_MSVC() && _MSC_VER < 1920
  template <class _Set, class _Ty, class... _Us>
  USTDEX_API static auto _insert_fn(_m_list<_Ty, _Us...>*) ->
    typename _bulk_insert<sizeof...(_Us) == 0>::template call<typename _Set::template _maybe_insert<_Ty>, _Us...>;

  template <class _Set, class... _Us>
  using call USTDEX_ATTR_NODEBUG_ALIAS = decltype(_insert_fn<_Set>(static_cast<_m_list<_Us...>*>(nullptr)));
#else
  template <class _Set, class _Ty, class... _Us>
  using call USTDEX_ATTR_NODEBUG_ALIAS =
    typename _bulk_insert<sizeof...(_Us) == 0>::template call<typename _Set::template _maybe_insert<_Ty>, _Us...>;
#endif
};
} // namespace _set

// When comparing sets for equality, use conjunction<> to short-circuit the set
// comparison if the sizes are different.
template <class _ExpectedSet, class... _Ts>
using _m_set_eq =
  std::conjunction<std::bool_constant<sizeof...(_Ts) == _ExpectedSet::_size()>, _m_set_contains<_ExpectedSet, _Ts...>>;

template <class _ExpectedSet, class... _Ts>
inline constexpr bool _m_set_eq_v = _m_set_eq<_ExpectedSet, _Ts...>::value;

template <class... _Ts>
using _m_set = _set::_tupl<_Ts...>;

template <class _Set, class... _Ts>
using _m_set_insert = typename _set::_bulk_insert<sizeof...(_Ts) == 0>::template call<_Set, _Ts...>;

template <class... _Ts>
using _m_make_set = _m_set_insert<_m_set<>, _Ts...>;

template <class _Ty, class... _Ts>
inline constexpr bool _m_contains = (USTDEX_IS_SAME(_Ty, _Ts) || ...);

} // namespace ustdex

USTDEX_PRAGMA_POP()

#include "epilogue.hpp"

#endif
