# Hướng Dẫn: Hệ Thống Cover & Slots (Final)

## ✅ Tính Năng Đã Hoàn Thiện

### 1. Quản Lý Slots (SlotManager)
- **Lưu trữ:** ROM path, Tên Game, Cover path.
- **File config:** `game_slots.txt` (Lưu cùng thư mục với file chạy `.exe`).
- **Cơ chế:**
  - **Auto-Load:** Tự động load khi mở app.
  - **Auto-Save:** Tự động lưu NGAY LẬP TỨC khi có thay đổi (Add, Delete, Change Cover).

### 2. Hệ Thống Cover (Ảnh Bìa)
- **Change Cover:** Click chuột phải (hoặc nút 3 chấm) -> Chọn "Change Cover".
- **Import Tự Động:**
  - Khi chọn ảnh, app sẽ **COPY** ảnh đó vào thư mục `covers/` nằm cạnh file `.exe`.
  - Tên ảnh được đổi theo tên game (vd: `Contra.png`) để dễ quản lý.
  - **Lợi ích:** Không sợ mất ảnh khi xóa file gốc, dễ dàng copy game sang máy khác.

### 3. Add Shortcut
- **Chức năng:** Tạo shortcut ra Desktop để mở nhanh game.
- **Cơ chế:**
  - Tạo file Windows Shortcut (`.lnk`).
  - Tự động gán icon là ảnh cover của game.
  - *Lưu ý:* Windows hiển thị tốt nhất với file `.ico`, file ảnh thường (.png/.jpg) có thể không hiện icon ngay.

## 📁 Cấu Trúc Thư Mục

```
GameEnter/
├── nes_app.exe          # File chạy
├── game_slots.txt       # File lưu danh sách slots (Tự tạo)
└── covers/              # Thư mục chứa ảnh bìa (Tự tạo)
    ├── Contra.png
    ├── Super_Mario_3.jpg
    └── ...
```

## 🔧 Hướng Dẫn Code (Dành cho Dev)

### Auto-Save
Code được thêm vào các điểm thay đổi trạng thái trong `main_sdl.cpp`:
```cpp
// Sau khi Add/Delete/Change Cover
std::vector<SlotManager::Slot> slots_to_save;
// ... populate vector ...
SlotManager::save_slots(slots_file, slots_to_save);
```

### Import Cover
Hàm helper `import_cover_image` trong `main_sdl.cpp`:
- Tạo thư mục `covers` nếu chưa có.
- Copy file với `fs::copy_options::overwrite_existing`.
- Trả về đường dẫn tương đối/tuyệt đối mới.

### Shortcut
Sử dụng PowerShell command thông qua `system()` để tạo file `.lnk` vì C++ standard không hỗ trợ tạo shortcut Windows native dễ dàng.

---
**Trạng thái:** ✅ STABLE (Hoạt động ổn định trên Windows)
