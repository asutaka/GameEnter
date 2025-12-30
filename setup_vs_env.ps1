# Setup Visual Studio Environment in PowerShell

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
        Write-Host "Found Visual Studio at: $path" -ForegroundColor Green
        break
    }
}

if (-not $vsDevCmd) {
    Write-Host "Visual Studio Build Tools not found!" -ForegroundColor Red
    exit 1
}

Write-Host "Loading Visual Studio environment..." -ForegroundColor Yellow

# Run VsDevCmd.bat and capture environment variables
$tempFile = [System.IO.Path]::GetTempFileName()
$cmdLine = "`"$vsDevCmd`" && set > `"$tempFile`""
cmd /c $cmdLine

# Parse and set environment variables
Get-Content $tempFile | ForEach-Object {
    if ($_ -match '^([^=]+)=(.*)$') {
        $name = $matches[1]
        $value = $matches[2]
        Set-Item -Path "env:$name" -Value $value -Force
    }
}

Remove-Item $tempFile
Write-Host "Visual Studio environment loaded!" -ForegroundColor Green
