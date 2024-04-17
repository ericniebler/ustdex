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

include(CTest)

# Add Catch2
CPMAddPackage("gh:catchorg/Catch2@3.5.4")

# Discover the Catch2 test built by the application
include(${Catch2_SOURCE_DIR}/extras/Catch.cmake)

add_executable(just_test test_just.cpp)
target_link_libraries(just_test ustdex Catch2::Catch2WithMain)

add_test(just_test just_test)
