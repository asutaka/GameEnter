# 🎮 Multiplayer Roadmap

## Phase 1: Local Multiplayer (Hiện Tại)
Mục tiêu: Cho phép 2 người chơi cùng lúc trên 1 máy tính.

- [x] **1.1 Refactor Input System**
  - [x] Tách logic Input ra khỏi `main_sdl.cpp` (nếu cần) hoặc tổ chức lại.
  - [x] Tạo struct/class `Controller` để quản lý mapping phím.
  - [x] Hỗ trợ config phím cho Player 1 và Player 2.

- [x] **1.2 Update Emulator Core**
  - [x] Đảm bảo `Emulator` class nhận input cho cả 2 port ($4016 và $4017).
  - [x] Update hàm `handle_input` để nhận state của 2 controller.

- [x] **1.3 SDL Integration**
  - [x] Map bàn phím cho P2 (VD: Mũi tên để di chuyển, Numpad để đánh).
  - [x] Map Gamepad 2 (nếu có) vào Player 2.

- [x] **1.4 Testing**
  - [x] Test với game 2 người (Contra, Tank 1990).

## Phase 2: Network Multiplayer (Tương Lai)
Mục tiêu: Chơi qua mạng LAN/Internet.

- [ ] **2.1 Architecture Design**
  - [ ] Chọn mô hình: P2P (Peer-to-Peer) với Lockstep hoặc Server-Client.
  - [ ] Nghiên cứu thư viện mạng (ENet hoặc SDL_Net).

- [ ] **2.2 Serialization**
  - [ ] Đồng bộ Input giữa 2 máy.
  - [ ] Save/Load State để đồng bộ trạng thái ban đầu (nếu cần).

- [ ] **2.3 Network Loop**
  - [ ] Gửi input packet mỗi frame.
  - [ ] Xử lý lag/delay (Input buffering).

## Phase 3: UI & Lobby
- [ ] Màn hình chọn chế độ (1 Player / 2 Players Local / Online).
- [ ] Room list / Direct IP connect.
