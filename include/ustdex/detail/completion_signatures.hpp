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

namespace ustdex {
  template <class... Ts>
  struct completion_signatures { };

  namespace _impl {
    template <
      class Sig,
      class WantedTag,
      template <class...>
      class Then,
      template <class, class...>
      class Else>
    extern _undefined<Sig> _gather_sig;

    template <
      class ActualTag,
      class... Ts,
      class WantedTag,
      template <class...>
      class Then,
      template <class, class...>
      class Else>
    extern Else<ActualTag, Ts...> _gather_sig<ActualTag(Ts...), WantedTag, Then, Else>;

    template <
      class... Ts,
      class WantedTag,
      template <class...>
      class Then,
      template <class, class...>
      class Else>
    extern Then<Ts...> _gather_sig<WantedTag(Ts...), WantedTag, Then, Else>;

    template <
      class Sig,
      class WantedTag,
      template <class...>
      class Then,
      template <class, class...>
      class Else>
    using _gather_sig_t = decltype(_gather_sig<Sig, WantedTag, Then, Else>);

    template <
      class Sigs,
      class WantedTag,
      template <class...>
      class Then,
      template <class, class...>
      class Else,
      template <class...>
      class Variant,
      class... More>
    extern _undefined<Sigs> _gather_completion_signatures_v;

    template <
      class... Sigs,
      class WantedTag,
      template <class...>
      class Then,
      template <class, class...>
      class Else,
      template <class...>
      class Variant,
      class... More>
    extern Variant<_gather_sig_t<Sigs, WantedTag, Then, Else>..., More...>
      _gather_completion_signatures_v<
        completion_signatures<Sigs...>,
        WantedTag,
        Then,
        Else,
        Variant,
        More...>;

    template <
      class Sigs,
      class WantedTag,
      template <class...>
      class Then,
      template <class, class...>
      class Else,
      template <class...>
      class Variant,
      class... More>
    using _gather_completion_signatures =
      decltype(_gather_completion_signatures_v<Sigs, WantedTag, Then, Else, Variant, More...>);

    template <class Sig, template <class...> class V, template <class> class E, class S>
    extern _undefined<Sig> _transform_sig;

    template <class... Values, template <class...> class V, template <class> class E, class S>
    extern V<Values...> _transform_sig<set_value_t(Values...), V, E, S>;

    template <class Error, template <class...> class V, template <class> class E, class S>
    extern E<Error> _transform_sig<set_error_t(Error), V, E, S>;

    template <template <class...> class V, template <class> class E, class S>
    extern S _transform_sig<set_stopped_t(), V, E, S>;

    template <class Sig, template <class...> class V, template <class> class E, class S>
    using _transform_sig_t = decltype(_transform_sig<Sig, V, E, S>);

    template <
      class Sigs,
      template <class...>
      class V,
      template <class>
      class E,
      class S,
      template <class...>
      class Variant,
      class... More>
    extern _undefined<Sigs> _transform_completion_signatures_v;

    template <
      class... Sigs,
      template <class...>
      class V,
      template <class>
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
      template <class>
      class E,
      class S,
      template <class...>
      class Variant,
      class... More>
    using _transform_completion_signatures =
      decltype(_transform_completion_signatures_v<Sigs, V, E, S, Variant, More...>);

    template <class... Ts>
    using _set_value_transform_t = completion_signatures<set_value_t(Ts...)>;

    template <class Ty>
    using _set_error_transform_t = completion_signatures<set_error_t(Ty)>;

    template <class... Ts, class... Us>
    auto operator*(_mset<Ts...> &, completion_signatures<Us...> &)
      -> _mset_insert_ref<_mset<Ts...>, Us...> &;

    template <class... Ts, class... What>
    auto operator*(_mset<Ts...> &, _mexception<What...> &) -> _mexception<What...> &;

    template <class... What, class... Us>
    auto operator*(_mexception<What...> &, completion_signatures<Us...> &)
      -> _mexception<What...> &;

    template <class... Sigs>
    using _concat_completion_signatures = _minvoke<
      USTDEX_REMOVE_REFERENCE(decltype((DECLVAL(_mset<> &) * ... * DECLVAL(Sigs &)))),
      _mquote<completion_signatures>>;

    template <class Tag, class... Ts>
    using _default_completions = completion_signatures<Tag(Ts...)>;
  } // namespace _impl

  template <
    class Sigs,
    class MoreSigs = completion_signatures<>,
    template <class...> class ValueTransform = _impl::_set_value_transform_t,
    template <class> class ErrorTransform = _impl::_set_error_transform_t,
    class StoppedSigs = completion_signatures<set_stopped_t()>>
  using transform_completion_signatures = _impl::_transform_completion_signatures<
    Sigs,
    ValueTransform,
    ErrorTransform,
    StoppedSigs,
    _impl::_concat_completion_signatures,
    MoreSigs>;

  template <
    class Sndr,
    class Env,
    class MoreSigs = completion_signatures<>,
    template <class...> class ValueTransform = _impl::_set_value_transform_t,
    template <class> class ErrorTransform = _impl::_set_error_transform_t,
    class StoppedSigs = completion_signatures<set_stopped_t()>>
  using transform_completion_signatures_of = transform_completion_signatures<
    completion_signatures_of_t<Sndr, Env>,
    MoreSigs,
    ValueTransform,
    ErrorTransform,
    StoppedSigs>;
} // namespace ustdex
