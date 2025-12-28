# 🎮 Roadmap: Online Multiplayer cho NES Emulator

## 📋 Tổng quan
Mục tiêu: Cho phép 2 người chơi chơi game NES cùng nhau qua mạng (1 host, 1 remote)

**Thời gian ước tính:** 2-3 tuần
**Độ khó:** Trung bình → Cao

---

## 🎯 Phase 1: Local Multiplayer (Tuần 1)
**Mục tiêu:** Hỗ trợ 2 người chơi trên cùng 1 máy

### Task 1.1: Cấu trúc Input cho 2 Player
- [ ] Thêm `player2_input` vào `Emulator` class
- [ ] Sửa `set_controller()` để nhận `player_id` (0 hoặc 1)
- [ ] Test với 2 tay cầm gamepad

**File cần sửa:**
- `core/emulator.h`
- `core/emulator.cpp`
- `core/input.h`

**Code mẫu:**
```cpp
// emulator.h
class Emulator {
    uint8_t player1_buttons_;
    uint8_t player2_buttons_;
public:
    void set_controller(int player_id, uint8_t buttons);
};
```

### Task 1.2: UI cho Local Multiplayer
- [ ] Thêm menu "2 Players Local" vào Home Screen
- [ ] Hiển thị "P1" và "P2" indicator khi đang chơi
- [ ] Mapping phím: Player 1 (WASD+ZX), Player 2 (Arrow+NM)

**File cần sửa:**
- `desktop/main_sdl.cpp`

### Task 1.3: Testing
- [ ] Test với Contra (2 players)
- [ ] Test với Ice Climber
- [ ] Verify cả 2 controller hoạt động độc lập

**Thời gian:** 2-3 ngày

---

## 🌐 Phase 2: Network Foundation (Tuần 1-2)
**Mục tiêu:** Thiết lập kết nối mạng cơ bản

### Task 2.1: Cài đặt SDL_net
- [ ] Download SDL_net library
- [ ] Thêm vào CMakeLists.txt / build system
- [ ] Test connection đơn giản (ping-pong)

**File mới:**
- `network/network_manager.h`
- `network/network_manager.cpp`

**Code mẫu:**
```cpp
// network_manager.h
class NetworkManager {
public:
    bool init();
    bool host_game(uint16_t port);
    bool join_game(const char* ip, uint16_t port);
    bool send_input(uint8_t input);
    bool recv_input(uint8_t& input);
    void close();
private:
    TCPsocket socket_;
    bool is_host_;
};
```

### Task 2.2: Packet Structure
- [ ] Định nghĩa packet format
- [ ] Implement serialize/deserialize
- [ ] Thêm checksum validation

**Code mẫu:**
```cpp
struct GamePacket {
    uint32_t magic;         // 0x4E455350 ("NESP")
    uint32_t frame_number;
    uint8_t player1_input;
    uint8_t player2_input;
    uint32_t checksum;
    
    uint32_t calculate_checksum();
    bool validate();
};
```

### Task 2.3: UI cho Network
- [ ] Menu "Host Game" với port selection
- [ ] Menu "Join Game" với IP input
- [ ] Hiển thị "Waiting for player..." khi host
- [ ] Hiển thị "Connecting..." khi join

**Thời gian:** 3-4 ngày

---

## 🔄 Phase 3: Synchronization (Tuần 2)
**Mục tiêu:** Đồng bộ hóa game state giữa 2 máy

### Task 3.1: Deterministic Emulation
- [ ] Verify emulator chạy deterministic
- [ ] Test: cùng ROM + cùng input → cùng output
- [ ] Fix bất kỳ random behavior nào

**Testing:**
```cpp
// Test determinism
void test_determinism() {
    Emulator emu1, emu2;
    emu1.load_rom("contra.nes");
    emu2.load_rom("contra.nes");
    
    for (int i = 0; i < 1000; i++) {
        emu1.set_controller(0, test_input[i]);
        emu2.set_controller(0, test_input[i]);
        emu1.run_frame();
        emu2.run_frame();
        
        assert(memcmp(emu1.get_framebuffer(), 
                      emu2.get_framebuffer(), 
                      256*240*4) == 0);
    }
}
```

### Task 3.2: Frame Synchronization
- [ ] Implement frame counter
- [ ] Gửi frame number trong mỗi packet
- [ ] Xử lý khi 2 máy bị lệch frame

**Code mẫu:**
```cpp
void sync_frame() {
    // Host gửi trước
    if (is_host_) {
        send_packet(frame_num, p1_input, 0);
        recv_packet(frame_num, 0, p2_input);
    } 
    // Client nhận trước
    else {
        recv_packet(frame_num, p1_input, 0);
        send_packet(frame_num, 0, p2_input);
    }
    
    // Verify frame number
    if (local_frame != remote_frame) {
        handle_desync();
    }
}
```

### Task 3.3: Initial State Sync
- [ ] Host gửi save state cho client khi kết nối
- [ ] Client load save state
- [ ] Verify cả 2 bắt đầu từ cùng trạng thái

**Thời gian:** 3-4 ngày

---

## ⚡ Phase 4: Latency Handling (Tuần 2-3)
**Mục tiêu:** Xử lý độ trễ mạng

### Task 4.1: Input Delay Buffer
- [ ] Implement ring buffer cho input
- [ ] Delay 2-4 frames trước khi apply input
- [ ] Cho phép user điều chỉnh delay

