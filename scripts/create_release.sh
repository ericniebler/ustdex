#!/bin/bash
# Script used to build CMake Release files.
# Calling this script without args will not build tests.
# To build test call this script with "withTests" argument 

echo "Started building Release CMAKE files..."

# Clearing build folder if it exists
rm -r build/Release &> /dev/null

echo -n "Deciding if tests need to be built... "
if [ -n "$1" ]; then
    if [ "$1" == "withTests" ]; then
        BUILD_TESTS="ON"
        echo "YES"
    fi
else
    BUILD_TESTS="OFF"
    echo "NO"
fi

cmake --preset Release -DBUILD_TESTING=$BUILD_TESTS
RESULT=$?
if [ "$RESULT" -ne 0 ]; then
    exit $RESULT
fi

echo "Finished building Release CMAKE files!"

if [ "$(uname -s)" == "Linux" ]; then
    cpu_count=$(nproc)
elif [ "$(uname -s)" == "Darwin" ]; then
    cpu_count=$(sysctl -n hw.ncpu)
else
    exit 1
fi

echo "Bulding Release version using ${cpu_count} threads..."
cmake --build build/Release/ -j$cpu_count
RESULT=$?
echo "Finished building Release!"
exit $RESULT
