<#
Automates configuring, building, and running the Starboy project on Windows.

Usage (from project root):
  powershell -ExecutionPolicy Bypass -File .\scripts\build_and_run.ps1

This script will:
 - verify `cmake` is on PATH
 - check for `vcpkg` and suggest the toolchain file
 - run CMake configure and build (Release x64)
 - run the built executable if build succeeds
#>

function Write-Info($m){ Write-Host "[INFO] $m" -ForegroundColor Cyan }
function Write-Err($m){ Write-Host "[ERROR] $m" -ForegroundColor Red }

Write-Info "Checking cmake..."
$cm = Get-Command cmake -ErrorAction SilentlyContinue
if (-not $cm) {
    Write-Err "cmake not found on PATH. Install CMake (winget/choco or from https://cmake.org) and re-open PowerShell."
    exit 1
}

Write-Info "Looking for vcpkg (default C:\dev\vcpkg\vcpkg.exe)..."
$defaultVcpkg = 'C:\dev\vcpkg\vcpkg.exe'
$vcpkg = $null
if (Test-Path $defaultVcpkg) { $vcpkg = $defaultVcpkg }
else {
    $found = Get-ChildItem -Path C:\ -Filter vcpkg.exe -Recurse -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($found) { $vcpkg = $found.FullName }
}

if ($vcpkg) {
    Write-Info "vcpkg found: $vcpkg"
    $toolchain = (Split-Path $vcpkg -Parent) + '\scripts\buildsystems\vcpkg.cmake'
    if (-not (Test-Path $toolchain)) {
        Write-Warn "vcpkg toolchain file not found at expected path: $toolchain"
        $toolchain = Read-Host "Enter path to vcpkg toolchain file (or press Enter to skip)"
        if ([string]::IsNullOrWhiteSpace($toolchain)) { $toolchain = $null }
    }
} else {
    Write-Info "vcpkg not found. You can still build if SDL2 is installed and CMake can find it."
    $toolchain = $null
}

$buildDir = Join-Path -Path (Get-Location) -ChildPath 'build'
Write-Info "Configuring with CMake (build directory: $buildDir)"
$cmArgs = @('-S', '.', '-B', $buildDir, '-A', 'x64')
if ($toolchain) { $cmArgs += '-DCMAKE_TOOLCHAIN_FILE=' + $toolchain }

Write-Info "Running: cmake $($cmArgs -join ' ')"
cmake @cmArgs
if ($LASTEXITCODE -ne 0) { Write-Err "CMake configure failed."; exit $LASTEXITCODE }

Write-Info "Building (Release)..."
cmake --build $buildDir --config Release
if ($LASTEXITCODE -ne 0) { Write-Err "Build failed."; exit $LASTEXITCODE }

Write-Info "Running the built executable..."
$exe = Join-Path $buildDir 'Release\starboy.exe'
if (-not (Test-Path $exe)) {
    # try non-config Release path
    $exe = Join-Path $buildDir 'starboy.exe'
}

if (-not (Test-Path $exe)) { Write-Err "Executable not found: $exe"; exit 1 }

Write-Info "Starting: $exe"
Start-Process -FilePath $exe -NoNewWindow
