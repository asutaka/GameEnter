#define SDL_MAIN_HANDLED
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <winsock2.h>
#include <windows.h>
#include <commdlg.h>
#endif
#include <SDL2/SDL.h>
#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <iomanip>
#include <cmath>
#include <vector>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <filesystem>
#include "../core/emulator.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"



#include "slot_manager.h"
#include "../core/network/network_manager.h"
#include "../core/network/network_discovery.h"
#include "../core/config/config_manager.h"
#include "systems/Scene.h"

using namespace nes;
namespace fs = std::filesystem; // Alias for convenience

// Global Network Manager
NetworkManager net_manager;
// Global Network Discovery
NetworkDiscovery discovery;
// Global Config Manager
ConfigManager config;

// Multiplayer Message Types
#define MSG_START_GAME  0xFFFFFFFF  // Signal to start game
#define MSG_PAUSE_GAME  0xFFFFFFFE  // Signal to pause game
#define MSG_RESUME_GAME 0xFFFFFFFD  // Signal to resume game

// Timer State
bool timer_running = false;
std::chrono::time_point<std::chrono::high_resolution_clock> timer_start_time;
bool timer_show_final = false;
std::chrono::time_point<std::chrono::high_resolution_clock> timer_final_display_start;
double timer_final_value = 0.0;

// Settings UI State
std::string settings_nickname;
std::string settings_avatar_path;
bool settings_loaded = false;
bool settings_recorder_enabled = false;
int active_input_field = -1; // 0: Nickname

// Screen dimensions
const int SCREEN_WIDTH = 256;
const int SCREEN_HEIGHT = 240;
const int SCALE = 3; // 3x scale = 768x720

#include "systems/ReplaySystem.h"
#include "systems/FontSystem.h"

#define STB_IMAGE_IMPLEMENTATION
#include "systems/UISystem.h"
#include "systems/Slot.h"
#include "systems/HomeScene.h"
#include "systems/LobbyScene.h"
#include "systems/SettingsScene.h"

// --- Global Replay Instances ---
Recorder recorder;
ReplayPlayer replay_player;



// --- Font System (stb_truetype) ---

// --- Global Font Instances ---
FontSystem font_small;
FontSystem font_body;
FontSystem font_title;








// Input Handling
void handle_input(Emulator& emu, const Uint8* keys, const VirtualJoystick& joystick, const std::vector<VirtualButton>& buttons, const std::vector<SDL_GameController*>& controllers) {
    // Check if we're playing back a replay
    uint8_t p1_buttons = 0;
    uint8_t p2_buttons = 0;
    
    if (replay_player.is_playing) {
        // Use replay inputs
        if (!replay_player.get_next_frame(p1_buttons, p2_buttons)) {
            // Replay finished or not playing
            p1_buttons = 0;
            p2_buttons = 0;
        }
        
        // Set controller inputs from replay
        emu.set_controller(0, p1_buttons);
        emu.set_controller(1, p2_buttons);
        
        // Don't record when playing back
        return;
    }
    
    // --- Normal input handling (not replay) ---
    // --- Player 1 ---
    p1_buttons = 0;
    
    // 1. Keyboard P1
    if (keys[SDL_SCANCODE_Z])      p1_buttons |= (1 << Input::BUTTON_A);
    if (keys[SDL_SCANCODE_X])      p1_buttons |= (1 << Input::BUTTON_B);
    if (keys[SDL_SCANCODE_A])      p1_buttons |= (1 << Input::BUTTON_SELECT); 
    if (keys[SDL_SCANCODE_S])      p1_buttons |= (1 << Input::BUTTON_START);
    if (keys[SDL_SCANCODE_UP])     p1_buttons |= (1 << Input::BUTTON_UP);
    if (keys[SDL_SCANCODE_DOWN])   p1_buttons |= (1 << Input::BUTTON_DOWN);
    if (keys[SDL_SCANCODE_LEFT])   p1_buttons |= (1 << Input::BUTTON_LEFT);
    if (keys[SDL_SCANCODE_RIGHT])  p1_buttons |= (1 << Input::BUTTON_RIGHT);
    
    // 2. Virtual Controls (P1 only)
    if (controllers.empty()) {
        if (joystick.up)    p1_buttons |= (1 << Input::BUTTON_UP);
        if (joystick.down)  p1_buttons |= (1 << Input::BUTTON_DOWN);
        if (joystick.left)  p1_buttons |= (1 << Input::BUTTON_LEFT);
        if (joystick.right) p1_buttons |= (1 << Input::BUTTON_RIGHT);

        for (const auto& btn : buttons) {
            if (btn.pressed) p1_buttons |= (1 << btn.nes_button_mapping);
        }
    }

    // 3. Game Controller 1
    if (!controllers.empty()) {
        SDL_GameController* ctrl = controllers[0];
        if (SDL_GameControllerGetButton(ctrl, SDL_CONTROLLER_BUTTON_A)) p1_buttons |= (1 << Input::BUTTON_A);
        if (SDL_GameControllerGetButton(ctrl, SDL_CONTROLLER_BUTTON_B)) p1_buttons |= (1 << Input::BUTTON_B);
        if (SDL_GameControllerGetButton(ctrl, SDL_CONTROLLER_BUTTON_X)) p1_buttons |= (1 << Input::BUTTON_B);
        if (SDL_GameControllerGetButton(ctrl, SDL_CONTROLLER_BUTTON_Y)) p1_buttons |= (1 << Input::BUTTON_A);
        if (SDL_GameControllerGetButton(ctrl, SDL_CONTROLLER_BUTTON_START)) p1_buttons |= (1 << Input::BUTTON_START);
        if (SDL_GameControllerGetButton(ctrl, SDL_CONTROLLER_BUTTON_BACK))  p1_buttons |= (1 << Input::BUTTON_SELECT);
        
        if (SDL_GameControllerGetButton(ctrl, SDL_CONTROLLER_BUTTON_DPAD_UP))    p1_buttons |= (1 << Input::BUTTON_UP);
        if (SDL_GameControllerGetButton(ctrl, SDL_CONTROLLER_BUTTON_DPAD_DOWN))  p1_buttons |= (1 << Input::BUTTON_DOWN);
        if (SDL_GameControllerGetButton(ctrl, SDL_CONTROLLER_BUTTON_DPAD_LEFT))  p1_buttons |= (1 << Input::BUTTON_LEFT);
        if (SDL_GameControllerGetButton(ctrl, SDL_CONTROLLER_BUTTON_DPAD_RIGHT)) p1_buttons |= (1 << Input::BUTTON_RIGHT);

        int16_t axisX = SDL_GameControllerGetAxis(ctrl, SDL_CONTROLLER_AXIS_LEFTX);
        int16_t axisY = SDL_GameControllerGetAxis(ctrl, SDL_CONTROLLER_AXIS_LEFTY);
        const int DEADZONE = 8000;
        if (axisY < -DEADZONE) p1_buttons |= (1 << Input::BUTTON_UP);
        if (axisY > DEADZONE)  p1_buttons |= (1 << Input::BUTTON_DOWN);
        if (axisX < -DEADZONE) p1_buttons |= (1 << Input::BUTTON_LEFT);
        if (axisX > DEADZONE)  p1_buttons |= (1 << Input::BUTTON_RIGHT);
    }
    
    emu.set_controller(0, p1_buttons);

    // --- Player 2 ---
    p2_buttons = 0;

    // 1. Keyboard P2 (IJKL + O/P)
    if (keys[SDL_SCANCODE_P])      p2_buttons |= (1 << Input::BUTTON_A);
    if (keys[SDL_SCANCODE_O])      p2_buttons |= (1 << Input::BUTTON_B);
    if (keys[SDL_SCANCODE_7])      p2_buttons |= (1 << Input::BUTTON_SELECT);
    if (keys[SDL_SCANCODE_8])      p2_buttons |= (1 << Input::BUTTON_START);
    if (keys[SDL_SCANCODE_I])      p2_buttons |= (1 << Input::BUTTON_UP);
    if (keys[SDL_SCANCODE_K])      p2_buttons |= (1 << Input::BUTTON_DOWN);
    if (keys[SDL_SCANCODE_J])      p2_buttons |= (1 << Input::BUTTON_LEFT);
    if (keys[SDL_SCANCODE_L])      p2_buttons |= (1 << Input::BUTTON_RIGHT);

    // 2. Game Controller 2
    if (controllers.size() > 1) {
        SDL_GameController* ctrl = controllers[1];
        if (SDL_GameControllerGetButton(ctrl, SDL_CONTROLLER_BUTTON_A)) p2_buttons |= (1 << Input::BUTTON_A);
        if (SDL_GameControllerGetButton(ctrl, SDL_CONTROLLER_BUTTON_B)) p2_buttons |= (1 << Input::BUTTON_B);
        if (SDL_GameControllerGetButton(ctrl, SDL_CONTROLLER_BUTTON_X)) p2_buttons |= (1 << Input::BUTTON_B);
        if (SDL_GameControllerGetButton(ctrl, SDL_CONTROLLER_BUTTON_Y)) p2_buttons |= (1 << Input::BUTTON_A);
        if (SDL_GameControllerGetButton(ctrl, SDL_CONTROLLER_BUTTON_START)) p2_buttons |= (1 << Input::BUTTON_START);
        if (SDL_GameControllerGetButton(ctrl, SDL_CONTROLLER_BUTTON_BACK))  p2_buttons |= (1 << Input::BUTTON_SELECT);
        
        if (SDL_GameControllerGetButton(ctrl, SDL_CONTROLLER_BUTTON_DPAD_UP))    p2_buttons |= (1 << Input::BUTTON_UP);
        if (SDL_GameControllerGetButton(ctrl, SDL_CONTROLLER_BUTTON_DPAD_DOWN))  p2_buttons |= (1 << Input::BUTTON_DOWN);
        if (SDL_GameControllerGetButton(ctrl, SDL_CONTROLLER_BUTTON_DPAD_LEFT))  p2_buttons |= (1 << Input::BUTTON_LEFT);
        if (SDL_GameControllerGetButton(ctrl, SDL_CONTROLLER_BUTTON_DPAD_RIGHT)) p2_buttons |= (1 << Input::BUTTON_RIGHT);

        int16_t axisX = SDL_GameControllerGetAxis(ctrl, SDL_CONTROLLER_AXIS_LEFTX);
        int16_t axisY = SDL_GameControllerGetAxis(ctrl, SDL_CONTROLLER_AXIS_LEFTY);
        const int DEADZONE = 8000;
        if (axisY < -DEADZONE) p2_buttons |= (1 << Input::BUTTON_UP);
        if (axisY > DEADZONE)  p2_buttons |= (1 << Input::BUTTON_DOWN);
        if (axisX < -DEADZONE) p2_buttons |= (1 << Input::BUTTON_LEFT);
        if (axisX > DEADZONE)  p2_buttons |= (1 << Input::BUTTON_RIGHT);
    }

    emu.set_controller(1, p2_buttons);
    
    // Record Inputs
    recorder.record_frame(p1_buttons, p2_buttons);
}

