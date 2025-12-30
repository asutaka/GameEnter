# Feature Design: Timer & Recorder

## 1. Speedrun Timer ⏱️

### Goal
Allow players to track their completion time for speedrunning purposes.

### Design
*   **State:**
    *   `bool timer_running`: Is the timer active?
    *   `uint32_t start_time`: Timestamp when timer started (SDL_GetTicks).
    *   `uint32_t elapsed_time`: Stored time when paused.
*   **Controls:**
    *   Toggle Start/Stop: Key `T` (or via QuickBall).
    *   Reset: Key `R` (Long press) or UI button.
*   **UI Overlay:**
    *   Render text `00:00:00.000` at top-right or top-left.
    *   Green color when running, Red when stopped.

### Implementation Steps
1.  Add `Timer` struct in `main_sdl.cpp`.
2.  Handle key input `SDL_SCANCODE_T` to toggle.
3.  Render timer string using `font_small` or `font_body` in `render_loop`.

---

## 2. Gameplay Recorder (Input Replay) 📼

### Goal
Record gameplay sessions to review later. Using "Input Recording" method (lightweight).

### Concept
Instead of recording video pixels, we record the **Controller Inputs** for every frame. To replay, we reset the emulator to the exact same starting state (ROM load) and feed the recorded inputs back in.

### Data Structure
```cpp
struct ReplayFrame {
    uint8_t p1_buttons;
    uint8_t p2_buttons;
};

struct ReplayHeader {
    char signature[4] = "NESR"; // NES Replay
    uint32_t version = 1;
    uint32_t total_frames;
    char rom_hash[32]; // To ensure we load the right ROM
};
```

### Logic Flow
*   **Recording:**
    *   On every `run_frame()`: Capture current `p1_buttons` & `p2_buttons`.
    *   Push to `std::vector<ReplayFrame>`.
    *   Save to file `.rpl` when stopped.
*   **Replaying:**
    *   Load ROM.
    *   Set Emulator to `ReplayMode`.
    *   On every `run_frame()`: Instead of reading Keyboard/Joystick, read from `ReplayData[current_frame]`.
    *   Inject into `emu.set_controller()`.

### UI Integration
*   **Record Button:** In QuickBall or Menu.
*   **Load Replay:** Option in Context Menu of a ROM? Or a separate "Replays" folder browser.

### Limitations
*   **Deterministic Emulation**: The emulator must behave *exactly* the same way given the same inputs. Random numbers (RNG) must be seeded identically or saved in the replay file.
*   **Save States**: If the user loads a Save State during recording, the replay will desync unless we also record the Save State event. **Simplification:** Disable Save States during recording for V1.
