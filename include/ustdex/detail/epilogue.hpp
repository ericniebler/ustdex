//===----------------------------------------------------------------------===//
//
// Part of CUDA Experimental in CUDA C++ Core Libraries,
// under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// SPDX-FileCopyrightText: Copyright (c) 2024 NVIDIA CORPORATION & AFFILIATES.
//
//===----------------------------------------------------------------------===//
#if !defined(USTDEX_PROLOGUE_INCLUDED)
#  error epilogue.hpp included without a prior inclusion of prologue.hpp
#endif

#undef USTDEX_PROLOGUE_INCLUDED

#if !USTDEX_HAS_DEFAULT_NAMESPACE()
#  undef ustdex
#  if defined(USTDEX_POP_USTDEX_MACRO)
#    pragma pop_macro("ustdex")
#    undef USTDEX_POP_USTDEX_MACRO
#  endif
#endif
