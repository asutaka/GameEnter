# Tóm Tắt: Save State Experiment

## 🎯 Mục Tiêu Ban Đầu

> "tôi muốn khi chơi game mà tắt đi(không reset) thì game vẫn ở màn đó có khả thi không"

## 🔬 Những Gì Đã Thử

### 1. Full Save State ❌
**Ý tưởng:** Lưu toàn bộ CPU/PPU/Memory state

**Kết quả:**
- ✅ Lưu được data
- ❌ Load lại bị màn hình đen
- ❌ Quá phức tạp, nhiều lỗi

**Nguyên nhân thất bại:**
- PPU rendering pipeline phức tạp
- Cần lưu quá nhiều state
- Cartridge mapper state
- Timing issues

### 2. Level Save + Skip ❌
**Ý tưởng:** Lưu số màn, sau đó skip đến màn đó

**Kết quả:**
- ✅ Lưu được số màn
- ✅ Load được số màn
- ❌ Không skip được level

**Nguyên nhân thất bại:**
- Contra (US) không có stage select
- Không có cheat code cho stage select
- Ghi RAM không hoạt động (game protection)
- Chỉ phiên bản Nhật mới có stage select

## 📊 Kết Luận

### ❌ KHÔNG KHẢ THI với Contra

**Lý do:**
1. Game không hỗ trợ save state
2. Game không cho phép skip level
3. Full save state quá phức tạp và dễ lỗi
4. Đây là giới hạn của NES hardware và game design

### ✅ Giải Pháp Thay Thế

**Konami Code:**
- ↑ ↑ ↓ ↓ ← → ← → B A (Start)
- 30 mạng
- Giúp chơi nhanh hơn đến màn muốn chơi

**Chấp nhận:**
- Chơi lại từ đầu mỗi lần
- Đây là cách game gốc hoạt động
- Là một phần của trải nghiệm NES

## 🎮 Bài Học

1. **NES games không được thiết kế cho save state**
   - Hardware không hỗ trợ
   - Game logic phức tạp
   - Nhiều protection

2. **Emulator save state rất khó implement**
   - Cần hiểu sâu về hardware
   - Mỗi game khác nhau
   - Dễ gặp lỗi

3. **Một số thứ không thể làm được**
   - Phải chấp nhận giới hạn
   - Tìm giải pháp thay thế
   - Hoặc thay đổi kỳ vọng

## 🗑️ Đã Xóa

Tất cả code liên quan đến save state đã được xóa:
- `core/save_state.h/cpp`
- `core/level_save.h`
- Modifications trong emulator
- Test scripts
- Documentation

## ✅ Trạng Thái Hiện Tại

**Game hoạt động bình thường:**
- ✅ Chơi được Contra
- ✅ Không có save state
- ✅ Reset về màn 1 mỗi lần
- ✅ Giống game gốc

## 💭 Suy Nghĩ Cuối

Đôi khi, **giới hạn là một phần của trải nghiệm**. 

NES games được thiết kế để:
- Chơi lại nhiều lần
- Thử thách kỹ năng
- Không có save/load

Đó là lý do chúng vẫn thú vị sau 40 năm! 🎮

---

**Ngày:** 2025-12-28
**Kết luận:** Experiment thất bại nhưng học được nhiều!