// Scene enum moved to systems/Scene.h

// Redundant HomePanel enum and home_active_panel moved to HomeScene.h

// QuickBall Class
struct QuickBall {
    int x, y, r;
    bool expanded;
    
    struct Item {
        int x, y, r;
        int id; // 0: Share, 1: Snapshot, 3: Home, 4: Timer, 10: Games, 11: Replays, 12: Duo
    };
    std::vector<Item> items;

    void init(int start_x, int start_y) {
        set_pos(start_x, start_y);
        set_layout_normal();
        expanded = false;
    }

    void set_pos(int start_x, int start_y) {
        x = start_x; y = start_y; r = 25;
    }

    void set_layout_normal() {
        items.clear();
        // 4 Items Arc Layout (Reordered to match Home style)
        // 1. Home (Far Left) - ID 3
        items.push_back({x - 60, y - 10, 20, 3});
        // 2. Timer (Mid Left) - ID 4
        items.push_back({x - 25, y - 60, 20, 4});
        // 3. Snapshot (Mid Right) - ID 1
        items.push_back({x + 25, y - 60, 20, 1});
        // 4. Share (Far Right) - ID 0
        items.push_back({x + 60, y - 10, 20, 0});
    }

    void set_layout_replay() {
        items.clear();
        // Expand Upwards (Vertical Stack)
        // 1. Snapshot (id 1) - Bottom
        items.push_back({x, y - 60, 20, 1});
        // 2. Home (id 3) - Above Snapshot
        items.push_back({x, y - 110, 20, 3});
        // 4. Timer (id 4)
        items.push_back({x, y - 210, 20, 4});
        // 5. Share (id 0)
        items.push_back({x, y - 260, 20, 0});
    }

    void set_layout_home() {
        items.clear();
        // 4 Items Arc Layout (Reordered)
        // 1. Games (Far Left) - ID 10
        items.push_back({x - 60, y - 10, 20, 10});
        // 2. Duo (Mid Left) - ID 12
        items.push_back({x - 25, y - 60, 20, 12});
        // 3. Settings (Mid Right) - ID 13
        items.push_back({x + 25, y - 60, 20, 13});
        // 4. Replays (Far Right) - ID 11
        items.push_back({x + 60, y - 10, 20, 11});
    }

