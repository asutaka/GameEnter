$file = "e:\Data\GameEnter\core\ppu\ppu.cpp"
$text = Get-Content $file -Raw Encoding UTF8
$text = $text -replace '\\u0026', '&'
$text = $text -replace '\\u003c', '<'
$text = $text -replace '\\u003e', '>'
Set-Content $file -Value $text -NoNewline -Encoding UTF8
Write-Host "Fixed Unicode escapes in ppu.cpp"
