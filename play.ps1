#!/usr/bin/env pwsh
# Helper script to play Donkey Kong with instructions

Write-Host ""
Write-Host "╔═══════════════════════════════════════════════════════════╗" -ForegroundColor Cyan
Write-Host "║           🎮 NES EMULATOR - DONKEY KONG 🎮               ║" -ForegroundColor Cyan
Write-Host "╚═══════════════════════════════════════════════════════════╝" -ForegroundColor Cyan
Write-Host ""

Write-Host "📋 HƯỚNG DẪN QUAN TRỌNG:" -ForegroundColor Yellow
Write-Host "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" -ForegroundColor Yellow
Write-Host ""
Write-Host "  1️⃣  Game sẽ bắt đầu với MÀN HÌNH XÁM" -ForegroundColor White
Write-Host "      Đây là BÌNH THƯỜNG - game đang khởi tạo" -ForegroundColor Gray
Write-Host ""
Write-Host "  2️⃣  NHẤN PHÍM 'S' (START) nhiều lần" -ForegroundColor Green
Write-Host "      Để bỏ qua title screen và bắt đầu game" -ForegroundColor Gray
Write-Host ""
Write-Host "  3️⃣  Chờ 1-2 giây sau khi nhấn START" -ForegroundColor White
Write-Host "      PPU sẽ kích hoạt và hiện graphics" -ForegroundColor Gray
Write-Host ""

Write-Host "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" -ForegroundColor Yellow
Write-Host ""
Write-Host "🎮 ĐIỀU KHIỂN:" -ForegroundColor Cyan
Write-Host "   ⬆️⬇️⬅️➡️  = Mũi tên (Di chuyển)" -ForegroundColor White
Write-Host "   Z      = Nút A (Nhảy)" -ForegroundColor White
Write-Host "   X      = Nút B" -ForegroundColor White
Write-Host "   S      = START (Bắt đầu)" -ForegroundColor Green
Write-Host "   A      = SELECT" -ForegroundColor White
Write-Host "   R      = RESET (Chơi lại)" -ForegroundColor Yellow
Write-Host "   ESC    = THOÁT" -ForegroundColor Red
Write-Host ""

Write-Host "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" -ForegroundColor Yellow
Write-Host ""

# Check files exist
if (-not (Test-Path "build\Release\nes_app.exe")) {
    Write-Host "❌ ERROR: nes_app.exe not found!" -ForegroundColor Red
    Write-Host "   Build the project first: .\build.ps1" -ForegroundColor Yellow
    exit 1
}

if (-not (Test-Path "tests\test_roms\donkeykong.nes")) {
    Write-Host "❌ ERROR: donkeykong.nes not found!" -ForegroundColor Red
    Write-Host "   Place ROM file at: tests\test_roms\donkeykong.nes" -ForegroundColor Yellow
    exit 1
}

Write-Host "✅ Files ready. Starting emulator..." -ForegroundColor Green
Write-Host ""
Write-Host "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" -ForegroundColor Yellow
Write-Host ""

# Start game
& "build\Release\nes_app.exe" "tests\test_roms\donkeykong.nes"

Write-Host ""
Write-Host "Game closed. Thanks for playing! 👋" -ForegroundColor Cyan
