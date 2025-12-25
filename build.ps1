# Build script cho NES Emulator
# Usage: .\build.ps1 [clean|debug|release|test]

param(
    [string]$Action = "debug"
)

$ErrorActionPreference = "Stop"

Write-Host "=== NES Emulator Build Script ===" -ForegroundColor Cyan
Write-Host ""

# Functions
function Clean-Build {
    Write-Host "🧹 Cleaning build directory..." -ForegroundColor Yellow
    if (Test-Path "build") {
        Remove-Item -Recurse -Force "build"
        Write-Host "✅ Build directory cleaned" -ForegroundColor Green
    } else {
        Write-Host "ℹ️  No build directory to clean" -ForegroundColor Gray
    }
}

function Configure-CMake {
    Write-Host "⚙️  Configuring CMake..." -ForegroundColor Yellow
    
    # Auto-detect generator
    $generator = $null
    
    # Check for MinGW
    if (Get-Command "g++" -ErrorAction SilentlyContinue) {
        $generator = "MinGW Makefiles"
        Write-Host "✓ Found MinGW compiler" -ForegroundColor Green
    }
    # Check for Visual Studio
    elseif (Get-Command "cl" -ErrorAction SilentlyContinue) {
        $generator = "Visual Studio 17 2022"
        Write-Host "✓ Found Visual Studio compiler" -ForegroundColor Green
    }
    else {
        Write-Host "❌ No C++ compiler found!" -ForegroundColor Red
        Write-Host ""
        Write-Host "Please install a C++ compiler:" -ForegroundColor Yellow
        Write-Host "  - MinGW: See SETUP_COMPILER.md" -ForegroundColor Gray
        Write-Host "  - Visual Studio Build Tools" -ForegroundColor Gray
        Write-Host ""
        exit 1
    }
    
    # Configure
    Write-Host "Generator: $generator" -ForegroundColor Cyan
    cmake -B build -S . -G "$generator"
    
    if ($LASTEXITCODE -ne 0) {
        Write-Host "❌ CMake configuration failed!" -ForegroundColor Red
        exit 1
    }
    
    Write-Host "✅ CMake configured successfully" -ForegroundColor Green
}

function Build-Project {
    param([string]$Config = "Debug")
    
    Write-Host "🔨 Building project ($Config)..." -ForegroundColor Yellow
    
    # Configure if needed
    if (-not (Test-Path "build/CMakeCache.txt")) {
        Configure-CMake
    }
    
    # Build
    cmake --build build --config $Config
    
    if ($LASTEXITCODE -ne 0) {
        Write-Host "❌ Build failed!" -ForegroundColor Red
        exit 1
    }
    
    Write-Host "✅ Build succeeded!" -ForegroundColor Green
}

function Run-Tests {
    Write-Host "🧪 Running tests..." -ForegroundColor Yellow
    
    # Check if nestest.nes exists
    $nestestPath = "tests\test_roms\nestest.nes"
    if (-not (Test-Path $nestestPath)) {
        Write-Host "⚠️  nestest.nes not found!" -ForegroundColor Yellow
        Write-Host "Downloading..." -ForegroundColor Gray
        
        New-Item -ItemType Directory -Force -Path "tests\test_roms" | Out-Null
        $url = "https://raw.githubusercontent.com/christopherpow/nes-test-roms/master/other/nestest.nes"
        curl -L -o $nestestPath $url
        
        if ($LASTEXITCODE -ne 0) {
            Write-Host "❌ Failed to download nestest.nes" -ForegroundColor Red
            exit 1
        }
        Write-Host "✅ Downloaded nestest.nes" -ForegroundColor Green
    }
    
    # Find executable
    $exe = $null
    if (Test-Path "build\desktop\Debug\NESEmulator.exe") {
        $exe = "build\desktop\Debug\NESEmulator.exe"
    } elseif (Test-Path "build\desktop\Release\NESEmulator.exe") {
        $exe = "build\desktop\Release\NESEmulator.exe"
    } elseif (Test-Path "build\desktop\NESEmulator.exe") {
        $exe = "build\desktop\NESEmulator.exe"
    }
    
    if (-not $exe) {
        Write-Host "❌ Executable not found! Build first." -ForegroundColor Red
        exit 1
    }
    
    Write-Host "Running: $exe" -ForegroundColor Cyan
    & $exe $nestestPath
    
    if ($LASTEXITCODE -ne 0) {
        Write-Host "⚠️  Test execution had errors (exit code: $LASTEXITCODE)" -ForegroundColor Yellow
    } else {
        Write-Host "✅ Test completed!" -ForegroundColor Green
    }
    
    # Check if log exists
    if (Test-Path "cpu_trace.log") {
        Write-Host ""
        Write-Host "📄 Log file created: cpu_trace.log" -ForegroundColor Cyan
        Write-Host "   Compare with: tests\test_roms\nestest.log" -ForegroundColor Gray
    }
}

# Main script
switch ($Action.ToLower()) {
    "clean" {
        Clean-Build
    }
    "debug" {
        Build-Project -Config "Debug"
    }
    "release" {
        Build-Project -Config "Release"
    }
    "test" {
        Build-Project -Config "Debug"
        Write-Host ""
        Run-Tests
    }
    default {
        Write-Host "❌ Unknown action: $Action" -ForegroundColor Red
        Write-Host ""
        Write-Host "Usage: .\build.ps1 [clean|debug|release|test]" -ForegroundColor Yellow
        Write-Host ""
        Write-Host "Actions:" -ForegroundColor Cyan
        Write-Host "  clean   - Clean build directory" -ForegroundColor Gray
        Write-Host "  debug   - Build debug version (default)" -ForegroundColor Gray
        Write-Host "  release - Build optimized version" -ForegroundColor Gray
        Write-Host "  test    - Build and run nestest" -ForegroundColor Gray
        exit 1
    }
}

Write-Host ""
Write-Host "✨ Done!" -ForegroundColor Green
