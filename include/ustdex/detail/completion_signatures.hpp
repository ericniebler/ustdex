/*
 * SPDX-FileCopyrightText: Copyright (c) 2025 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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

#ifndef USTDEX_ASYNC_DETAIL_COMPLETION_SIGNATURES
#define USTDEX_ASYNC_DETAIL_COMPLETION_SIGNATURES

#include "config.hpp"
#include "cpos.hpp"
#include "exception.hpp"
#include "preprocessor.hpp"
#include "tuple.hpp" // IWYU pragma: keep
#include "type_traits.hpp"

// include this last:
#include "prologue.hpp"

namespace ustdex
{
template <class ValueTuplesList = _m_list<>, class ErrorsList = _m_list<>, bool HasStopped = false>
struct _partitioned_completions;

template <class Tag>
struct _partitioned_fold_fn;

// The following overload of binary operator* is used to build up the cache of
// completion signatures. We fold over operator*, accumulating the completion
// signatures in the cache. `_m_undefined` is used here to prevent the
// instantiation of the intermediate types.
template <class Partitioned, class Tag, class... Args>
USTDEX_API auto operator*(_m_undefined<Partitioned>&, Tag (*)(Args...))
  -> _call_result_t<_partitioned_fold_fn<Tag>, Partitioned&, _m_undefined_ptr<_m_list<Args...>>>;

// This unary overload is used to extract the cache from the `_m_undefined` type.
template <class Partitioned>
USTDEX_API auto _unpack_partitioned_completions(_m_undefined<Partitioned>&) -> Partitioned;

template <class... Sigs>
using _partition_completion_signatures_t USTDEX_ATTR_NODEBUG_ALIAS = //
  decltype(ustdex::_unpack_partitioned_completions(
    (declval<_m_undefined<_partitioned_completions<>>&>() * ... * static_cast<Sigs*>(nullptr))));

struct _concat_completion_signatures_helper;

template <class... Sigs>
using _concat_completion_signatures_t =
  _call_result_t<_call_result_t<_concat_completion_signatures_helper, const Sigs&...>>;

struct _concat_completion_signatures_fn
{
  template <class... Sigs>
  USTDEX_TRIVIAL_API constexpr auto operator()(const Sigs&...) const noexcept
    -> _concat_completion_signatures_t<Sigs...>;
};

#if defined(__cpp_constexpr_exceptions) // C++26, https://wg21.link/p3068
template <class... What, class... Values>
[[noreturn, nodiscard]] constexpr completion_signatures<> invalid_completion_signature(Values... values);
#else
template <class... What, class... Values>
[[nodiscard]] USTDEX_TRIVIAL_API USTDEX_CONSTEVAL auto invalid_completion_signature(Values... values);
#endif

struct IN_COMPLETION_SIGNATURES_APPLY;
struct IN_COMPLETION_SIGNATURES_TRANSFORM_REDUCE;
struct FUNCTION_IS_NOT_CALLABLE_WITH_THESE_SIGNATURES;

template <class Fn, class Sig>
using _completion_if = _m_if<_callable<Fn, Sig*>, completion_signatures<Sig>, completion_signatures<>>;

// A typelist for completion signatures
template <class... Sigs>
struct USTDEX_TYPE_VISIBILITY_DEFAULT completion_signatures
{
  template <class Fn, class Continuation = _m_quote<ustdex::completion_signatures>>
  using _transform USTDEX_ATTR_NODEBUG_ALIAS = _m_call<Continuation, _m_apply<Fn, Sigs>...>;

  template <template <class...> class Fn, template <class...> class Continuation = ustdex::completion_signatures>
  using _transform_q USTDEX_ATTR_NODEBUG_ALIAS = Continuation<_m_apply_q<Fn, Sigs>...>;

  using _partitioned USTDEX_ATTR_NODEBUG_ALIAS = _partition_completion_signatures_t<Sigs...>;

  template <class Fn, class... More>
  using call USTDEX_ATTR_NODEBUG_ALIAS = _m_call<Fn, Sigs..., More...>;

  constexpr completion_signatures()    = default;

  template <class Tag>
  USTDEX_API constexpr auto count(Tag) const noexcept -> std::size_t;

  template <class Fn>
  USTDEX_API constexpr auto apply(Fn) const -> _call_result_t<Fn, Sigs*...>;

  template <class Fn>
  USTDEX_API constexpr auto filter(Fn) const -> _concat_completion_signatures_t<_completion_if<Fn, Sigs>...>;

  template <class Tag>
  USTDEX_API constexpr auto select(Tag) const noexcept;

  template <class Transform, class Reduce>
  USTDEX_API constexpr auto transform_reduce(Transform, Reduce) const
    -> _call_result_t<Reduce, _call_result_t<Transform, Sigs*>...>;

  template <class... OtherSigs>
  USTDEX_API constexpr auto operator+(const completion_signatures<OtherSigs...>&) const noexcept;
};

completion_signatures() -> completion_signatures<>;

template <class... Sigs>
template <class Tag>
USTDEX_API constexpr std::size_t completion_signatures<Sigs...>::count(Tag) const noexcept
{
  if constexpr (Tag() == set_value)
  {
    return _partitioned::_count_values::value;
  }
  else if constexpr (Tag() == set_error)
  {
    return _partitioned::_count_errors::value;
  }
  else
  {
    return _partitioned::_count_stopped::value;
  }
}

template <class... Sigs>
template <class Fn>
USTDEX_API constexpr auto completion_signatures<Sigs...>::apply(Fn _fn) const -> _call_result_t<Fn, Sigs*...>
{
  return _fn(static_cast<Sigs*>(nullptr)...);
}

template <class Fn, class Sig>
USTDEX_API constexpr auto _filer_one(Fn _fn, Sig* _sig) -> _completion_if<Fn, Sig>
{
  if constexpr (_callable<Fn, Sig*>)
  {
    _fn(_sig);
  }
  return {};
}

template <class... Sigs>
template <class Fn>
USTDEX_API constexpr auto completion_signatures<Sigs...>::filter(Fn _fn) const
  -> _concat_completion_signatures_t<_completion_if<Fn, Sigs>...>
{
  return concat_completion_signatures(ustdex::_filer_one(_fn, static_cast<Sigs*>(nullptr))...);
}

template <class... Sigs>
template <class Tag>
USTDEX_API constexpr auto completion_signatures<Sigs...>::select(Tag) const noexcept
{
  if constexpr (Tag() == set_value)
  {
    return _partitioned::template _value_types<_m_quote_f<set_value_t>::call, completion_signatures>();
  }
  else if constexpr (Tag() == set_error)
  {
    return _partitioned::template _error_types<completion_signatures, _m_quote_f<set_error_t>::call>();
  }
  else
  {
    return _partitioned::template _stopped_types<completion_signatures>();
  }
}

template <class... Sigs>
template <class Transform, class Reduce>
USTDEX_API constexpr auto completion_signatures<Sigs...>::transform_reduce(Transform _transform, Reduce _reduce) const
  -> _call_result_t<Reduce, _call_result_t<Transform, Sigs*>...>
{
  return _reduce(_transform(static_cast<Sigs*>(nullptr))...);
}

template <class Ty>
USTDEX_CONCEPT _valid_completion_signatures = _is_specialization_of<Ty, completion_signatures>;

template <class Derived>
struct USTDEX_TYPE_VISIBILITY_DEFAULT _compile_time_error // : ::std::exception
{
  _compile_time_error() = default;

  const char* what() const noexcept // override
  {
    return USTDEX_TYPEID(Derived*).name();
  }
};

template <class Data, class... What>
struct USTDEX_TYPE_VISIBILITY_DEFAULT
_sender_type_check_failure : _compile_time_error<_sender_type_check_failure<Data, What...>>
{
  _sender_type_check_failure() = default;

  explicit constexpr _sender_type_check_failure(Data data)
      : data_(data)
  {}

  Data data_{};
};

struct USTDEX_TYPE_VISIBILITY_DEFAULT dependent_sender_error // : ::std::exception
{
  USTDEX_TRIVIAL_API char const* what() const noexcept       // override
  {
    return what_;
  }

  char const* what_;
};

template <class Sndr>
struct USTDEX_TYPE_VISIBILITY_DEFAULT _dependent_sender_error : dependent_sender_error
{
  USTDEX_TRIVIAL_API constexpr _dependent_sender_error() noexcept
      : dependent_sender_error{"This sender needs to know its execution " //
                               "environment before it can know how it will complete."}
  {}

  USTDEX_HOST_DEVICE _dependent_sender_error operator+();

  template <class Ty>
  USTDEX_HOST_DEVICE _dependent_sender_error& operator,(Ty&);

  template <class... What>
  USTDEX_HOST_DEVICE ERROR<What...>&operator,(ERROR<What...>&);
};

#if defined(__cpp_constexpr_exceptions) // C++26, https://wg21.link/p3068
#  define USTDEX_LET_COMPLETIONS(...)                  \
    if constexpr ([[maybe_unused]] __VA_ARGS__; false) \
    {                                                  \
    }                                                  \
    else

template <class... What, class... Values>
[[noreturn, nodiscard]] USTDEX_API consteval completion_signatures<> invalid_completion_signature(Values... values)
{
  if constexpr (sizeof...(Values) == 1)
  {
    throw _sender_type_check_failure<Values..., What...>(values...);
  }
  else
  {
    throw _sender_type_check_failure<_tuple<Values...>, What...>(_tupl{values...});
  }
}

template <class... Sndr>
[[noreturn, nodiscard]] USTDEX_API consteval auto _dependent_sender() -> completion_signatures<>
{
  throw _dependent_sender_error<Sndr...>();
}
#else

#  define USTDEX_PP_EAT_AUTO_auto(ID)     ID USTDEX_PP_EAT USTDEX_PP_LPAREN
#  define USTDEX_PP_EXPAND_AUTO_auto(_ID) auto _ID

#  define USTDEX_LET_COMPLETIONS_ID(...) \
    USTDEX_PP_EXPAND(USTDEX_PP_CAT(USTDEX_PP_EAT_AUTO_, __VA_ARGS__) USTDEX_PP_RPAREN)

#  define USTDEX_LET_COMPLETIONS(...)                                                                        \
    if constexpr (USTDEX_PP_CAT(USTDEX_PP_EXPAND_AUTO_, __VA_ARGS__);                                        \
                  !::ustdex::_valid_completion_signatures<decltype(USTDEX_LET_COMPLETIONS_ID(__VA_ARGS__))>) \
    {                                                                                                        \
      return USTDEX_LET_COMPLETIONS_ID(__VA_ARGS__);                                                         \
    }                                                                                                        \
    else

template <class... What, class... Values>
[[nodiscard]] USTDEX_TRIVIAL_API USTDEX_CONSTEVAL auto invalid_completion_signature([[maybe_unused]] Values... values)
{
  return ERROR<What...>();
}

template <class... Sndr>
[[nodiscard]] USTDEX_TRIVIAL_API USTDEX_CONSTEVAL auto _dependent_sender() -> _dependent_sender_error<Sndr...>
{
  return _dependent_sender_error<Sndr...>();
}
#endif

USTDEX_PRAGMA_PUSH()
// warning C4913: user defined binary operator ',' exists but no overload could convert all operands, default built-in
// binary operator ',' used
USTDEX_PRAGMA_IGNORE_MSVC(4913)

#define USTDEX_GET_COMPLSIGS(...)     std::remove_reference_t<Sndr>::template get_completion_signatures<__VA_ARGS__>()

#define USTDEX_CHECKED_COMPLSIGS(...) (__VA_ARGS__, void(), ustdex::_checked_complsigs<decltype(__VA_ARGS__)>())

struct A_GET_COMPLETION_SIGNATURES_CUSTOMIZATION_RETURNED_A_TYPE_THAT_IS_NOT_A_COMPLETION_SIGNATURES_SPECIALIZATION
{};

template <class Completions>
USTDEX_TRIVIAL_API USTDEX_CONSTEVAL auto _checked_complsigs()
{
  USTDEX_LET_COMPLETIONS(auto(_cs) = Completions())
  {
    if constexpr (_valid_completion_signatures<Completions>)
    {
      return _cs;
    }
    else
    {
      return invalid_completion_signature<
        A_GET_COMPLETION_SIGNATURES_CUSTOMIZATION_RETURNED_A_TYPE_THAT_IS_NOT_A_COMPLETION_SIGNATURES_SPECIALIZATION,
        WITH_SIGNATURES(Completions)>();
    }
  }
}

template <class Sndr, class... Env>
inline constexpr bool _has_get_completion_signatures = false;

// clang-format off
template <class Sndr>
inline constexpr bool _has_get_completion_signatures<Sndr> =
  USTDEX_REQUIRES_EXPR((Sndr))
  (
    (USTDEX_GET_COMPLSIGS(Sndr))
  );

template <class Sndr, class Env>
inline constexpr bool _has_get_completion_signatures<Sndr, Env> =
  USTDEX_REQUIRES_EXPR((Sndr, Env))
  (
    (USTDEX_GET_COMPLSIGS(Sndr, Env))
  );
// clang-format on

struct COULD_NOT_DETERMINE_COMPLETION_SIGNATURES_FOR_THIS_SENDER
{};

template <class Sndr, class... Env>
USTDEX_TRIVIAL_API USTDEX_CONSTEVAL auto _get_completion_signatures_helper()
{
  if constexpr (_has_get_completion_signatures<Sndr, Env...>)
  {
    return USTDEX_CHECKED_COMPLSIGS(USTDEX_GET_COMPLSIGS(Sndr, Env...));
  }
  else if constexpr (_has_get_completion_signatures<Sndr>)
  {
    return USTDEX_CHECKED_COMPLSIGS(USTDEX_GET_COMPLSIGS(Sndr));
  }
  // else if constexpr (_is_awaitable<Sndr, _env_promise<Env>...>)
  // {
  //   using Result = _await_result_t<Sndr, _env_promise<Env>...>;
  //   return completion_signatures{_set_value_v<Result>, _set_error_v<>, _set_stopped_v};
  // }
  else if constexpr (sizeof...(Env) == 0)
  {
    return _dependent_sender<Sndr>();
  }
  else
  {
    return invalid_completion_signature<COULD_NOT_DETERMINE_COMPLETION_SIGNATURES_FOR_THIS_SENDER,
                                        WITH_SENDER(Sndr),
                                        WITH_ENVIRONMENT(Env...)>();
  }
}

template <class Sndr, class... Env>
USTDEX_TRIVIAL_API USTDEX_CONSTEVAL auto get_completion_signatures()
{
  static_assert(sizeof...(Env) <= 1, "At most one environment is allowed.");
  if constexpr (sizeof...(Env) == 0)
  {
    return ustdex::_get_completion_signatures_helper<Sndr>();
  }
  else
  {
    // BUGBUG TODO:
    // Apply a lazy sender transform if one exists before computing the completion signatures:
    // using NewSndr = _transform_sender_result_t<_late_domain_of_t<Sndr, Env>, Sndr, Env>;
    using NewSndr = Sndr;
    return ustdex::_get_completion_signatures_helper<NewSndr, Env...>();
  }
}

// BUGBUG TODO
template <class Env>
using FWD_ENV_T = Env;

template <class Parent, class Child, class... Env>
constexpr auto get_child_completion_signatures()
{
  return ustdex::get_completion_signatures<_copy_cvref_t<Parent, Child>, FWD_ENV_T<Env>...>();
}

#undef USTDEX_GET_COMPLSIGS
#undef USTDEX_CHECKED_COMPLSIGS

USTDEX_PRAGMA_POP()

template <class Completions>
using _partitioned_completions_of  = typename Completions::_partitioned;

constexpr int _invalid_disposition = -1;

// A metafunction to determine whether a type is a completion signature, and if
// so, what its disposition is.
template <class>
inline constexpr int _signature_disposition = _invalid_disposition;

template <class... Values>
inline constexpr _disposition_t _signature_disposition<set_value_t(Values...)> = _disposition_t::_value;

template <class Error>
inline constexpr _disposition_t _signature_disposition<set_error_t(Error)> = _disposition_t::_error;

template <>
inline constexpr _disposition_t _signature_disposition<set_stopped_t()> = _disposition_t::_stopped;

template <class WantedTag>
struct _gather_sigs_fn;

template <>
struct _gather_sigs_fn<set_value_t>
{
  template <class Sigs, template <class...> class Tuple, template <class...> class Variant>
  using call = typename _partitioned_completions_of<Sigs>::template _value_types<Tuple, Variant>;
};

template <>
struct _gather_sigs_fn<set_error_t>
{
  template <class Sigs, template <class...> class Tuple, template <class...> class Variant>
  using call = typename _partitioned_completions_of<Sigs>::template _error_types<Variant, Tuple>;
};

template <>
struct _gather_sigs_fn<set_stopped_t>
{
  template <class Sigs, template <class...> class Tuple, template <class...> class Variant>
  using call = typename _partitioned_completions_of<Sigs>::template _stopped_types<Variant, Tuple<>>;
};

template <class Sigs, class WantedTag, template <class...> class TaggedTuple, template <class...> class Variant>
using _gather_completion_signatures = typename _gather_sigs_fn<WantedTag>::template call<Sigs, TaggedTuple, Variant>;

// _partitioned_completions is a cache of completion signatures for fast
// access. The completion_signatures<Sigs...>::_partitioned nested struct
// inherits from _partitioned_completions. If the cache is never accessed,
// it is never instantiated.
template <class... ValueTuples, class... Errors, bool HasStopped>
struct _partitioned_completions<_m_list<ValueTuples...>, _m_list<Errors...>, HasStopped>
{
  template <template <class...> class Tuple, template <class...> class Variant>
  using _value_types = Variant<_m_apply_q<Tuple, ValueTuples>...>;

  template <template <class...> class Variant, template <class> class Transform = _identity_t>
  using _error_types = Variant<Transform<Errors>...>;

  template <template <class...> class Variant, class Type = set_stopped_t()>
  using _stopped_types = _m_apply_q<Variant, _m_if<HasStopped, _m_list<Type>, _m_list<>>>;

  using _count_values  = std::integral_constant<std::size_t, sizeof...(ValueTuples)>;
  using _count_errors  = std::integral_constant<std::size_t, sizeof...(Errors)>;
  using _count_stopped = std::integral_constant<std::size_t, HasStopped>;

  struct _nothrow_decay_copyable
  {
    // These aliases are placed in a separate struct to avoid computing them
    // if they are not needed.
    using _fn     = _m_quote<_nothrow_decay_copyable_t>;
    using _values = std::conjunction<_m_apply_q<_nothrow_decay_copyable_t, ValueTuples>...>;
    using _errors = _nothrow_decay_copyable_t<Errors...>;
    using _all    = std::conjunction<_values, _errors>;
  };
};

template <>
struct _partitioned_fold_fn<set_value_t>
{
  template <class... ValueTuples, class Errors, bool HasStopped, class Values>
  USTDEX_API auto operator()(_partitioned_completions<_m_list<ValueTuples...>, Errors, HasStopped>&,
                             _m_undefined_ptr<Values>) const
    -> _m_undefined<_partitioned_completions<_m_list<ValueTuples..., Values>, Errors, HasStopped>>&;
};

template <>
struct _partitioned_fold_fn<set_error_t>
{
  template <class Values, class... Errors, bool HasStopped, class Error>
  USTDEX_API auto operator()(_partitioned_completions<Values, _m_list<Errors...>, HasStopped>&,
                             _m_undefined_ptr<_m_list<Error>>) const
    -> _m_undefined<_partitioned_completions<Values, _m_list<Errors..., Error>, HasStopped>>&;
};

template <>
struct _partitioned_fold_fn<set_stopped_t>
{
  template <class Values, class Errors, bool HasStopped>
  USTDEX_API auto operator()(_partitioned_completions<Values, Errors, HasStopped>&, _ignore) const
    -> _m_undefined<_partitioned_completions<Values, Errors, true>>&;
};

// make_completion_signatures
template <class Tag, class... As>
USTDEX_API auto _normalize_helper(As&&...) -> Tag (*)(As...);

template <class Tag, class... As>
USTDEX_API auto _normalize(Tag (*)(As...)) -> decltype(ustdex::_normalize_helper<Tag>(declval<As>()...));

template <class... Sigs>
USTDEX_API auto _make_unique(Sigs*...) -> _m_apply<_m_quote<completion_signatures>, _m_make_set<Sigs...>>;

template <class... Sigs>
using _make_completion_signatures_t =
  decltype(ustdex::_make_unique(ustdex::_normalize(static_cast<Sigs*>(nullptr))...));

template <class... ExplicitSigs, class... DeducedSigs>
USTDEX_TRIVIAL_API constexpr auto make_completion_signatures(DeducedSigs*...) noexcept
  -> _make_completion_signatures_t<ExplicitSigs..., DeducedSigs...>
{
  return {};
}

// concat_completion_signatures
extern const completion_signatures<>& _empty_completion_signatures;

struct _concat_completion_signatures_helper
{
  USTDEX_TRIVIAL_API constexpr auto operator()() const noexcept -> completion_signatures<> (*)()
  {
    return nullptr;
  }

  template <class... Sigs>
  USTDEX_TRIVIAL_API constexpr auto operator()(const completion_signatures<Sigs...>&) const noexcept
    -> _make_completion_signatures_t<Sigs...> (*)()
  {
    return nullptr;
  }

  template <class Self = _concat_completion_signatures_helper,
            class... As,
            class... Bs,
            class... Cs,
            class... Ds,
            class... Rest>
  USTDEX_TRIVIAL_API constexpr auto operator()(
    const completion_signatures<As...>&,
    const completion_signatures<Bs...>&,
    const completion_signatures<Cs...>& = _empty_completion_signatures,
    const completion_signatures<Ds...>& = _empty_completion_signatures,
    const Rest&...) const noexcept
  {
    using Tmp       = completion_signatures<As..., Bs..., Cs..., Ds...>;
    using SigsFnPtr = _call_result_t<Self, const Tmp&, const Rest&...>;
    return static_cast<SigsFnPtr>(nullptr);
  }

  template <class Ap, class Bp = _ignore, class Cp = _ignore, class Dp = _ignore, class... Rest>
  USTDEX_TRIVIAL_API constexpr auto
  operator()(const Ap&, const Bp& = {}, const Cp& = {}, const Dp& = {}, const Rest&...) const noexcept
  {
    if constexpr (!_valid_completion_signatures<Ap>)
    {
      return static_cast<Ap (*)()>(nullptr);
    }
    else if constexpr (!_valid_completion_signatures<Bp>)
    {
      return static_cast<Bp (*)()>(nullptr);
    }
    else if constexpr (!_valid_completion_signatures<Cp>)
    {
      return static_cast<Cp (*)()>(nullptr);
    }
    else
    {
      static_assert(!_valid_completion_signatures<Dp>);
      return static_cast<Dp (*)()>(nullptr);
    }
  }
};

template <class... Sigs>
using _concat_completion_signatures_t =
  _call_result_t<_call_result_t<_concat_completion_signatures_helper, const Sigs&...>>;

template <class... Sigs>
USTDEX_TRIVIAL_API constexpr auto _concat_completion_signatures_fn::operator()(const Sigs&...) const noexcept
  -> _concat_completion_signatures_t<Sigs...>
{
  return {};
}

inline constexpr _concat_completion_signatures_fn concat_completion_signatures{};

template <class... Sigs>
template <class... OtherSigs>
USTDEX_API constexpr auto
completion_signatures<Sigs...>::operator+(const completion_signatures<OtherSigs...>& _other) const noexcept
{
  if constexpr (sizeof...(OtherSigs) == 0) // short-circuit some common cases
  {
    return *this;
  }
  else if constexpr (sizeof...(Sigs) == 0)
  {
    return _other;
  }
  else
  {
    return concat_completion_signatures(*this, _other);
  }
}

template <class Sigs, template <class...> class Tuple, template <class...> class Variant>
using _value_types = typename Sigs::_partitioned::template _value_types<Tuple, Variant>;

template <class Sndr, class Env, template <class...> class Tuple, template <class...> class Variant>
using value_types_of_t = _value_types<completion_signatures_of_t<Sndr, Env>, Tuple, _m_try_q<Variant>::template call>;

template <class Sigs, template <class...> class Variant>
using _error_types = typename Sigs::_partitioned::template _error_types<Variant, _identity_t>;

template <class Sndr, class Env, template <class...> class Variant>
using error_types_of_t = _error_types<completion_signatures_of_t<Sndr, Env>, Variant>;

template <class Sigs, template <class...> class Variant, class Type>
using _stopped_types = typename Sigs::_partitioned::template _stopped_types<Variant, Type>;

template <class Sigs>
inline constexpr bool _sends_stopped = Sigs::_partitioned::_count_stopped::value != 0;

template <class Sndr, class... Env>
inline constexpr bool sends_stopped = _sends_stopped<completion_signatures_of_t<Sndr, Env...>>;

template <class Tag>
struct _default_transform_fn
{
  template <class... Ts>
  USTDEX_TRIVIAL_API constexpr auto operator()() const noexcept -> completion_signatures<Tag(Ts...)>
  {
    return {};
  }
};

template <class Tag>
inline constexpr _default_transform_fn<Tag> _default_transform{};

struct _swallow_transform
{
  template <class... Ts>
  USTDEX_TRIVIAL_API constexpr auto operator()() const noexcept -> completion_signatures<>
  {
    return {};
  }
};

template <class Tag>
struct _decay_transform
{
  template <class... Ts>
  USTDEX_TRIVIAL_API constexpr auto operator()() const noexcept -> completion_signatures<Tag(std::decay_t<Ts>...)>
  {
    return {};
  }
};

template <class Fn, class... As>
using _meta_call_result_t = decltype(declval<Fn>().template operator()<As...>());

template <class Ay, class... As, class Fn>
USTDEX_TRIVIAL_API constexpr auto _transform_expr(const Fn& _fn) -> _meta_call_result_t<const Fn&, Ay, As...>
{
  return _fn.template operator()<Ay, As...>();
}

template <class Fn>
USTDEX_TRIVIAL_API constexpr auto _transform_expr(const Fn& _fn) -> _call_result_t<const Fn&>
{
  return _fn();
}

template <class Fn, class... As>
using _transform_expr_t = decltype(ustdex::_transform_expr<As...>(declval<const Fn&>()));

struct IN_TRANSFORM_COMPLETION_SIGNATURES;
struct A_TRANSFORM_FUNCTION_RETURNED_A_TYPE_THAT_IS_NOT_A_COMPLETION_SIGNATURES_SPECIALIZATION;
struct COULD_NOT_CALL_THE_TRANSFORM_FUNCTION_WITH_THE_GIVEN_TEMPLATE_ARGUMENTS;

// transform_completion_signatures:
template <class... As, class Fn>
USTDEX_API constexpr auto _apply_transform(const Fn& _fn)
{
  if constexpr (_m_callable_q<_transform_expr_t, Fn, As...>)
  {
    using _completions = _transform_expr_t<Fn, As...>;
    if constexpr (_valid_completion_signatures<_completions> || _m_is_error<_completions>
                  || std::is_base_of_v<dependent_sender_error, _completions>)
    {
      return ustdex::_transform_expr<As...>(_fn);
    }
    else
    {
      (void) ustdex::_transform_expr<As...>(_fn); // potentially throwing
      return invalid_completion_signature<
        IN_TRANSFORM_COMPLETION_SIGNATURES,
        A_TRANSFORM_FUNCTION_RETURNED_A_TYPE_THAT_IS_NOT_A_COMPLETION_SIGNATURES_SPECIALIZATION,
        WITH_FUNCTION(Fn),
        WITH_ARGUMENTS(As...)>();
    }
  }
  else
  {
    return invalid_completion_signature< //
      IN_TRANSFORM_COMPLETION_SIGNATURES,
      COULD_NOT_CALL_THE_TRANSFORM_FUNCTION_WITH_THE_GIVEN_TEMPLATE_ARGUMENTS,
      WITH_FUNCTION(Fn),
      WITH_ARGUMENTS(As...)>();
  }
}

template <class ValueFn, class ErrorFn, class StoppedFn>
struct _transform_one
{
  ValueFn _value_fn;
  ErrorFn _error_fn;
  StoppedFn _stopped_fn;

  template <class Tag, class... Ts>
  USTDEX_API constexpr auto operator()(Tag (*)(Ts...)) const
  {
    if constexpr (Tag() == set_value)
    {
      return _apply_transform<Ts...>(_value_fn);
    }
    else if constexpr (Tag() == set_error)
    {
      return _apply_transform<Ts...>(_error_fn);
    }
    else
    {
      return _apply_transform<Ts...>(_stopped_fn);
    }
  }
};

template <class TransformOne>
struct _transform_all_fn
{
  TransformOne _tfx1;

  template <class... Sigs>
  USTDEX_API constexpr auto operator()(Sigs*... _sigs) const
  {
    return concat_completion_signatures(_tfx1(_sigs)...);
  }
};

template <class TransformOne>
_transform_all_fn(TransformOne) -> _transform_all_fn<TransformOne>;

template <class Completions,
          class ValueFn   = decltype(_default_transform<set_value_t>),
          class ErrorFn   = decltype(_default_transform<set_error_t>),
          class StoppedFn = decltype(_default_transform<set_stopped_t>),
          class ExtraSigs = completion_signatures<>>
USTDEX_API constexpr auto transform_completion_signatures(
  Completions, //
  ValueFn _value_fn     = {},
  ErrorFn _error_fn     = {},
  StoppedFn _stopped_fn = {},
  ExtraSigs             = {})
{
  USTDEX_LET_COMPLETIONS(auto(_completions) = Completions())
  {
    USTDEX_LET_COMPLETIONS(auto(_extra) = ExtraSigs())
    {
      _transform_one<ValueFn, ErrorFn, StoppedFn> _tfx1{_value_fn, _error_fn, _stopped_fn};
      return concat_completion_signatures(_completions.apply(_transform_all_fn{_tfx1}), _extra);
    }
  }
}

#if USTDEX_HOST_ONLY()
USTDEX_API inline constexpr auto _eptr_completion() noexcept
{
  return completion_signatures<set_error_t(::std::exception_ptr)>();
}
#else
USTDEX_API inline constexpr auto _eptr_completion() noexcept
{
  return completion_signatures{};
}
#endif

template <bool PotentiallyThrowing>
USTDEX_API constexpr auto _eptr_completion_if() noexcept
{
  if constexpr (PotentiallyThrowing)
  {
    return _eptr_completion();
  }
  else
  {
    return completion_signatures{};
  }
}

#if defined(__cpp_constexpr_exceptions) // C++26, https://wg21.link/p3068
// When asked for its completions without an envitonment, a dependent sender
// will throw an exception of a type derived from `dependent_sender_error`.
template <class Sndr>
USTDEX_API constexpr bool _is_dependent_sender() noexcept
try
{
  (void) get_completion_signatures<Sndr>();
  return false; // didn't throw, not a dependent sender
}
catch (dependent_sender_error&)
{
  return true;
}
catch (...)
{
  return false; // different kind of exception was thrown; not a dependent sender
}
#else
template <class Sndr>
USTDEX_API constexpr bool _is_dependent_sender() noexcept
{
  using Completions = decltype(get_completion_signatures<Sndr>());
  return std::is_base_of_v<dependent_sender_error, Completions>;
}
#endif

} // namespace ustdex

#include "epilogue.hpp"

#endif
