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
#include "exception.hpp"

namespace ustdex {
  template <class... Ts>
  struct completion_signatures { };

  template <class Sig, template <class...> class V, template <class...> class E, class S>
  extern _undefined<Sig> _transform_sig;

  template <class... Values, template <class...> class V, template <class...> class E, class S>
  extern V<Values...> _transform_sig<set_value_t(Values...), V, E, S>;

  template <class Error, template <class...> class V, template <class...> class E, class S>
  extern E<Error> _transform_sig<set_error_t(Error), V, E, S>;

  template <template <class...> class V, template <class...> class E, class S>
  extern S _transform_sig<set_stopped_t(), V, E, S>;

  template <class Sig, template <class...> class V, template <class...> class E, class S>
  using _transform_sig_t = decltype(_transform_sig<Sig, V, E, S>);

  template <
    class Sigs,
    template <class...>
    class V,
    template <class...>
    class E,
    class S,
    template <class...>
    class Variant,
    class... More>
  extern DIAGNOSTIC<Sigs> _transform_completion_signatures_v;

  template <
    class... What,
    template <class...>
    class V,
    template <class...>
    class E,
    class S,
    template <class...>
    class Variant,
    class... More>
  extern ERROR<What...>
    _transform_completion_signatures_v<ERROR<What...>, V, E, S, Variant, More...>;

  template <
    class... Sigs,
    template <class...>
    class V,
    template <class...>
    class E,
    class S,
    template <class...>
    class Variant,
    class... More>
  extern Variant<_transform_sig_t<Sigs, V, E, S>..., More...>
    _transform_completion_signatures_v<
      completion_signatures<Sigs...>,
      V,
      E,
      S,
      Variant,
      More...>;

  template <
    class Sigs,
    template <class...>
    class V,
    template <class...>
    class E,
    class S,
    template <class...>
    class Variant,
    class... More>
  using _transform_completion_signatures =
    decltype(_transform_completion_signatures_v<Sigs, V, E, S, Variant, More...>);

  template <class WantedTag>
  struct _gather_sigs_fn;

  template <>
  struct _gather_sigs_fn<set_value_t> {
    template <
      class Sigs,
      template <class...>
      class Then,
      template <class...>
      class Else,
      template <class...>
      class Variant,
      class... More>
    using _f = _transform_completion_signatures<
      Sigs,
      Then,
      _mbind_front_q<Else, set_error_t>::template _f,
      Else<set_stopped_t>,
      Variant,
      More...>;
  };

  template <>
  struct _gather_sigs_fn<set_error_t> {
    template <
      class Sigs,
      template <class...>
      class Then,
      template <class...>
      class Else,
      template <class...>
      class Variant,
      class... More>
    using _f = _transform_completion_signatures<
      Sigs,
      _mbind_front_q<Else, set_value_t>::template _f,
      Then,
      Else<set_stopped_t>,
      Variant,
      More...>;
  };

  template <>
  struct _gather_sigs_fn<set_stopped_t> {
    template <
      class Sigs,
      template <class...>
      class Then,
      template <class...>
      class Else,
      template <class...>
      class Variant,
      class... More>
    using _f = _transform_completion_signatures<
      Sigs,
      _mbind_front_q<Else, set_value_t>::template _f,
      _mbind_front_q<Else, set_error_t>::template _f,
      Then<>,
      Variant,
      More...>;
  };

  template <
    class Sigs,
    class WantedTag,
    template <class...>
    class Then,
    template <class...>
    class Else,
    template <class...>
    class Variant,
    class... More>
  using _gather_completion_signatures =
    typename _gather_sigs_fn<WantedTag>::template _f<Sigs, Then, Else, Variant, More...>;

  template <class... Ts>
  using _set_value_transform_t = completion_signatures<set_value_t(Ts...)>;

  template <class Ty>
  using _set_error_transform_t = completion_signatures<set_error_t(Ty)>;

  template <class... Ts, class... Us>
  auto operator*(_mset<Ts...> &, completion_signatures<Us...> &)
    -> _mset_insert<_mset<Ts...>, Us...> &;

