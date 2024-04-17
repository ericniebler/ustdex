#!/bin/bash
# Script used to run tests.
# Specify whether you want Release or Debug tests to be run.
# Passing no args will run Debug tests

echo "Deciding tests build type..."
if [ -n "$1" ]; then
    if [ "$1" == "Debug" ] || [ "$1" == "Release" ]; then
        BUILD_TYPE="$1"
    else
        echo "Wrong build type argument"
        exit 1
    fi
else
    BUILD_TYPE="Debug"
fi
echo "Picked build type: $BUILD_TYPE for tests"

if [ "$(uname -s)" == "Linux" ]; then
    cpu_count=$(nproc)
elif [ "$(uname -s)" == "Darwin" ]; then
    cpu_count=$(sysctl -n hw.ncpu)
else
    exit 1
fi

echo "Running tests using ${cpu_count} threads..."
ctest --output-on-failure --test-dir build/$BUILD_TYPE/tests -j$cpu_count
RESULT=$?
echo "Finished running tests..."
exit $RESULT
