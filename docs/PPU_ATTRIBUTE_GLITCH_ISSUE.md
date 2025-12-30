# PPU Attribute Glitch Issue

## Vấn đề

Khi chạy game Contra (và các game khác), xuất hiện **attribute color glitches** khi có scrolling:

### Triệu chứng:
- ✅ **Khi đứng yên**: Không có lỗi, màu sắc hiển thị đúng
- ❌ **Khi di chuyển (scroll)**: Màu sắc bị sai ở vùng xoay quanh nhân vật
- ❌ **Sau khi di chuyển**: Dù đứng lại, lỗi vẫn còn (do scroll position đã thay đổi)

### Mô tả chi tiết:
- Màu sắc của background tiles bị sai
- Thường xuất hiện ở vùng xoay quanh sprites (nhân vật)
- Ví dụ: Màu xanh của quần nhân vật xuất hiện ở vùng núi phía trên
- Có những đường kẻ ngang màu vàng/trắng trên title screen

## Nguyên nhân có thể

Vấn đề liên quan đến **attribute data không được fetch/apply đúng cách khi có scrolling**.

Attribute table trong NES:
- Mỗi attribute byte điều khiển màu cho 4x4 tiles (32x32 pixels)
- Chia thành 4 quadrants 2x2 tiles
- Bits 0-1: top-left, 2-3: top-right, 4-5: bottom-left, 6-7: bottom-right

## Những gì đã thử

### 1. Sửa attribute shift calculation
**Thử nghiệm:**
```cpp
// Cũ
uint8_t shift = ((v_ & 0x40) >> 4) | (v_ & 0x02);

// Mới (đã thử)
uint8_t shift = ((v_ & 0x02) << 1) | ((v_ & 0x40) >> 6);
```
**Kết quả:** Không hiệu quả

### 2. Thay đổi timing của tile fetch và scroll increment
**Thử nghiệm:**
- Fetch trước, increment sau
- Increment trước, fetch sau  
- Fetch trước shift
- Fetch sau shift
**Kết quả:** Không có sự khác biệt

### 3. Skip shift ở cycle 1
**Lý do:** Preserve pre-fetched data
**Kết quả:** Không hiệu quả

### 4. Clear shifters ở đầu scanline
**Lý do:** Prevent garbage data
**Kết quả:** Gây ra vấn đề khác (8 pixels đầu bị trống)

### 5. Sửa palette mirroring
**Đã sửa:**
- Backdrop color mirroring ($3F10/$3F14/$3F18/$3F1C → $3F00)
- Palette reading logic
**Kết quả:** Cải thiện một số vấn đề khác, nhưng attribute glitch vẫn còn

### 6. Sửa sprite rendering
**Đã sửa:**
- Sprite priority logic
- Sprite Y positioning
- Clear sprite shifters
**Kết quả:** Sprites hiển thị đúng hơn, nhưng attribute glitch vẫn còn

## Code hiện tại

### Attribute fetch (ppu.cpp:451-476)
```cpp
void PPU::fetch_background_tile() {
    // Fetch nametable byte
    uint16_t nt_addr = 0x2000 | (v_ & 0x0FFF);
    uint8_t nt_byte = ppu_read(nt_addr);
    
    // Fetch attribute byte
    uint16_t attr_addr = 0x23C0 | (v_ & 0x0C00) | ((v_ >> 4) & 0x38) | ((v_ >> 2) & 0x07);
    uint8_t attr_byte = ppu_read(attr_addr);
    
    // Calculate attribute shift
    uint8_t shift = ((v_ & 0x40) >> 4) | (v_ & 0x02);
    uint8_t pal = (attr_byte >> shift) & 0x03;
    
    // Fetch pattern bytes
    uint16_t pat_addr = (ctrl_.bg_pattern ? 0x1000 : 0x0000) + (nt_byte * 16) + ((v_ >> 12) & 0x07);
    uint8_t pat_lo = ppu_read(pat_addr);
    uint8_t pat_hi = ppu_read(pat_addr + 8);
    
    // Load into shifters
    bg_shifters_.pattern_lo = (bg_shifters_.pattern_lo & 0xFF00) | pat_lo;
    bg_shifters_.pattern_hi = (bg_shifters_.pattern_hi & 0xFF00) | pat_hi;
    bg_shifters_.attribute_lo = (bg_shifters_.attribute_lo & 0xFF00) | ((pal & 0x01) ? 0xFF : 0x00);
    bg_shifters_.attribute_hi = (bg_shifters_.attribute_hi & 0xFF00) | ((pal & 0x02) ? 0xFF : 0x00);
}
```

### Render pixel (ppu.cpp:345-365)
```cpp
void PPU::render_pixel() {
    // ...
    uint16_t bit_mux = 0x8000 >> x_;  // Fine X scroll
    uint8_t p0 = (bg_shifters_.pattern_lo & bit_mux) ? 1 : 0;
    uint8_t p1 = (bg_shifters_.pattern_hi & bit_mux) ? 1 : 0;
    bg_pixel = (p1 << 1) | p0;
    
    uint8_t a0 = (bg_shifters_.attribute_lo & bit_mux) ? 1 : 0;
    uint8_t a1 = (bg_shifters_.attribute_hi & bit_mux) ? 1 : 0;
    bg_palette = (a1 << 1) | a0;
    // ...
}
```

## Giải pháp có thể

### 1. Implement đúng 8-phase PPU fetch cycle
Thay vì fetch tất cả trong 1 cycle, implement đúng theo spec:
- Cycle 1-2: Fetch NT byte
- Cycle 3-4: Fetch AT byte
- Cycle 5-6: Fetch pattern low
- Cycle 7-8: Fetch pattern high + reload shifters

### 2. Thêm internal latches
Theo spec NES PPU, cần có internal latches để lưu tile data tạm:
- NT latch
- AT latch  
- Pattern low latch
- Pattern high latch

### 3. Kiểm tra lại attribute address calculation
Có thể công thức tính attribute address bị sai trong một số edge cases.

### 4. Debug với emulator reference
So sánh với Mesen hoặc FCEUX để xem đâu là sự khác biệt.

## Tình trạng hiện tại

### ✅ Hoạt động tốt:
- Background rendering cơ bản
- Scrolling (tiles hiển thị đúng vị trí)
- Sprite rendering
- Palette system
- Input handling

### ❌ Vẫn còn lỗi:
- Attribute colors bị sai khi có scrolling
- Chỉ ảnh hưởng đến màu sắc, không ảnh hưởng đến vị trí tiles

## Ghi chú

Vấn đề này là một **known issue** trong NES emulation và thường cần phải implement PPU rất chính xác theo spec mới giải quyết được.

Nhiều emulator đơn giản hóa PPU fetch cycle như code hiện tại, và cũng gặp vấn đề tương tự.

## Tài liệu tham khảo

- [NES Dev Wiki - PPU Scrolling](https://www.nesdev.org/wiki/PPU_scrolling)
- [NES Dev Wiki - PPU Rendering](https://www.nesdev.org/wiki/PPU_rendering)
- [NES Dev Wiki - PPU Attribute Tables](https://www.nesdev.org/wiki/PPU_attribute_tables)

---
**Ngày tạo:** 2025-12-27  
**Trạng thái:** Open Issue  
**Độ ưu tiên:** Medium (game vẫn chơi được, chỉ có lỗi visual nhỏ)
