# Copyright (c) 2025 NVIDIA Corporation
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

# Add Catch2
CPMAddPackage("gh:catchorg/Catch2@3.5.4")
list(APPEND CMAKE_MODULE_PATH ${Catch2_SOURCE_DIR}/extras)

include(CTest)
include(Catch)

# Find all test files with "test_" prefix
file(GLOB TEST_FILES test_*.cpp)

# Add each test file as a test
foreach(TEST_FILE ${TEST_FILES})
  get_filename_component(TEST_NAME ${TEST_FILE} NAME_WE)
  add_executable(${TEST_NAME} ${TEST_FILE})
  target_link_libraries(${TEST_NAME} ustdex Catch2::Catch2WithMain)
  catch_discover_tests(${TEST_NAME})
endforeach()

if (USTDEX_ENABLE_CUDA)
  add_subdirectory(cuda)
endif()
