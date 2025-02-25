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
#pragma once

#include "config.hpp"

// clang-format off
#define USTDEX_PP_EAT(...)
#define USTDEX_PP_EMPTY()
#define USTDEX_PP_EVAL(MACRO, ...)     MACRO(__VA_ARGS__)
#define USTDEX_PP_EXPAND(...)          __VA_ARGS__
#define USTDEX_PP_DEFER(MACRO)         MACRO USTDEX_PP_EMPTY()
#define USTDEX_PP_PARENS ()
#define USTDEX_PP_LPAREN (
#define USTDEX_PP_RPAREN )

#define USTDEX_PP_CHECK(...)           USTDEX_PP_EXPAND(USTDEX_PP_DEFER(USTDEX_PP_CHECK_N)(__VA_ARGS__, 0, ))
#define USTDEX_PP_CHECK_N(XP, NP, ...) NP
#define USTDEX_PP_PROBE(XP)            XP, 1,
#define USTDEX_PP_PROBE_N(XP, NP, ...) XP, NP,

#define USTDEX_PP_IS_PAREN(Xp)         USTDEX_PP_CHECK(USTDEX_PP_IS_PAREN_PROBE Xp)
#define USTDEX_PP_IS_PAREN_PROBE(...)  USTDEX_PP_PROBE(~)

#define USTDEX_PP_CAT_I(ARG, ...)      ARG##__VA_ARGS__
#define USTDEX_PP_CAT(ARG, ...)        USTDEX_PP_CAT_I(ARG, __VA_ARGS__)

#define USTDEX_PP_CAT1_I(ARG, ...)     ARG##__VA_ARGS__
#define USTDEX_PP_CAT1(ARG, ...)       USTDEX_PP_CAT1_I(ARG, __VA_ARGS__)

#define USTDEX_PP_CAT2_I(ARG, ...)     ARG##__VA_ARGS__
#define USTDEX_PP_CAT2(ARG, ...)       USTDEX_PP_CAT2_I(ARG, __VA_ARGS__)

#define USTDEX_PP_CAT3_I(ARG, ...)     ARG##__VA_ARGS__
#define USTDEX_PP_CAT3(ARG, ...)       USTDEX_PP_CAT3_I(ARG, __VA_ARGS__)

#define USTDEX_PP_CAT4_I(ARG, ...)     ARG##__VA_ARGS__
#define USTDEX_PP_CAT4(ARG, ...)       USTDEX_PP_CAT4_I(ARG, __VA_ARGS__)

#if defined(__COUNTER__)
#  define USTDEX_PP_COUNTER() __COUNTER__
#else
#  define USTDEX_PP_COUNTER() __LINE__
#endif

#if defined(__cpp_concepts)
#  define USTDEX_TEMPLATE(...)         template <__VA_ARGS__>
#  define USTDEX_REQUIRES(...)         requires __VA_ARGS__
#  define USTDEX_AND                   &&
#  define USTDEX_CONCEPT               concept
#else
#  define USTDEX_TEMPLATE(...)         template <__VA_ARGS__
#  define USTDEX_REQUIRES(...)         , bool _ustdex_true_ = true, _ustdex_enable_if_t < __VA_ARGS__ && _ustdex_true_, int > = 0 >
#  define USTDEX_AND                   &&_ustdex_true_, int > = 0, _ustdex_enable_if_t <
#  define USTDEX_CONCEPT               inline constexpr bool
#endif
// clang-format on

////////////////////////////////////////////////////////////////////////////////
// USTDEX_PP_FOR_EACH
//   Inspired by "Recursive macros with C++20 __VA_OPT__", by David Mazières
//   https://www.scs.stanford.edu/~dm/blog/va-opt.html
#define USTDEX_PP_EXPAND_R3(...)                                                      \
  USTDEX_PP_EXPAND(USTDEX_PP_EXPAND(USTDEX_PP_EXPAND(USTDEX_PP_EXPAND(__VA_ARGS__)))) \
/**/
#define USTDEX_PP_EXPAND_R2(...)                                                                  \
  USTDEX_PP_EXPAND_R3(USTDEX_PP_EXPAND_R3(USTDEX_PP_EXPAND_R3(USTDEX_PP_EXPAND_R3(__VA_ARGS__)))) \
/**/
#define USTDEX_PP_EXPAND_R1(...)                                                                  \
  USTDEX_PP_EXPAND_R2(USTDEX_PP_EXPAND_R2(USTDEX_PP_EXPAND_R2(USTDEX_PP_EXPAND_R2(__VA_ARGS__)))) \
