//===----------------------------------------------------------------------===//
//
// Part of CUDA Experimental in CUDA C++ Core Libraries,
// under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// SPDX-FileCopyrightText: Copyright (c) 2024 NVIDIA CORPORATION & AFFILIATES.
//
//===----------------------------------------------------------------------===//
#pragma once

// clang-format off
#define USTDEX_PP_STRINGIZE(...) #__VA_ARGS__
#define USTDEX_PP_CAT_I(A, ...)  A##__VA_ARGS__
#define USTDEX_PP_CAT(A, ...)    USTDEX_PP_CAT_I(A, __VA_ARGS__)
#define USTDEX_PP_EVAL(M, ...)   M(__VA_ARGS__)
#define USTDEX_PP_EXPAND(...)    __VA_ARGS__
#define USTDEX_PP_EAT(...)

#define USTDEX_PP_CHECK(...)          USTDEX_PP_EXPAND(USTDEX_PP_CHECK_(__VA_ARGS__, 0, ))
#define USTDEX_PP_CHECK_(XP, NP, ...) NP
#define USTDEX_PP_PROBE(...)          USTDEX_PP_PROBE_(__VA_ARGS__, 1)
#define USTDEX_PP_PROBE_(XP, NP, ...) XP, NP,
// clang-format on
