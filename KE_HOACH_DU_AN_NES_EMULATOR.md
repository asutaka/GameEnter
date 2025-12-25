# 🎮 Kế Hoạch Dự Án NES Emulator

## 📋 Tổng Quan Dự Án

**Mục tiêu**: Xây dựng NES emulator thương mại cho Android với tính năng multiplayer và kiếm tiền.

**Thị trường mục tiêu**: Game thủ mobile, người yêu thích retro gaming

**Chiến lược kiếm tiền**:
- AdMob (banner, interstitial, rewarded ads)
- Phiên bản Premium ($2.99-4.99)
- Affiliate links (bán game)
- Mua trong ứng dụng (themes, tính năng)

**Dự kiến doanh thu**: $100-300/tháng sau 6-12 tháng

---

## 🛠️ Công Nghệ Sử Dụng

### Core Emulator
- **Ngôn ngữ**: C++ (C++17 hoặc mới hơn)
- **Nền tảng**: Cross-platform (PC trước, sau đó Android)
- **Build System**: CMake
- **Testing**: Google Test

### Android App
- **Ngôn ngữ**: Kotlin
- **UI**: Jetpack Compose / XML
- **NDK**: Tích hợp C++
- **Kiến trúc**: MVVM
- **Backend**: Firebase (Auth, Firestore, Storage)
- **Quảng cáo**: Google AdMob
- **Mạng**: Retrofit + OkHttp

### Công Cụ Phát Triển
- **IDE**: 
  - Visual Studio / CLion (C++ core)
  - Android Studio (Android app)
- **Quản lý phiên bản**: Git
- **Debug**: GDB, Android Studio Debugger

---

## 📅 Lộ Trình (Tổng: 5-7 tháng)

### Giai Đoạn 1: PC Emulator Core (2-3 tháng)

#### Tháng 1: Giả Lập CPU
**Tuần 1-2: Setup & CPU 6502**
- [ ] Tạo cấu trúc project
- [ ] Cấu hình CMake
- [ ] Bộ lệnh CPU 6502 (opcodes chính thức)
- [ ] Test ROMs cho CPU (nestest.nes)

**Tuần 3-4: Bộ Nhớ & Mappers**
- [ ] Hệ thống quản lý bộ nhớ
- [ ] Implement Mapper 0 (NROM)
- [ ] ROM loader (định dạng file .nes)
- [ ] Test với game Donkey Kong

#### Tháng 2: PPU (Đồ Họa)
**Tuần 1-2: PPU Cơ Bản**
- [ ] Render background
- [ ] Tile/pattern tables
- [ ] Nametables & scrolling
- [ ] Palettes (bảng màu)

**Tuần 3-4: Sprites & Nâng Cao**
- [ ] Render sprite (OAM)
- [ ] Phát hiện Sprite 0 hit
- [ ] Timing của PPU
- [ ] Test với Super Mario Bros

#### Tháng 3: APU & Hoàn Thiện
**Tuần 1-2: APU (Âm Thanh)**
- [ ] Pulse channels (1 & 2)
- [ ] Triangle channel
- [ ] Noise channel
- [ ] DMC channel (cơ bản)
- [ ] Audio mixer

**Tuần 3-4: Mappers Nâng Cao**
- [ ] Mapper 1 (MMC1)
- [ ] Mapper 2 (UxROM)
- [ ] Mapper 3 (CNROM)
- [ ] **Mapper 4 (MMC3)** - Cho game Contra
- [ ] Test với Mega Man, Contra

---

### Giai Đoạn 2: Chuyển Sang Android (1-2 tháng)

#### Tháng 4: Tích Hợp Android
**Tuần 1-2: Setup NDK**
- [ ] Tạo project Android
- [ ] NDK/JNI bridge
- [ ] Tích hợp C++ core
- [ ] Render bằng OpenGL ES
- [ ] Output audio (OpenSL ES / AAudio)

**Tuần 3-4: UI Cơ Bản**
- [ ] File picker (load ROM)
- [ ] Màn hình game
- [ ] Virtual controller (D-pad, nút A/B)
- [ ] Xử lý touch input
- [ ] Settings cơ bản

---

### Giai Đoạn 3: Tính Năng & Kiếm Tiền (1-2 tháng)