/**/
#define USTDEX_PP_EXPAND_R(...)                                                                   \
  USTDEX_PP_EXPAND_R1(USTDEX_PP_EXPAND_R1(USTDEX_PP_EXPAND_R1(USTDEX_PP_EXPAND_R1(__VA_ARGS__)))) \
/**/
#define USTDEX_PP_FOR_EACH_AGAIN() USTDEX_PP_FOR_EACH_HELPER
#define USTDEX_PP_FOR_EACH_HELPER(MACRO, A1, ...) \
  MACRO(A1) __VA_OPT__(USTDEX_PP_FOR_EACH_AGAIN USTDEX_PP_PARENS(MACRO, __VA_ARGS__)) /**/
#define USTDEX_PP_FOR_EACH(MACRO, ...)                                          \
  __VA_OPT__(USTDEX_PP_EXPAND_R(USTDEX_PP_FOR_EACH_HELPER(MACRO, __VA_ARGS__))) \
  /**/
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// USTDEX_REQUIRES_EXPR
// Usage:
//   template <typename T>
//   USTDEX_PP_CONCEPT equality_comparable =
//     USTDEX_REQUIRES_EXPR((T), T const& lhs, T const& rhs) (
//       lhs == rhs,
//       lhs != rhs
//     );
//
// Can only be used as the last requirement in a concept definition.
#if defined(__cpp_concepts)
#  define USTDEX_REQUIRES_EXPR(TY, ...) requires(__VA_ARGS__) USTDEX_REQUIRES_EXPR_2
#  define USTDEX_REQUIRES_EXPR_2(...)   {USTDEX_PP_FOR_EACH(USTDEX_REQUIRES_EXPR_M, __VA_ARGS__)}
#else
#  define USTDEX_REQUIRES_EXPR_TPARAM_PROBE_variadic USTDEX_PP_PROBE(~)
#  define USTDEX_REQUIRES_EXPR_TPARAM_variadic

#  define USTDEX_REQUIRES_EXPR_DEF_TPARAM(TY)         \
    , USTDEX_PP_CAT(USTDEX_REQUIRES_EXPR_DEF_TPARAM_, \
                    USTDEX_PP_EVAL(USTDEX_PP_CHECK, USTDEX_PP_CAT(USTDEX_REQUIRES_EXPR_TPARAM_PROBE_, TY)))(TY)
#  define USTDEX_REQUIRES_EXPR_DEF_TPARAM_0(TY) class TY
#  define USTDEX_REQUIRES_EXPR_DEF_TPARAM_1(TY) class... USTDEX_PP_CAT(USTDEX_REQUIRES_EXPR_TPARAM_, TY)

#  define USTDEX_REQUIRES_EXPR_EXPAND_TPARAM(TY)         \
    , USTDEX_PP_CAT(USTDEX_REQUIRES_EXPR_EXPAND_TPARAM_, \
                    USTDEX_PP_EVAL(USTDEX_PP_CHECK, USTDEX_PP_CAT(USTDEX_REQUIRES_EXPR_TPARAM_PROBE_, TY)))(TY)
#  define USTDEX_REQUIRES_EXPR_EXPAND_TPARAM_0(TY) TY
#  define USTDEX_REQUIRES_EXPR_EXPAND_TPARAM_1(TY) USTDEX_PP_CAT(USTDEX_REQUIRES_EXPR_TPARAM_, TY)...

#  define USTDEX_REQUIRES_EXPR_TPARAMS(...)        USTDEX_PP_FOR_EACH(USTDEX_REQUIRES_EXPR_DEF_TPARAM, __VA_ARGS__)

#  define USTDEX_REQUIRES_EXPR_EXPAND_TPARAMS(...) USTDEX_PP_FOR_EACH(USTDEX_REQUIRES_EXPR_EXPAND_TPARAM, __VA_ARGS__)

