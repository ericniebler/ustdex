# Script used to build the project in Debug mode.
# build_cmake_debug.ps1 must be run before this one:
# - the first time the project is built
# - whenever a new file is added or removed

try {
    $cpu_count = (Get-WmiObject -Class Win32_ComputerSystem).NumberOfLogicalProcessors
}
catch {
    $cpu_count = 1
}

Write-Host "Building Debug version using $($cpu_count) threads..."
cmake --build build/Debug/ -j $cpu_count
Write-Host "Finished building Debug!"
