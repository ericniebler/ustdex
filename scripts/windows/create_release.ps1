# Script used to build CMake Release files.
# Calling this script without args will not build tests.
# To build tests, call this script with "withTests" argument

Write-Host "Started building Release CMake files..."

# Clearing build folder if it exists
Remove-Item -Path "build/Release" -Recurse -ErrorAction SilentlyContinue

Write-Host "Deciding if tests need to be built..."
if ($args.Length -gt 0) {
    if ($args[0] -eq "withTests") {
        $BUILD_TESTS = "ON"
        Write-Host "Building with tests"
    }
} else {
    $BUILD_TESTS = "OFF"
    Write-Host "Building without tests"
}

cmake --preset Release -DBUILD_TESTING=$BUILD_TESTS

Write-Host "Finished building Release CMake files!"

try {
    $cpu_count = (Get-WmiObject -Class Win32_ComputerSystem).NumberOfLogicalProcessors
}
catch {
    $cpu_count = 1
}

Write-Host "Building Release version using $cpu_count threads..."
cmake --build "build/Release" -j $cpu_count
Write-Host "Finished building Release!"
