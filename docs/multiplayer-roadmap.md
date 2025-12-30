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

## Phase 2: Network Multiplayer (Đang thực hiện)
Mục tiêu: Chơi qua mạng LAN/Internet sử dụng mô hình P2P Lockstep.

- [ ] **2.1 Network Core (Winsock)**
  - [ ] Tạo class `NetworkManager` quản lý kết nối TCP/UDP.
  - [ ] Implement chế độ **Server (Host)**: Lắng nghe kết nối.
  - [ ] Implement chế độ **Client (Guest)**: Kết nối đến IP.
  - [ ] Xử lý gửi/nhận packet bất đồng bộ (Non-blocking I/O).

- [ ] **2.2 Lockstep Protocol**
  - [ ] Định nghĩa cấu trúc gói tin `InputPacket` (Frame ID + Button State).
  - [ ] Implement logic "Wait for Input": Game loop sẽ tạm dừng nếu chưa nhận được input từ đối thủ.
  - [ ] Xử lý đồng bộ Start Game (Cả 2 cùng load ROM và bắt đầu).

- [ ] **2.3 UI Integration**
  - [ ] Thêm menu "Multiplayer" ở màn hình Home.
  - [ ] Popup nhập IP Address để kết nối.
  - [ ] Hiển thị trạng thái kết nối (Connected/Disconnected).

## Phase 3: UI & Lobby
- [ ] Màn hình chọn chế độ (1 Player / 2 Players Local / Online).
- [ ] Room list / Direct IP connect.
