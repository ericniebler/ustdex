# Script used to run tests.
# Specify whether you want Release or Debug tests to be run.
# Passing no args will run Debug tests

Write-Host "Deciding tests build type..."

if ($args.Length -gt 0) {
    $buildType = $args[0]
    if ($buildType -eq "Debug" -or $buildType -eq "Release") {
        $BUILD_TYPE = $buildType
    } else {
        Write-Host "Wrong build type argument"
        exit 1
    }
} else {
    $BUILD_TYPE = "Debug"
}

Write-Host "Picked build type: $BUILD_TYPE for tests"

try {
    $cpu_count = (Get-WmiObject -Class Win32_ComputerSystem).NumberOfLogicalProcessors
}
catch {
    $cpu_count = 1
}

Write-Host "Running tests using $cpu_count threads..."
ctest --output-on-failure --test-dir "build/$BUILD_TYPE/tests" -j $cpu_count
$RESULT = $LASTEXITCODE
Write-Host "Finished running tests..."
exit $RESULT