**Code mẫu:**
```cpp
class InputBuffer {
    std::queue<uint8_t> buffer_;
    int delay_frames_ = 3;
public:
    void push(uint8_t input) {
        buffer_.push(input);
    }
    
    uint8_t pop() {
        if (buffer_.size() > delay_frames_) {
            uint8_t input = buffer_.front();
            buffer_.pop();
            return input;
        }
        return 0; // No input yet
    }
};
```

### Task 4.2: Ping/Latency Display
- [ ] Measure round-trip time
- [ ] Hiển thị ping trên UI
- [ ] Warning nếu ping > 100ms

### Task 4.3: Disconnect Handling
- [ ] Detect khi mất kết nối
- [ ] Pause game và hiển thị "Connection lost"
- [ ] Cho phép reconnect

**Thời gian:** 2-3 ngày

---

## 🎨 Phase 5: Polish & UX (Tuần 3)
**Mục tiêu:** Hoàn thiện trải nghiệm người dùng

### Task 5.1: Lobby System
- [ ] Hiển thị danh sách game đang host (LAN discovery)
- [ ] Show player names
- [ ] Ready/Not Ready status

### Task 5.2: In-Game Features
- [ ] Chat text đơn giản
- [ ] Voice indicator (nếu có)
- [ ] Pause synchronization (cả 2 phải đồng ý)

### Task 5.3: Settings
- [ ] Network settings page
- [ ] Input delay adjustment
- [ ] Port configuration
- [ ] Connection quality indicator

**Thời gian:** 3-4 ngày

---

## 🚀 Phase 6: Advanced (Optional - Tuần 4+)
**Chỉ làm nếu có thời gian**

### Task 6.1: Rollback Netcode
- [ ] Save state mỗi frame
- [ ] Rollback khi input đến muộn
- [ ] Resimulate từ frame cũ

**Độ khó:** ⭐⭐⭐⭐⭐ (Rất cao!)

### Task 6.2: NAT Traversal
- [ ] Implement STUN/TURN
- [ ] Hole punching
- [ ] Relay server cho kết nối Internet

### Task 6.3: Matchmaking
- [ ] Server trung gian
- [ ] Room codes
- [ ] Friend list

---

## 📊 Checklist tổng hợp

### Week 1: Foundation
- [ ] Local 2-player hoạt động
- [ ] SDL_net được cài đặt
- [ ] Kết nối TCP cơ bản hoạt động
- [ ] UI cho Host/Join

### Week 2: Core Netplay
- [ ] Đồng bộ input giữa 2 máy
- [ ] Frame sync hoạt động
- [ ] Có thể chơi game qua LAN
- [ ] Ping display

### Week 3: Polish
- [ ] Xử lý disconnect
- [ ] Input delay buffer
- [ ] UI/UX hoàn thiện
- [ ] Testing kỹ lưỡng

---

## 🛠️ Dependencies cần cài

```bash
# Windows (vcpkg)
vcpkg install sdl2-net:x64-windows

# Hoặc download manual
https://github.com/libsdl-org/SDL_net/releases
```

---

## 📝 Testing Plan

### Test Cases
1. **Local 2P:** Cả 2 controller hoạt động độc lập
2. **LAN Connection:** Kết nối thành công qua cùng WiFi
3. **Sync Test:** Chơi 5 phút không bị desync
4. **Latency Test:** Thử với ping 50ms, 100ms, 200ms
5. **Disconnect Test:** Ngắt kết nối và reconnect
6. **Different ROMs:** Verify báo lỗi nếu khác ROM

---

## 🎯 Success Criteria

### Minimum Viable Product (MVP)
- ✅ 2 người chơi qua LAN
- ✅ Sync ổn định trong 10 phút
- ✅ Ping < 50ms: smooth gameplay
- ✅ UI đơn giản nhưng đủ dùng

### Nice to Have
- 🌟 Rollback netcode
- 🌟 Internet play (NAT traversal)
- 🌟 Lobby system
- 🌟 Replay system

---

## 📚 Resources

### Documentation
- [SDL_net Documentation](https://www.libsdl.org/projects/SDL_net/docs/)
- [Gaffer on Games - Networking](https://gafferongames.com/)
- [Rollback Netcode Explanation](https://ki.infil.net/w02-netcode.html)

### Reference Implementations
- RetroArch Netplay
- GGPO (Fighting games)
- Parsec (Low latency streaming)

---

## ⚠️ Potential Issues

### Known Challenges
1. **Determinism:** NES emulator phải 100% deterministic
2. **Latency:** Khó chơi nếu ping > 100ms
3. **NAT:** Cần port forwarding hoặc relay server
4. **Cheating:** Không có server authority

### Mitigation
- Test determinism kỹ lưỡng
- Implement input delay
- Dùng UPnP cho auto port forward
- Accept risk (casual play only)

---

## 🎉 Milestones

- **Day 3:** Local 2P working
- **Day 7:** First network connection
- **Day 14:** Playable over LAN
- **Day 21:** Polished MVP ready

---

**Bắt đầu từ đâu?**
Tôi đề xuất: **Phase 1, Task 1.1** - Thêm support cho Player 2 input trước!

Bạn muốn tôi bắt đầu implement ngay không? 🚀
