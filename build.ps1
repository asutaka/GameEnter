# Build script cho NES Emulator
# Usage: .\build.ps1 [clean|debug|release|test]

param(
    [string]$Action = "debug"
)

$ErrorActionPreference = "Stop"

Write-Host "=== NES Emulator Build Script ===" -ForegroundColor Cyan
Write-Host ""

# Functions

function CleanBuild
{
    Write-Host "Cleaning build directory..." -ForegroundColor Yellow
    if (Test-Path "build") {
        Remove-Item -Recurse -Force "build"
        Write-Host "Build directory cleaned" -ForegroundColor Green
    } else {
        Write-Host "No build directory to clean" -ForegroundColor Gray
    }
}

function ConfigureCMake
{
    Write-Host "Configuring CMake..." -ForegroundColor Yellow
    
    # Auto-detect generator
    $generator = $null
    
    # Check for MinGW
    if (Get-Command "g++" -ErrorAction SilentlyContinue) {
        $generator = "MinGW Makefiles"
        Write-Host "Found MinGW compiler" -ForegroundColor Green
    }
    # Check for Visual Studio
    elseif (Get-Command "cl" -ErrorAction SilentlyContinue) {
        $generator = "Visual Studio 17 2022"
        Write-Host "Found Visual Studio compiler" -ForegroundColor Green
    }
    else {
        Write-Host "No C++ compiler found!" -ForegroundColor Red
        exit 1
    }
    
    # Configure
    Write-Host "Generator: $generator" -ForegroundColor Cyan
    cmake -B build -S . -G "$generator"
    
    if ($LASTEXITCODE -ne 0) {
        Write-Host "CMake configuration failed!" -ForegroundColor Red
        exit 1
    }
    
    Write-Host "CMake configured successfully" -ForegroundColor Green
}

function BuildProject
{
    param([string]$Config = "Debug")
    
    Write-Host "Building project ($Config)..." -ForegroundColor Yellow
    
    # Configure if needed
    if (-not (Test-Path "build/CMakeCache.txt")) {
        ConfigureCMake
    }
    
    # Build
    cmake --build build --config $Config
    
    if ($LASTEXITCODE -ne 0) {
        Write-Host "Build failed!" -ForegroundColor Red
        exit 1
    }
    
    Write-Host "Build succeeded!" -ForegroundColor Green

    # Copy SDL2 DLL if needed
    $sdlDllSrc = "build\_deps\sdl2-build\$Config\SDL2d.dll"
    if (-not (Test-Path $sdlDllSrc)) {
        # Try non-debug name
        $sdlDllSrc = "build\_deps\sdl2-build\$Config\SDL2.dll"
    }
    
    if (Test-Path $sdlDllSrc) {
        $dest = "build\$Config"
        if (Test-Path $dest) {
            Copy-Item -Path $sdlDllSrc -Destination $dest -Force -ErrorAction SilentlyContinue
            Write-Host "Copied SDL2 DLL to $dest" -ForegroundColor Gray
        }
    }
}

function RunTests
{
    Write-Host "Running tests..." -ForegroundColor Yellow
    
    # Check if nestest.nes exists
    $nestestPath = "tests\test_roms\nestest.nes"
    if (-not (Test-Path $nestestPath)) {
        Write-Host "nestest.nes not found! Downloading..." -ForegroundColor Yellow
        
        if (-not (Test-Path "tests\test_roms")) {
            New-Item -ItemType Directory -Force -Path "tests\test_roms" | Out-Null
        }
        
        $url = "https://raw.githubusercontent.com/christopherpow/nes-test-roms/master/other/nestest.nes"
        Invoke-WebRequest -Uri $url -OutFile $nestestPath
        
        Write-Host "Downloaded nestest.nes" -ForegroundColor Green
    }
    
    # Find executable - HARDCODED to typical path to avoid complex logic issues
    $exe = "build\Debug\nes_app.exe"
    
    if (-not (Test-Path $exe)) {
        Write-Host "Executable not found at $exe" -ForegroundColor Red
        exit 1
    }
    
    Write-Host "Running: $exe" -ForegroundColor Cyan
    & $exe $nestestPath
    
    if ($LASTEXITCODE -ne 0) {
        Write-Host "Test execution had errors (exit code: $LASTEXITCODE)" -ForegroundColor Yellow
    } else {
        Write-Host "Test completed!" -ForegroundColor Green
    }
}

# Main script
switch ($Action.ToLower()) {
    "clean" {
        CleanBuild
    }
    "debug" {
        BuildProject -Config "Debug"
    }
    "release" {
        BuildProject -Config "Release"
    }
    "test" {
        BuildProject -Config "Debug"
        Write-Host ""
        RunTests
    }
    default {
        Write-Host "Unknown action: $Action" -ForegroundColor Red
        exit 1
    }
}

Write-Host ""
Write-Host "Done!" -ForegroundColor Green