#### Tháng 5: Tính Năng Core
**Tuần 1-2: Hệ Thống Save**
- [ ] Save states (lưu local)
- [ ] Auto-save khi thoát
- [ ] Nhiều slot save
- [ ] Cloud save (Firebase Storage)

**Tuần 2-3: Tài Khoản & Kiếm Tiền**
- [ ] Firebase Authentication
- [ ] User profiles
- [ ] Tích hợp AdMob (banner, interstitial)
- [ ] In-app billing (phiên bản premium)

**Tuần 4: Thư Viện Game**
- [ ] ROM scanner
- [ ] UI thư viện game
- [ ] Lấy cover art (từ API)
- [ ] Game yêu thích/gần đây

#### Tháng 6: Multiplayer
**Tuần 1-2: Bluetooth Multiplayer**
- [ ] Tìm kiếm Bluetooth
- [ ] Kết nối Host/client
- [ ] Đồng bộ input
- [ ] UI lobby

**Tuần 3-4: WiFi & Hoàn Thiện**
- [ ] Hỗ trợ WiFi Direct
- [ ] Test multiplayer
- [ ] Cải thiện UI/UX
- [ ] Tối ưu hiệu năng

---

### Giai Đoạn 4: Chuẩn Bị Ra Mắt (1 tháng)

#### Tháng 7: Testing & Phát Hành
**Tuần 1-2: Testing**
- [ ] Test tương thích (50+ games)
- [ ] Test hiệu năng (máy cấu hình thấp)
- [ ] Sửa bug
- [ ] Beta testing (Google Play Beta)

**Tuần 3-4: Ra Mắt**
- [ ] Play Store listing (screenshots, mô tả)
- [ ] Privacy policy & điều khoản
- [ ] Tài liệu marketing
- [ ] Soft launch
- [ ] Ra mắt chính thức

---

## 🎯 Lộ Trình Theo Game

### Mốc 1: Donkey Kong ✅
**Mục tiêu**: Giả lập cơ bản hoạt động
- CPU: Core 6502
- PPU: Background + sprites
- Mapper: NROM (0)
- Chưa cần audio

### Mốc 2: Super Mario Bros ✅
**Mục tiêu**: Scrolling hoạt động
- PPU: Engine scrolling
- Input: Xử lý controller
- Mapper: NROM (0)
- Audio cơ bản

### Mốc 3: Mega Man ✅
**Mục tiêu**: Gameplay phức tạp
- PPU: Xử lý sprite nâng cao
- APU: Tất cả channels
- Mapper: NROM (0)
- Timing chính xác

### Mốc 4: Contra ✅ (MỤC TIÊU CUỐI)
**Mục tiêu**: Mapper MMC3 hoạt động
- Mapper: MMC3 (4)
- Scanline IRQ
- Scrolling nâng cao
- Audio đầy đủ
- **Nếu Contra chạy được = emulator production-ready**

---

## 🏗️ Cấu Trúc Project

```
NESEmulator/
├── core/                   # C++ emulator core
│   ├── cpu/               # CPU 6502
│   │   ├── cpu.h
│   │   ├── cpu.cpp
│   │   └── opcodes.cpp
│   ├── ppu/               # Picture Processing Unit
│   │   ├── ppu.h
│   │   ├── ppu.cpp
│   │   └── renderer.cpp
│   ├── apu/               # Audio Processing Unit
│   │   ├── apu.h
│   │   ├── apu.cpp
│   │   └── channels.cpp
│   ├── mappers/           # Cartridge mappers
│   │   ├── mapper.h
│   │   ├── mapper0.cpp    # NROM
│   │   ├── mapper1.cpp    # MMC1
│   │   ├── mapper2.cpp    # UxROM
│   │   ├── mapper3.cpp    # CNROM
│   │   └── mapper4.cpp    # MMC3 (cho Contra)
│   ├── memory/            # Quản lý bộ nhớ
│   │   ├── memory.h
│   │   └── memory.cpp
│   ├── cartridge/         # ROM loader
│   │   ├── cartridge.h
│   │   └── cartridge.cpp
│   └── emulator.h/cpp     # Class emulator chính
├── desktop/               # Phiên bản PC (SDL2)
│   ├── main.cpp
│   ├── renderer_sdl.cpp
│   └── audio_sdl.cpp
├── android/               # Android app
│   ├── app/
│   │   ├── src/main/
│   │   │   ├── cpp/       # JNI bridge
│   │   │   ├── java/      # Kotlin code
│   │   │   └── res/       # Resources
│   │   └── build.gradle
│   └── build.gradle
├── tests/                 # Unit tests
│   ├── cpu_tests.cpp
│   ├── ppu_tests.cpp
│   └── test_roms/         # nestest.nes, etc.
├── docs/                  # Tài liệu
│   ├── KIEN_TRUC_NES.md
│   ├── MAPPER_SPECS.md
│   └── API.md
├── CMakeLists.txt
└── README.md
```

