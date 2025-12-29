# 🧪 TEST INSTRUCTIONS - FINAL VERSION

## ⚠️ QUAN TRỌNG: ĐÓNG TẤT CẢ INSTANCES CŨ!

Trước khi test, **BẮT BUỘC** phải:
1. Đóng tất cả cửa sổ nes_app.exe đang chạy
2. Build lại project (đã build rồi)
3. Chạy 2 instances MỚI

---

## 🎮 HƯỚNG DẪN TEST CHI TIẾT:

### Bước 1: Đóng instances cũ
```powershell
Stop-Process -Name "nes_app" -Force -ErrorAction SilentlyContinue
```

### Bước 2: Chạy Instance 1 (Host)
1. Mở `build\Debug\nes_app.exe`
2. Click tab **"Duo"**
3. Click **"Browse"** → Chọn ROM (Contra hoặc bất kỳ)
4. Click vào ô **"Host Name"** → Gõ: **"Player 1"**
5. Click **"Create Host"**

**Console sẽ hiện:**
```
🎮 Creating host: Player 1 | ROM: [ROM name]
📡 Broadcasting host on LAN...
🌐 TCP server started on port 6503
✅ ROM loaded, entering lobby...
```

**Màn hình:** Lobby với "Waiting for Player 2..."

---

### Bước 3: Chạy Instance 2 (Client)
1. Mở `build\Debug\nes_app.exe` (cửa sổ mới)
2. Click tab **"Duo"**
3. Đợi vài giây → Thấy **"Player 1"** trong "AVAILABLE HOSTS"
4. Click **"Connect"**

**Console sẽ hiện:**
```
🔗 Connecting to host: Player 1
🌐 Connecting to 127.0.0.1:6503
✅ ROM loaded, entering lobby as client...
```

**Màn hình:** Lobby với "Waiting for host to start..."

---

### Bước 4: Instance 1 - Start Game
**Console Instance 1 sẽ hiện:**
```
✅ Player 2 connected!
```

**Màn hình:** Start button chuyển màu xanh

1. Click **"Start"**

**Console Instance 1:**
```
🎮 Host starting game!
```

**Console Instance 2 SẼ HIỆN:**
```
🎮 Received START from host, entering game!
```

**✅ CẢ 2 INSTANCES VÀO GAME!**

---

## 🎯 KIỂM TRA PAUSE/SELECT:

**Trong game:**

**Instance 1 (Host):**
- Bấm **Enter** → Cả 2 pause ✅
- Bấm **Enter** lại → Cả 2 resume ✅
- Bấm **Shift** (Select) → Hoạt động ✅

**Instance 2 (Client):**
- Bấm **Enter** → **KHÔNG pause** ✅
- Bấm **Shift** → **KHÔNG hoạt động** ✅
- Bấm **Arrow Keys, Z, X** → Hoạt động bình thường ✅

---

## ❌ NẾU KHÔNG HOẠT ĐỘNG:

### Vấn đề: Client không vào game

**Nguyên nhân có thể:**
1. Đang dùng instances cũ (chưa có fix)
2. ROM khác nhau
3. Network bị block

**Giải pháp:**
1. **Kill tất cả nes_app.exe:**
   ```powershell
   taskkill /F /IM nes_app.exe
   ```
2. **Build lại:**
   ```powershell
   .\build.ps1
   ```
3. **Chạy lại từ đầu**

---

### Vấn đề: Client vẫn pause được

**Nguyên nhân:**
- Đang dùng build cũ

**Giải pháp:**
- Kill processes
- Build lại
- Test lại

---

## 📝 EXPECTED CONSOLE OUTPUT:

### Host:
```
🎮 Creating host: Player 1 | ROM: Contra (USA)
📡 Broadcasting host on LAN...
🌐 TCP server started on port 6503
✅ ROM loaded, entering lobby...
✅ Player 2 connected!
🎮 Host starting game!
⏸️ Pausing game...
▶️ Resuming game...
```

### Client:
```
🔗 Connecting to host: Player 1
🌐 Connecting to 127.0.0.1:6503
✅ ROM loaded, entering lobby as client...
🎮 Received START from host, entering game!
⏸️ Remote player paused game
▶️ Remote player resumed game
```

---

## ✅ SUCCESS CRITERIA:

1. ✅ Host creates lobby
2. ✅ Client sees host
3. ✅ Client connects
4. ✅ Host sees "Player 2 connected"
5. ✅ **Host clicks Start → BOTH enter game**
6. ✅ Host pause → Both pause
7. ✅ Client pause → Nothing happens

---

**Nếu tất cả đều OK → MULTIPLAYER HOÀN CHỈNH! 🎊**

---

## 🧭 QUICKBALL HOME MENU TEST:

1.  **Launch Emulator:**
    *   Verify **NO TABS** at the top.
    *   Verify **QuickBall** at bottom center.

2.  **Navigation:**
    *   Click QuickBall → Expand.
    *   **Left (Grid):** Go to Games.
    *   **Top (Play):** Go to Replays.
    *   **Right (2 Dots):** Go to Duo.

3.  **Context Switching:**
    *   **Enter Game:** QuickBall changes to Normal layout (Share, Snapshot, Home, Timer).
    *   **Return Home:** QuickBall changes back to Home layout.
    *   **Enter Replay:** QuickBall changes to Replay layout.
