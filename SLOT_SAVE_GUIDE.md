# Hệ Thống Lưu Slots

## 🎯 Chức Năng

**Lưu danh sách ROM đã thêm vào slots** để khi mở lại app, các slots vẫn còn đó.

## 🎮 Cách Hoạt Động

### Khi Thêm ROM
1. Người dùng click "Add ROM"
2. Chọn file ROM
3. ROM được thêm vào slot
4. Hiển thị trong UI

### Khi Thoát App
1. Tự động lưu tất cả slots vào `game_slots.txt`
2. Chỉ lưu ROM còn tồn tại
3. Bỏ qua ROM đã bị xóa

### Khi Mở Lại App
1. Tự động load slots từ `game_slots.txt`
2. Check từng ROM xem còn tồn tại không
3. Nếu ROM không tồn tại → Bỏ qua, hiển thị warning
4. Nếu ROM còn tồn tại → Load vào slot

## 📁 File Format

**File:** `game_slots.txt`

**Format:**
```
<rom_path_1>
<rom_name_1>
<rom_path_2>
<rom_name_2>
...
```

**Ví dụ:**
```
tests\test_roms\contra.nes
Contra
tests\test_roms\Super Mario Bros. 3 (USA) (Rev 1).nes
Super Mario Bros. 3
tests\test_roms\donkey kong.nes
Donkey Kong
```

## ✅ Tính Năng

### 1. Auto-Save Khi Thoát
- ✅ Tự động lưu tất cả slots
- ✅ Không cần thao tác thủ công
- ✅ Lưu vào file text đơn giản

### 2. Auto-Load Khi Khởi Động
- ✅ Tự động load slots đã lưu
- ✅ Load cả cover images
- ✅ Hiển thị ngay trong UI

### 3. Validate ROM Path
- ✅ Check ROM còn tồn tại không
- ✅ Bỏ qua ROM đã bị xóa/di chuyển
- ✅ Hiển thị warning trong console

### 4. Console Feedback
```
📂 Đã load 3 slots từ: game_slots.txt
✅ Load slot: Contra
✅ Load slot: Super Mario Bros. 3
⚠️  ROM không tồn tại, bỏ qua: old_game.nes
```

## 🔧 Implementation

### Files
- `desktop/slot_manager.h` - SlotManager class (header-only)
- `desktop/main_sdl.cpp` - Integration vào main loop

### Code
```cpp
// Load khi khởi động
std::vector<SlotManager::Slot> saved_slots;
if (SlotManager::load_slots("game_slots.txt", saved_slots)) {
    // Convert và hiển thị
}

// Save khi thoát
std::vector<SlotManager::Slot> slots_to_save;
for (const auto& slot : slots) {
    if (slot.occupied) {
        slots_to_save.push_back(SlotManager::Slot(slot.rom_path, slot.name));
    }
}
SlotManager::save_slots("game_slots.txt", slots_to_save);
```

## 📝 Lưu Ý

1. **File location:** `game_slots.txt` ở cùng thư mục với executable
2. **ROM path:** Lưu đường dẫn tuyệt đối hoặc tương đối
3. **Validation:** Tự động bỏ qua ROM không tồn tại
4. **Cover images:** Tự động load lại khi khởi động

## 🎯 Use Cases

### Case 1: Sử Dụng Bình Thường
1. Thêm 5 ROM vào slots
2. Chơi game
3. Thoát app → Auto-save
4. Mở lại app → 5 ROM vẫn còn ✅

### Case 2: ROM Bị Xóa
1. Có 5 ROM trong slots
2. Xóa 2 ROM khỏi ổ đĩa
3. Mở lại app → Chỉ load 3 ROM còn lại ✅
4. Console hiển thị warning cho 2 ROM bị xóa

### Case 3: Di Chuyển ROM
1. Có ROM tại `C:\Games\contra.nes`
2. Di chuyển sang `D:\ROMs\contra.nes`
3. Mở lại app → ROM không load (path cũ không tồn tại)
4. Thêm lại ROM từ path mới

## 🚀 Ưu Điểm

- ✅ **Đơn giản** - Chỉ 1 file header
- ✅ **Tự động** - Không cần thao tác thủ công
- ✅ **An toàn** - Validate path trước khi load
- ✅ **Rõ ràng** - File text có thể đọc/sửa bằng tay
- ✅ **Nhẹ** - Không tốn nhiều dung lượng

## 🎮 Kết Luận

Hệ thống lưu slots **hoàn toàn khả thi** và **đơn giản**!

Không giống save state (phức tạp, dễ lỗi), lưu slots chỉ là:
- Lưu danh sách file paths
- Check file còn tồn tại không
- Load lại khi khởi động

**Đơn giản, hiệu quả, không lỗi!** ✨