---

## 📚 Tài Liệu Học Tập

### Kiến Trúc NES
- [NESDev Wiki](https://www.nesdev.org/wiki/) - **Kinh thánh của NES emulation**
- [Bộ Lệnh 6502](http://www.6502.org/tutorials/6502opcodes.html)
- [Tài Liệu PPU](https://www.nesdev.org/wiki/PPU)
- [Tài Liệu APU](https://www.nesdev.org/wiki/APU)

### Phát Triển Emulator
- [Writing NES Emulator in Rust](https://bugzmanov.github.io/nes_ebook/) - Khái niệm áp dụng cho C++
- [NES Emulator from Scratch](https://www.youtube.com/watch?v=F8kx56OZQhg) - YouTube series
- [FCEUX Source Code](https://github.com/TASEmulators/fceux) - Tham khảo implementation

### Test ROMs
- [nestest.nes](https://github.com/christopherpow/nes-test-roms) - CPU test
- [blargg's test ROMs](https://github.com/christopherpow/nes-test-roms/tree/master/blargg_ppu_tests_2005.09.15b)

### Android NDK
- [Hướng Dẫn Android NDK](https://developer.android.com/ndk/guides)
- [JNI Tips](https://developer.android.com/training/articles/perf-jni)

---

## 🎮 Ghi Chú Đặc Biệt Cho Game Contra

### Yêu Cầu Mapper 4 (MMC3)

**Contra sử dụng mapper MMC3 với các tính năng:**

1. **PRG ROM Banking**
   - Các bank 8KB có thể chuyển đổi
   - Bank cố định tại $C000-$DFFF
   - Bank cuối cố định tại $E000-$FFFF

2. **CHR ROM Banking**
   - Các bank 2KB và 1KB có thể chuyển đổi
   - Tổng 8 banks

3. **Scanline Counter (QUAN TRỌNG cho Contra)**
   ```cpp
   // Pseudo-code
   class Mapper4 {
       uint8_t irq_counter;
       uint8_t irq_latch;
       bool irq_enabled;
       
       void clock_scanline() {
           if (irq_counter == 0) {
               irq_counter = irq_latch;
           } else {
               irq_counter--;
           }
           
           if (irq_counter == 0 && irq_enabled) {
               trigger_irq();  // Cho split-screen scrolling
           }
       }
   };
   ```

4. **Kiểm Soát Mirroring**
   - Chuyển đổi horizontal/vertical

### Thách Thức Đặc Biệt Với Contra

| Thách thức | Giải pháp |
|-----------|----------|
| **Split-screen scrolling** | Timing chính xác cho scanline IRQ |
| **Sprite flickering** | Xử lý OAM đúng cách |
| **Đồng bộ audio** | APU chính xác từng frame |
| **Lỗi scrolling** | Timing chính xác của PPU |

### Kiểm Tra Contra

**Các checkpoint:**
- [ ] Màn hình title hiển thị đúng
- [ ] Sprite người chơi di chuyển mượt
- [ ] Background scroll không bị lỗi
- [ ] Thanh trạng thái giữ cố định (split-screen)
- [ ] Audio phát đúng
- [ ] Chế độ 2 người chơi hoạt động
- [ ] Không crash trong khi chơi

---

## 💰 Chiến Lược Kiếm Tiền

### Nguồn Thu Nhập

#### 1. AdMob (Chính)
**Implementation:**
```kotlin
// Banner ad (hiển thị liên tục)
AdView ở dưới màn hình game

// Interstitial ad (giữa các game)
Hiển thị sau: Game over, Thoát game, Mỗi 5 game

// Rewarded ad (tùy chọn)
Phần thưởng: Tiếp tục game, Mở khóa save slot, Tắt ads 1 giờ
```

**CPM dự kiến**: $0.50 - $2.00 (tùy khu vực)

**Doanh thu ước tính** (1,000 DAU):
- 1,000 user × 5 lần hiển thị ads/ngày = 5,000 impressions
- 5,000 × $1 CPM / 1000 = **$5/ngày = $150/tháng**

#### 2. Phiên Bản Premium
**Giá**: $2.99 - $4.99

**Tính năng**:
- ✅ Không quảng cáo
- ✅ Không giới hạn save slot
- ✅ Đồng bộ cloud
- ✅ Themes độc quyền
- ✅ Early access các tính năng mới

**Tỷ lệ chuyển đổi**: 2-5%

**Doanh thu ước tính** (1,000 DAU, 3% conversion):
- 30 mua/tháng × $3.99 = **$120/tháng**

#### 3. Affiliate Links
**Đối tác**:
- Amazon Associates (game vật lý)
- eBay Partner Network
- Nintendo eShop (nếu có)

**Hoa hồng**: 3-8%

**Doanh thu ước tính**: **$20-50/tháng** (thận trọng)

#### 4. Mua Trong Ứng Dụng (Tùy chọn)
**Items**:
- Premium themes ($0.99)
- Controller skins ($0.99)
- Cheat code packs ($1.99)

**Doanh thu ước tính**: **$30-100/tháng**

### Tổng Dự Báo Doanh Thu

| Tháng | DAU | Doanh thu |
|-------|-----|---------|
| 1-3 | 100 | $20-50 |
| 4-6 | 500 | $100-200 |
| 7-12 | 1,000+ | $300-500 |
| 12+ | 2,000+ | $500-1,000 |

**Hòa vốn**: Tháng 6-8

---

## 🚀 Chiến Lược Marketing

### Trước Ra Mắt
- [ ] Tạo landing page
- [ ] Xây dựng email list
- [ ] Đăng Reddit (r/emulation, r/AndroidGaming)
- [ ] Video demo trên YouTube

### Ra Mắt
- [ ] Press release đến các blog Android
- [ ] Ra mắt trên Product Hunt
- [ ] Reddit AMA
- [ ] Liên hệ influencer

### Sau Ra Mắt
- [ ] Cập nhật thường xuyên (tính năng mới)
- [ ] Tương tác cộng đồng
- [ ] Implement feedback từ user
- [ ] Content marketing (blog posts)

---

## ⚠️ Giảm Thiểu Rủi Ro

### Rủi Ro Pháp Lý
**Rủi ro**: Nintendo DMCA takedown

**Giảm thiểu**:
- ✅ Không bundle ROM
- ✅ Disclaimer rõ ràng: "Chỉ dùng ROM bạn sở hữu hợp pháp"
- ✅ Không dùng trademark/logo Nintendo
- ✅ Tuân thủ Google Play policies

### Rủi Ro Kỹ Thuật
**Rủi ro**: Vấn đề độ chính xác giả lập

**Giảm thiểu**:
- ✅ Dùng test ROMs nhiều
- ✅ Tham khảo emulator hiện có (FCEUX, Nestopia)
- ✅ Community testing (beta program)

### Rủi Ro Thị Trường
**Rủi ro**: Thu hút user thấp

**Giảm thiểu**:
- ✅ Tính năng độc đáo (multiplayer, UI/UX)
- ✅ Ngân sách marketing ($100-500)
- ✅ ASO (App Store Optimization)
- ✅ Mô hình freemium (barrier thấp)

---

## 📊 Chỉ Số Thành Công

### KPIs (Key Performance Indicators)

| Chỉ số | Mục tiêu (Tháng 6) | Mục tiêu (Tháng 12) |
|--------|------------------|-------------------|
| **Lượt tải** | 5,000 | 20,000 |
| **DAU** | 500 | 2,000 |
| **Retention (D7)** | 20% | 30% |
| **Doanh thu** | $200/tháng | $500/tháng |
| **Rating** | 4.0+ | 4.5+ |
| **Premium Conversion** | 2% | 3% |

### Cột Mốc

- [ ] **Tuần 4**: CPU emulation hoạt động (nestest.nes pass)
- [ ] **Tuần 8**: Donkey Kong chơi được
- [ ] **Tuần 12**: Super Mario Bros chơi được
- [ ] **Tuần 16**: Contra chơi được (MMC3 hoạt động)
- [ ] **Tuần 20**: Android app hoạt động
- [ ] **Tuần 24**: Beta launch
- [ ] **Tuần 28**: Ra mắt chính thức

---

## 🔧 Best Practices Phát Triển

### Chất Lượng Code
- Viết unit tests cho lệnh CPU
- Dùng test ROMs để validate
- Code reviews (nếu có team)
- Static analysis tools (cppcheck)

### Hiệu Năng
- Profile thường xuyên (CPU/GPU usage)
- Target 60 FPS trên máy tầm trung
- Tối ưu hot paths (PPU rendering)
- Hiệu quả pin

### Tài Liệu
- Comment logic phức tạp (đặc biệt PPU/APU timing)
- Tài liệu API
- Hướng dẫn user
- Hướng dẫn developer (cho contributors)

---

## 🎯 Bước Tiếp Theo (Tuần 1)

### Hành Động Ngay Lập Tức
1. [ ] **Setup môi trường phát triển**
   - Cài Visual Studio / CLion
   - Cài CMake
   - Setup Git repository

2. [ ] **Tạo cấu trúc project**
   - Khởi tạo CMake project
   - Tạo cấu trúc folder
   - Setup build system

3. [ ] **Bắt đầu implement CPU**
   - Implement CPU struct
   - Implement 10 opcodes đầu tiên
   - Viết tests

4. [ ] **Học tài liệu**
   - Đọc NESDev Wiki (phần CPU)
   - Tải nestest.nes
   - Học bộ lệnh 6502

### Mục Tiêu Tuần 1
**CPU thực thi được lệnh đầu tiên thành công**

---

## 📞 Hỗ Trợ & Cộng Đồng

### Tài Nguyên
- **NESDev Forums**: https://forums.nesdev.org/
- **r/EmuDev**: Reddit community
- **Discord**: EmuDev server

### Câu Hỏi Nên Hỏi Ở Đâu
- CPU timing → NESDev forums
- Android NDK issues → Stack Overflow
- Mapper implementation → Xem source code FCEUX

---

## 📝 Ghi Chú

### Tại Sao C++ Hơn Rust?
- ✅ Bạn đã biết C++
- ✅ Time to market nhanh hơn (tiết kiệm 2-3 tháng)
- ✅ Android NDK support tốt hơn
- ✅ Nhiều code tham khảo hơn
- ✅ Cộng đồng emulation lớn hơn

### Tại Sao PC Trước?
- ✅ Debug dễ hơn
- ✅ Iteration nhanh hơn
- ✅ Tooling tốt hơn
- ✅ Có thể dùng SDL2 để visualization nhanh
- ✅ Port sang Android đơn giản

### Tại Sao Contra Là Mục Tiêu Cuối?
- ✅ Game phổ biến (tốt cho marketing)
- ✅ MMC3 mapper (cover 25% thư viện NES)
- ✅ Chế độ 2 người (test multiplayer)
- ✅ Đủ phức tạp để chứng minh chất lượng
- ✅ Nếu Contra chạy được, hầu hết game sẽ chạy

---

## 🏁 Kết Luận

**Đây là dự án đầy tham vọng nhưng khả thi.**

**Chìa Khóa Thành Công:**
1. ⏰ **Quản lý thời gian** - Tuân thủ timeline
2. 🎯 **Tập trung** - Không thêm feature quá sớm
3. 📚 **Học hỏi** - Dùng NESDev Wiki nhiều
4. 🧪 **Testing** - Dùng test ROMs nghiêm túc
5. 💰 **Kiếm tiền** - Implement sớm, tối ưu sau

**Nhớ rằng**: Mục tiêu là **ship sản phẩm hoạt động**, không phải emulator hoàn hảo. Độ chính xác có thể cải thiện dần qua updates.

**Chúc may mắn! 🚀**

---

**Cập nhật lần cuối**: 2025-12-25
**Phiên bản**: 1.0 (Tiếng Việt)
**Tác giả**: Dự Án NES Emulator Của Bạn
