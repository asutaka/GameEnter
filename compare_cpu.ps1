# Simple CPU state comparison (ignoring PPU and exact CYC)
# Compares only: PC, A, X, Y, P, SP

param(
    [int]$MaxLines = 100
)

$output = Get-Content "cpu_trace.log"
$expected = Get-Content "tests\test_roms\nestest.log"

Write-Host "=== CPU State Comparison (PC, A, X, Y, P, SP) ===" -ForegroundColor Cyan
Write-Host ""

$firstDiff = -1
$totalDiffs = 0

$minLen = [Math]::Min($output.Length, $expected.Length)
$checkLen = [Math]::Min($minLen, $MaxLines)

for ($i = 0; $i -lt $checkLen; $i++) {
    $lineNum = $i + 1
    
    # Extract CPU registers using regex: PC  ...  A:XX X:XX Y:XX P:XX SP:XX
    if ($output[$i] -match '^([0-9A-F]{4}).*A:([0-9A-F]{2}) X:([0-9A-F]{2}) Y:([0-9A-F]{2}) P:([0-9A-F]{2}) SP:([0-9A-F]{2})') {
        $outPC = $matches[1]
        $outA = $matches[2]
        $outX = $matches[3]
        $outY = $matches[4]
        $outP = $matches[5]
        $outSP = $matches[6]
    } else {
        Write-Host "❌ Line $lineNum - Cannot parse output" -ForegroundColor Red
        continue
    }
    
    if ($expected[$i] -match '^([0-9A-F]{4}).*A:([0-9A-F]{2}) X:([0-9A-F]{2}) Y:([0-9A-F]{2}) P:([0-9A-F]{2}) SP:([0-9A-F]{2})') {
        $expPC = $matches[1]
        $expA = $matches[2]
        $expX = $matches[3]
        $expY = $matches[4]
        $expP = $matches[5]
        $expSP = $matches[6]
    } else {
        Write-Host "❌ Line $lineNum - Cannot parse expected" -ForegroundColor Red
        continue
    }
    
    # Compare
    if ($outPC -ne $expPC -or $outA -ne $expA -or $outX -ne $expX -or 
        $outY -ne $expY -or $outP -ne $expP -or $outSP -ne $expSP) {
        
        if ($firstDiff -eq -1) {
            $firstDiff = $lineNum
            Write-Host "❌ First difference at line $lineNum" -ForegroundColor Red
            Write-Host ""
            Write-Host "Expected:" -ForegroundColor Yellow
            Write-Host "  PC:$expPC A:$expA X:$expX Y:$expY P:$expP SP:$expSP" -ForegroundColor Gray
            Write-Host ""
            Write-Host "Got:" -ForegroundColor Yellow  
            Write-Host "  PC:$outPC A:$outA X:$outX Y:$outY P:$outP SP:$outSP" -ForegroundColor Gray
            Write-Host ""
            
            # Show differences
            if ($outPC -ne $expPC) { Write-Host "  ❌ PC:  $outPC != $expPC" -ForegroundColor Red }
            if ($outA -ne $expA) { Write-Host "  ❌ A:   $outA != $expA" -ForegroundColor Red }
            if ($outX -ne $expX) { Write-Host "  ❌ X:   $outX != $expX" -ForegroundColor Red }
            if ($outY -ne $expY) { Write-Host "  ❌ Y:   $outY != $expY" -ForegroundColor Red }
            if ($outP -ne $expP) { Write-Host "  ❌ P:   $outP != $expP" -ForegroundColor Red }
            if ($outSP -ne $expSP) { Write-Host "  ❌ SP:  $outSP != $expSP" -ForegroundColor Red }
            Write-Host ""
        }
        $totalDiffs++
    }
}

Write-Host "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" -ForegroundColor Gray
Write-Host ""

if ($firstDiff -eq -1) {
    Write-Host "✅ First $MaxLines lines match perfectly!" -ForegroundColor Green
    Write-Host ""
    Write-Host "CPU implementation is working correctly!" -ForegroundColor Cyan
    exit 0
} else {
    Write-Host "Summary:" -ForegroundColor Cyan
    Write-Host "  First diff: Line $firstDiff" -ForegroundColor Yellow
    Write-Host "  Total diffs: $totalDiffs / $MaxLines" -ForegroundColor Yellow  
    Write-Host ""
    Write-Host "Full log comparison:" -ForegroundColor Cyan
    Write-Host "  Output:   $($output[$firstDiff-1])" -ForegroundColor Gray
    Write-Host "  Expected: $($expected[$firstDiff-1])" -ForegroundColor Gray
    exit 1
}
