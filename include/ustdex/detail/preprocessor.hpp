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

#define USTDEX_PP_CAT_I(A, ...) A##__VA_ARGS__
#define USTDEX_PP_CAT(A, ...)   USTDEX_PP_CAT_I(A, __VA_ARGS__)
#define USTDEX_PP_EAT(...)
#define USTDEX_PP_EVAL(M, ...) M(__VA_ARGS__)
#define USTDEX_PP_EXPAND(...)  __VA_ARGS__
