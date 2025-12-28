# Hướng Dẫn: Thêm "Change Cover" Vào Context Menu

## ✅ Đã Hoàn Thành

1. **✅ SlotManager đã hỗ trợ lưu cover_path**
   - Lưu 3 thông tin: ROM path, name, cover path
   - Format file: `game_slots.txt`
   ```
   <rom_path>
   <name>
   <cover_path>
   ```

2. **✅ Auto-save/load cover_path**
   - Khi thoát: Lưu cover_path của từng slot
   - Khi khởi động: Load cover_path và texture

## 🚧 Cần Làm Thêm

### Thêm "Change Cover" Vào Context Menu (3 Chấm)

Bạn cần tìm trong `main_sdl.cpp` nơi xử lý context menu của slot (nút 3 chấm).

**Các bước:**

#### 1. Tìm Context Menu Items
Tìm code tương tự:
```cpp
struct MenuItem {
    int id;
    std::string text;
    // ...
};

std::vector<MenuItem> menu_items = {
    {1, "Play"},
    {2, "Delete"},
    // ...
};
```

#### 2. Thêm "Change Cover"
```cpp
std::vector<MenuItem> menu_items = {
    {1, "Play"},
    {2, "Delete"},
    {3, "Change Cover"},  // ← THÊM DÒNG NÀY
};
```

#### 3. Xử Lý Click "Change Cover"
Tìm code xử lý menu item click:
```cpp
if (item.id == 1) {
    // Play game
} else if (item.id == 2) {
    // Delete slot
} else if (item.id == 3) {  // ← THÊM PHẦN NÀY
    // Change cover
    #ifdef _WIN32
    OPENFILENAME ofn;
    char szFile[260] = {0};
    
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = "Images\\0*.PNG;*.JPG;*.JPEG;*.BMP\\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
    
    if (GetOpenFileName(&ofn) == TRUE) {
        // Lưu cover path
        slots[selected_slot].cover_path = szFile;
        
        // Load texture mới
        if (slots[selected_slot].cover_texture) {
            SDL_DestroyTexture(slots[selected_slot].cover_texture);
        }
        slots[selected_slot].cover_texture = load_texture(renderer, szFile);
        
        std::cout << "✅ Đã thay đổi cover: " << szFile << std::endl;
    }
    #endif
}
```

#### 4. Lưu Cover Path Khi Add ROM
Tìm code add ROM và cập nhật:
```cpp
// Khi add ROM thành công
slots[i].rom_path = rom_path;
slots[i].name = game_name;
slots[i].occupied = true;

// Tìm cover tự động
std::string cover_path = find_cover_image(rom_path);
if (!cover_path.empty()) {
    slots[i].cover_path = cover_path;  // ← LƯU COVER PATH
    slots[i].cover_texture = load_texture(renderer, cover_path);
}
```

## 📝 Lưu Ý

1. **File Dialog trên Windows**
   - Dùng `GetOpenFileName` (đã có trong code)
   - Filter: PNG, JPG, JPEG, BMP

2. **Destroy Texture Cũ**
   - Nhớ `SDL_DestroyTexture` trước khi load texture mới
   - Tránh memory leak

3. **Save Ngay**
   - Có thể save slots ngay sau khi change cover
   - Hoặc đợi đến khi thoát app

## 🎯 Kết Quả Mong Đợi

Sau khi implement:
1. Click nút 3 chấm trên slot
2. Chọn "Change Cover"
3. File dialog mở ra
4. Chọn ảnh mới
5. Cover thay đổi ngay lập tức
6. Khi thoát app → Cover path được lưu
7. Khi mở lại app → Cover mới được load

## 🔍 Tìm Code

Để tìm context menu, search trong `main_sdl.cpp`:
- "menu" hoặc "MenuItem"
- "right click" hoặc "context"
- Hoặc tìm nơi render nút 3 chấm

## ✅ Đã Sẵn Sàng

SlotManager đã sẵn sàng lưu/load cover_path. 
Bạn chỉ cần:
1. Thêm menu item "Change Cover"
2. Xử lý file dialog
3. Update `slots[i].cover_path`

Done! 🎮✨