    bool handle_event(const SDL_Event& e, Scene& scene, Emulator& emu, int& home_active_panel, HomeScene& homeScene) {
        if (e.type == SDL_MOUSEBUTTONDOWN) {
            int mx = e.button.x;
            int my = e.button.y;
            // std::cout << "DEBUG: QuickBall click at " << mx << "," << my << " | QB at " << x << "," << y << std::endl;
            int dx = mx - x; int dy = my - y;
            if (dx*dx + dy*dy <= r*r) {
                expanded = !expanded;
                std::cout << "DEBUG: QuickBall " << (expanded ? "expanded" : "collapsed") << std::endl;
                return true;
            }
            if (expanded) {
                for (const auto& item : items) {
                    // Skip Timer and Share during replay
                    bool is_replaying = replay_player.is_playing || replay_player.get_current_frame() > 0;
                    if (is_replaying && (item.id == 4 || item.id == 0)) continue;

                    int idx = mx - item.x; int idy = my - item.y;
                    if (idx*idx + idy*idy <= item.r*item.r) {
                        if (item.id == 0) { // Share
                             std::cout << "[QuickBall] Share: Not Implemented" << std::endl;
                        } else if (item.id == 1) { // Snapshot
                             const uint8_t* fb = emu.get_framebuffer();
                             std::vector<uint8_t> temp_fb(256 * 240 * 4);
                             std::memcpy(temp_fb.data(), fb, temp_fb.size());
                             SDL_Surface* ss = SDL_CreateRGBSurfaceWithFormatFrom(temp_fb.data(), 256, 240, 32, 256*4, SDL_PIXELFORMAT_RGBA32);
                             if (ss) {
                                 fs::path snap_dir = nes::get_app_dir() / "snapshots";
                                 if (!fs::exists(snap_dir)) fs::create_directories(snap_dir);
                                 auto now = std::chrono::system_clock::now();
                                 auto in_time_t = std::chrono::system_clock::to_time_t(now);
                                 std::stringstream ss_name;
                                 ss_name << (snap_dir / "snapshot_").string() << std::put_time(std::localtime(&in_time_t), "%Y%m%d_%H%M%S") << ".bmp";
                                 SDL_SaveBMP(ss, ss_name.str().c_str());
                                 SDL_FreeSurface(ss);
                                 std::cout << "[QuickBall] Snapshot saved: " << ss_name.str() << std::endl;
                             }
                        } else if (item.id == 3) { // Home
                            recorder.stop_recording();
                            scene = SCENE_HOME;
                            set_layout_home();
                        } else if (item.id == 4) { // Timer
                            if (!timer_running) {
                                timer_running = true;
                                timer_start_time = std::chrono::high_resolution_clock::now();
                                timer_show_final = false;
                            } else {
                                timer_running = false;
                                auto now = std::chrono::high_resolution_clock::now();
                                std::chrono::duration<double> elapsed = now - timer_start_time;
                                timer_final_value = elapsed.count();
                                timer_show_final = true;
                                timer_final_display_start = now;
                            }
                        } else if (item.id == 10) { // Games
                            scene = SCENE_HOME;
                            home_active_panel = HOME_PANEL_ROM_GRID;
                        } else if (item.id == 11) { // Replays
                            scene = SCENE_HOME;
                            homeScene.replay_files = scan_replay_files();
                            home_active_panel = HOME_PANEL_LIBRARY;
                        } else if (item.id == 12) { // Duo
                            scene = SCENE_HOME;
                            home_active_panel = HOME_PANEL_FAVORITES;
                        } else if (item.id == 13) { // Settings
                            scene = SCENE_SETTINGS;
                            settings_nickname = config.get_nickname();
                            settings_avatar_path = config.get_avatar_path();
                            settings_recorder_enabled = config.get_gameplay_recorder_enabled();
                            settings_loaded = true;
                            set_layout_home(); // Ensure Home layout
                            // Collapse QuickBall when entering settings
                            expanded = false; 
                        }
                        expanded = false;
                        return true;
                    }
                }
                expanded = false;
                return true;
            }
        }
        return false;
    }

    void render(SDL_Renderer* renderer) {
        if (expanded) {
            // Draw a subtle overlay when expanded
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 40);
            SDL_Rect overlay = {0, 0, SCREEN_WIDTH * SCALE, SCREEN_HEIGHT * SCALE};
            SDL_RenderFillRect(renderer, &overlay);

            for (const auto& item : items) {
                // Skip Timer and Share during replay
                bool is_replaying = replay_player.is_playing || replay_player.get_current_frame() > 0;
                if (is_replaying && (item.id == 4 || item.id == 0)) continue;

                // 1. Shadow
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 30);
                draw_filled_circle(renderer, item.x + 2, item.y + 2, item.r);

                // 2. White Background (Pure white for contrast)
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                draw_filled_circle(renderer, item.x, item.y, item.r);
                
                // 3. Border (Darker gray for definition)
                SDL_SetRenderDrawColor(renderer, 150, 150, 150, 255);
                draw_circle_outline(renderer, item.x, item.y, item.r);
                
                // 4. Icon (Darker, more premium color)
                SDL_SetRenderDrawColor(renderer, 34, 43, 50, 255); // Dark Slate
                if (item.id == 0) { // Share
                    int ix = item.x, iy = item.y;
                    draw_filled_circle(renderer, ix - 6, iy, 4);
                    draw_filled_circle(renderer, ix + 6, iy - 6, 4);
                    draw_filled_circle(renderer, ix + 6, iy + 6, 4);
                    SDL_RenderDrawLine(renderer, ix - 6, iy, ix + 6, iy - 6);
                    SDL_RenderDrawLine(renderer, ix - 6, iy, ix + 6, iy + 6);
                    SDL_RenderDrawLine(renderer, ix - 6, iy + 1, ix + 6, iy - 5);
                    SDL_RenderDrawLine(renderer, ix - 6, iy - 1, ix + 6, iy + 5);
                } else if (item.id == 1) { // Snapshot (Camera)
                    int ix = item.x, iy = item.y;
                    SDL_Rect body = {ix - 8, iy - 5, 16, 11};
                    SDL_RenderFillRect(renderer, &body);
                    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                    draw_filled_circle(renderer, ix, iy + 1, 3);
                    SDL_SetRenderDrawColor(renderer, 34, 43, 50, 255);
                    SDL_Rect top = {ix - 4, iy - 8, 8, 3};
                    SDL_RenderFillRect(renderer, &top);
                } else if (item.id == 3) { // Home
                    int ix = item.x, iy = item.y;
                    draw_filled_triangle(renderer, ix - 10, iy - 1, ix + 10, iy - 1, ix, iy - 10);
                    SDL_Rect body = {ix - 7, iy - 1, 14, 10};
                    SDL_RenderFillRect(renderer, &body);
                    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                    SDL_Rect door = {ix - 2, iy + 3, 4, 6};
                    SDL_RenderFillRect(renderer, &door);
                } else if (item.id == 4) { // Timer (Stopwatch)
                    int ix = item.x, iy = item.y;
                    draw_filled_circle(renderer, ix, iy + 1, 10);
                    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                    draw_filled_circle(renderer, ix, iy + 1, 7);
                    SDL_SetRenderDrawColor(renderer, 34, 43, 50, 255);
                    SDL_RenderDrawLine(renderer, ix, iy + 1, ix, iy - 4);
                    SDL_RenderDrawLine(renderer, ix, iy + 1, ix + 3, iy + 1);
                    SDL_Rect btn = {ix - 2, iy - 11, 4, 3};
                    SDL_RenderFillRect(renderer, &btn);
                } else if (item.id == 10) { // Games (Grid)
                    int ix = item.x, iy = item.y;
                    SDL_Rect r1 = {ix - 7, iy - 7, 6, 6}; SDL_RenderFillRect(renderer, &r1);
                    SDL_Rect r2 = {ix + 1, iy - 7, 6, 6}; SDL_RenderFillRect(renderer, &r2);
                    SDL_Rect r3 = {ix - 7, iy + 1, 6, 6}; SDL_RenderFillRect(renderer, &r3);
                    SDL_Rect r4 = {ix + 1, iy + 1, 6, 6}; SDL_RenderFillRect(renderer, &r4);
                } else if (item.id == 11) { // Replays (Play Button)
                    int ix = item.x, iy = item.y;
                    draw_filled_triangle(renderer, ix - 5, iy - 8, ix - 5, iy + 8, ix + 7, iy);
                } else if (item.id == 12) { // Duo (Users)
                    int ix = item.x, iy = item.y;
                    draw_filled_circle(renderer, ix - 4, iy - 3, 4);
                    draw_filled_circle(renderer, ix + 4, iy - 3, 4);
                    SDL_Rect body1 = {ix - 8, iy + 2, 8, 5}; SDL_RenderFillRect(renderer, &body1);
                    SDL_Rect body2 = {ix, iy + 2, 8, 5}; SDL_RenderFillRect(renderer, &body2);
                } else if (item.id == 13) { // Settings (Modern Cog)
                    int ix = item.x, iy = item.y;
                    // Main body
                    draw_filled_circle(renderer, ix, iy, 7);
                    // 6 Rounded teeth for a modern "flower" cog look
                    for (int i = 0; i < 6; i++) {
                        float angle = i * (3.14159f / 3.0f);
                        int tx = ix + (int)(cosf(angle) * 7);
                        int ty = iy + (int)(sinf(angle) * 7);
                        draw_filled_circle(renderer, tx, ty, 3);
                    }
                    // Inner hole
                    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                    draw_filled_circle(renderer, ix, iy, 3);
                    SDL_SetRenderDrawColor(renderer, 34, 43, 50, 255);
                }
            }
        }
        
        // Main Button
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        // Shadow
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 50);
        draw_filled_circle(renderer, x + 3, y + 3, r);
        
        // Outer Circle
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        draw_filled_circle(renderer, x, y, r);
        SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
        draw_circle_outline(renderer, x, y, r);
        
        // Inner Circle (Accent)
        SDL_SetRenderDrawColor(renderer, 34, 43, 50, 220);
        draw_filled_circle(renderer, x, y, r - 10);
        
        // Center Dot
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        draw_filled_circle(renderer, x, y, 4);
        
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    }
};

