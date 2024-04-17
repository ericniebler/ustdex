#!/bin/bash
# Script used to build CMake Debug files

echo "Started building Debug CMake files..."

# Clearing build folder if it exists
rm -r build/debug &> /dev/null

echo "Installing pre-commit hook"
rm ./.git/hooks/pre-commit
cp ./scripts/git_hooks/pre-commit ./.git/hooks/pre-commit

cmake --preset Debug -DCMAKE_CXX_COMPILER=/usr/bin/clang++ -DCMAKE_C_COMPILER=/usr/bin/clang
RESULT=$?
echo "Finished building Debug CMake files!"
exit $RESULT
