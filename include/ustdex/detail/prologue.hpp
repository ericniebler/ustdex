//===----------------------------------------------------------------------===//
//
// Part of CUDA Experimental in CUDA C++ Core Libraries,
// under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// SPDX-FileCopyrightText: Copyright (c) 2024 NVIDIA CORPORATION & AFFILIATES.
//
//===----------------------------------------------------------------------===//
#include "config.hpp"

#if defined(USTDEX_PROLOGUE_INCLUDED)
#  error multiple inclusion of prologue.hpp
#endif

#define USTDEX_PROLOGUE_INCLUDED

#if !USTDEX_HAS_DEFAULT_NAMESPACE()
#  if defined(ustdex)
#    pragma push_macro("ustdex")
#    define USTDEX_POP_USTDEX_MACRO
#    undef ustdex
#  endif
// This will be undefined in the epilogue
#  define ustdex USTDEX_NAMESPACE
#endif
