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

cmake_minimum_required(VERSION 3.26)
project(ustdex VERSION 0.0.1 LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules")

# Ensure that we link with the threading library
set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
find_package(Threads REQUIRED
  BUILD_EXPORT_SET stdexec-exports
  INSTALL_EXPORT_SET stdexec-exports
)

option(USTDEX_ENABLE_CUDA "Enable ustdex CUDA support" OFF)
if (NOT "${CMAKE_CUDA_COMPILER}" STREQUAL "")
  message(STATUS "ustdex: CUDA compiler detected : ${CMAKE_CUDA_COMPILER}")
  set(USTDEX_ENABLE_CUDA ON)
endif()

if (USTDEX_ENABLE_CUDA)
  enable_language(CUDA)
  message(STATUS "ustdex: CUDA compiler id : ${CMAKE_CUDA_COMPILER_ID}")
  if (CMAKE_CUDA_COMPILER_ID STREQUAL "NVIDIA")
    set(CMAKE_CUDA_FLAGS "--expt-relaxed-constexpr --extended-lambda --keep \
                        -DUSTDEX_CUDA=1 ${CMAKE_CUDA_FLAGS}")
  elseif("${CMAKE_CUDA_COMPILER_ID}" STREQUAL "NVHPC")
    set(CMAKE_CUDA_FLAGS "-DUSTDEX_CUDA=1 ${CMAKE_CUDA_FLAGS}")
  else()
    set(CMAKE_CUDA_FLAGS "-DUSTDEX_CUDA=1 ${CMAKE_CUDA_FLAGS}")
  endif()
endif()

add_library(ustdex INTERFACE)

list(APPEND ustdex_export_targets ustdex)

# Set library version
set_target_properties(ustdex PROPERTIES
                      VERSION "${USTDEX_VERSION}"
                      SOVERSION "${USTDEX_VERSION_MAJOR}")

# Declare the public include directories
include(GNUInstallDirs)
target_include_directories(ustdex INTERFACE
                           $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/>
                           $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

target_link_libraries(ustdex INTERFACE Threads::Threads)

target_compile_features(ustdex INTERFACE cxx_std_17)
target_include_directories(ustdex INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

if (USTDEX_ENABLE_CUDA)
  target_compile_features(ustdex INTERFACE cuda_std_20)
endif()

# Don't require building everything when installing
set(CMAKE_SKIP_INSTALL_ALL_DEPENDENCY ON)

install(TARGETS ustdex
        DESTINATION ${CMAKE_INSTALL_LIBDIR}
        EXPORT ustdex-exports)

install(
  DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(
  FILES ${CMAKE_CURRENT_BINARY_DIR}/include/ustdex_version_config.hpp
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

option(USTDEX_BUILD_DOCS "Build ustdex documentation" OFF)
if (USTDEX_BUILD_DOCS)
  add_subdirectory(docs)
endif()
add_subdirectory(examples)
