---
description: Duo Multiplayer Implementation Workflow
---

# Duo Multiplayer Implementation Workflow

This workflow guides you through implementing the Duo (2-player network multiplayer) feature for the NES emulator.

## Overview

**Updated Flow:**
1. Host selects ROM first
2. Host enters host name
3. Host creates lobby (broadcasts with ROM info)
4. Client sees hosts with ROM names
5. Client can only connect if they have the same ROM
6. Both enter game simultaneously when host starts

---

## Phase 1: UI Foundation (Duo Panel)

### Step 1.1: Design Duo Panel Layout

Update `main_sdl.cpp` in the `HOME_PANEL_FAVORITES` (Duo) rendering section:

**Layout:**
```
┌─────────────────────────────────────────────┐
│ CREATE HOST                                 │
│ ┌─────────────────────────────────────────┐ │
│ │ Selected ROM: [Contra (USA)    ] [📁]  │ │
│ │ Host Name:    [____________]           │ │
│ │                        [Create Host]   │ │
│ └─────────────────────────────────────────┘ │
│                                             │
│ AVAILABLE HOSTS                             │
│ ┌─────────────────────────────────────────┐ │
│ │ 🟢 John's Game                          │ │
│ │    ROM: Contra (USA)        [Connect]  │ │
│ │ ─────────────────────────────────────── │ │
│ │ 🟢 Mike's Room                          │ │
│ │    ROM: Mario Bros ❌ Not Found        │ │
│ └─────────────────────────────────────────┘ │
└─────────────────────────────────────────────┘
```

**Variables to add:**
```cpp
// Duo Panel State
std::string duo_selected_rom_path = "";
std::string duo_selected_rom_name = "";
std::string duo_host_name = "";
bool duo_rom_selector_open = false;
std::vector<HostInfo> available_hosts;

struct HostInfo {
    std::string host_name;
    std::string rom_name;
    std::string rom_path;
    std::string ip_address;
    uint16_t port;
    bool has_rom_locally; // Client checks if they have this ROM
    uint32_t last_seen_timestamp;
};
```

### Step 1.2: Implement ROM Selector Button

Add clickable area for ROM selector (📁 button):
- When clicked, show ROM grid overlay
- User selects ROM from their library
- Update `duo_selected_rom_path` and `duo_selected_rom_name`

### Step 1.3: Implement Host Name Input

Add text input field:
- Click to activate
- Type to enter host name
- Store in `duo_host_name`

### Step 1.4: Implement Create Host Button

Add button logic:
- **Enabled only if:** ROM selected AND host name not empty
- When clicked: trigger `create_duo_host()`

### Step 1.5: Implement Host List Rendering

Display available hosts:
- Show host name, ROM name
- Show "Connect" button (enabled only if `has_rom_locally == true`)
- Show "❌ Not Found" if ROM missing

---

## Phase 2: Network Discovery System

### Step 2.1: Create NetworkDiscovery Class

Create `core/network/discovery.h` and `discovery.cpp`:

```cpp
class NetworkDiscovery {
public:
    void start_broadcasting(const std::string& host_name, 
                           const std::string& rom_name,
                           const std::string& rom_path,
                           uint16_t port);
    void stop_broadcasting();
    
    void start_listening();
    void stop_listening();
    
    std::vector<HostInfo> get_available_hosts();
    
private:
    void broadcast_thread();
    void listen_thread();
    
    // UDP socket for broadcast
    int broadcast_socket;
    int listen_socket;
    
    std::vector<HostInfo> hosts;
    std::mutex hosts_mutex;
};
```

**Protocol Message:**
```cpp
struct DiscoveryMessage {
    char magic[4] = "NESD"; // NES Discovery
    char host_name[32];
    char rom_name[64];
    char rom_path[256];
    uint16_t tcp_port;
    uint32_t host_id;
};
```

### Step 2.2: Implement UDP Broadcast (Host)

Host broadcasts every 2 seconds:
- Send `DiscoveryMessage` via UDP multicast (239.255.0.1:6502)
- Include ROM path for validation

### Step 2.3: Implement UDP Listener (Client)

Client listens for broadcasts:
- Receive `DiscoveryMessage`
- Check if ROM exists locally: `check_rom_exists(rom_path)`
- Update `available_hosts` list
- Remove hosts not seen for >5 seconds

### Step 2.4: ROM Validation