  template <class... Ts, class... What>
  auto operator*(_mset<Ts...> &, ERROR<What...> &) -> ERROR<What...> &;

  template <class... What, class... Us>
  auto operator*(ERROR<What...> &, completion_signatures<Us...> &) -> ERROR<What...> &;

  template <class... Sigs>
  using _concat_completion_signatures = //
    _mapply_q<
      completion_signatures,
      decltype(+(DECLVAL(_mset<> &) * ... * DECLVAL(Sigs &)))>;

  template <class Tag, class... Ts>
  using _default_completions = completion_signatures<Tag(Ts...)>;

  template <
    class Sigs,
    class MoreSigs = completion_signatures<>,
    template <class...> class ValueTransform = _set_value_transform_t,
    template <class> class ErrorTransform = _set_error_transform_t,
    class StoppedSigs = completion_signatures<set_stopped_t()>>
  using transform_completion_signatures = //
    _transform_completion_signatures<
      Sigs,
      ValueTransform,
      ErrorTransform,
      StoppedSigs,
      _concat_completion_signatures,
      MoreSigs>;

  template <
    class Sndr,
    class Rcvr,
    class MoreSigs = completion_signatures<>,
    template <class...> class ValueTransform = _set_value_transform_t,
    template <class> class ErrorTransform = _set_error_transform_t,
    class StoppedSigs = completion_signatures<set_stopped_t()>>
  using transform_completion_signatures_of = //
    transform_completion_signatures<
      completion_signatures_of_t<Sndr, Rcvr>,
      MoreSigs,
      ValueTransform,
      ErrorTransform,
      StoppedSigs>;

  template <
    class Sigs,
    template <class...>
    class Tuple,
    template <class...>
    class Variant>
  using _value_types = //
    _transform_completion_signatures<
      Sigs,
      _mcompose_q<_mlist_ref, Tuple>::template _f,
      _malways<_mlist_ref<>>::_f,
      _mlist_ref<>,
      _mtry<_mconcat_into<_mtry_quote<Variant>>>::template _f>;

  template <
    class Sndr,
    class Rcvr,
    template <class...>
    class Tuple,
    template <class...>
    class Variant>
  using value_types_of_t =
    _value_types<completion_signatures_of_t<Sndr, Rcvr>, Tuple, Variant>;

  template <
    class Sigs,
    template <class...>
    class Variant>
  using _error_types = //
    _transform_completion_signatures<
      Sigs,
      _malways<_mlist_ref<>>::_f,
      _mlist_ref,
      _mlist_ref<>,
      _mconcat_into<_mtry_quote<Variant>>::template _f>;

  template <class Sndr, class Rcvr, template <class...> class Variant>
  using error_types_of_t = _error_types<completion_signatures_of_t<Sndr, Rcvr>, Variant>;

  template <class Sigs>
  USTDEX_DEVICE_CONSTANT constexpr bool _sends_stopped = //
    _transform_completion_signatures<
      Sigs,
      _malways<_mfalse>::_f,
      _malways<_mfalse>::_f,
      _mtrue,
      _mor>::value;

  template <class Sndr, class Rcvr = receiver_archetype>
  USTDEX_DEVICE_CONSTANT constexpr bool sends_stopped = //
    _sends_stopped<completion_signatures_of_t<Sndr, Rcvr>>;

  using _eptr_completion = completion_signatures<set_error_t(std::exception_ptr)>;

  template <bool NoExcept>
  using _eptr_completion_if = _mif<NoExcept, completion_signatures<>, _eptr_completion>;

  template <class>
  inline constexpr bool _is_completion_signatures = false;

  template <class... Sigs>
  inline constexpr bool _is_completion_signatures<completion_signatures<Sigs...>> = true;

  template <class Sndr>
  using _is_non_dependent_detail_ = //
    _mif<_is_completion_signatures<completion_signatures_of_t<Sndr>>>;

  template <class Sndr>
  inline constexpr bool _is_non_dependent_sender =
    _mvalid_q<_is_non_dependent_detail_, Sndr>;

} // namespace ustdex
