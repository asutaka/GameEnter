# Log Comparison Script
# Compares cpu_trace.log with nestest.log and shows first difference

param(
    [string]$OutputLog = "cpu_trace.log",
    [string]$ExpectedLog = "tests\test_roms\nestest.log",
    [int]$MaxLines = 100
)

Write-Host "=== NES CPU Test Log Comparison ===" -ForegroundColor Cyan
Write-Host ""

# Check if files exist
if (-not (Test-Path $OutputLog)) {
    Write-Host "❌ Output log not found: $OutputLog" -ForegroundColor Red
    Write-Host "   Run: .\build.ps1 test" -ForegroundColor Yellow
    exit 1
}

if (-not (Test-Path $ExpectedLog)) {
    Write-Host "❌ Expected log not found: $ExpectedLog" -ForegroundColor Red
    exit 1
}

Write-Host "📄 Output:   $OutputLog" -ForegroundColor Gray
Write-Host "📄 Expected: $ExpectedLog" -ForegroundColor Gray
Write-Host ""

# Read files
$output = Get-Content $OutputLog
$expected = Get-Content $ExpectedLog

# Compare
$lineNum = 0
$firstDiff = -1
$totalDiffs = 0

$minLines = [Math]::Min($output.Length, $expected.Length)
$linesToCheck = [Math]::Min($minLines, $MaxLines)

Write-Host "Comparing first $linesToCheck lines..." -ForegroundColor Yellow
Write-Host ""

for ($i = 0; $i -lt $linesToCheck; $i++) {
    $lineNum = $i + 1
    
    if ($output[$i] -ne $expected[$i]) {
        if ($firstDiff -eq -1) {
            $firstDiff = $lineNum
            Write-Host "❌ First difference at line $lineNum" -ForegroundColor Red
            Write-Host ""
            Write-Host "Expected:" -ForegroundColor Yellow
            Write-Host "  $($expected[$i])" -ForegroundColor Gray
            Write-Host ""
            Write-Host "Got:" -ForegroundColor Yellow
            Write-Host "  $($output[$i])" -ForegroundColor Gray
            Write-Host ""
        }
        $totalDiffs++
    }
}

Write-Host "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" -ForegroundColor Gray
Write-Host ""

if ($firstDiff -eq -1) {
    Write-Host "✅ First $linesToCheck lines match!" -ForegroundColor Green
    Write-Host ""
    Write-Host "CPU implementation is working correctly!" -ForegroundColor Cyan
    
    if ($output.Length -gt $linesToCheck) {
        Write-Host ""
        Write-Host "ℹ️  Total output lines: $($output.Length)" -ForegroundColor Gray
        Write-Host "   (only checked first $linesToCheck)" -ForegroundColor Gray
    }
} else {
    Write-Host "Summary:" -ForegroundColor Cyan
    Write-Host "  First diff: Line $firstDiff" -ForegroundColor Yellow
    Write-Host "  Total diffs: $totalDiffs / $linesToCheck" -ForegroundColor Yellow
    Write-Host ""
    Write-Host "🔍 Debug Steps:" -ForegroundColor Cyan
    Write-Host "  1. Check instruction at line $firstDiff in nestest.log" -ForegroundColor Gray
    Write-Host "  2. Find corresponding opcode implementation" -ForegroundColor Gray
    Write-Host "  3. Verify addressing mode" -ForegroundColor Gray
    Write-Host "  4. Check flag updates (P register)" -ForegroundColor Gray
    Write-Host "  5. Fix and rebuild" -ForegroundColor Gray
}

Write-Host ""

# Show context if there's a difference
if ($firstDiff -ne -1 -and $firstDiff -gt 1) {
    Write-Host "Context (lines $($firstDiff-1) to $($firstDiff+1)):" -ForegroundColor Cyan
    Write-Host ""
    
    for ($i = [Math]::Max(0, $firstDiff-2); $i -lt [Math]::Min($expected.Length, $firstDiff+1); $i++) {
        $line = $i + 1
        $marker = if ($line -eq $firstDiff) { "❌" } else { "  " }
        
        Write-Host "$marker Line $($line):" -ForegroundColor Gray
        Write-Host "   Expected: $($expected[$i])" -ForegroundColor Gray
        if ($i -lt $output.Length) {
            Write-Host "   Got:      $($output[$i])" -ForegroundColor Gray
        } else {
            Write-Host "   Got:      (missing)" -ForegroundColor Red
        }
        Write-Host ""
    }
}

# Exit code
if ($firstDiff -eq -1) {
    exit 0
} else {
    exit 1
}