#  define USTDEX_REQUIRES_EXPR(TY, ...)            USTDEX_REQUIRES_EXPR_IMPL(TY, USTDEX_PP_COUNTER(), __VA_ARGS__)
#  define USTDEX_REQUIRES_EXPR_IMPL(TY, ID, ...)                                                                 \
    ustdex::_requires_expr_impl<struct USTDEX_PP_CAT(_ustdex_requires_expr_detail_, ID)                          \
                                  USTDEX_REQUIRES_EXPR_EXPAND_TPARAMS TY>::                                      \
      _ustdex_is_satisfied(static_cast<ustdex::_tag<void USTDEX_REQUIRES_EXPR_EXPAND_TPARAMS TY>*>(nullptr), 0); \
    struct USTDEX_PP_CAT(_ustdex_requires_expr_detail_, ID)                                                      \
    {                                                                                                            \
      using _ustdex_self_t = USTDEX_PP_CAT(_ustdex_requires_expr_detail_, ID);                                   \
      template <class USTDEX_REQUIRES_EXPR_TPARAMS TY>                                                           \
      USTDEX_API static auto _ustdex_well_formed(__VA_ARGS__) USTDEX_REQUIRES_EXPR_2

#  define USTDEX_REQUIRES_EXPR_2(...)                                                         \
    ->decltype(USTDEX_PP_FOR_EACH(USTDEX_REQUIRES_EXPR_M, __VA_ARGS__) void()) {}             \
    template <class... Args, class = decltype(&_ustdex_self_t::_ustdex_well_formed<Args...>)> \
    USTDEX_API static constexpr bool _ustdex_is_satisfied(ustdex::_tag<Args...>*, int)        \
    {                                                                                         \
      return true;                                                                            \
    }                                                                                         \
    USTDEX_API static constexpr bool _ustdex_is_satisfied(void*, long)                        \
    {                                                                                         \
      return false;                                                                           \
    }                                                                                         \
    }
#endif

////////////////////////////////////////////////////////////////////////////////
#define USTDEX_REQUIRES_EXPR_M0(REQ) USTDEX_REQUIRES_EXPR_SELECT_(REQ)(REQ)
#define USTDEX_REQUIRES_EXPR_M1(REQ) USTDEX_PP_EXPAND REQ
#define USTDEX_REQUIRES_EXPR_(...)   {USTDEX_PP_FOR_EACH(USTDEX_REQUIRES_EXPR_M, __VA_ARGS__)}
#define USTDEX_REQUIRES_EXPR_SELECT_(REQ)      \
  USTDEX_PP_CAT3(USTDEX_REQUIRES_EXPR_SELECT_, \
                 USTDEX_PP_EVAL(USTDEX_PP_CHECK, USTDEX_PP_CAT3(USTDEX_REQUIRES_EXPR_SELECT_PROBE_, REQ)))

#define USTDEX_REQUIRES_EXPR_SELECT_PROBE_requires     USTDEX_PP_PROBE_N(~, 1)
#define USTDEX_REQUIRES_EXPR_SELECT_PROBE_noexcept     USTDEX_PP_PROBE_N(~, 2)
#define USTDEX_REQUIRES_EXPR_SELECT_PROBE_typename     USTDEX_PP_PROBE_N(~, 3)
#define USTDEX_REQUIRES_EXPR_SELECT_PROBE_Same_as      USTDEX_PP_PROBE_N(~, 4)

#define USTDEX_REQUIRES_EXPR_SELECT_0                  USTDEX_PP_EXPAND
#define USTDEX_REQUIRES_EXPR_SELECT_1                  USTDEX_REQUIRES_EXPR_REQUIRES_OR_NOEXCEPT
#define USTDEX_REQUIRES_EXPR_SELECT_2                  USTDEX_REQUIRES_EXPR_REQUIRES_OR_NOEXCEPT
#define USTDEX_REQUIRES_EXPR_SELECT_3                  USTDEX_REQUIRES_EXPR_REQUIRES_OR_NOEXCEPT
#define USTDEX_REQUIRES_EXPR_SELECT_4                  USTDEX_REQUIRES_EXPR_SAME_AS

#define USTDEX_REQUIRES_EXPR_REQUIRES_OR_NOEXCEPT(REQ) USTDEX_PP_CAT4(USTDEX_REQUIRES_EXPR_REQUIRES_, REQ)
#define USTDEX_PP_EAT_TYPENAME_PROBE_typename          USTDEX_PP_PROBE(~)
#define USTDEX_PP_EAT_TYPENAME_SELECT_(Xp, ...)  \
  USTDEX_PP_CAT3(USTDEX_PP_EAT_TYPENAME_SELECT_, \
                 USTDEX_PP_EVAL(USTDEX_PP_CHECK, USTDEX_PP_CAT3(USTDEX_PP_EAT_TYPENAME_PROBE_, Xp)))