Add function to check ROM availability:
```cpp
bool check_rom_exists(const std::string& rom_path) {
    // Check if file exists
    // Could also check MD5/SHA1 hash for exact match
    return std::filesystem::exists(rom_path);
}
```

---

## Phase 3: Lobby System

### Step 3.1: Create Lobby Scene

Add new scene enum:
```cpp
enum Scene { 
    SCENE_HOME, 
    SCENE_GAME, 
    SCENE_SETTINGS, 
    SCENE_MULTIPLAYER_LOBBY,
    SCENE_LOBBY  // New
};
```

### Step 3.2: Implement Create Host Flow

```cpp
void create_duo_host() {
    // 1. Start TCP server
    network_server.start(6503);
    
    // 2. Start broadcasting
    discovery.start_broadcasting(duo_host_name, 
                                duo_selected_rom_name,
                                duo_selected_rom_path,
                                6503);
    
    // 3. Load ROM into emulator (but don't start yet)
    emu.load_rom(duo_selected_rom_path);
    
    // 4. Enter lobby scene
    current_scene = SCENE_LOBBY;
    lobby_is_host = true;
    lobby_player2_connected = false;
}
```

### Step 3.3: Design Lobby UI (Host)

```
┌─────────────────────────────────────┐
│ Lobby: John's Game                  │
│ ROM: Contra (USA)                   │
│                                     │
│ 🎮 Player 1: John (Host)            │
│ ⏳ Waiting for Player 2...          │
│                                     │
│ [Cancel]              [Start] ←disabled│
└─────────────────────────────────────┘
```

When Player 2 connects:
- Update UI: "🎮 Player 2: Mike"
- Enable [Start] button

### Step 3.4: Implement Connect Flow (Client)

```cpp
void connect_to_host(const HostInfo& host) {
    // 1. Connect TCP to host
    if (network_client.connect(host.ip_address, host.port)) {
        // 2. Send join request
        JoinRequest req;
        strcpy(req.player_name, config.get_nickname().c_str());
        network_client.send(req);
        
        // 3. Wait for response
        JoinResponse resp = network_client.receive();
        
        if (resp.accepted) {
            // 4. Load ROM
            emu.load_rom(host.rom_path);
            
            // 5. Enter lobby
            current_scene = SCENE_LOBBY;
            lobby_is_host = false;
        }
    }
}
```

### Step 3.5: Design Lobby UI (Client)

```
┌─────────────────────────────────────┐
│ Lobby: John's Game                  │
│ ROM: Contra (USA)                   │
│                                     │
│ 🎮 Player 1: John (Host)            │
│ 🎮 Player 2: Mike (You)             │
│                                     │
│ Waiting for host to start...       │
│ [Leave]                             │
└─────────────────────────────────────┘
```

---

## Phase 4: Network Protocol

### Step 4.1: Define Message Types

```cpp
enum MessageType {
    MSG_JOIN_REQUEST,
    MSG_JOIN_RESPONSE,
    MSG_START_GAME,
    MSG_INPUT,
    MSG_DISCONNECT,
    MSG_PING
};

struct JoinRequest {
    MessageType type = MSG_JOIN_REQUEST;
    char player_name[32];
};

struct JoinResponse {
    MessageType type = MSG_JOIN_RESPONSE;
    bool accepted;
    char reason[64]; // If rejected
};

struct StartGameMessage {
    MessageType type = MSG_START_GAME;
    uint32_t countdown; // 3, 2, 1, 0
};

struct InputMessage {
    MessageType type = MSG_INPUT;
    uint32_t frame_number;
    uint8_t buttons;
};
```

### Step 4.2: Implement TCP Server (Host)

Create `core/network/server.h`:
```cpp
class NetworkServer {
public:
    bool start(uint16_t port);
    void stop();
    
    void wait_for_client();
    void send_start_game();
    void send_input(uint8_t buttons, uint32_t frame);
    uint8_t receive_input(uint32_t frame);
    
private:
    int server_socket;
    int client_socket;
};
```

### Step 4.3: Implement TCP Client

Create `core/network/client.h`:
```cpp
class NetworkClient {
public:
    bool connect(const std::string& ip, uint16_t port);
    void disconnect();
    
    void send_join_request(const std::string& name);
    bool receive_join_response();
    void wait_for_start();
    
    void send_input(uint8_t buttons, uint32_t frame);
    uint8_t receive_input(uint32_t frame);
    
private:
    int socket;
};
```

