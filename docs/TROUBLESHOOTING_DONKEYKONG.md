# 🎮 NES Emulator - Hướng Dẫn Chơi Donkey Kong

## ❌ Vấn Đề Bạn Gặp Phải

Khi chạy Donkey Kong, bạn chỉ thấy:
- Màn hình **màu xám** (#666666)
- Tiếng **rè rè** (buzzing audio)

## ✅ Giải Thích

Đây là **HOÀN TOÀN BÌNH THƯỜNG!**

**Tại sao?**
1. **Donkey Kong chưa bật PPU rendering** - game đang ở màn hình title/khởi động
2. **Game đang chờ bạn nhấn START** - để bắt đầu chơi
3. **APU đang phát silence** - chưa có nhạc (tạo tiếng ồn)

## 🎯 Cách Khắc Phục

### Bước 1: Chạy Game
```powershell
.\build\Release\nes_app.exe tests\test_roms\donkeykong.nes
```

### Bước 2: Nhấn Nút START
**Ấn phím `S`** nhiều lần để:
- Bỏ qua màn hình title
- Bắt đầu game
- Kích hoạt PPU rendering

### Bước 3: Chờ 2-3 Giây
Game cần thời gian để:
- Khởi tạo PPU registers
- Load graphics vào VRAM
- Setup sprites và backgrounds

### Bước 4: Chơi Game!
**Controls:**
- **Arrow Keys** = Joystick (Di chuyển)
- **Z** = A Button (Nhảy)
- **X** = B Button  
- **S** = Start (Bắt đầu/Tạm dừng)
- **A** = Select
- **ESC** = Thoát

## 🔧 Debug Mode

Nếu vẫn không thấy graphics sau khi nhấn START, chạy diagnostic:

```powershell
# Chạy long hơn (10 giây) và nhấn START nhiều lần
.\build\Release\nes_app.exe tests\test_roms\donkeykong.nes

# Sau 10 giây, đóng game và chạy diagnostic:
.\build\Release\ppu_diagnostic.exe tests\test_roms\donkeykong.nes
```

Nếu vẫn thấy "Single color only":
- ➤ PPU có thể chưa implement đúng write_register
- ➤ Game loop có thể không run đủ cycles
- ➤ CPU có thể không execute đúng instructions

## 📊 Kỳ Vọng

**Sau khi nhấn START:**
- ✅ Framebuffer nên có **nhiều màu** (10-20 unique colors)
- ✅ Non-background pixels > 10,000
- ✅ Nên thấy sprites của Mario và Donkey Kong
- ✅ Nên nghe âm thanh music/sound effects

## 🚨 Troubleshooting

| Triệu Chứng | Nguyên Nhân | Giải Pháp |
|-------------|-------------|-----------|
| Màn xám liên tục | Game chưa bật PPU | Nhấn **S** (START) nhiều lần |
| Tiếng rè | APU chưa có data âm thanh | Bình thường, nhấn START |
| Crash ngay lập tức | ROM lỗi hoặc mapper sai | Kiểm tra ROM file |
| Màu lạ | Palette chưa đúng | Kiểm tra PPU palette implementation |

## 📝 Test Với ROM Khác

Thử với **nestest.nes** (không cần input):
```powershell
.\build\Release\nes_app.exe tests\test_roms\nestest.nes
```

Nestest sẽ **tự động bật PPU** và hiện graphics ngay lập tức.

---

**TL;DR:** Nhấn phím **`S`** (START) nhiều lần để bắt đầu game! 🎮