#define USTDEX_PP_EAT_TYPENAME_(...)         USTDEX_PP_EVAL2(USTDEX_PP_EAT_TYPENAME_SELECT_, __VA_ARGS__, )(__VA_ARGS__)
#define USTDEX_PP_EAT_TYPENAME_SELECT_0(...) __VA_ARGS__
#define USTDEX_PP_EAT_TYPENAME_SELECT_1(...) USTDEX_PP_CAT3(USTDEX_PP_EAT_TYPENAME_, __VA_ARGS__)
#define USTDEX_PP_EAT_TYPENAME_typename

#if defined(__cpp_concepts)
// This macro is applied to each requirement in a USTDEX_REQUIRES_EXPR body
#  define USTDEX_REQUIRES_EXPR_M(REQ)                               \
    USTDEX_PP_CAT2(USTDEX_REQUIRES_EXPR_M, USTDEX_PP_IS_PAREN(REQ)) \
    (REQ);

// This macro handles nested `requires` expressions
#  define USTDEX_REQUIRES_EXPR_REQUIRES_requires(...) requires __VA_ARGS__

// This macro handles nested `typename` requirements
#  define USTDEX_REQUIRES_EXPR_REQUIRES_typename(...) typename USTDEX_PP_EAT_TYPENAME_(__VA_ARGS__)

// This macro handles nested `noexcept` requirements
#  define USTDEX_REQUIRES_EXPR_REQUIRES_noexcept(...) \
    {                                                 \
      __VA_ARGS__                                     \
    } noexcept

// This macro is a special case for `{ expr } -> same_as<T>` requirements
#  define USTDEX_REQUIRES_EXPR_SAME_AS(REQ)                              \
    {USTDEX_PP_CAT4(USTDEX_PP_EAT_SAME_AS_, REQ)}->USTDEX_CONCEPT_VSTD:: \
      same_as<USTDEX_PP_EVAL(USTDEX_REQUIRES_EXPR_SAME_AS_AUX, USTDEX_PP_CAT4(USTDEX_REQUIRES_EXPR_SAME_AS_, REQ))>
#  define USTDEX_PP_EAT_SAME_AS_Same_as(...)
#  define USTDEX_REQUIRES_EXPR_SAME_AS_AUX(TYPE, ...) USTDEX_PP_EXPAND TYPE
#  define USTDEX_REQUIRES_EXPR_SAME_AS_Same_as(...)   (__VA_ARGS__),
#else

// This macro is applied to each requirement in a USTDEX_REQUIRES_EXPR body
#  define USTDEX_REQUIRES_EXPR_M(REQ) void(), USTDEX_PP_CAT2(USTDEX_REQUIRES_EXPR_M, USTDEX_PP_IS_PAREN(REQ))(REQ),

// This macro handles nested `requires` expressions
#  define USTDEX_REQUIRES_EXPR_REQUIRES_requires(...) \
    ustdex::_m_if<__VA_ARGS__, int> {}

// This macro handles nested `typename` requirements
#  define USTDEX_REQUIRES_EXPR_REQUIRES_typename(...) static_cast<ustdex::_tag<__VA_ARGS__>*>(nullptr)

// This macro handles nested `noexcept` requirements
#  if USTDEX_GCC() && (__GNUC__ < 14)
// GCC < 14 can't mangle noexcept expressions, so just check that the
// expression is well-formed.
// https://gcc.gnu.org/bugzilla/show_bug.cgi?id=70790
#    define USTDEX_REQUIRES_EXPR_REQUIRES_noexcept(...) __VA_ARGS__
#  else
#    define USTDEX_REQUIRES_EXPR_REQUIRES_noexcept(...) \
      ustdex::_m_if<noexcept(__VA_ARGS__), int> {}
#  endif

// This macro is a special case for `{ expr } -> same_as<T>` requirements
#  define USTDEX_REQUIRES_EXPR_SAME_AS(REQ) \
    ustdex::_m_if<ustdex::_is_same<USTDEX_PP_CAT4(USTDEX_REQUIRES_EXPR_SAME_AS_, REQ) USTDEX_PP_RPAREN>, int> {}
#  define USTDEX_REQUIRES_EXPR_SAME_AS_Same_as(...) __VA_ARGS__, decltype USTDEX_PP_LPAREN
#endif

namespace ustdex
{
template <class...>
struct _tag_;

template <class... Ts>
using _tag = _tag_<Ts...>*;

template <class _Tp, class... _Args>
extern _Tp _make_dependent;

template <class _Impl, class... _Args>
using _requires_expr_impl = decltype(_make_dependent<_Impl, _Args...>);
} // namespace ustdex
