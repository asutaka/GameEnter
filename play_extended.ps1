#!/usr/bin/env pwsh
# Run emulator for extended time to let game initialize

Write-Host "=== Extended Gaming Session ===" -ForegroundColor Cyan
Write-Host "Running Donkey Kong for 10 seconds to allow full init..."
Write-Host ""
Write-Host "IMPORTANT: Press 'S' (START) repeatedly during first few seconds!" -ForegroundColor Yellow
Write-Host ""

Start-Sleep -Seconds 1

Write-Host "Starting game..." -ForegroundColor Green
.\build\Release\nes_app.exe tests\test_roms\donkeykong.nes
