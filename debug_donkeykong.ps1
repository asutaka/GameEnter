#!/usr/bin/env pwsh
# Script để debug Donkey Kong rendering

Write-Host "=== Donkey Kong Debug Tool ===" -ForegroundColor Cyan
Write-Host ""

# Kiểm tra ROM file
$romPath = "tests\test_roms\donkeykong.nes"
if (-not (Test-Path $romPath)) {
    Write-Host "ERROR: ROM file not found: $romPath" -ForegroundColor Red
    exit 1
}

$romInfo = Get-Item $romPath
Write-Host "ROM File: $romPath" -ForegroundColor Green
Write-Host "Size: $($romInfo.Length) bytes" -ForegroundColor Green
Write-Host ""

# Kiểm tra executable
$exePath = "build\Release\nes_app.exe"
if (-not (Test-Path $exePath)) {
    Write-Host "ERROR: Executable not found: $exePath" -ForegroundColor Red
    Write-Host "Try building first with: .\build.ps1" -ForegroundColor Yellow
    exit 1
}

Write-Host "Executable: $exePath" -ForegroundColor Green
Write-Host ""

# Run với debug output
Write-Host "Starting emulator..." -ForegroundColor Cyan
Write-Host "Expected: You should see graphics within 1-2 seconds" -ForegroundColor Yellow
Write-Host "If you only see gray screen + buzzing sound, PPU may not be rendering" -ForegroundColor Yellow
Write-Host ""
Write-Host "Controls:" -ForegroundColor White
Write-Host "  Arrow Keys = D-Pad" -ForegroundColor White
Write-Host "  Z = A Button" -ForegroundColor White
Write-Host "  X = B Button" -ForegroundColor White
Write-Host "  S = Start" -ForegroundColor White
Write-Host "  ESC = Quit" -ForegroundColor White
Write-Host ""

& $exePath $romPath
