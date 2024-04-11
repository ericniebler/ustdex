# Copyright (c) 2024 NVIDIA Corporation
#
# Licensed under the Apache License Version 2.0 with LLVM Exceptions
# (the "License"); you may not use this file except in compliance with
# the License. You may obtain a copy of the License at
#
#   https://llvm.org/LICENSE.txt
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

cmake_minimum_required(VERSION 3.29)
project(ustdex CXX)

option(USTDEX_ENABLE_CUDA "Enable ustdex CUDA support" OFF)

if (NOT "${CMAKE_CUDA_COMPILER}" STREQUAL "")
  message(STATUS "ustdex: CUDA compiler detected")
  set(USTDEX_ENABLE_CUDA ON)
endif()

if (USTDEX_ENABLE_CUDA)
  message(STATUS "ustdex: CUDA support enabled")
  enable_language(CUDA)
  set(CMAKE_CUDA_ARCHITECTURES 86)
  set(CMAKE_CUDA_FLAGS "--expt-relaxed-constexpr --extended-lambda --keep ${CMAKE_CUDA_FLAGS}")
endif()

add_library(ustdex INTERFACE)
target_compile_features(ustdex INTERFACE cxx_std_17)
target_include_directories(ustdex INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_subdirectory(examples)
