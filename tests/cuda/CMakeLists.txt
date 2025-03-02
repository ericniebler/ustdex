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

# Find all test files with "test_" prefix
file(GLOB TEST_FILES test_*.cu)

# Add each test file as a test
foreach(TEST_FILE ${TEST_FILES})
  get_filename_component(TEST_NAME ${TEST_FILE} NAME_WE)
  add_executable(cuda_${TEST_NAME} ${TEST_FILE})
  target_link_libraries(cuda_${TEST_NAME} ustdex Catch2::Catch2WithMain)
  catch_discover_tests(cuda_${TEST_NAME})
endforeach()
