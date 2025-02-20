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

#if defined(USTDEX_ASYNC_PROLOGUE_INCLUDED)
#  _error multiple inclusion of prologue.cuh
#endif

#define USTDEX_ASYNC_PROLOGUE_INCLUDED

#include "config.hpp"

USTDEX_PRAGMA_PUSH()
USTDEX_PRAGMA_IGNORE_GNU("-Wsubobject-linkage")
USTDEX_PRAGMA_IGNORE_GNU("-Wunused-value")
USTDEX_PRAGMA_IGNORE_MSVC(4848) // [[no_unique_address]] prior to C++20 as a vendor extension
USTDEX_PRAGMA_IGNORE_EDG(attribute_requires_external_linkage)

USTDEX_PRAGMA_IGNORE_GNU("-Wmissing-braces")
USTDEX_PRAGMA_IGNORE_MSVC(5246) // missing braces around initializer