QuickBall quickBall;



// Forward declarations for panel functions
// Redundant forward declaration removed
void render_rom_grid_panel(SDL_Renderer* renderer, std::vector<Slot>& slots, int scroll_y, FontSystem& font_body);
void handle_rom_grid_events(const SDL_Event& e, std::vector<Slot>& slots, int& scroll_y, int& mouse_down_slot, 
                            bool& showing_delete_popup, bool& showing_context_menu, int& delete_candidate_index,
                            int& context_menu_slot, int& menu_x, int& menu_y, SDL_Renderer* renderer, 
                            const std::string& slots_file, Emulator& emu, Scene& current_scene, ConfigManager& config, Recorder& recorder);

int main(int argc, char* argv[]) {
    // FIX: Set working directory to EXE location as early as possible
    char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);
    std::string::size_type pos = std::string(buffer).find_last_of("\\/");
    if (pos != std::string::npos) {
        std::string exe_dir = std::string(buffer).substr(0, pos);
        SetCurrentDirectoryA(exe_dir.c_str());
        std::cout << "[App] Set working directory to: " << exe_dir << std::endl;
    }

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER) < 0) {
        std::cerr << "SDL Init Failed: " << SDL_GetError() << std::endl;
        return 1;
    }

    SDL_AudioSpec want, have;
    SDL_zero(want);
    want.freq = 44100;
    want.format = AUDIO_F32;
    want.channels = 1;
    want.samples = 2048;
    
    SDL_AudioDeviceID audio_device = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
    if (audio_device != 0) SDL_PauseAudioDevice(audio_device, 0);
    
    // Enable text input for text fields
    SDL_StartTextInput();

    SDL_Window* window = SDL_CreateWindow("Game Enter NES", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH * SCALE, SCREEN_HEIGHT * SCALE, SDL_WINDOW_SHOWN);
    if (!window) return 1;

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) return 1;

    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, SCREEN_WIDTH, SCREEN_HEIGHT);

    // --- Font Setup ---
    FontSystem font_title, font_body, font_small;
    std::string font_path = "C:\\Windows\\Fonts\\arial.ttf";
    
    if (!font_title.init(renderer, font_path, 32.0f)) std::cerr << "Failed to init title font" << std::endl;
    if (!font_body.init(renderer, font_path, 18.0f)) std::cerr << "Failed to init body font" << std::endl;
    if (!font_small.init(renderer, font_path, 14.0f)) std::cerr << "Failed to init small font" << std::endl;

    // Init Discovery
    if (!discovery.init()) {
        std::cerr << "Failed to init discovery" << std::endl;
    }

    // Load Config
    config.load();

    Emulator emu;
    HomeScene homeScene;
    homeScene.init(config.get_nickname()); // Initialize with nickname from config
    LobbyScene lobbyScene;
    SettingsScene settingsScene;
    Scene current_scene = SCENE_HOME;
    
    // Lobby State
    bool lobby_is_host = false;
    bool lobby_player2_connected = false;
    std::string lobby_rom_path = "";
    std::string lobby_rom_name = "";
    std::string lobby_host_name = "";
    
    // Multiplayer Game State
    bool multiplayer_active = false;
    uint32_t multiplayer_frame_id = 0;
    bool multiplayer_paused = false;

    // Slots
    std::vector<Slot> slots(12);
    
    std::filesystem::path data_dir = nes::get_app_dir() / "data";
    if (!std::filesystem::exists(data_dir)) {
        std::filesystem::create_directories(data_dir);
    }
    std::string slots_file = (data_dir / "game_slots.txt").string();
    
    // Load slots đã lưu từ file (nếu có)
    std::vector<SlotManager::Slot> saved_slots;
    if (SlotManager::load_slots(slots_file, saved_slots)) {
        // Convert SlotManager::Slot sang local Slot struct
        for (size_t i = 0; i < saved_slots.size() && i < slots.size(); i++) {
            slots[i].rom_path = saved_slots[i].rom_path;
            slots[i].name = saved_slots[i].name;
            slots[i].occupied = true;
            
            // Load cover image từ saved path hoặc tìm tự động
            std::string cover_path = saved_slots[i].cover_path;
            if (cover_path.empty() || !std::filesystem::exists(cover_path)) {
                // Nếu không có cover path hoặc file không tồn tại, tìm tự động
                cover_path = find_cover_image(saved_slots[i].rom_path);
            }
            
            // Lưu cover_path vào slot
            slots[i].cover_path = cover_path;
            
            if (!cover_path.empty()) {
                slots[i].cover_texture = load_texture(renderer, cover_path);
            }
        }
    }
    
    // Helper lambda to start game
    auto start_game = [&](std::string path) {
        if (emu.load_rom(path.c_str())) {
            emu.reset();
            // Run a few frames to init PPU
            for (int k = 0; k < 10; k++) emu.run_frame();
            // Minimal PPU init hacks (same as before)
            emu.memory_.read(0x2002);
            emu.memory_.write(0x2006, 0x3F); emu.memory_.write(0x2006, 0x00);
            emu.memory_.write(0x2007, 0x0F); emu.memory_.write(0x2007, 0x30);
            emu.memory_.write(0x2007, 0x16); emu.memory_.write(0x2007, 0x27);
            
            replay_player.unload_replay();
            current_scene = SCENE_GAME;
            quickBall.set_layout_normal();
            
            if (config.get_gameplay_recorder_enabled()) {
                std::string rom_name = fs::path(path).filename().string();
                recorder.start_recording(rom_name);
            }
            return true;
        }
        return false;
    };
    
    // --- Hook up HomeScene Callbacks ---
    homeScene.on_start_game = [&](std::string path) {
        return start_game(path);
    };

    homeScene.on_start_replay = [&]() {
        current_scene = SCENE_GAME;
        quickBall.set_layout_normal();
    };

    homeScene.on_create_host = [&](std::string host_name, std::string rom_name, std::string rom_path) {
        std::cout << "🎮 Creating host: " << host_name << std::endl;
        discovery.start_advertising(config.get_device_id(), host_name, rom_name, rom_path, 6503);
        net_manager.start_host(6503);
        
        lobby_is_host = true;
        lobby_player2_connected = false;
        lobby_rom_path = rom_path;
        lobby_rom_name = rom_name;
        lobby_host_name = host_name;
        
        if (emu.load_rom(lobby_rom_path.c_str())) {
            emu.reset();
            std::cout << "✅ ROM loaded, entering lobby..." << std::endl;
            current_scene = SCENE_LOBBY;
        }
    };

    homeScene.on_connect_host = [&](const NetworkDiscovery::Peer& host) {
        std::cout << "🔗 Connecting to host: " << host.username << std::endl;
        lobby_is_host = false;
        lobby_rom_path = host.rom_path;
        lobby_rom_name = host.game_name;
        lobby_host_name = host.username;
        
        net_manager.connect_to(host.ip, host.port);
        
        if (emu.load_rom(lobby_rom_path.c_str())) {
            emu.reset();
            std::cout << "✅ ROM loaded, entering lobby as client..." << std::endl;
            current_scene = SCENE_LOBBY;
            lobby_player2_connected = true; 
        }
    };

    lobbyScene.on_start_multiplayer = [&]() {
        multiplayer_active = true;
        multiplayer_frame_id = 0;
        current_scene = SCENE_GAME;
        quickBall.set_layout_normal();
    };

    // Pre-load if arg provided
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--id" && i + 1 < argc) {
            config.set_device_id(argv[++i]);
            config.set_nickname("Player 2"); 
        } else if (arg.find(".nes") != std::string::npos) {
            // Direct launch from shortcut
            if (start_game(arg)) {
                std::cout << "🚀 Launched directly from shortcut: " << arg << std::endl;
            }
        }
    }

    // Variables moved to HomeScene.h
    // scroll_y, popups, library, duo ...
    
    // --- UI SETUP ---
    VirtualJoystick joystick;
    joystick.init(100, (SCREEN_HEIGHT * SCALE) - 100, 60);

    int center_x = (SCREEN_WIDTH * SCALE) / 2;
    int bottom_y = (SCREEN_HEIGHT * SCALE) - 50;


    quickBall.init(center_x, bottom_y);
    quickBall.set_layout_home(); // Start with Home Layout

    std::vector<VirtualButton> buttons;
    int btn_radius = 35;
    int base_x = (SCREEN_WIDTH * SCALE) - 120;
    int base_y = (SCREEN_HEIGHT * SCALE) - 120;
    int offset = 55;

    VirtualButton btn;
    btn.init(base_x, base_y + offset, btn_radius, BTN_CROSS, Input::BUTTON_B); buttons.push_back(btn);
    btn.init(base_x - offset, base_y, btn_radius, BTN_SQUARE, Input::BUTTON_B); buttons.push_back(btn);
    btn.init(base_x, base_y - offset, btn_radius, BTN_TRIANGLE, Input::BUTTON_A); buttons.push_back(btn);
    btn.init(base_x + offset, base_y, btn_radius, BTN_CIRCLE, Input::BUTTON_A); buttons.push_back(btn);

    VirtualButton btnSelect; btnSelect.init_rect(center_x - 80, bottom_y, 50, 25, BTN_RECT, Input::BUTTON_SELECT); buttons.push_back(btnSelect);
    VirtualButton btnStart; btnStart.init(center_x + 80, bottom_y, 20, BTN_SMALL_TRIANGLE, Input::BUTTON_START); buttons.push_back(btnStart);

    std::vector<SDL_GameController*> connected_controllers;

    bool quit = false;
    SDL_Event e;
    auto fps_timer = std::chrono::high_resolution_clock::now();
    int frame_count = 0;
    
    while (!quit) {
        auto frame_start = std::chrono::high_resolution_clock::now();

        // Ensure we always have an empty slot for "Add ROM" in Home Screen
        if (current_scene == SCENE_HOME) {
             bool full = true;
             for (const auto& s : slots) {
                 if (!s.occupied) {
                     full = false;
                     break;
                 }
             }
             if (full) {
                 slots.resize(slots.size() + 3);
             }
        }
        
        // Poll network connection state in lobby
        if (current_scene == SCENE_LOBBY) {
            if (lobby_is_host) {
                // Host: Check for P2 connection
                if (net_manager.is_connected() && !lobby_player2_connected) {
                    lobby_player2_connected = true;
                    std::cout << "✅ Player 2 connected!" << std::endl;
                }
            } else {
                // Client: Check for START signal from host
                nes::NetworkManager::Packet start_packet;
                if (net_manager.pop_remote_input(start_packet)) {
                    if (start_packet.frame_id == MSG_START_GAME) {
                        // START signal received!
                        std::cout << "🎮 Received START from host, entering game!" << std::endl;
                        multiplayer_active = true;
                        multiplayer_frame_id = 0;
                        current_scene = SCENE_GAME;
                        quickBall.set_layout_normal();
                    }
                }
            }
        }
        
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                recorder.stop_recording();
                quit = true;
            }
            if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) {
                if (current_scene == SCENE_GAME) {
                    recorder.stop_recording();
                    current_scene = SCENE_HOME;
                    quickBall.set_layout_home();
                }
                else {
                    recorder.stop_recording();
                    quit = true;
                }
            }
            if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_r && current_scene == SCENE_GAME) {
                if (!replay_player.is_playing && replay_player.get_current_frame() == 0) {
                    emu.reset();
                }
            }
            
            if (e.type == SDL_CONTROLLERDEVICEADDED) {
                SDL_GameController* c = SDL_GameControllerOpen(e.cdevice.which);
                if (c) connected_controllers.push_back(c);
            }
            if (e.type == SDL_CONTROLLERDEVICEREMOVED) {
                SDL_GameController* c = SDL_GameControllerFromInstanceID(e.cdevice.which);
                if (c) {
                    auto it = std::find(connected_controllers.begin(), connected_controllers.end(), c);
                    if (it != connected_controllers.end()) {
                        connected_controllers.erase(it);
                        SDL_GameControllerClose(c);
                    }
                }
            }
            

            if (current_scene == SCENE_GAME) {
                // Check QuickBall first (only if NOT replaying)
                bool is_replaying_now = replay_player.is_playing || replay_player.get_current_frame() > 0;
                if (!is_replaying_now && quickBall.handle_event(e, current_scene, emu, homeScene.active_panel, homeScene)) {
                    // Event consumed by QuickBall
                } 
                // Check Replay Controls if Replay is active
                else if ((replay_player.is_playing || replay_player.get_current_frame() > 0) && e.type == SDL_MOUSEBUTTONDOWN) {
                    int mx = e.button.x;
                    int my = e.button.y;
                    
                    // Coordinates must match Render logic
                    int btn_radius = 25; 
                    int bottom_y = SCREEN_HEIGHT * SCALE - 50;

                    // Layout Order (Right to Left): [Home] [Play] [Fast] [Slow]
                    int home_x = SCREEN_WIDTH * SCALE - 40;
                    int start_x = home_x - 70; // Play/Pause
                    int ff_x = start_x - 60;   // Fast Forward
                    int rw_x = ff_x - 60;      // Slow Down
                    
                    int home_y = bottom_y;

                    // Helper for circle click
                    auto is_in_circle = [&](int mx, int my, int cx, int cy, int r) {
                        return (mx - cx)*(mx - cx) + (my - cy)*(my - cy) <= r*r;
                    };

                    // Play/Pause
                    if (is_in_circle(mx, my, start_x, bottom_y, btn_radius)) {
                        if (replay_player.is_playing) {
                             replay_player.pause_playback();
                             if (audio_device != 0) SDL_ClearQueuedAudio(audio_device);
                        }
                        else replay_player.resume_playback();
                    }
                    // Fast Forward (>>)
                    else if (is_in_circle(mx, my, ff_x, bottom_y, btn_radius)) {
                        float s = replay_player.playback_speed;
                        if (s < 4.0f) replay_player.set_speed(s * 2.0f);
                    }
                    // Slow Down (<<)
                    else if (is_in_circle(mx, my, rw_x, bottom_y, btn_radius)) {
                        float s = replay_player.playback_speed;
                        if (s > 0.5f) replay_player.set_speed(s / 2.0f);
                    }
                    // Home
                    else if (is_in_circle(mx, my, home_x, home_y, btn_radius)) {
                            replay_player.unload_replay(); // Stop and unload
                            current_scene = SCENE_HOME;
                            homeScene.active_panel = HOME_PANEL_LIBRARY;
                            quickBall.set_layout_home();
                    }
                }
                else if (connected_controllers.empty()) {
                    joystick.handle_event(e);
                    for (auto& b : buttons) b.handle_event(e);
                }
            }

            // Home Screen Interactions
            // Home Screen Interactions
            if (current_scene == SCENE_HOME) {
                 if (quickBall.handle_event(e, current_scene, emu, homeScene.active_panel, homeScene)) {
                     // Event consumed
                 } else {
                     homeScene.handle_event(e, slots, config, discovery, emu, replay_player, SCREEN_WIDTH, SCREEN_HEIGHT, SCALE);
                 }
            }

            if (current_scene == SCENE_LOBBY) {
                 if (quickBall.handle_event(e, current_scene, emu, homeScene.active_panel, homeScene)) {
                     // Event consumed
                 } else {
                     lobbyScene.handle_event(e, lobby_is_host, lobby_player2_connected, current_scene, homeScene.active_panel, discovery, net_manager, SCREEN_WIDTH, SCREEN_HEIGHT, SCALE);
                 }
            }

            if (current_scene == SCENE_SETTINGS) {
                 if (!settings_loaded) {
                     settings_nickname = config.get_nickname();
                     settings_avatar_path = config.get_avatar_path();
                     settings_recorder_enabled = config.get_gameplay_recorder_enabled();
                     settings_loaded = true;
                 }
                 if (quickBall.handle_event(e, current_scene, emu, homeScene.active_panel, homeScene)) {
                     // Event consumed
                 } else {
                     settingsScene.handle_event(e, settings_nickname, settings_avatar_path, settings_recorder_enabled, active_input_field, config, SCREEN_WIDTH, SCREEN_HEIGHT, SCALE, 
                        []() { return open_file_dialog("Image Files\0*.png;*.jpg;*.jpeg;*.bmp\0All Files\0*.*\0"); }, 
                        [](const std::string& p, const std::string& n) { return import_avatar_image(p, n); });
                     
                     // Sync back to homeScene in case nickname was saved
                     homeScene.init(config.get_nickname());
                 }
            } else {
                settings_loaded = false;
            }

        }

        SDL_SetRenderDrawColor(renderer, 240, 240, 240, 255); // White BG
        SDL_RenderClear(renderer);

        // --- Helper: Render Shared Header ---
        auto render_main_header = [&]() {
            int header_h = 100;
            SDL_Rect header_rect = {0, 0, SCREEN_WIDTH * SCALE, header_h};
            SDL_SetRenderDrawColor(renderer, 34, 43, 50, 255);
            SDL_RenderFillRect(renderer, &header_rect);
            font_title.draw_text(renderer, "Game Enter NES", 20, 50, {255, 255, 255, 255});
            std::string status = connected_controllers.empty() ? "No devices connected, play by touch." : "Gamepad connected.";
            font_small.draw_text(renderer, status, 20, 80, {220, 220, 220, 255});
        };

        if (current_scene == SCENE_HOME) {
            homeScene.render(renderer, slots, font_title, font_body, font_small, SCREEN_WIDTH, SCREEN_HEIGHT, SCALE);
            render_main_header();
            quickBall.render(renderer);
        } else if (current_scene == SCENE_LOBBY) {
            lobbyScene.render(renderer, lobby_is_host, lobby_host_name, lobby_rom_name, lobby_player2_connected, font_title, font_body, font_small, SCREEN_WIDTH, SCREEN_HEIGHT, SCALE);
            render_main_header();
            quickBall.render(renderer);
        } else if (current_scene == SCENE_SETTINGS) {
            settingsScene.render(renderer, settings_nickname, settings_avatar_path, settings_recorder_enabled, active_input_field, font_title, font_body, font_small, SCREEN_WIDTH, SCREEN_HEIGHT, SCALE, load_texture);
            render_main_header();
            quickBall.render(renderer);
        } else {
            // --- GAME SCENE ---
            
            // Check if replay has finished and return to Library
            if (replay_player.replay_finished) {
                std::cout << "🔙 Replay finished, returning to Library" << std::endl;
                current_scene = SCENE_HOME;
                homeScene.active_panel = HOME_PANEL_LIBRARY;
                quickBall.set_layout_home(); // Reset QuickBall layout
                replay_player.replay_finished = false;  // Reset flag
                continue;  // Skip this frame and go to next iteration
            }
            
            const Uint8* currentKeyStates = SDL_GetKeyboardState(NULL);
            // handle_input(emu, currentKeyStates, joystick, buttons, connected_controllers); // REMOVED from here
            
            // Handle Playback Speed Logic
            // Handle Playback Speed Logic
            bool emulator_ran = false;

            // Handle Playback Speed Logic
            if (replay_player.is_playing) {
                // Only run replay if QuickBall is NOT expanded
                if (!quickBall.expanded) {
                    float speed = replay_player.playback_speed;
                    replay_player.speed_accumulator += speed;
                    
                    int frames_to_run = 0;
                    while (replay_player.speed_accumulator >= 1.0f) {
                        frames_to_run++;
                        replay_player.speed_accumulator -= 1.0f;
                    }
                    
                    for (int k = 0; k < frames_to_run; k++) {
                        // Update inputs for this specific frame from replay
                        handle_input(emu, currentKeyStates, joystick, buttons, connected_controllers);
                        
                        // If replay finishes mid-loop, stop
                        if (!replay_player.is_playing) break; 

                        emu.run_frame();
                        emulator_ran = true;
                    }
                }
            } else {
                // Normal gameplay or Paused Replay
                // Check if we are in "Paused Replay" mode
                // If frames are loaded but not playing, it's a paused replay.
                bool is_replay_paused = !replay_player.frames.empty();
                
                if (!is_replay_paused) {
                     // Handle Input
                     handle_input(emu, currentKeyStates, joystick, buttons, connected_controllers);
                     
                     if (multiplayer_active && net_manager.is_connected()) {
                         // Multiplayer Mode: Lockstep synchronization
                         
                         // Save local input BEFORE processing remote (for Client to send as P2)
                         uint8_t local_input = emu.get_controller_state(0);
                         
                         // Client: Clear P1 immediately (P1 will be set from Host's input only)
                         if (!lobby_is_host) {
                             emu.set_controller(0, 0); // Clear P1, will be overwritten by Host's input
                         }
                         
                         // Process remote input
                         nes::NetworkManager::Packet remote_packet;
                         while (net_manager.pop_remote_input(remote_packet)) {
                             if (lobby_is_host) {
                                 // Host receives Client's input as P2
                                 uint8_t p2_input = remote_packet.input_state;
                                 emu.set_controller(1, p2_input);
                             } else {
                                 // Client receives Host's input as P1
                                 uint8_t p1_input = remote_packet.input_state;
                                 emu.set_controller(0, p1_input);
                             }
                         }
                         
                         // Apply local input to correct controller
                         if (!lobby_is_host) {
                             // Client: Apply own input to P2
                             emu.set_controller(1, local_input);
                         }
                         // Host: P1 already set by handle_input(), no need to reapply
                         
                         // Send local input
                         net_manager.send_input(multiplayer_frame_id, local_input);
                         
                         // Run frame
                         emu.run_frame();
                         emulator_ran = true;
                         multiplayer_frame_id++;
                     } else {
                         // Single player mode
                         emu.run_frame();
                         emulator_ran = true;
                     }
                }
                // If Paused Replay: Do nothing (freeze state)
            }
            
            const uint8_t* framebuffer = emu.get_framebuffer();
            SDL_UpdateTexture(texture, NULL, framebuffer, SCREEN_WIDTH * 4);
            SDL_RenderCopy(renderer, texture, NULL, NULL);
            
            bool is_replaying = replay_player.is_playing || replay_player.get_current_frame() > 0;
            
            // Only update QuickBall position/layout if NOT replaying
            if (!is_replaying) {
                // Ensure layout is Normal/Home
                if (current_scene == SCENE_HOME) quickBall.set_layout_home();
                else quickBall.set_layout_normal();
                
                quickBall.set_pos(center_x, bottom_y);
            }

            if (connected_controllers.empty() && !is_replaying) {
                joystick.render(renderer);
                // Render buttons, but hide Select and Start for client in multiplayer
                for (auto& b : buttons) {
                    // Skip Select and Start buttons if client in multiplayer (only Host controls these)
                    if (multiplayer_active && !lobby_is_host) {
                        if (b.nes_button_mapping == Input::BUTTON_SELECT || 
                            b.nes_button_mapping == Input::BUTTON_START) {
                            continue; // Skip rendering Select and Start
                        }
                    }
                    b.render(renderer);
                }
            }
            
            // Hide QuickBall in Replay
            if (!is_replaying) {
                quickBall.render(renderer);
            }
            
            // --- REPLAY PLAYBACK UI ---
            if (replay_player.is_playing || replay_player.get_current_frame() > 0) {
                // Coordinates
                int btn_radius = 25;
                int bottom_y = SCREEN_HEIGHT * SCALE - 50;
                
                // Layout Order (Right to Left): [Home] [Play] [Fast] [Slow]
                int home_x = SCREEN_WIDTH * SCALE - 40;
                int start_x = home_x - 70; // Play/Pause
                int ff_x = start_x - 60;   // Fast Forward
                int rw_x = ff_x - 60;      // Slow Down
                
                int home_y = bottom_y;

                // Replay name
                std::string status = replay_player.is_playing ? "Playing: " : "Paused: ";
                status += replay_player.replay_name;
                font_small.draw_text(renderer, status, 20, 20, {255, 255, 255, 255});

                // --- Top Right Buttons (Moved to Bottom Right) ---
                int tr_radius = 25;

                // Home Button
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 200);
                draw_filled_circle_aa(renderer, home_x, home_y, tr_radius);
                SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
                draw_circle_outline_aa(renderer, home_x, home_y, tr_radius);
                // Home Icon (House)
                SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
                draw_filled_triangle(renderer, home_x, home_y - 8, home_x - 8, home_y, home_x + 8, home_y); // Roof
                SDL_Rect h_base = {home_x - 5, home_y, 10, 8}; SDL_RenderFillRect(renderer, &h_base); // Base

                // --- Playback Controls ---

                // 1. Play/Pause Button
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 200);
                draw_filled_circle_aa(renderer, start_x, bottom_y, btn_radius);
                SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
                draw_circle_outline_aa(renderer, start_x, bottom_y, btn_radius);
                
                // Icon
                SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
                if (replay_player.is_playing) {
                    // Pause Icon (||)
                    SDL_Rect p1 = {start_x - 5, bottom_y - 8, 4, 16};
                    SDL_Rect p2 = {start_x + 1, bottom_y - 8, 4, 16};
                    SDL_RenderFillRect(renderer, &p1);
                    SDL_RenderFillRect(renderer, &p2);
                } else {
                    // Play Icon (Triangle)
                    int tx1 = start_x - 3, ty1 = bottom_y - 8;
                    int tx2 = start_x + 7, ty2 = bottom_y;
                    int tx3 = start_x - 3, ty3 = bottom_y + 8;
                    draw_filled_triangle(renderer, tx1, ty1, tx2, ty2, tx3, ty3);
                }

                // 2. Fast Forward (>>)
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 200);
                draw_filled_circle_aa(renderer, ff_x, bottom_y, btn_radius);
                SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
                draw_circle_outline_aa(renderer, ff_x, bottom_y, btn_radius);
                
                // Icon (>>)
                SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
                draw_filled_triangle(renderer, ff_x - 6, bottom_y - 6, ff_x + 2, bottom_y, ff_x - 6, bottom_y + 6);
                draw_filled_triangle(renderer, ff_x + 2, bottom_y - 6, ff_x + 10, bottom_y, ff_x + 2, bottom_y + 6);

                // 3. Slow Down (<<)
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 200);
                draw_filled_circle_aa(renderer, rw_x, bottom_y, btn_radius);
                SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
                draw_circle_outline_aa(renderer, rw_x, bottom_y, btn_radius);

                // Icon (<<)
                SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
                draw_filled_triangle(renderer, rw_x + 6, bottom_y - 6, rw_x - 2, bottom_y, rw_x + 6, bottom_y + 6);
                draw_filled_triangle(renderer, rw_x - 2, bottom_y - 6, rw_x - 10, bottom_y, rw_x - 2, bottom_y + 6);

                // Time Display (Bottom Left)
                int cur_seconds = (int)(replay_player.get_current_frame() / 60.0f);
                int tot_seconds = (int)(replay_player.get_total_frames() / 60.0f);
                
                std::stringstream time_ss;
                time_ss << std::setfill('0') << std::setw(2) << (cur_seconds / 60) << ":" 
                        << std::setw(2) << (cur_seconds % 60) << " / "
                        << std::setw(2) << (tot_seconds / 60) << ":" 
                        << std::setw(2) << (tot_seconds % 60);
                
                // Progress Bar (Extended to left, lowered)
                int bar_start_x = 30; // Start from left
                int bar_end_x = rw_x - 30; // Before Slow Down Button
                int bar_w = bar_end_x - bar_start_x;
                int bar_y = bottom_y + 10; // Lowered
                int bar_h = 8;

                // Time Display (Above Progress Bar - Left)
                font_small.draw_text(renderer, time_ss.str(), bar_start_x, bar_y - 15, {255, 255, 255, 255});

                // Speed Display (Above Progress Bar - Right)
                std::stringstream speed_ss;
                speed_ss << replay_player.playback_speed << "x";
                // Align right: bar_end_x - approx width (30px)
                font_small.draw_text(renderer, speed_ss.str(), bar_end_x - 30, bar_y - 15, {255, 255, 255, 255});
                
                if (bar_w > 0) {
                    // Background
                    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 150);
                    SDL_Rect bar_bg = {bar_start_x - 2, bar_y - 2, bar_w + 4, bar_h + 4};
                    SDL_RenderFillRect(renderer, &bar_bg);
                    
                    // Progress
                    float progress = replay_player.get_progress();
                    int progress_w = (int)(bar_w * progress);
                    SDL_SetRenderDrawColor(renderer, 50, 150, 250, 200);
                    SDL_Rect bar_progress = {bar_start_x, bar_y, progress_w, bar_h};
                    SDL_RenderFillRect(renderer, &bar_progress);
                    
                    // Border
                    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 200);
                    SDL_RenderDrawRect(renderer, &bar_bg);
                    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
                }

                // Handle Clicks for Controls
                // We need to check mouse state here or in handle_event. 
                // Since we are in the render loop, it's better to check 'e' from the event loop, but 'e' is not available here easily without restructuring.
                // However, we can check mouse state directly for simple UI or better, move this logic to handle_input/event loop.
                // But to keep it simple and localized, let's use the mouse_down_slot logic or similar.
                // Actually, we should handle this in the event loop.
                // Let's modify the event loop to check these coordinates.

            }
            
            if (audio_device != 0 && emulator_ran) {
                const std::vector<float>& samples = emu.get_audio_samples();
                if (!samples.empty()) SDL_QueueAudio(audio_device, samples.data(), samples.size() * sizeof(float));
            }

            // --- TIMER OVERLAY ---
            if (timer_running || timer_show_final) {
                auto now = std::chrono::high_resolution_clock::now();
                double current_val = 0.0;
                
                if (timer_running) {
                    std::chrono::duration<double> elapsed = now - timer_start_time;
                    current_val = elapsed.count();
                } else {
                    current_val = timer_final_value;
                }

                // Format: MM:SS.mmm
                int minutes = (int)(current_val / 60);
                int seconds = (int)current_val % 60;
                int millis = (int)((current_val - (int)current_val) * 1000);
                
                std::stringstream ss;
                ss << std::setfill('0') << std::setw(2) << minutes << ":"
                   << std::setw(2) << seconds << "."
                   << std::setw(3) << millis;
                std::string time_str = ss.str();

                if (timer_running) {
                    // Top Right, Red
                    float w = font_title.get_text_width(time_str);
                    font_title.draw_text(renderer, time_str, SCREEN_WIDTH * SCALE - w - 20, 20, {255, 50, 50, 255});
                } else if (timer_show_final) {
                    std::chrono::duration<double> display_elapsed = now - timer_final_display_start;
                    if (display_elapsed.count() < 3.0) {
                        // Center, Large
                        float w = font_title.get_text_width(time_str);
                        int cx = (SCREEN_WIDTH * SCALE) / 2;
                        int cy = (SCREEN_HEIGHT * SCALE) / 2;
                        
                        // Background box
                        SDL_Rect bg = {cx - (int)w/2 - 20, cy - 25, (int)w + 40, 50};
                        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 200);
                        SDL_RenderFillRect(renderer, &bg);
                        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

                        font_title.draw_text(renderer, time_str, cx - w/2, cy + 10, {255, 255, 255, 255});
                    } else {
                        timer_show_final = false;
                    }
                }
            }
        }
        
        SDL_RenderPresent(renderer);
        
        auto current_time = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> elapsed_ms = current_time - frame_start;
        if (elapsed_ms.count() < 16.667) SDL_Delay((Uint32)(16.667 - elapsed_ms.count()));
        
        frame_count++;
        std::chrono::duration<double> elapsed_sec = current_time - fps_timer;
        if (elapsed_sec.count() >= 1.0) {
            std::string title = "Game Enter NES";
            if (current_scene == SCENE_GAME) title += " - Running";
            SDL_SetWindowTitle(window, title.c_str());
            frame_count = 0;
            fps_timer = current_time;
        }
    }

    // Save slots trước khi thoát
    std::vector<SlotManager::Slot> slots_to_save;
    for (const auto& slot : slots) {
        if (slot.occupied) {
            slots_to_save.push_back(SlotManager::Slot(slot.rom_path, slot.name, slot.cover_path));
        }
    }
    SlotManager::save_slots(slots_file, slots_to_save);

    font_title.cleanup();
    font_body.cleanup();
    font_small.cleanup();
    for (auto c : connected_controllers) SDL_GameControllerClose(c);
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    if (audio_device != 0) SDL_CloseAudioDevice(audio_device);
    discovery.shutdown();
    SDL_Quit();

    return 0;
}
