<#
Helper script: check for Visual Studio C++ build tools and run vcpkg to install SDL2.

Usage:
 - Open PowerShell as Administrator (recommended for installing Build Tools).
 - Run: `.	ools\install_vs_and_run_vcpkg.ps1` or `powershell -ExecutionPolicy Bypass -File .\scripts\install_vs_and_run_vcpkg.ps1`

This script will:
 - Look for `vswhere.exe` to detect an existing Visual Studio with C++ tools.
 - If not found and `winget` is available, offer to install Visual Studio Build Tools (you must confirm the UAC prompt).
 - After a successful detection, prompt for the path to `vcpkg.exe` (defaults to `C:\dev\vcpkg\vcpkg.exe`) and run `vcpkg install sdl2:x64-windows`.
#>

function Write-Info($m){ Write-Host "[INFO] $m" -ForegroundColor Cyan }
function Write-Warn($m){ Write-Host "[WARN] $m" -ForegroundColor Yellow }
function Write-Err($m){ Write-Host "[ERROR] $m" -ForegroundColor Red }

$vswhere = "$env:ProgramFiles(x86)\Microsoft Visual Studio\Installer\vswhere.exe"
function Test-VsWithVCTools {
    if (-not (Test-Path $vswhere)) { return $false }
    try {
        $out = & $vswhere -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath 2>$null
        return -not [string]::IsNullOrWhiteSpace($out)
    } catch {
        return $false
    }
}

Write-Info "Checking for Visual Studio with C++ (MSVC) tools..."
if (Test-VsWithVCTools) {
    Write-Info "Visual Studio with C++ tools detected."
} else {
    Write-Warn "No Visual Studio with C++ toolchain detected."
    if (Get-Command winget -ErrorAction SilentlyContinue) {
        Write-Info "winget detected. I can launch the Visual Studio Build Tools installer for you (requires admin)."
        $ans = Read-Host "Install Visual Studio Build Tools now using winget? (Y/N)"
        if ($ans -match '^[Yy]') {
            Write-Info "Launching winget to install Visual Studio Build Tools (this will request elevation)."
            try {
                Start-Process -FilePath winget -ArgumentList 'install --id Microsoft.VisualStudio.2022.BuildTools -e --accept-package-agreements --accept-source-agreements' -Verb RunAs -Wait
            } catch {
                Write-Err "Failed to start winget installer. Please run it manually or install Visual Studio Build Tools via the installer at https://aka.ms/vs/17/release/vs_BuildTools.exe"
                exit 1
            }
        } else {
            Write-Info "OK — please install Visual Studio Community or Build Tools and include 'Desktop development with C++' workload, then re-run this script."
            Write-Info "Download link: https://aka.ms/vs/17/release/vs_BuildTools.exe"
            exit 1
        }
    } else {
        Write-Warn "winget is not available on this system. Please install Visual Studio Build Tools (select 'Desktop development with C++') manually:"
        Write-Host "  https://aka.ms/vs/17/release/vs_BuildTools.exe" -ForegroundColor White
        exit 1
    }

    Write-Info "Re-checking for Visual Studio with C++ tools..."
    if (Test-VsWithVCTools) {
        Write-Info "Visual Studio detected. Continuing."
    } else {
        Write-Err "Visual Studio still not detected. Make sure you selected 'Desktop development with C++' workload and re-run."
        exit 1
    }
}

# Ask for vcpkg path
$defaultVcpkg = 'C:\dev\vcpkg\vcpkg.exe'
$vcpkgPath = Read-Host "Path to vcpkg.exe (press Enter to use $defaultVcpkg)"
if ([string]::IsNullOrWhiteSpace($vcpkgPath)) { $vcpkgPath = $defaultVcpkg }
if (-not (Test-Path $vcpkgPath)) {
    Write-Err "vcpkg.exe not found at: $vcpkgPath"
    Write-Info "If you haven't installed vcpkg, follow: https://github.com/microsoft/vcpkg#quick-start"
    exit 1
}

Write-Info "Running: $vcpkgPath install sdl2:x64-windows"
& $vcpkgPath install sdl2:x64-windows
if ($LASTEXITCODE -ne 0) {
    Write-Err "vcpkg failed. Check the output above for errors."
    exit $LASTEXITCODE
}

Write-Info "vcpkg finished. You can now run the CMake commands from the README to build the project."
exit 0
