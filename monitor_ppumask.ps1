#!/usr/bin/env pwsh
# Monitor PPUMASK over time to see if rendering gets enabled

Write-Host "=== PPUMASK Monitor ===" -ForegroundColor Cyan
Write-Host "Testing Super Mario Bros 3..."
Write-Host ""

.\build\Release\cpu_exec_test.exe "C:\Users\asutaka\Downloads\Super Mario Bros. 3 (USA) (Rev 1).nes" 2>&1 | Tee-Object -FilePath "smb3_test.log"

Write-Host ""
Write-Host "=== Analyzing PPUMASK writes ===" -ForegroundColor Cyan

$content = Get-Content "smb3_test.log"
$ppumask_writes = $content | Select-String "PPUMASK.*BG=(\d+).*SPR=(\d+)"

Write-Host "Total PPUMASK writes: $($ppumask_writes.Count)"
Write-Host ""

# Check if any write enabled rendering
$rendering_enabled = $ppumask_writes | Where-Object { $_ -match "BG=1" -or $_ -match "SPR=1" }

if ($rendering_enabled) {
    Write-Host "✓ Rendering WAS enabled!" -ForegroundColor Green
    Write-Host "First enabling write:"
    $rendering_enabled | Select-Object -First 1
} else {
    Write-Host "✗ Rendering NEVER enabled" -ForegroundColor Red
    Write-Host "All PPUMASK values:"
    $ppumask_writes | Select-Object -Last 10
}

Write-Host ""
Write-Host "=== Result ===" -ForegroundColor Cyan
$last_line = $content | Select-String "Different pixels" | Select-Object -Last 1
Write-Host $last_line
