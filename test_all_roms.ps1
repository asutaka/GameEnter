#!/usr/bin/env pwsh
# Test multiple ROMs to find which ones work

$roms = @(
    @{Name="Super Mario Bros 3"; Path="C:\Users\asutaka\Downloads\Super Mario Bros. 3 (USA) (Rev 1).nes"},
    @{Name="Contra"; Path="C:\Users\asutaka\Downloads\Contra (USA).nes"},
    @{Name="Donkey Kong (Downloads)"; Path="C:\Users\asutaka\Downloads\donkey kong.nes"},
    @{Name="Donkey Kong (tests)"; Path="tests\test_roms\donkeykong.nes"}
)

Write-Host "=== Testing Multiple NES ROMs ===" -ForegroundColor Cyan
Write-Host ""

foreach ($rom in $roms) {
    Write-Host "Testing: $($rom.Name)" -ForegroundColor Yellow
    
    if (-not (Test-Path $rom.Path)) {
        Write-Host "  ⊗ ROM not found: $($rom.Path)" -ForegroundColor Red
        Write-Host ""
        continue
    }
    
    Write-Host "  → Running diagnostic..." -ForegroundColor Gray
    
    # Run diagnostic
    $output = & "build\Release\ppu_diagnostic.exe" $rom.Path 2>&1 | Out-String
    
    # Check results
    if ($output -match "RENDERING OK") {
        Write-Host "  ✓ RENDERING WORKS!" -ForegroundColor Green
    } elseif ($output -match "NO GRAPHICS") {
        Write-Host "  ✗ No graphics rendered" -ForegroundColor Red
    } else {
        Write-Host "  ? Unknown result" -ForegroundColor Yellow
    }
    
    # Extract key info
    if ($output -match "Unique colors: (\d+)") {
        $colors = $Matches[1]
        Write-Host "  → Unique colors: $colors" -ForegroundColor Gray
    }
    
    if ($output -match "Non-background pixels: (\d+)") {
        $pixels = $Matches[1]
        Write-Host "  → Non-background pixels: $pixels" -ForegroundColor Gray
    }
    
    Write-Host ""
}

Write-Host ""
Write-Host "=== Recommendation ===" -ForegroundColor Cyan
Write-Host "Now test with debug output to see PPU register writes:" -ForegroundColor White
Write-Host ""
Write-Host '  .\build\Release\nes_app.exe "C:\Users\asutaka\Downloads\Super Mario Bros. 3 (USA) (Rev 1).nes"' -ForegroundColor Green
Write-Host ""
Write-Host "Watch console for [PPU] debug messages!" -ForegroundColor Yellow