---

## Phase 5: Game Synchronization

### Step 5.1: Implement Start Game (Host)

```cpp
void on_lobby_start_clicked() {
    // 1. Send countdown messages
    for (int i = 3; i > 0; i--) {
        send_start_countdown(i);
        std::this_thread::sleep_for(1s);
    }
    
    // 2. Send final start
    send_start_countdown(0);
    
    // 3. Reset emulator
    emu.reset();
    
    // 4. Enter game
    current_scene = SCENE_GAME;
    is_network_game = true;
    is_network_host = true;
}
```

### Step 5.2: Implement Start Game (Client)

```cpp
void on_receive_start_countdown(uint32_t count) {
    if (count > 0) {
        // Show countdown on screen
        show_countdown(count);
    } else {
        // Start game
        emu.reset();
        current_scene = SCENE_GAME;
        is_network_game = true;
        is_network_host = false;
    }
}
```

### Step 5.3: Implement Lockstep Game Loop

Modify game loop to sync inputs:

```cpp
void network_game_loop() {
    uint32_t current_frame = 0;
    
    while (current_scene == SCENE_GAME) {
        // 1. Get local input
        uint8_t local_input = get_local_input();
        
        // 2. Send to peer
        if (is_network_host) {
            network_server.send_input(local_input, current_frame);
        } else {
            network_client.send_input(local_input, current_frame);
        }
        
        // 3. Receive peer input (BLOCKING)
        uint8_t remote_input;
        if (is_network_host) {
            remote_input = network_server.receive_input(current_frame);
        } else {
            remote_input = network_client.receive_input(current_frame);
        }
        
        // 4. Apply inputs to emulator
        if (is_network_host) {
            emu.set_controller(0, local_input);   // Host is P1
            emu.set_controller(1, remote_input);  // Client is P2
        } else {
            emu.set_controller(0, remote_input);  // Host is P1
            emu.set_controller(1, local_input);   // Client is P2
        }
        
        // 5. Run emulator frame
        emu.run_frame();
        
        // 6. Render
        render_frame();
        
        current_frame++;
    }
}
```

### Step 5.4: Handle Disconnection

Add disconnect detection:
- Timeout if no input received for 5 seconds
- Show "Connection Lost" message
- Return to Duo panel

---

## Phase 6: Testing & Polish

### Step 6.1: Test Scenarios

- [ ] Host creates lobby, client connects
- [ ] Client tries to connect without ROM (should be disabled)
- [ ] Both players play Contra together
- [ ] Host disconnects mid-game
- [ ] Client disconnects mid-game
- [ ] Network lag simulation

### Step 6.2: Add Visual Feedback

- Loading indicators
- Connection status icons
- Ping/latency display
- "Waiting for input..." when peer is slow

### Step 6.3: Error Handling

- Port already in use
- Connection refused
- ROM mismatch (extra validation)
- Network timeout

---

## Implementation Order

**Recommended sequence:**

1. **Phase 1 (UI)** - Get the panel looking right
2. **Phase 2.1-2.3** - Basic discovery (no ROM validation yet)
3. **Phase 3.1-3.3** - Lobby for host
4. **Phase 3.4-3.5** - Lobby for client
5. **Phase 4** - Network protocol
6. **Phase 2.4** - Add ROM validation
7. **Phase 5** - Game sync
8. **Phase 6** - Testing

---

## Key Validation Points

✅ **ROM Validation Checkpoints:**

1. **Host Creation:** ROM must be selected before creating
2. **Client Discovery:** Check ROM exists locally
3. **Connect Button:** Only enabled if ROM found
4. **Join Request:** Server validates ROM path matches
5. **Game Start:** Both sides confirm ROM loaded

---

## Estimated Time

- Phase 1 (UI): 3-4 hours
- Phase 2 (Discovery): 4-5 hours
- Phase 3 (Lobby): 4-5 hours
- Phase 4 (Protocol): 3-4 hours
- Phase 5 (Sync): 5-6 hours
- Phase 6 (Testing): 3-4 hours

**Total: ~22-28 hours**

---

## Notes

- Use existing `Config` class for player nickname
- Reuse `SlotManager` ROM list for selector
- Consider adding MD5 hash validation for ROM matching
- Add option to download ROM from host (future enhancement)
