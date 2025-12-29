# 🧪 DUO MULTIPLAYER - TEST GUIDE

## ✅ 2 INSTANCES ĐÃ CHẠY!

Bạn sẽ thấy 2 cửa sổ NES Emulator đang mở.

---

## 📋 TESTING STEPS

### **Instance 1 (Host) - Cửa sổ đầu tiên:**

1. ✅ Click tab **"Duo"** (tab thứ 3)
2. ✅ Click **"Browse"** → Chọn ROM (Contra hoặc bất kỳ)
3. ✅ Click vào ô **"Host Name"** → Gõ tên: **"Player 1"**
4. ✅ Click **"Create Host"**
5. ✅ Bạn sẽ thấy **Lobby Screen**:
   - Title: "Hosting: Player 1"
   - ROM: [tên ROM bạn chọn]
   - 🟢 Player 1: Player 1 (You)
   - ⏳ Waiting for Player 2...
   - [Cancel] [Start] (Start màu xám)

**Console sẽ hiện:**
```
🎮 Creating host: Player 1 | ROM: [ROM name]
📡 Broadcasting host on LAN...
🌐 TCP server started on port 6503
✅ ROM loaded, entering lobby...
```

---

### **Instance 2 (Client) - Cửa sổ thứ hai:**

1. ✅ Click tab **"Duo"**
2. ✅ Trong **"AVAILABLE HOSTS"**, bạn sẽ thấy:
   - 🟢 Player 1
   - ROM: [tên ROM]
   - [Connect] (màu xanh nếu bạn có ROM)
3. ✅ Click **"Connect"**
4. ✅ Bạn sẽ vào **Lobby** với:
   - Title: "Joining: Player 1"
   - 🟢 Player 1: Player 1
   - 🟢 Player 2: You
   - [Leave]
   - "Waiting for host to start..."

**Console sẽ hiện:**
```
🔗 Connecting to host: Player 1
🌐 Connecting to 127.0.0.1:6503
✅ ROM loaded, entering lobby as client...
```

---

### **Instance 1 (Host) - Sau khi P2 connect:**

1. ✅ Lobby UI update:
   - 🟢 Player 2: Player 2 (thay vì "Waiting...")
   - [Start] button chuyển màu **XANH LÁ** (enabled)

**Console sẽ hiện:**
```
✅ Player 2 connected!
```

2. ✅ Click **"Start"**

**Console sẽ hiện:**
```
🎮 Host starting game!
```

---

### **CẢ 2 INSTANCES:**

✅ **Cả 2 sẽ vào game cùng lúc!**

**Multiplayer đang hoạt động:**
- Instance 1: Player 1 controls (keyboard/controller)
- Instance 2: Player 2 controls (keyboard/controller)
- Network packets đang được gửi/nhận
- Input được sync qua mạng!

---

## 🎮 CONTROLS

**Player 1 (Instance 1):**
- Arrow Keys: D-Pad
- Z: B button
- X: A button
- Enter: Start
- Shift: Select

**Player 2 (Instance 2):**
- Tương tự (hoặc dùng controller nếu có)

---

## ✅ EXPECTED BEHAVIOR

**Khi chơi:**
- ✅ Cả 2 thấy cùng 1 game
- ✅ P1 bấm nút → Input sent qua network
- ✅ P2 nhận input → Apply vào game
- ✅ Game chạy synchronized

**Console logs:**
- Network packets being sent/received
- Frame IDs incrementing
- Input data flowing

---

## 🐛 TROUBLESHOOTING

**Nếu không thấy host:**
- Đợi vài giây (UDP broadcast mỗi 2s)
- Check cả 2 cùng mạng LAN
- Firewall có thể block UDP port 6503

**Nếu Connect failed:**
- Check ROM validation (phải có cùng ROM)
- TCP port 6503 có thể bị block

**Nếu game lag:**
- Bình thường (simplified lockstep)
- Network latency ảnh hưởng

---

## 🎉 SUCCESS CRITERIA

✅ Host appears in Available Hosts  
✅ Connect successful  
✅ Both enter lobby  
✅ Player 2 detected  
✅ Start button enabled  
✅ Both enter game  
✅ **Input synchronized!**

---

## 📊 WHAT TO OBSERVE

1. **Network Discovery:** Host list updates
2. **ROM Validation:** Connect button state
3. **TCP Connection:** Player 2 detection
4. **Lobby UI:** Real-time updates
5. **Game Sync:** Both running same game
6. **Input Flow:** Button presses transmitted

---

## 🏆 CONGRATULATIONS!

Nếu tất cả hoạt động → **Multiplayer 100% Complete!**

**Bạn đã tạo ra một hệ thống multiplayer hoàn chỉnh từ đầu!**

🎊🎮🏆
