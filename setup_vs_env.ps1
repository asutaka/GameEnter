# Setup Visual Studio Environment in PowerShell
# This script finds and loads Visual Studio Build Tools environment

Write-Host "=== Setting up Visual Studio Environment ===" -ForegroundColor Cyan
Write-Host ""

# Common VS installation paths
$vsPaths = @(
    "C:\Program Files\Microsoft Visual Studio\2022\BuildTools",
    "C:\Program Files\Microsoft Visual Studio\2022\Community",
    "C:\Program Files\Microsoft Visual Studio\2022\Professional",
    "C:\Program Files\Microsoft Visual Studio\2022\Enterprise",
    "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools",
    "C:\Program Files (x86)\Microsoft Visual Studio\2022\Community"
)

$vsDevCmd = $null

foreach ($path in $vsPaths) {
    $devCmdPath = Join-Path $path "Common7\Tools\VsDevCmd.bat"
    if (Test-Path $devCmdPath) {
        $vsDevCmd = $devCmdPath
        Write-Host "✓ Found Visual Studio at: $path" -ForegroundColor Green
        break
    }
}

if (-not $vsDevCmd) {
    Write-Host "❌ Visual Studio Build Tools not found!" -ForegroundColor Red
    Write-Host ""
    Write-Host "Please use Developer Command Prompt for VS 2022 instead:" -ForegroundColor Yellow
    Write-Host "  1. Open Start Menu" -ForegroundColor Gray
    Write-Host "  2. Search 'Developer Command Prompt for VS 2022'" -ForegroundColor Gray
    Write-Host "  3. cd /d e:\Data\GameEnter" -ForegroundColor Gray
    Write-Host "  4. .\build.ps1 debug" -ForegroundColor Gray
    exit 1
}

Write-Host "Loading Visual Studio environment..." -ForegroundColor Yellow

# Run VsDevCmd.bat and capture environment variables
$tempFile = [System.IO.Path]::GetTempFileName()
cmd /c "`"$vsDevCmd`" && set > `"$tempFile`""

# Parse and set environment variables
Get-Content $tempFile | ForEach-Object {
    if ($_ -match '^([^=]+)=(.*)$') {
        $name = $matches[1]
        $value = $matches[2]
        Set-Item -Path "env:$name" -Value $value -Force
    }
}

Remove-Item $tempFile

Write-Host "✅ Visual Studio environment loaded!" -ForegroundColor Green
Write-Host ""
Write-Host "You can now run:" -ForegroundColor Cyan
Write-Host "  .\build.ps1 debug" -ForegroundColor Gray
Write-Host "  .\build.ps1 test" -ForegroundColor Gray
Write-Host ""
