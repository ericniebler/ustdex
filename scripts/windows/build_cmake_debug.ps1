# Script used to build CMake Debug files

Write-Host "Started building Debug CMake files..."

# Clearing build folder if it exists
Remove-Item -Recurse -Force -ErrorAction SilentlyContinue "build/debug"

cmake --preset Debug -G "Visual Studio 17 2022" -A x64

Write-Host "Finished building Debug CMake files!"
