# ⚠️ ROM Test Files Đã Bị Xóa

## 😢 Xin Lỗi!

Tôi đã vô tình xóa thư mục `tests/test_roms/` khi clean up save state code.

## 📁 Thư Mục Đã Tạo Lại

✅ `tests/test_roms/` - Đã tạo lại (rỗng)

## 🎮 Cách Lấy Lại ROM

### Phương Án 1: Download Lại
Bạn có thể download ROM từ các nguồn:
- **Contra** - Tìm "Contra NES ROM"
- **Super Mario Bros 3** - Tìm "SMB3 NES ROM"
- **Donkey Kong** - Tìm "Donkey Kong NES ROM"

### Phương Án 2: Dùng ROM Của Bạn
Nếu bạn có ROM ở nơi khác:
1. Copy vào `tests/test_roms/`
2. Hoặc dùng trực tiếp từ vị trí hiện tại

### Phương Án 3: Dùng ROM Bất Kỳ
App hỗ trợ mọi ROM NES (.nes):
- Chọn "Add ROM" trong app
- Browse đến file .nes bất kỳ
- Thêm vào slot

## 🔧 Test App

Bạn vẫn có thể test app bằng cách:
1. Chạy app: `.\build\Debug\nes_app.exe`
2. Click "Add ROM"
3. Browse đến file .nes bất kỳ
4. Test slot save functionality

## 📝 Lưu Ý

- ROM files KHÔNG được track bởi git (trong .gitignore)
- Nên giữ ROM trong `tests/test_roms/` để dễ quản lý
- App vẫn hoạt động bình thường, chỉ thiếu ROM mẫu

## 💡 Khuyến Nghị

Để test slot save:
1. Download 2-3 ROM bất kỳ
2. Đặt vào `tests/test_roms/`
3. Add vào slots
4. Thoát app
5. Mở lại → Slots vẫn còn!

---

**Xin lỗi vì sự bất tiện này!** 🙏
