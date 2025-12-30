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

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "slot_manager.h"
#include "../core/network/network_manager.h"
#include "../core/network/network_discovery.h"
#include "../core/config/config_manager.h"

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

// --- Global Replay Instances ---
Recorder recorder;
ReplayPlayer replay_player;



// --- Font System (stb_truetype) ---

// --- Global Font Instances ---
FontSystem font_small;
FontSystem font_body;
FontSystem font_title;


// Helper to load texture from file
SDL_Texture* load_texture(SDL_Renderer* renderer, const std::string& path) {
    int w, h, comp;
    unsigned char* data = stbi_load(path.c_str(), &w, &h, &comp, 4); // Force 4 channels (RGBA)
    if (!data) return nullptr;

    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC, w, h);
    if (texture) {
        SDL_UpdateTexture(texture, NULL, data, w * 4);
        SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    }
    stbi_image_free(data);
    return texture;
}

// Helper to find cover image with fallback logic
std::string find_cover_image(const std::string& rom_path) {
    std::string base_path = rom_path.substr(0, rom_path.find_last_of("/\\") + 1);
    std::string full_filename = rom_path.substr(rom_path.find_last_of("/\\") + 1);
    std::string name_no_ext = full_filename.substr(0, full_filename.find_last_of("."));
    
    std::vector<std::string> search_paths;
    search_paths.push_back(base_path); // ROM folder
    search_paths.push_back("images/"); // Images folder
    
    std::string search_name = name_no_ext;
    
    while (true) {
        for (const auto& path : search_paths) {
            // Check PNG
            std::string png_path = path + search_name + ".png";
            std::ifstream f_png(png_path);
            if (f_png.good()) return png_path;

            // Check JPG
            std::string jpg_path = path + search_name + ".jpg";
            std::ifstream f_jpg(jpg_path);
            if (f_jpg.good()) return jpg_path;
        }

        // Strip last word for next iteration
        size_t last_space = search_name.find_last_of(" ");
        if (last_space == std::string::npos) break; // No more spaces, stop
        
        search_name = search_name.substr(0, last_space);
    }
    
    return "";
}

// Helper to draw filled circle with Anti-Aliasing
void draw_filled_circle(SDL_Renderer* renderer, int cx, int cy, int radius) {
    Uint8 r, g, b, a;
    SDL_GetRenderDrawColor(renderer, &r, &g, &b, &a);

    for (int dy = -radius; dy <= radius; dy++) {
        for (int dx = -radius; dx <= radius; dx++) {
            int dist_sq = dx * dx + dy * dy;
            if (dist_sq <= (radius * radius)) {
                float dist = std::sqrt((float)dist_sq);
                if (dist > radius - 1.0f) {
                    float alpha_factor = radius - dist;
                    SDL_SetRenderDrawColor(renderer, r, g, b, (Uint8)(a * alpha_factor));
                    SDL_RenderDrawPoint(renderer, cx + dx, cy + dy);
                    SDL_SetRenderDrawColor(renderer, r, g, b, a);
                } else {
                    SDL_RenderDrawPoint(renderer, cx + dx, cy + dy);
                }
            }
        }
    }
}

// Helper to draw filled circle with Anti-Aliasing (Improved)
void draw_filled_circle_aa(SDL_Renderer* renderer, int cx, int cy, int radius) {
    if (radius <= 0) return;
    Uint8 r, g, b, a;
    SDL_GetRenderDrawColor(renderer, &r, &g, &b, &a);

    SDL_BlendMode old_mode;
    SDL_GetRenderDrawBlendMode(renderer, &old_mode);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    for (int dy = -radius - 1; dy <= radius + 1; dy++) {
        for (int dx = -radius - 1; dx <= radius + 1; dx++) {
            float dist = std::sqrt((float)(dx * dx + dy * dy));
            if (dist <= radius + 1.0f) {
                float alpha_factor = std::clamp(radius + 0.5f - dist, 0.0f, 1.0f);
                if (alpha_factor > 0.0f) {
                    SDL_SetRenderDrawColor(renderer, r, g, b, (Uint8)(a * alpha_factor));
                    SDL_RenderDrawPoint(renderer, cx + dx, cy + dy);
                }
            }
        }
    }

    SDL_SetRenderDrawBlendMode(renderer, old_mode);
    SDL_SetRenderDrawColor(renderer, r, g, b, a);
}

// Helper to draw circle outline with Anti-Aliasing
void draw_circle_outline_aa(SDL_Renderer* renderer, int cx, int cy, int radius) {
    Uint8 r, g, b, a;
    SDL_GetRenderDrawColor(renderer, &r, &g, &b, &a);

    SDL_BlendMode old_mode;
    SDL_GetRenderDrawBlendMode(renderer, &old_mode);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    for (int dy = -radius - 1; dy <= radius + 1; dy++) {
        for (int dx = -radius - 1; dx <= radius + 1; dx++) {
            float dist = std::sqrt((float)(dx * dx + dy * dy));
            float diff = std::abs(dist - radius);
            if (diff < 1.0f) {
                float alpha_factor = 1.0f - diff;
                SDL_SetRenderDrawColor(renderer, r, g, b, (Uint8)(a * alpha_factor));
                SDL_RenderDrawPoint(renderer, cx + dx, cy + dy);
            }
        }
    }
    SDL_SetRenderDrawBlendMode(renderer, old_mode);
    SDL_SetRenderDrawColor(renderer, r, g, b, a);
}

// Helper to draw circle outline
void draw_circle_outline(SDL_Renderer* renderer, int cx, int cy, int radius) {
    Uint8 r, g, b, a;
    SDL_GetRenderDrawColor(renderer, &r, &g, &b, &a);

    for (int dy = -radius - 1; dy <= radius + 1; dy++) {
        for (int dx = -radius - 1; dx <= radius + 1; dx++) {
            float dist = std::sqrt((float)(dx * dx + dy * dy));
            float diff = std::abs(dist - radius);
            if (diff < 1.0f) {
                float alpha_factor = 1.0f - diff;
                SDL_SetRenderDrawColor(renderer, r, g, b, (Uint8)(a * alpha_factor));
                SDL_RenderDrawPoint(renderer, cx + dx, cy + dy);
                SDL_SetRenderDrawColor(renderer, r, g, b, a);
            }
        }
    }
}

// Helper to draw filled triangle with Anti-Aliasing
void draw_filled_triangle(SDL_Renderer* renderer, int x1, int y1, int x2, int y2, int x3, int y3) {
    // Ensure Counter-Clockwise winding
    if ((x2 - x1) * (y3 - y1) - (y2 - y1) * (x3 - x1) < 0) {
        std::swap(x2, x3);
        std::swap(y2, y3);
    }

    // Bounding box
    int minx = std::min({x1, x2, x3});
    int maxx = std::max({x1, x2, x3});
    int miny = std::min({y1, y2, y3});
    int maxy = std::max({y1, y2, y3});

    // Pad for AA
    minx -= 1; miny -= 1; maxx += 1; maxy += 1;

    // Current Color
    Uint8 r, g, b, a;
    SDL_GetRenderDrawColor(renderer, &r, &g, &b, &a);

    // Edge constants
    float A0 = (float)(y2 - y3); float B0 = (float)(x3 - x2); float C0 = -A0*x2 - B0*y2;
    float A1 = (float)(y3 - y1); float B1 = (float)(x1 - x3); float C1 = -A1*x3 - B1*y3;
    float A2 = (float)(y1 - y2); float B2 = (float)(x2 - x1); float C2 = -A2*x1 - B2*y1;

    // Normalization factors (1 / length of normal)
    float invLen0 = 1.0f / std::hypot(A0, B0);
    float invLen1 = 1.0f / std::hypot(A1, B1);
    float invLen2 = 1.0f / std::hypot(A2, B2);

    for (int y = miny; y <= maxy; y++) {
        for (int x = minx; x <= maxx; x++) {
            float px = x + 0.5f;
            float py = y + 0.5f;

            // Signed distances to edges
            float d0 = (A0 * px + B0 * py + C0) * invLen0;
            float d1 = (A1 * px + B1 * py + C1) * invLen1;
            float d2 = (A2 * px + B2 * py + C2) * invLen2;

            // Distance to the shape (min of distances to edges)
            float min_dist = std::min({d0, d1, d2});

            // Alpha calculation: 0.5 + dist. 
            // Inside: dist > 0. On edge: dist = 0 (alpha 0.5). Outside: dist < 0.
            float alpha_factor = std::clamp(0.5f + min_dist, 0.0f, 1.0f);

            if (alpha_factor > 0.0f) {
                SDL_SetRenderDrawColor(renderer, r, g, b, (Uint8)(a * alpha_factor));
                SDL_RenderDrawPoint(renderer, x, y);
            }
        }
    }
    // Restore color
    SDL_SetRenderDrawColor(renderer, r, g, b, a);
}

// Helper to draw NES Cartridge Icon
void draw_nes_cartridge(SDL_Renderer* renderer, int x, int y, int scale) {
    // Colors
    SDL_Color body_col = {120, 144, 156, 255}; // Blue Grey 400
    SDL_Color dark_col = {55, 71, 79, 255};    // Blue Grey 800
    SDL_Color label_col = {207, 216, 220, 255};// Blue Grey 100
    SDL_Color accent_col = {180, 20, 20, 255}; // Red Accent

    int w = 14 * scale;
    int h = 16 * scale;

    // 1. Main Body
    SDL_SetRenderDrawColor(renderer, body_col.r, body_col.g, body_col.b, body_col.a);
    SDL_Rect body = {x - w/2, y - h/2, w, h};
    SDL_RenderFillRect(renderer, &body);

    // 2. Grip Lines (Top)
    SDL_SetRenderDrawColor(renderer, dark_col.r, dark_col.g, dark_col.b, dark_col.a);
    for(int i=0; i<3; i++) {
        SDL_Rect grip = {x - w/2 + 2*scale + i*4*scale, y - h/2, 2*scale, 3*scale};
        SDL_RenderFillRect(renderer, &grip);
    }

    // 3. Label Area
    SDL_SetRenderDrawColor(renderer, label_col.r, label_col.g, label_col.b, label_col.a);
    SDL_Rect label = {x - w/2 + 2*scale, y - h/2 + 4*scale, w - 4*scale, h - 6*scale};
    SDL_RenderFillRect(renderer, &label);

    // 4. Label Accent (Red Stripe)
    SDL_SetRenderDrawColor(renderer, accent_col.r, accent_col.g, accent_col.b, accent_col.a);
    SDL_Rect stripe = {x - w/2 + 2*scale, y - h/2 + 5*scale, w - 4*scale, 2*scale};
    SDL_RenderFillRect(renderer, &stripe);

    // 5. Connector (Bottom)
    SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
    SDL_Rect conn = {x - w/2 + 1*scale, y + h/2 - 1*scale, w - 2*scale, 1*scale};
    SDL_RenderFillRect(renderer, &conn);
}



// Virtual Joystick Class
struct VirtualJoystick {
    int base_x, base_y;
    int base_radius;
    int knob_x, knob_y;
    int knob_radius;
    bool active;
    bool up, down, left, right;

    void init(int x, int y, int r) {
        base_x = x;
        base_y = y;
        base_radius = r;
        knob_x = x;
        knob_y = y;
        knob_radius = r / 3;
        active = false;
        up = down = left = right = false;
    }

    void handle_event(const SDL_Event& e) {
        if (e.type == SDL_MOUSEBUTTONDOWN) {
            int dx = e.button.x - base_x;
            int dy = e.button.y - base_y;
            if (dx*dx + dy*dy < (base_radius * 1.5) * (base_radius * 1.5)) {
                active = true;
                update_knob(e.button.x, e.button.y);
            }
        } else if (e.type == SDL_MOUSEBUTTONUP) {
            active = false;
            reset_knob();
        } else if (e.type == SDL_MOUSEMOTION && active) {
            update_knob(e.motion.x, e.motion.y);
        }
    }

    void update_knob(int target_x, int target_y) {
        int dx = target_x - base_x;
        int dy = target_y - base_y;
        double distance = std::sqrt(dx*dx + dy*dy);
        
        if (distance > base_radius) {
            double angle = std::atan2(dy, dx);
            knob_x = base_x + (int)(std::cos(angle) * base_radius);
            knob_y = base_y + (int)(std::sin(angle) * base_radius);
        } else {
            knob_x = target_x;
            knob_y = target_y;
        }

        up = down = left = right = false;
        if (distance > base_radius * 0.2) { 
            double angle = std::atan2(dy, dx) * 180 / M_PI;
            if (angle < 0) angle += 360;

            if (angle >= 337.5 || angle < 22.5) right = true;
            else if (angle >= 22.5 && angle < 67.5) { right = true; down = true; }
            else if (angle >= 67.5 && angle < 112.5) down = true;
            else if (angle >= 112.5 && angle < 157.5) { down = true; left = true; }
            else if (angle >= 157.5 && angle < 202.5) left = true;
            else if (angle >= 202.5 && angle < 247.5) { left = true; up = true; }
            else if (angle >= 247.5 && angle < 292.5) up = true;
            else if (angle >= 292.5 && angle < 337.5) { up = true; right = true; }
        }
    }

    void reset_knob() {
        knob_x = base_x;
        knob_y = base_y;
        up = down = left = right = false;
    }

    void render(SDL_Renderer* renderer) {
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 50, 50, 50, 128); 
        draw_filled_circle(renderer, base_x, base_y, base_radius);
        
        SDL_SetRenderDrawColor(renderer, 200, 200, 200, 100);
        draw_circle_outline(renderer, base_x, base_y, base_radius);

        SDL_SetRenderDrawColor(renderer, 150, 150, 150, 150);
        int arrow_dist = base_radius - 15;
        int arrow_size = 8;

        draw_filled_triangle(renderer, base_x, base_y - arrow_dist - arrow_size, base_x - arrow_size, base_y - arrow_dist, base_x + arrow_size, base_y - arrow_dist);
        draw_filled_triangle(renderer, base_x, base_y + arrow_dist + arrow_size, base_x - arrow_size, base_y + arrow_dist, base_x + arrow_size, base_y + arrow_dist);
        draw_filled_triangle(renderer, base_x - arrow_dist - arrow_size, base_y, base_x - arrow_dist, base_y - arrow_size, base_x - arrow_dist, base_y + arrow_size);
        draw_filled_triangle(renderer, base_x + arrow_dist + arrow_size, base_y, base_x + arrow_dist, base_y - arrow_size, base_x + arrow_dist, base_y + arrow_size);

        SDL_SetRenderDrawColor(renderer, 100, 150, 255, 180);
        draw_filled_circle(renderer, knob_x, knob_y, knob_radius);
    }
};

// Virtual Button Class
enum ButtonType { BTN_SQUARE, BTN_TRIANGLE, BTN_CIRCLE, BTN_CROSS, BTN_RECT, BTN_SMALL_TRIANGLE };

struct VirtualButton {
    int x, y, width, height;
    ButtonType type;
    bool pressed;
    int nes_button_mapping; 

    void init(int cx, int cy, int size, ButtonType t, int mapping) {
        x = cx; y = cy; width = size; height = size; type = t; pressed = false; nes_button_mapping = mapping;
    }
    
    void init_rect(int cx, int cy, int w, int h, ButtonType t, int mapping) {
        x = cx; y = cy; width = w; height = h; type = t; pressed = false; nes_button_mapping = mapping;
    }

    void handle_event(const SDL_Event& e) {
        if (e.type == SDL_MOUSEBUTTONDOWN) {
            int mx = e.button.x;
            int my = e.button.y;
            bool hit = false;
            
            if (type == BTN_RECT) {
                if (mx >= x - width/2 && mx <= x + width/2 && my >= y - height/2 && my <= y + height/2) hit = true;
            } else {
                int dx = mx - x; int dy = my - y;
                if (dx*dx + dy*dy < width * width) hit = true;
            }
            if (hit) pressed = true;
        } else if (e.type == SDL_MOUSEBUTTONUP) {
            pressed = false;
        }
    }

    void render(SDL_Renderer* renderer) {
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        if (pressed) SDL_SetRenderDrawColor(renderer, 150, 150, 150, 180);
        else SDL_SetRenderDrawColor(renderer, 50, 50, 50, 150);

        if (type == BTN_RECT) {
            SDL_Rect r = {x - width/2, y - height/2, width, height};
            SDL_RenderFillRect(renderer, &r);
            SDL_SetRenderDrawColor(renderer, 200, 200, 200, 200);
            SDL_RenderDrawRect(renderer, &r);
            SDL_Rect r2 = {x - width/4, y - height/4, width/2, height/2};
            SDL_RenderDrawRect(renderer, &r2);
        } else if (type == BTN_SMALL_TRIANGLE) {
            draw_filled_circle(renderer, x, y, width);
            SDL_SetRenderDrawColor(renderer, 200, 200, 200, 200);
            draw_circle_outline(renderer, x, y, width);
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            int s = width / 2;
            int x1 = x - s/2, y1 = y - s; int x2 = x + s, y2 = y; int x3 = x - s/2, y3 = y + s;
            SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
            SDL_RenderDrawLine(renderer, x2, y2, x3, y3);
            SDL_RenderDrawLine(renderer, x3, y3, x1, y1);
        } else {
            draw_filled_circle(renderer, x, y, width);
            SDL_SetRenderDrawColor(renderer, 220, 220, 220, 180);
            draw_circle_outline(renderer, x, y, width);
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 200);
            int s = width / 2; 
            switch (type) {
                case BTN_SQUARE: { SDL_Rect r = {x - s/2, y - s/2, s, s}; SDL_RenderDrawRect(renderer, &r); break; }
                case BTN_TRIANGLE: {
                    int x1 = x, y1 = y - s/2 - 2; int x2 = x + s/2 + 2, y2 = y + s/2; int x3 = x - s/2 - 2, y3 = y + s/2;
                    SDL_RenderDrawLine(renderer, x1, y1, x2, y2); SDL_RenderDrawLine(renderer, x2, y2, x3, y3); SDL_RenderDrawLine(renderer, x3, y3, x1, y1); break;
                }
                case BTN_CIRCLE: { draw_circle_outline(renderer, x, y, s/2 + 2); break; }
                case BTN_CROSS: {
                    int len = s / 2 + 2; SDL_RenderDrawLine(renderer, x - len, y - len, x + len, y + len); SDL_RenderDrawLine(renderer, x + len, y - len, x - len, y + len); break;
                }
                default: break;
            }
        }
    }
};

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

enum Scene { SCENE_HOME, SCENE_GAME, SCENE_SETTINGS, SCENE_LOBBY };

// Home Screen Panels
enum HomePanel { 
    HOME_PANEL_ROM_GRID = 0,  // Default panel showing ROM grid
    HOME_PANEL_LIBRARY = 1,   // PlayBack (formerly Library)
    HOME_PANEL_FAVORITES = 2  // Duo (formerly Favorites)
};

int home_active_panel = HOME_PANEL_ROM_GRID; // Track current active panel

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

    bool handle_event(const SDL_Event& e, Scene& scene, Emulator& emu) {
        if (e.type == SDL_MOUSEBUTTONDOWN) {
            int mx = e.button.x;
            int my = e.button.y;
            int dx = mx - x; int dy = my - y;
            if (dx*dx + dy*dy <= r*r) {
                expanded = !expanded;
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
                                 if (!fs::exists("snapshots")) fs::create_directory("snapshots");
                                 auto now = std::chrono::system_clock::now();
                                 auto in_time_t = std::chrono::system_clock::to_time_t(now);
                                 std::stringstream ss_name;
                                 ss_name << "snapshots/snapshot_" << std::put_time(std::localtime(&in_time_t), "%Y%m%d_%H%M%S") << ".bmp";
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

// Helper to open file dialog (Windows only for now, simple wrapper)
#ifdef _WIN32
#include <windows.h>
#include <commdlg.h>
std::string open_file_dialog(const char* filter = "NES ROMs\0*.nes\0All Files\0*.*\0") {
    OPENFILENAMEA ofn;
    char szFile[260] = {0};
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = filter;
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if (GetOpenFileNameA(&ofn) == TRUE) {
        return std::string(ofn.lpstrFile);
    }
    return "";
}
#else
std::string open_file_dialog(const char* filter = "") { return ""; } // Not implemented for other OS
#endif

// Helper: Import cover image to local storage
std::string import_cover_image(const std::string& source_path, const std::string& game_name) {
    try {
        // 1. Get Exe Directory
        char buffer[MAX_PATH];
        GetModuleFileNameA(NULL, buffer, MAX_PATH);
        std::string::size_type pos = std::string(buffer).find_last_of("\\/");
        std::string exe_dir = std::string(buffer).substr(0, pos);
        
        // 2. Create 'images/covers' directory if not exists
        fs::path covers_dir = fs::path(exe_dir) / "images" / "covers";
        if (!fs::exists(covers_dir)) {
            fs::create_directories(covers_dir);
        }

        // 3. Generate Destination Path
        // Sanitize game name for filename
        std::string safe_name = game_name;
        std::replace(safe_name.begin(), safe_name.end(), ' ', '_');
        std::replace(safe_name.begin(), safe_name.end(), ':', '-');
        std::replace(safe_name.begin(), safe_name.end(), '/', '-');
        std::replace(safe_name.begin(), safe_name.end(), '\\', '-');
        
        std::string ext = fs::path(source_path).extension().string();
        fs::path dest_path = covers_dir / (safe_name + ext);

        // 4. Copy File
        // Use copy_options::overwrite_existing to update if exists
        fs::copy_file(source_path, dest_path, fs::copy_options::overwrite_existing);
        
        std::cout << "📥 Imported cover to: " << dest_path.string() << std::endl;
        return dest_path.string();
    } catch (const std::exception& e) {
        std::cerr << "❌ Error importing cover: " << e.what() << std::endl;
        return source_path; // Fallback to original path
    }
}

// Helper: Import avatar image to local storage
std::string import_avatar_image(const std::string& source_path, const std::string& device_id) {
    try {
        // 1. Get Exe Directory
        char buffer[MAX_PATH];
        GetModuleFileNameA(NULL, buffer, MAX_PATH);
        std::string::size_type pos = std::string(buffer).find_last_of("\\/");
        std::string exe_dir = std::string(buffer).substr(0, pos);
        
        // 2. Create 'images/avatars' directory if not exists
        fs::path avatars_dir = fs::path(exe_dir) / "images" / "avatars";
        if (!fs::exists(avatars_dir)) {
            fs::create_directories(avatars_dir);
        }

        // 3. Generate Destination Path
        // Use DeviceID as filename to avoid conflicts and easy lookup
        std::string ext = fs::path(source_path).extension().string();
        fs::path dest_path = avatars_dir / (device_id + ext);

        // 4. Copy File
        fs::copy_file(source_path, dest_path, fs::copy_options::overwrite_existing);
        
        std::cout << "📥 Imported avatar to: " << dest_path.string() << std::endl;
        return dest_path.string();
    } catch (const std::exception& e) {
        std::cerr << "❌ Error importing avatar: " << e.what() << std::endl;
        return source_path; // Fallback to original path
    }
}

// Forward declarations for panel functions
struct Slot; // Forward declare Slot struct
void render_rom_grid_panel(SDL_Renderer* renderer, std::vector<Slot>& slots, int scroll_y, FontSystem& font_body);
void handle_rom_grid_events(const SDL_Event& e, std::vector<Slot>& slots, int& scroll_y, int& mouse_down_slot, 
                            bool& showing_delete_popup, bool& showing_context_menu, int& delete_candidate_index,
                            int& context_menu_slot, int& menu_x, int& menu_y, SDL_Renderer* renderer, 
                            const std::string& slots_file, Emulator& emu, Scene& current_scene, ConfigManager& config, Recorder& recorder);

int main(int argc, char* argv[]) {
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
    Scene current_scene = SCENE_HOME;
    
    // Slots
    struct Slot {
        std::string rom_path;
        std::string name;
        std::string cover_path;  // Path to cover image
        bool occupied = false;
        SDL_Texture* cover_texture = nullptr; // Add texture support
    };
    std::vector<Slot> slots(12); // Default 12 slots
    
    // Determine absolute path for slots file based on Executable location
    char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);
    std::string::size_type pos = std::string(buffer).find_last_of("\\/");
    std::string exe_dir = std::string(buffer).substr(0, pos);
    std::string slots_file = exe_dir + "\\game_slots.txt";
    
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

    // Scroll State
    int scroll_y = 0;
    int scroll_speed = 30;

    // Popup & Context Menu State
    int mouse_down_slot = -1;
    bool showing_delete_popup = false;
    int delete_candidate_index = -1;
    
    bool showing_delete_replay_popup = false;
    int delete_replay_index = -1;
    
    bool showing_context_menu = false;
    int context_menu_slot = -1;
    int menu_x = 0;
    int menu_y = 0;

    // Library Panel State
    std::vector<ReplayFileInfo> replay_files;
    int library_scroll_y = 0;
    int selected_replay_index = -1;

    // Duo Panel State
    std::string duo_selected_rom_path = "";
    std::string duo_selected_rom_name = "";
    // Tự động điền Nickname từ Settings (nếu có), nếu không thì mặc định là "Player 1"
    std::string duo_host_name = (!settings_nickname.empty()) ? settings_nickname : "Player 1";
    bool duo_rom_selector_open = false;
    int duo_active_input_field = -1; // -1: none, 0: host name
    
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
    
    // --- UI SETUP ---
    VirtualJoystick joystick;
    joystick.init(100, (SCREEN_HEIGHT * SCALE) - 100, 60);

    int center_x = (SCREEN_WIDTH * SCALE) / 2;
    int bottom_y = (SCREEN_HEIGHT * SCALE) - 50;

    QuickBall quickBall;
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
            
            // Text Input for Duo Panel
            if (current_scene == SCENE_HOME && home_active_panel == HOME_PANEL_FAVORITES) {
                if (duo_active_input_field == 0) { // Host name input active
                    if (e.type == SDL_TEXTINPUT) {
                        duo_host_name += e.text.text;
                    } else if (e.type == SDL_KEYDOWN) {
                        if (e.key.keysym.sym == SDLK_BACKSPACE && !duo_host_name.empty()) {
                            duo_host_name.pop_back();
                        } else if (e.key.keysym.sym == SDLK_RETURN || e.key.keysym.sym == SDLK_ESCAPE) {
                            duo_active_input_field = -1; // Deactivate input
                        }
                    }
                }
            }

            if (current_scene == SCENE_GAME) {
                // Check QuickBall first
                if (quickBall.handle_event(e, current_scene, emu)) {
                    // Event consumed by QuickBall
                } 
                // Check Replay Controls if Replay is active
                else if ((replay_player.is_playing || replay_player.get_current_frame() > 0) && e.type == SDL_MOUSEBUTTONDOWN) {
                    int mx = e.button.x;
                    int my = e.button.y;
                    
                    // Coordinates must match Render logic
                    int btn_radius = 25; // Diameter 50
                    int bottom_y = SCREEN_HEIGHT * SCALE - 50;
                    // QuickBall is at Right-50
                    // Controls shift left: Play(R-110), Fast(R-170), Slow(R-230)
                    int start_x = SCREEN_WIDTH * SCALE - 110; // Play/Pause
                    int ff_x = start_x - 60; // Fast Forward
                    int rw_x = ff_x - 60; // Slow Down

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
                }
                else if (connected_controllers.empty()) {
                    joystick.handle_event(e);
                    for (auto& b : buttons) b.handle_event(e);
                }
            }

            // Home Screen Interactions
            if (current_scene == SCENE_HOME) {
                // QuickBall for Home Navigation
                if (quickBall.handle_event(e, current_scene, emu)) {
                    // Event consumed
                }

                // Scrolling
                if (e.type == SDL_MOUSEWHEEL && !showing_delete_popup && !showing_context_menu) {
                    if (home_active_panel == HOME_PANEL_ROM_GRID) {
                        // ROM Grid scrolling
                        scroll_y -= e.wheel.y * scroll_speed;
                        if (scroll_y < 0) scroll_y = 0;
                        
                        int slot_h = 250;
                        int gap = 20;
                        int cols = 3;
                        int rows = (int)(slots.size() + cols - 1) / cols;
                        int content_height = 150 + rows * (slot_h + gap) + 50; 
                        int view_height = SCREEN_HEIGHT * SCALE;
                        int max_scroll = (std::max)(0, content_height - view_height);
                        
                        if (scroll_y > max_scroll) scroll_y = max_scroll;
                    } else if (home_active_panel == HOME_PANEL_LIBRARY) {
                        // Library scrolling
                        library_scroll_y -= e.wheel.y * scroll_speed;
                        if (library_scroll_y < 0) library_scroll_y = 0;
                        
                        int item_height = 80;
                        int item_margin = 10;
                        int content_height = replay_files.size() * (item_height + item_margin);
                        int view_height = SCREEN_HEIGHT * SCALE - 160; // Account for header and tabs
                        int max_scroll = (std::max)(0, content_height - view_height);
                        
                        if (library_scroll_y > max_scroll) library_scroll_y = max_scroll;
                    }
                }

                if (e.type == SDL_MOUSEBUTTONDOWN) {
                    int mx = e.button.x;
                    int my = e.button.y;




                    if (showing_delete_popup) {
                        // Handle Popup Clicks
                        int cx = (SCREEN_WIDTH * SCALE) / 2;
                        int cy = (SCREEN_HEIGHT * SCALE) / 2;
                        int bw = 100; int bh = 40;
                        
                        // Yes Button
                        if (mx >= cx - 110 && mx <= cx - 10 && my >= cy + 20 && my <= cy + 60) {
                            // Delete
                            if (delete_candidate_index >= 0 && delete_candidate_index < (int)slots.size()) {
                                if (slots[delete_candidate_index].cover_texture) {
                                    SDL_DestroyTexture(slots[delete_candidate_index].cover_texture);
                                }
                                slots.erase(slots.begin() + delete_candidate_index);
                                // Ensure min size
                                while (slots.size() < 12) slots.push_back(Slot());
                                
                                // Auto-save
                                std::vector<SlotManager::Slot> slots_to_save;
                                for (const auto& slot : slots) {
                                    if (slot.occupied) {
                                        slots_to_save.push_back(SlotManager::Slot(slot.rom_path, slot.name, slot.cover_path));
                                    }
                                }
                                SlotManager::save_slots(slots_file, slots_to_save);
                            }
                            showing_delete_popup = false;
                            delete_candidate_index = -1;
                        }
                        // No Button
                        else if (mx >= cx + 10 && mx <= cx + 110 && my >= cy + 20 && my <= cy + 60) {
                            showing_delete_popup = false;
                            delete_candidate_index = -1;
                        }
                    } else if (showing_delete_replay_popup) {
                        // Handle Replay Delete Popup Clicks
                        int cx = (SCREEN_WIDTH * SCALE) / 2;
                        int cy = (SCREEN_HEIGHT * SCALE) / 2;
                        
                        // Yes Button
                        if (mx >= cx - 110 && mx <= cx - 10 && my >= cy + 20 && my <= cy + 60) {
                            // Delete Replay File
                            if (delete_replay_index >= 0 && delete_replay_index < (int)replay_files.size()) {
                                std::string file_path = replay_files[delete_replay_index].full_path;
                                if (std::remove(file_path.c_str()) == 0) {
                                    std::cout << "Deleted replay: " << file_path << std::endl;
                                    replay_files.erase(replay_files.begin() + delete_replay_index);
                                } else {
                                    std::cerr << "Failed to delete replay: " << file_path << std::endl;
                                }
                            }
                            showing_delete_replay_popup = false;
                            delete_replay_index = -1;
                        }
                        // No Button
                        else if (mx >= cx + 10 && mx <= cx + 110 && my >= cy + 20 && my <= cy + 60) {
                            showing_delete_replay_popup = false;
                            delete_replay_index = -1;
                        }
                    } else if (showing_context_menu) {
                        // Handle Context Menu Clicks
                        int w = 180; int h = 160; // Increased height for 4 items
                        int item_h = 40;
                        
                        // Check which item clicked
                        if (mx >= menu_x && mx <= menu_x + w && my >= menu_y && my <= menu_y + h) {
                            int clicked_item = (my - menu_y) / item_h;
                            
                            // Item 0: Add Shortcut (App Only)
                            if (clicked_item == 0) {
                                if (context_menu_slot >= 0 && context_menu_slot < (int)slots.size()) {
                                    std::string desktop_path = getenv("USERPROFILE");
                                    desktop_path += "\\Desktop\\";
                                    // Suffix " (App)" to distinguish or keep same name? 
                                    // User wants "Add Shortcut" -> App Main. Let's keep name simple or specific?
                                    // Let's use "[Game Name]" for InGame and "[Game Name] (App)" for App?
                                    // Or just "[Game Name]" opening App. And "[Game Name] (Play)" opening Game.
                                    std::string shortcut_path = desktop_path + slots[context_menu_slot].name + ".lnk";
                                    
                                    char buffer[MAX_PATH];
                                    GetModuleFileNameA(NULL, buffer, MAX_PATH);
                                    std::string exe_path = buffer;
                                    std::string icon_path = slots[context_menu_slot].cover_path;

                                    std::string ps_cmd = "powershell.exe -ExecutionPolicy Bypass -NoProfile -Command \"";
                                    ps_cmd += "$ws = New-Object -ComObject WScript.Shell; ";
                                    ps_cmd += "$s = $ws.CreateShortcut('" + shortcut_path + "'); ";
                                    ps_cmd += "$s.TargetPath = '" + exe_path + "'; ";
                                    // NO ARGUMENTS for App Only
                                    
                                    if (!icon_path.empty()) {
                                        ps_cmd += "$s.IconLocation = '" + icon_path + "'; ";
                                    }
                                    ps_cmd += "$s.Save()\"";
                                    system(ps_cmd.c_str());
                                    std::cout << "✅ Created App shortcut: " << shortcut_path << std::endl;
                                }
                                showing_context_menu = false;
                            }
                            // Item 1: Add Shortcut InGame (Direct Play)
                            else if (clicked_item == 1) {
                                if (context_menu_slot >= 0 && context_menu_slot < (int)slots.size()) {
                                    std::string desktop_path = getenv("USERPROFILE");
                                    desktop_path += "\\Desktop\\";
                                    std::string shortcut_path = desktop_path + slots[context_menu_slot].name + " (Play).lnk";
                                    
                                    char buffer[MAX_PATH];
                                    GetModuleFileNameA(NULL, buffer, MAX_PATH);
                                    std::string exe_path = buffer;
                                    std::string rom_path = slots[context_menu_slot].rom_path;
                                    std::string icon_path = slots[context_menu_slot].cover_path;

                                    std::string ps_cmd = "powershell.exe -ExecutionPolicy Bypass -NoProfile -Command \"";
                                    ps_cmd += "$ws = New-Object -ComObject WScript.Shell; ";
                                    ps_cmd += "$s = $ws.CreateShortcut('" + shortcut_path + "'); ";
                                    ps_cmd += "$s.TargetPath = '" + exe_path + "'; ";
                                    // ADD ARGUMENT for Direct Play
                                    ps_cmd += "$s.Arguments = '\"" + rom_path + "\"'; "; 
                                    
                                    if (!icon_path.empty()) {
                                        ps_cmd += "$s.IconLocation = '" + icon_path + "'; ";
                                    }
                                    ps_cmd += "$s.Save()\"";
                                    system(ps_cmd.c_str());
                                    std::cout << "✅ Created InGame shortcut: " << shortcut_path << std::endl;
                                }
                                showing_context_menu = false;
                            }
                            // Item 2: Change Cover
                            else if (clicked_item == 2) {
                                #ifdef _WIN32
                                if (context_menu_slot >= 0 && context_menu_slot < (int)slots.size()) {
                                    OPENFILENAME ofn;
                                    char szFile[260] = {0};
                                    ZeroMemory(&ofn, sizeof(ofn));
                                    ofn.lStructSize = sizeof(ofn);
                                    ofn.hwndOwner = NULL;
                                    ofn.lpstrFile = szFile;
                                    ofn.nMaxFile = sizeof(szFile);
                                    ofn.lpstrFilter = "Images\0*.PNG;*.JPG;*.JPEG;*.BMP\0All Files\0*.*\0";
                                    ofn.nFilterIndex = 1;
                                    ofn.lpstrFileTitle = NULL;
                                    ofn.nMaxFileTitle = 0;
                                    ofn.lpstrInitialDir = NULL;
                                    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
                                    
                                    if (GetOpenFileName(&ofn) == TRUE) {
                                        std::string new_cover_path = import_cover_image(szFile, slots[context_menu_slot].name);
                                        slots[context_menu_slot].cover_path = new_cover_path;
                                        if (slots[context_menu_slot].cover_texture) SDL_DestroyTexture(slots[context_menu_slot].cover_texture);
                                        slots[context_menu_slot].cover_texture = load_texture(renderer, new_cover_path);
                                        
                                        // Auto-save logic...
                                        std::vector<SlotManager::Slot> slots_to_save;
                                        for (const auto& slot : slots) {
                                            if (slot.occupied) settings_nickname.empty(); // Dumy check to suppress warning
                                            if (slot.occupied) slots_to_save.push_back(SlotManager::Slot(slot.rom_path, slot.name, slot.cover_path));
                                        }
                                        SlotManager::save_slots(slots_file, slots_to_save);
                                    }
                                }
                                #endif
                                showing_context_menu = false;
                            }
                            // Item 3: Delete
                            else if (clicked_item == 3) {
                                delete_candidate_index = context_menu_slot;
                                showing_delete_popup = true;
                                showing_context_menu = false;
                            }
                        }
                        // Click outside -> Close
                        else {
                            showing_context_menu = false;
                        }
                    } else if (home_active_panel == HOME_PANEL_ROM_GRID) {
                        // Grid Clicks (only for ROM Grid panel)
                        int adj_my = my + scroll_y;
                        int slot_w = 200;
                        int slot_h = 250;
                        int gap = 20;
                        int cols = 3;
                        int start_x = (SCREEN_WIDTH * SCALE - (cols*slot_w + (cols-1)*gap)) / 2;
                        int start_y = 150; // Match the panel_content_y + 10 from rendering

                        // Find Add Button Index
                        int add_btn_index = -1;
                        for (size_t k = 0; k < slots.size(); ++k) {
                            if (!slots[k].occupied) {
                                add_btn_index = (int)k;
                                break;
                            }
                        }

                        for (size_t i=0; i<slots.size(); i++) {
                            int col = i % cols;
                            int row = (int)i / cols;
                            
                            int sx = start_x + col * (slot_w + gap);
                            int sy = start_y + row * (slot_h + gap);

                            if (mx >= sx && mx <= sx+slot_w && adj_my >= sy && adj_my <= sy+slot_h) {
                                if (i == add_btn_index) {
                                    // Add ROM (Immediate Action)
                                    std::string path = open_file_dialog();
                                    if (!path.empty()) {
                                        slots[i].rom_path = path;
                                        // Extract filename for name
                                        size_t last_slash = path.find_last_of("/\\");
                                        std::string filename = (last_slash == std::string::npos) ? path : path.substr(last_slash + 1);
                                        
                                        // Remove extension
                                        size_t last_dot = filename.find_last_of(".");
                                        if (last_dot != std::string::npos) filename = filename.substr(0, last_dot);
                                        
                                        slots[i].name = filename;
                                        slots[i].occupied = true;

                                        // Try to load cover image
                                        std::string cover_path = find_cover_image(path);
                                        if (!cover_path.empty()) {
                                            slots[i].cover_texture = load_texture(renderer, cover_path);
                                        }
                                        
                                        // Auto-save
                                        std::vector<SlotManager::Slot> slots_to_save;
                                        for (const auto& slot : slots) {
                                            if (slot.occupied) {
                                                slots_to_save.push_back(SlotManager::Slot(slot.rom_path, slot.name, slot.cover_path));
                                            }
                                        }
                                        SlotManager::save_slots(slots_file, slots_to_save);
                                    }
                                } else if (slots[i].occupied) {
                                    // Check if clicked on "3 dots" area (Top Right)
                                    int dots_x = sx + slot_w - 30;
                                    int dots_y = sy + 10;
                                    int dots_w = 20;
                                    int dots_h = 30;
                                    
                                    if (mx >= dots_x && mx <= dots_x + dots_w && adj_my >= dots_y && adj_my <= dots_y + dots_h) {
                                        showing_context_menu = true;
                                        context_menu_slot = (int)i;
                                        menu_x = mx;
                                        menu_y = my;
                                        // Adjust if menu goes off screen
                                        if (menu_x + 150 > SCREEN_WIDTH * SCALE) menu_x -= 150;
                                        if (menu_y + 80 > SCREEN_HEIGHT * SCALE) menu_y -= 80;
                                    } else {
                                        // Normal Click -> Load Game
                                        mouse_down_slot = (int)i;
                                    }
                                }
                            }
                        }
                    } else if (home_active_panel == HOME_PANEL_LIBRARY) {
                        // Library Panel Clicks
                        int panel_content_y = 140;
                        int refresh_x = SCREEN_WIDTH * SCALE - 100;
                        int refresh_y = panel_content_y + 10;
                        
                        // Check Refresh button click
                        if (mx >= refresh_x && mx <= refresh_x + 80 && my >= refresh_y && my <= refresh_y + 30) {
                            replay_files = scan_replay_files();
                            library_scroll_y = 0;
                            selected_replay_index = -1;
                            std::cout << "🔄 Refreshed replay library: " << replay_files.size() << " files found" << std::endl;
                        } else {
                            // Check replay item clicks
                            int list_start_y = panel_content_y + 20;
                            int item_height = 80;
                            int item_margin = 10;
                            int list_x = 40;
                            int list_width = SCREEN_WIDTH * SCALE - 80;
                            
                            for (size_t i = 0; i < replay_files.size(); i++) {
                                int item_y = list_start_y + i * (item_height + item_margin) - library_scroll_y;
                                
                                // Check if click is within item bounds
                                if (mx >= list_x && mx <= list_x + list_width && 
                                    my >= item_y && my <= item_y + item_height) {
                                    selected_replay_index = (int)i;
                                    
                                    // Check if clicked on play button
                                    // Check Play Button (Left)
                                    int play_x = list_x + 40;
                                    int play_y = item_y + item_height / 2;
                                    int dx = mx - play_x;
                                    int dy = my - play_y;
                                    
                                    // Check Delete Button (Right)
                                    int del_x = list_x + list_width - 40;
                                    int del_y = item_y + item_height / 2;
                                    int ddx = mx - del_x;
                                    int ddy = my - del_y;

                                    if (dx*dx + dy*dy <= 20*20) {
                                        // Play button clicked - Load and play replay
                                        std::cout << "▶️ Play replay: " << replay_files[i].display_name << std::endl;
                                        
                                        // Load replay file
                                        if (replay_player.load_replay(replay_files[i].full_path)) {
                                            // Try to find matching ROM from slots
                                            // Extract game name from replay filename
                                            std::string replay_filename = replay_files[i].filename;
                                            std::string game_name_from_replay;
                                            
                                            if (replay_filename.substr(0, 7) == "replay_") {
                                                std::string name = replay_filename.substr(7);
                                                size_t last_underscore = name.rfind('_');
                                                if (last_underscore != std::string::npos) {
                                                    size_t second_last = name.rfind('_', last_underscore - 1);
                                                    if (second_last != std::string::npos) {
                                                        game_name_from_replay = name.substr(0, second_last);
                                                        std::replace(game_name_from_replay.begin(), game_name_from_replay.end(), '_', ' ');
                                                    }
                                                }
                                            }
                                            
                                            // Find matching ROM in slots
                                            bool rom_found = false;
                                            std::cout << "   Looking for ROM matching: '" << game_name_from_replay << "'" << std::endl;
                                            for (const auto& slot : slots) {
                                                // Try substring match (game name from replay should contain slot name or vice versa)
                                                if (slot.occupied && (
                                                    game_name_from_replay.find(slot.name) != std::string::npos ||
                                                    slot.name.find(game_name_from_replay) != std::string::npos)) {
                                                    std::cout << "   ✅ Match found!" << std::endl;
                                                    // Load ROM
                                                    if (emu.load_rom(slot.rom_path.c_str())) {
                                                        emu.reset();
                                                        for (int k = 0; k < 10; k++) emu.run_frame();
                                                        emu.memory_.read(0x2002);
                                                        emu.memory_.write(0x2006, 0x3F); emu.memory_.write(0x2006, 0x00);
                                                        emu.memory_.write(0x2007, 0x0F); emu.memory_.write(0x2007, 0x30);
                                                        emu.memory_.write(0x2007, 0x16); emu.memory_.write(0x2007, 0x27);
                                                        
                                                        // Start replay playback
                                                        replay_player.start_playback();
                                                        current_scene = SCENE_GAME;
                                                        quickBall.set_layout_normal();
                                                        rom_found = true;
                                                        
                                                        std::cout << "✅ Started replay playback for: " << slot.name << std::endl;
                                                    }
                                                    break;
                                                }
                                            }
                                            
                                            if (!rom_found) {
                                                std::cerr << "❌ Could not find matching ROM for replay: " << game_name_from_replay << std::endl;
                                                std::cerr << "   Please make sure the ROM is loaded in a slot" << std::endl;
                                            }
                                        }
                                    } else if (ddx*ddx + ddy*ddy <= 20*20) {
                                        // Delete button clicked
                                        delete_replay_index = (int)i;
                                        showing_delete_replay_popup = true;
                                    } else {
                                        // Select item
                                        selected_replay_index = (int)i;
                                    }
                                    break;
                                }
                            }
                        }
                    } else if (home_active_panel == HOME_PANEL_FAVORITES) {
                        // Duo Panel Clicks
                        int panel_content_y = 140;
                        int content_x = 40;
                        int content_width = SCREEN_WIDTH * SCALE - 80;
                        
                        if (duo_rom_selector_open) {
                            // ROM Selector Dialog Clicks (Modern Redesign)
                            int dialog_w = 550;
                            int dialog_h = 450;
                            int dialog_x = (SCREEN_WIDTH * SCALE - dialog_w) / 2;
                            int dialog_y = (SCREEN_HEIGHT * SCALE - dialog_h) / 2;
                            
                            // Circular Close button check
                            int close_cx = dialog_x + dialog_w - 30;
                            int close_cy = dialog_y + 30;
                            int close_r = 15;
                            auto is_in_circle = [&](int mx, int my, int cx, int cy, int r) {
                                return (mx - cx)*(mx - cx) + (my - cy)*(my - cy) <= r*r;
                            };
                            
                            if (is_in_circle(mx, my, close_cx, close_cy, close_r)) {
                                duo_rom_selector_open = false;
                            } else {
                                // ROM item clicks
                                int list_y = dialog_y + 80;
                                int item_h = 55;
                                int item_margin = 10;
                                
                                int count = 0;
                                for (size_t i = 0; i < slots.size() && count < 6; i++) {
                                    if (!slots[i].occupied) continue;
                                    
                                    SDL_Rect item = {dialog_x + 20, list_y, dialog_w - 40, item_h};
                                    if (mx >= item.x && mx <= item.x + item.w &&
                                        my >= item.y && my <= item.y + item.h) {
                                        // Select this ROM
                                        duo_selected_rom_path = slots[i].rom_path;
                                        duo_selected_rom_name = slots[i].name;
                                        duo_rom_selector_open = false;
                                        std::cout << "✅ Selected ROM: " << duo_selected_rom_name << std::endl;
                                        break;
                                    }
                                    list_y += item_h + item_margin;
                                    count++;
                                }
                            }
                        } else {
                            // Main Duo Panel Clicks (Modern Redesign)
                            int start_y = 130; // Synced with Settings
                            int section_y = start_y;
                            int card_y = section_y + 40; // Card starts at 170
                            
                            int padding = 25;
                            int row_y = card_y + padding;
                            
                            // Row 1: ROM Selection (Browse Button)
                            row_y += 20; // Move to field level
                            SDL_Rect browse_btn = {content_x + content_width - 115, row_y, 90, 40};
                            if (mx >= browse_btn.x && mx <= browse_btn.x + browse_btn.w &&
                                my >= browse_btn.y && my <= browse_btn.y + browse_btn.h) {
                                duo_rom_selector_open = true;
                                std::cout << "📁 Opening ROM selector..." << std::endl;
                            }
                            
                            row_y += 60; // Move to Host Name row
                            row_y += 20; // Move to field level
                            
                            // Host Name Input Box
                            SDL_Rect name_field = {content_x + padding, row_y, 250, 40};
                            if (mx >= name_field.x && mx <= name_field.x + name_field.w &&
                                my >= name_field.y && my <= name_field.y + name_field.h) {
                                duo_active_input_field = 0;
                                std::cout << "✏️ Activated host name input" << std::endl;
                            } else {
                                // Click outside - deactivate
                                if (duo_active_input_field == 0) {
                                    duo_active_input_field = -1;
                                }
                            }
                            
                            // Create Host Button
                            bool ready = !duo_selected_rom_path.empty() && !duo_host_name.empty();
                            SDL_Rect host_btn = {content_x + content_width - 165, row_y, 140, 40};
                            if (ready && mx >= host_btn.x && mx <= host_btn.x + host_btn.w &&
                                my >= host_btn.y && my <= host_btn.y + host_btn.h) {
                                std::cout << "🎮 Creating host: " << duo_host_name << " | ROM: " << duo_selected_rom_name << std::endl;
                                
                                // Start broadcasting presence with ROM info
                                discovery.start_advertising(
                                    config.get_device_id(),
                                    duo_host_name,  // Use host name as username
                                    duo_selected_rom_name,
                                    duo_selected_rom_path,
                                    6503  // TCP port for connections
                                );
                                
                                std::cout << "📡 Broadcasting host on LAN..." << std::endl;
                                std::cout << "   Host: " << duo_host_name << std::endl;
                                std::cout << "   ROM: " << duo_selected_rom_name << std::endl;
                                std::cout << "   Path: " << duo_selected_rom_path << std::endl;
                                
                                // Start TCP server
                                net_manager.start_host(6503);
                                std::cout << "🌐 TCP server started on port 6503" << std::endl;
                                
                                // Transition to Lobby
                                lobby_is_host = true;
                                lobby_player2_connected = false;
                                lobby_rom_path = duo_selected_rom_path;
                                lobby_rom_name = duo_selected_rom_name;
                                lobby_host_name = duo_host_name;
                                
                                // Load ROM (but don't start yet)
                                if (emu.load_rom(lobby_rom_path.c_str())) {
                                    emu.reset();
                                    std::cout << "✅ ROM loaded, entering lobby..." << std::endl;
                                    current_scene = SCENE_LOBBY;
                                } else {
                                    std::cerr << "❌ Failed to load ROM!" << std::endl;
                                    discovery.stop_advertising();
                                }
                            }
                            
                            // Check Connect button clicks in host list
                            auto hosts = discovery.get_peers();
                            // Available Card starts at: card_y(CreateHost) + 180(Height) + 50(Gap) + 40(Title Offset)
                            int host_card_start_y = card_y + 180 + 50 + 40; 
                            int hy = host_card_start_y;
                            
                            for (size_t i = 0; i < hosts.size() && i < 3; i++) {
                                const auto& host = hosts[i];
                                
                                // Check if we have this ROM
                                bool has_rom = false;
                                for (const auto& slot : slots) {
                                    if (slot.occupied && slot.rom_path == host.rom_path) {
                                        has_rom = true;
                                        break;
                                    }
                                }
                                
                                // Connect button bounds
                                SDL_Rect conn_btn = {content_x + content_width - 120, hy + 20, 100, 40};
                                
                                if (has_rom && mx >= conn_btn.x && mx <= conn_btn.x + conn_btn.w &&
                                    my >= conn_btn.y && my <= conn_btn.y + conn_btn.h) {
                                    std::cout << "🔗 Connecting to host: " << host.username << std::endl;
                                    
                                    // Set lobby state as client
                                    lobby_is_host = false;
                                    lobby_rom_path = host.rom_path;
                                    lobby_rom_name = host.game_name;
                                    lobby_host_name = host.username;
                                    
                                    // Connect to host
                                    net_manager.connect_to(host.ip, host.port);
                                    std::cout << "🌐 Connecting to " << host.ip << ":" << host.port << std::endl;
                                    
                                    // Load ROM
                                    if (emu.load_rom(lobby_rom_path.c_str())) {
                                        emu.reset();
                                        std::cout << "✅ ROM loaded, entering lobby as client..." << std::endl;
                                        current_scene = SCENE_LOBBY;
                                        lobby_player2_connected = true; // We are P2
                                    } else {
                                        std::cerr << "❌ Failed to load ROM!" << std::endl;
                                        net_manager.disconnect();
                                    }
                                    break;
                                }
                                
                                hy += 90;
                            }
                        }
                    }
                }

                
                if (e.type == SDL_MOUSEBUTTONUP) {
                    if (!showing_delete_popup && !showing_context_menu && mouse_down_slot != -1) {
                        // Short Click -> Load Game
                        int i = mouse_down_slot;
                        if (i >= 0 && i < (int)slots.size() && slots[i].occupied) {
                             start_game(slots[i].rom_path);
                        }
                        mouse_down_slot = -1;
                    }
                }
            }

            // Lobby Scene Interactions
            if (current_scene == SCENE_LOBBY && e.type == SDL_MOUSEBUTTONDOWN) {
                int mx = e.button.x;
                int my = e.button.y;
                int cx = (SCREEN_WIDTH * SCALE) / 2;
                int cy = (SCREEN_HEIGHT * SCALE) / 2;
                
                if (lobby_is_host) {
                    // Cancel Button
                    SDL_Rect cancel_btn = {cx - 200, cy + 150, 100, 40};
                    if (mx >= cancel_btn.x && mx <= cancel_btn.x + cancel_btn.w &&
                        my >= cancel_btn.y && my <= cancel_btn.y + cancel_btn.h) {
                        std::cout << "❌ Host cancelled lobby" << std::endl;
                        discovery.stop_advertising();
                        net_manager.disconnect();
                        current_scene = SCENE_HOME;
                        home_active_panel = HOME_PANEL_FAVORITES;
                    }
                    
                    // Start Button
                    SDL_Rect start_btn = {cx + 100, cy + 150, 100, 40};
                    if (lobby_player2_connected && mx >= start_btn.x && mx <= start_btn.x + start_btn.w &&
                        my >= start_btn.y && my <= start_btn.y + start_btn.h) {
                        std::cout << "🎮 Host starting game!" << std::endl;
                        
                        // Send START signal to client
                        net_manager.send_input(MSG_START_GAME, 0xFF);
                        
                        multiplayer_active = true;
                        multiplayer_frame_id = 0;
                        current_scene = SCENE_GAME;
                        quickBall.set_layout_normal();
                    }
                } else {
                    // Leave Button (Client)
                    SDL_Rect leave_btn = {cx - 50, cy + 150, 100, 40};
                    if (mx >= leave_btn.x && mx <= leave_btn.x + leave_btn.w &&
                        my >= leave_btn.y && my <= leave_btn.y + leave_btn.h) {
                        std::cout << "❌ Client left lobby" << std::endl;
                        net_manager.disconnect();
                        current_scene = SCENE_HOME;
                        home_active_panel = HOME_PANEL_FAVORITES;
                    }
                }
            }

            // Settings Scene Interactions
            if (current_scene == SCENE_SETTINGS) {
                // QuickBall
                if (quickBall.handle_event(e, current_scene, emu)) {
                    // Event consumed
                }

                if (e.type == SDL_TEXTINPUT) {
                    if (active_input_field == 0) settings_nickname += e.text.text;
                } else if (e.type == SDL_KEYDOWN) {
                    if (e.key.keysym.sym == SDLK_BACKSPACE) {
                        std::string* target = nullptr;
                        if (active_input_field == 0) target = &settings_nickname;
                        
                        if (target && !target->empty()) {
                            target->pop_back();
                        }
                    } else if (e.key.keysym.sym == SDLK_RETURN || e.key.keysym.sym == SDLK_ESCAPE) {
                         active_input_field = -1;
                         SDL_StopTextInput();
                    }
                } else if (e.type == SDL_MOUSEBUTTONDOWN) {
                    int mx = e.button.x;
                    int my = e.button.y;
                    
                    // Layout Defines (Must match Render)
                    int content_x = 40;
                    int content_width = SCREEN_WIDTH * SCALE - 80;
                    int start_y = 130;  // Lowered by 20px as requested 
                    
                    // Profile Card Area
                    int profile_card_y = start_y + 40; // Header height assumed
                    
                    // Avatar Hitbox
                    SDL_Rect avatar_box = {content_x + 30, profile_card_y + 30, 100, 100};
                    if (mx >= avatar_box.x && mx <= avatar_box.x + avatar_box.w &&
                        my >= avatar_box.y && my <= avatar_box.y + avatar_box.h) {
                        std::string path = open_file_dialog(); // Simplified filter logic inside func or def
                        if (!path.empty()) {
                             std::string imported_path = import_avatar_image(path, config.get_device_id());
                             if (!imported_path.empty()) {
                                 settings_avatar_path = imported_path;
                             }
                        }
                    }

                    // Nickname Input Hitbox
                    SDL_Rect name_field = {avatar_box.x + 130, profile_card_y + 55, content_width - 190, 45};
                    if (mx >= name_field.x && mx <= name_field.x + name_field.w &&
                        my >= name_field.y && my <= name_field.y + name_field.h) {
                         active_input_field = 0; 
                         SDL_StartTextInput(); 
                    } else {
                         active_input_field = -1; 
                         SDL_StopTextInput(); 
                    }

                    // System Card Area
                    // 1. Tính vị trí Tiêu đề (Section Header)
                    // start_y + 40 (profile header) + 160 (profile card) + 50 (gap)
                    int section_title_y = profile_card_y + 160 + 50; 
                    
                    // 2. Tính vị trí Thẻ System (Card)
                    // Render logic: SDL_Rect sys_card = {content_x, section_y + 40, ...};
                    int sys_card_y = section_title_y + 40;

                    // 3. Tính vị trí Switch Hitbox
                    // Render logic: toggle_y = sys_card.y + 15 (padding) + 10 (offset) = sys_card.y + 25
                    // Tạo hitbox bao quanh dòng chứa switch (cao khoảng 50-60px)
                    SDL_Rect switch_hitbox = {content_x, sys_card_y + 10, content_width, 50};

                    if (mx >= switch_hitbox.x && mx <= switch_hitbox.x + switch_hitbox.w &&
                        my >= switch_hitbox.y && my <= switch_hitbox.y + switch_hitbox.h) {
                         settings_recorder_enabled = !settings_recorder_enabled;
                    }

                    // Save Button
                    // Located at bottom right
                    int btn_w = 140;
                    int btn_h = 45;
                    SDL_Rect save_btn = {content_x + content_width - btn_w, sys_card_y + 100, btn_w, btn_h};
                    
                    if (mx >= save_btn.x && mx <= save_btn.x + save_btn.w &&
                        my >= save_btn.y && my <= save_btn.y + save_btn.h) {
                        config.set_nickname(settings_nickname);
                        config.set_avatar_path(settings_avatar_path);
                        config.set_gameplay_recorder_enabled(settings_recorder_enabled);
                        config.save();
                        current_scene = SCENE_HOME; // Return to Home
                    }
                }
            }

        }

        SDL_SetRenderDrawColor(renderer, 240, 240, 240, 255); // White BG
        SDL_RenderClear(renderer);

        if (current_scene == SCENE_HOME) {
            // --- PANEL CONTAINER ---
            // Panel tabs (below header)
            // Panel content area
            int panel_content_y = 140; // Fixed position since tabs are removed
            
            // Render active panel content
            if (home_active_panel == HOME_PANEL_ROM_GRID) {
                // --- ROM GRID PANEL ---
                int slot_w = 200;
                int slot_h = 250;
                int gap = 20;
                int cols = 3;
                int start_x = (SCREEN_WIDTH * SCALE - (cols*slot_w + (cols-1)*gap)) / 2;
                int start_y = panel_content_y + 10; // Start below tabs

                // Find first empty slot for Add Button
                int add_btn_index = -1;
                for (size_t k = 0; k < slots.size(); ++k) {
                    if (!slots[k].occupied) {
                        add_btn_index = (int)k;
                        break;
                    }
                }

                for (size_t i=0; i<slots.size(); i++) {
                    int col = i % cols;
                    int row = (int)i / cols;
                    
                    int sx = start_x + col * (slot_w + gap);
                    int sy = start_y + row * (slot_h + gap) - scroll_y;
                    
                    // Culling
                    if (sy + slot_h < 0 || sy > SCREEN_HEIGHT * SCALE) continue;

                    SDL_Rect r = {sx, sy, slot_w, slot_h};
                    
                    // Card BG
                    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                    SDL_RenderFillRect(renderer, &r);
                    
                    if (i == add_btn_index) {
                        // --- Add Button ---
                        int cx = sx + slot_w/2;
                        int cy = sy + slot_h/2 - 20;
                        SDL_SetRenderDrawColor(renderer, 240, 240, 240, 255);
                        draw_filled_circle_aa(renderer, cx, cy, 40);
                        
                        // Plus Sign
                        SDL_SetRenderDrawColor(renderer, 34, 43, 50, 255);
                        SDL_Rect rv = {cx - 3, cy - 20, 6, 40};
                        SDL_Rect rh = {cx - 20, cy - 3, 40, 6};
                        SDL_RenderFillRect(renderer, &rv);
                        SDL_RenderFillRect(renderer, &rh);

                        font_body.draw_text(renderer, "Add ROM", sx + 60, sy + slot_h - 40, {34, 43, 50, 255});

                    } else if (slots[i].occupied) {
                        // --- Occupied Slot ---
                        
                        if (slots[i].cover_texture) {
                            // Draw Cover Image with Aspect Ratio & Style
                            int img_w, img_h;
                            SDL_QueryTexture(slots[i].cover_texture, NULL, NULL, &img_w, &img_h);

                            int max_w = slot_w - 20;
                            int max_h = slot_h - 60;
                            
                            float scale_w = (float)max_w / img_w;
                            float scale_h = (float)max_h / img_h;
                            float scale = (scale_w < scale_h) ? scale_w : scale_h;
                            int final_w = (int)(img_w * scale);
                            int final_h = (int)(img_h * scale);

                            int img_x = sx + (slot_w - final_w) / 2;
                            int img_y = sy + 10 + (max_h - final_h) / 2; // Center vertically in the image area
                            
                            SDL_Rect dst = {img_x, img_y, final_w, final_h};

                            // 1. Drop Shadow
                            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 50); // Soft shadow
                            SDL_Rect shadow = {img_x + 4, img_y + 4, final_w, final_h};
                            SDL_RenderFillRect(renderer, &shadow);
                            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

                            // 2. Image
                            SDL_RenderCopy(renderer, slots[i].cover_texture, NULL, &dst);

                            // 3. Border
                            SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255); // Light grey border
                            SDL_RenderDrawRect(renderer, &dst);
                        } else {
                            // Draw Default Cartridge Icon
                            draw_nes_cartridge(renderer, sx + slot_w/2, sy + slot_h/2 - 20, 8);
                        }

                        // Draw Name at bottom
                        std::string display_name = slots[i].name;
                        if (display_name.length() > 18) display_name = display_name.substr(0, 15) + "...";
                        
                        float text_w = font_body.get_text_width(display_name);
                        int tx = sx + (slot_w - (int)text_w) / 2;
                        
                        font_body.draw_text(renderer, display_name, tx, sy + slot_h - 40, {34, 43, 50, 255});

                        // Draw 3 Dots Menu Icon (Top Right)
                        int dx = sx + slot_w - 20;
                        int dy = sy + 25;
                        SDL_SetRenderDrawColor(renderer, 34, 43, 50, 255); // Premium Dark Slate
                        draw_filled_circle_aa(renderer, dx, dy - 6, 3);
                        draw_filled_circle_aa(renderer, dx, dy, 3);
                        draw_filled_circle_aa(renderer, dx, dy + 6, 3);
                    }
                }
            } else if (home_active_panel == HOME_PANEL_LIBRARY) {
                // --- LIBRARY PANEL ---
                int panel_content_y = 140; // Below tabs
                int list_start_y = panel_content_y + 20;
                int item_height = 80;
                int item_margin = 10;
                int list_x = 40;
                int list_width = SCREEN_WIDTH * SCALE - 80;
                
                // Scan for replay files if list is empty
                if (replay_files.empty()) {
                    replay_files = scan_replay_files();
                }
                
                if (replay_files.empty()) {
                    // No replays found
                    int cx = (SCREEN_WIDTH * SCALE) / 2;
                    int cy = (SCREEN_HEIGHT * SCALE) / 2;
                    
                    font_title.draw_text(renderer, "No Replays Found", cx - 120, cy - 30, {150, 150, 150, 255});
                    font_body.draw_text(renderer, "Record gameplay to see replays here", cx - 140, cy + 10, {180, 180, 180, 255});
                } else {
                    // Draw replay list
                    size_t limit = std::min(replay_files.size(), (size_t)50);
                    for (size_t i = 0; i < limit; i++) {
                        int item_y = list_start_y + i * (item_height + item_margin) - library_scroll_y;
                        
                        // Culling
                        if (item_y + item_height < panel_content_y || item_y > SCREEN_HEIGHT * SCALE) continue;
                        
                        SDL_Rect item_rect = {list_x, item_y, list_width, item_height};
                        
                        // Item background
                        bool is_selected = (selected_replay_index == (int)i);
                        if (is_selected) {
                            SDL_SetRenderDrawColor(renderer, 235, 245, 255, 255); // Light blue for selected
                        } else {
                            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); // White
                        }
                        SDL_RenderFillRect(renderer, &item_rect);
                        
                        // Item border
                        if (is_selected) {
                            SDL_SetRenderDrawColor(renderer, 100, 180, 255, 255); // Blue border for selected
                        } else {
                            SDL_SetRenderDrawColor(renderer, 230, 230, 230, 255); // Light gray
                        }
                        SDL_RenderDrawRect(renderer, &item_rect);
                        
                        // Selection Accent Bar (Left)
                        if (is_selected) {
                            SDL_Rect accent = {list_x, item_y, 6, item_height};
                            SDL_SetRenderDrawColor(renderer, 50, 150, 250, 255);
                            SDL_RenderFillRect(renderer, &accent);
                        }

                        // Icon (Play Circle) - At the start (Left)
                        int icon_x = list_x + 40;
                        int icon_y = item_y + item_height / 2;
                        int icon_r = 20;
                        
                        if (is_selected) {
                            SDL_SetRenderDrawColor(renderer, 50, 150, 250, 255); // Blue icon
                        } else {
                            SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255); // Gray icon
                        }
                        draw_filled_circle(renderer, icon_x, icon_y, icon_r);
                        
                        // Play Triangle inside
                        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                        int tr_x = icon_x + 2;
                        int tr_y = icon_y;
                        draw_filled_triangle(renderer, tr_x - 5, tr_y - 8, tr_x + 8, tr_y, tr_x - 5, tr_y + 8);
                        
                        // Draw replay info
                        const auto& replay = replay_files[i];
                        
                        // Title (Game name + date/time)
                        int text_x = list_x + 80;
                        int title_y = item_y + 20;
                        SDL_Color title_color = is_selected ? SDL_Color{30, 30, 30, 255} : SDL_Color{50, 50, 50, 255};
                        font_body.draw_text(renderer, replay.display_name, text_x, title_y, title_color);
                        
                        // Metadata Row (Time Only)
                        int meta_y = item_y + 50;
                        SDL_Color meta_color = {120, 120, 120, 255};
                        
                        // Duration
                        float duration_seconds = replay.total_frames / 60.0f; // Assuming 60 FPS
                        int minutes = (int)(duration_seconds / 60);
                        int seconds = (int)duration_seconds % 60;
                        std::stringstream meta_ss;
                        meta_ss << "Time: " << minutes << "m " << seconds << "s";
                        
                        font_small.draw_text(renderer, meta_ss.str(), text_x, meta_y, meta_color);
                        
                        // Delete Button (Trash Icon) - At the end (Right)
                        int del_x = list_x + list_width - 40;
                        int del_y = item_y + item_height / 2;
                        
                        // Trash Can Body
                        SDL_Rect bin = {del_x - 6, del_y - 6, 12, 14};
                        SDL_SetRenderDrawColor(renderer, 200, 80, 80, 255); // Reddish
                        SDL_RenderFillRect(renderer, &bin);
                        
                        // Lid
                        SDL_Rect lid = {del_x - 8, del_y - 8, 16, 2};
                        SDL_RenderFillRect(renderer, &lid);
                        
                        // Handle
                        SDL_Rect handle = {del_x - 3, del_y - 10, 6, 2};
                        SDL_RenderFillRect(renderer, &handle);
                    }
                }
                
                // (Refresh button removed)
                
                // Replay Delete Popup
                if (showing_delete_replay_popup) {
                    // Dim background
                    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 150);
                    SDL_Rect dim = {0, 0, SCREEN_WIDTH * SCALE, SCREEN_HEIGHT * SCALE};
                    SDL_RenderFillRect(renderer, &dim);
                    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
                    
                    int cx = (SCREEN_WIDTH * SCALE) / 2;
                    int cy = (SCREEN_HEIGHT * SCALE) / 2;
                    int w = 400; int h = 200;
                    SDL_Rect popup = {cx - w/2, cy - h/2, w, h};
                    
                    // Popup BG
                    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                    SDL_RenderFillRect(renderer, &popup);
                    SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
                    SDL_RenderDrawRect(renderer, &popup);
                    
                    // Text
                    font_title.draw_text(renderer, "Delete Replay?", cx - 100, cy - 60, {34, 43, 50, 255});
                    font_body.draw_text(renderer, "Are you sure you want to delete this replay?", cx - 160, cy - 10, {100, 100, 100, 255});
                    
                    // Buttons
                    SDL_Rect btn_yes = {cx - 110, cy + 20, 100, 40};
                    SDL_Rect btn_no = {cx + 10, cy + 20, 100, 40};
                    
                    // Yes (Red)
                    SDL_SetRenderDrawColor(renderer, 220, 80, 80, 255);
                    SDL_RenderFillRect(renderer, &btn_yes);
                    font_body.draw_text(renderer, "Delete", cx - 90, cy + 28, {255, 255, 255, 255});
                    
                    // No (Gray)
                    SDL_SetRenderDrawColor(renderer, 150, 150, 150, 255);
                    SDL_RenderFillRect(renderer, &btn_no);
                    font_body.draw_text(renderer, "Cancel", cx + 30, cy + 28, {255, 255, 255, 255});
                }
                
                
            } else if (home_active_panel == HOME_PANEL_FAVORITES) {
                // --- DUO PANEL (Modern Redesign) ---
                int content_x = 40;
                int content_width = SCREEN_WIDTH * SCALE - 80;
                int start_y = 130; // Synced with Settings
                
                // === CREATE HOST SECTION ===
                int section_y = start_y;
                
                // Section Title with Icon
                SDL_SetRenderDrawColor(renderer, 34, 43, 50, 255);
                draw_filled_circle_aa(renderer, content_x + 10, section_y + 12, 12);
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                draw_filled_circle_aa(renderer, content_x + 10, section_y + 8, 4); // Head
                SDL_Rect body_icon = {content_x + 6, section_y + 13, 8, 4}; SDL_RenderFillRect(renderer, &body_icon);
                
                font_title.draw_text(renderer, "CREATE HOST", content_x + 35, section_y + 22, {34, 43, 50, 255});
                
                int card_y = section_y + 40; // Card starts at 170
                
                // Premium Card Container
                SDL_Rect create_card = {content_x, card_y, content_width, 180};
                // Shadow
                SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 20);
                SDL_Rect card_shadow = {create_card.x + 4, create_card.y + 4, create_card.w, create_card.h};
                SDL_RenderFillRect(renderer, &card_shadow);
                SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
                
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                SDL_RenderFillRect(renderer, &create_card);
                SDL_SetRenderDrawColor(renderer, 230, 230, 230, 255);
                SDL_RenderDrawRect(renderer, &create_card);
                
                int padding = 25;
                int row_y = card_y + padding;
                
                // Row 1: ROM Selection
                font_small.draw_text(renderer, "SELECT GAME TO HOST", content_x + padding, row_y + 5, {120, 120, 120, 255});
                row_y += 20;
                
                SDL_Rect rom_field = {content_x + padding, row_y, content_width - 150, 40};
                SDL_SetRenderDrawColor(renderer, 248, 249, 250, 255);
                SDL_RenderFillRect(renderer, &rom_field);
                SDL_SetRenderDrawColor(renderer, 220, 220, 220, 255);
                SDL_RenderDrawRect(renderer, &rom_field);
                
                std::string rom_txt = duo_selected_rom_name.empty() ? "Choose a game..." : duo_selected_rom_name;
                SDL_Color rom_clr = duo_selected_rom_name.empty() ? SDL_Color{180, 180, 180, 255} : SDL_Color{34, 43, 50, 255};
                font_body.draw_text(renderer, rom_txt, rom_field.x + 12, rom_field.y + 26, rom_clr);
                
                SDL_Rect browse_btn = {content_x + content_width - 115, row_y, 90, 40};
                SDL_SetRenderDrawColor(renderer, 34, 43, 50, 255);
                SDL_RenderFillRect(renderer, &browse_btn);
                font_body.draw_text(renderer, "Browse", browse_btn.x + 15, browse_btn.y + 26, {255, 255, 255, 255});
                
                row_y += 60;
                
                // Row 2: Host Name
                font_small.draw_text(renderer, "HOST NAME", content_x + padding, row_y + 5, {120, 120, 120, 255});
                row_y += 20;
                
                SDL_Rect name_field = {content_x + padding, row_y, 250, 40};
                SDL_SetRenderDrawColor(renderer, 248, 249, 250, 255);
                SDL_RenderFillRect(renderer, &name_field);
                SDL_SetRenderDrawColor(renderer, duo_active_input_field == 0 ? 50 : 220, duo_active_input_field == 0 ? 150 : 220, duo_active_input_field == 0 ? 250 : 220, 255);
                SDL_RenderDrawRect(renderer, &name_field);
                
                font_body.draw_text(renderer, duo_host_name, name_field.x + 12, name_field.y + 26, {34, 43, 50, 255});
                if (duo_active_input_field == 0) {
                    float tw = font_body.get_text_width(duo_host_name);
                    SDL_SetRenderDrawColor(renderer, 34, 43, 50, 255);
                    SDL_RenderDrawLine(renderer, name_field.x + 12 + (int)tw, name_field.y + 10, name_field.x + 12 + (int)tw, name_field.y + 30);
                }
                
                // Create Button
                bool ready = !duo_selected_rom_path.empty() && !duo_host_name.empty();
                SDL_Rect host_btn = {content_x + content_width - 165, row_y, 140, 40};
                if (ready) {
                    SDL_SetRenderDrawColor(renderer, 46, 204, 113, 255); // Emerald Green
                } else {
                    SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
                }
                SDL_RenderFillRect(renderer, &host_btn);
                font_body.draw_text(renderer, "Start Hosting", host_btn.x + 15, host_btn.y + 26, {255, 255, 255, 255});
                
                section_y = card_y + 180 + 50;
                
                // === AVAILABLE HOSTS SECTION ===
                // Section Title with Icon
                SDL_SetRenderDrawColor(renderer, 34, 43, 50, 255);
                draw_filled_circle_aa(renderer, content_x + 10, section_y + 12, 12);
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                draw_filled_circle_aa(renderer, content_x + 10, section_y + 12, 6);
                SDL_SetRenderDrawColor(renderer, 34, 43, 50, 255);
                draw_filled_circle_aa(renderer, content_x + 10, section_y + 12, 3);
                
                font_title.draw_text(renderer, "AVAILABLE HOSTS", content_x + 35, section_y + 22, {34, 43, 50, 255});
                section_y += 50;
                
                auto hosts = discovery.get_peers();
                if (hosts.empty()) {
                    SDL_Rect empty_card = {content_x, section_y, content_width, 120};
                    SDL_SetRenderDrawColor(renderer, 245, 245, 245, 255);
                    SDL_RenderFillRect(renderer, &empty_card);
                    font_body.draw_text(renderer, "Searching for nearby players...", content_x + content_width/2 - 120, section_y + 66, {150, 150, 150, 255});
                } else {
                    int hy = section_y;
                    for (const auto& host : hosts) {
                        SDL_Rect h_card = {content_x, hy, content_width, 80};
                        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                        SDL_RenderFillRect(renderer, &h_card);
                        SDL_SetRenderDrawColor(renderer, 235, 235, 235, 255);
                        SDL_RenderDrawRect(renderer, &h_card);
                        
                        // Status Indicator
                        SDL_SetRenderDrawColor(renderer, 46, 204, 113, 255);
                        draw_filled_circle_aa(renderer, content_x + 25, hy + 40, 6);
                        
                        // Player Info
                        font_body.draw_text(renderer, host.username, content_x + 50, hy + 35, {34, 43, 50, 255});
                        font_small.draw_text(renderer, "Playing: " + host.game_name, content_x + 50, hy + 58, {120, 120, 120, 255});
                        
                        // Connect Button
                        bool has_rom = false;
                        for (const auto& s : slots) { if (s.occupied && s.rom_path == host.rom_path) { has_rom = true; break; } }
                        
                        SDL_Rect conn_btn = {content_x + content_width - 120, hy + 20, 100, 40};
                        if (has_rom) {
                            SDL_SetRenderDrawColor(renderer, 52, 152, 219, 255); // Peter River Blue
                        } else {
                            SDL_SetRenderDrawColor(renderer, 231, 76, 60, 255); // Alizarin Red
                        }
                        SDL_RenderFillRect(renderer, &conn_btn);
                        std::string btn_label = has_rom ? "Connect" : "Missing ROM";
                        font_small.draw_text(renderer, btn_label, conn_btn.x + (has_rom ? 25 : 15), conn_btn.y + 26, {255, 255, 255, 255});
                        
                        hy += 90;
                    }
                }
                
                // ROM Selector Overlay (Modern Redesign)
                if (duo_rom_selector_open) {
                    // Dim background
                    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);
                    SDL_Rect dim = {0, 0, SCREEN_WIDTH * SCALE, SCREEN_HEIGHT * SCALE};
                    SDL_RenderFillRect(renderer, &dim);
                    
                    // Selector Dialog
                    int dialog_w = 550;
                    int dialog_h = 450;
                    int dialog_x = (SCREEN_WIDTH * SCALE - dialog_w) / 2;
                    int dialog_y = (SCREEN_HEIGHT * SCALE - dialog_h) / 2;
                    
                    // Shadow
                    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 40);
                    SDL_Rect dialog_shadow = {dialog_x + 5, dialog_y + 5, dialog_w, dialog_h};
                    SDL_RenderFillRect(renderer, &dialog_shadow);
                    
                    SDL_Rect dialog = {dialog_x, dialog_y, dialog_w, dialog_h};
                    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                    SDL_RenderFillRect(renderer, &dialog);
                    SDL_SetRenderDrawColor(renderer, 230, 230, 230, 255);
                    SDL_RenderDrawRect(renderer, &dialog);
                    
                    // Header Area
                    SDL_Rect header = {dialog_x, dialog_y, dialog_w, 60};
                    SDL_SetRenderDrawColor(renderer, 248, 249, 250, 255);
                    SDL_RenderFillRect(renderer, &header);
                    SDL_SetRenderDrawColor(renderer, 230, 230, 230, 255);
                    SDL_RenderDrawLine(renderer, dialog_x, dialog_y + 60, dialog_x + dialog_w, dialog_y + 60);
                    
                    font_title.draw_text(renderer, "SELECT GAME", dialog_x + 20, dialog_y + 40, {34, 43, 50, 255});
                    
                    // Circular Close Button
                    int close_cx = dialog_x + dialog_w - 30;
                    int close_cy = dialog_y + 30;
                    int close_r = 15;
                    SDL_SetRenderDrawColor(renderer, 231, 76, 60, 255); // Alizarin Red
                    draw_filled_circle_aa(renderer, close_cx, close_cy, close_r);
                    // Draw 'X'
                    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                    SDL_RenderDrawLine(renderer, close_cx - 5, close_cy - 5, close_cx + 5, close_cy + 5);
                    SDL_RenderDrawLine(renderer, close_cx + 5, close_cy - 5, close_cx - 5, close_cy + 5);
                    
                    // ROM List (from slots)
                    int list_y = dialog_y + 80;
                    int item_h = 55;
                    int item_margin = 10;
                    
                    int count = 0;
                    for (size_t i = 0; i < slots.size() && count < 6; i++) {
                        if (!slots[i].occupied) continue;
                        
                        SDL_Rect item = {dialog_x + 20, list_y, dialog_w - 40, item_h};
                        SDL_SetRenderDrawColor(renderer, 252, 252, 252, 255);
                        SDL_RenderFillRect(renderer, &item);
                        SDL_SetRenderDrawColor(renderer, 235, 235, 235, 255);
                        SDL_RenderDrawRect(renderer, &item);
                        
                        // Icon Placeholder (Game Icon)
                        SDL_SetRenderDrawColor(renderer, 52, 152, 219, 255);
                        draw_filled_circle_aa(renderer, item.x + 25, item.y + item_h/2, 15);
                        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                        draw_filled_circle_aa(renderer, item.x + 25, item.y + item_h/2, 6);
                        
                        // ROM Name
                        font_body.draw_text(renderer, slots[i].name, item.x + 55, item.y + 24, {34, 43, 50, 255});
                        
                        // Path (truncated)
                        std::string path_display = slots[i].rom_path;
                        if (path_display.length() > 45) {
                            path_display = "..." + path_display.substr(path_display.length() - 42);
                        }
                        font_small.draw_text(renderer, path_display, item.x + 55, item.y + 45, {150, 150, 150, 255});
                        
                        list_y += item_h + item_margin;
                        count++;
                    }
                    
                    if (count == 0) {
                        font_body.draw_text(renderer, "No games found in slots.", dialog_x + 150, dialog_y + 226, {150, 150, 150, 255});
                    }
                    
                    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
                }
            }


            // --- HEADER (Draw last to be on top of scrolling content) ---
            // Solid Header: Premium Dark Slate (34, 43, 50)
            int header_h = 100;
            SDL_Rect header_rect = {0, 0, SCREEN_WIDTH * SCALE, header_h};
            SDL_SetRenderDrawColor(renderer, 34, 43, 50, 255);
            SDL_RenderFillRect(renderer, &header_rect);

            // Title
            font_title.draw_text(renderer, "Game Enter NES", 20, 50, {255, 255, 255, 255});
            // Status
            std::string status = connected_controllers.empty() ? "No devices connected, play by touch." : "Gamepad connected.";
            font_small.draw_text(renderer, status, 20, 80, {220, 220, 220, 255});



            // --- CONTEXT MENU ---
            if (showing_context_menu) {
                int w = 180; int h = 160; 
                int item_h = 40;
                SDL_Rect menu = {menu_x, menu_y, w, h};
                
                // 1. Shadow
                SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                for (int i = 1; i <= 5; i++) {
                    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 40 / i);
                    SDL_Rect shadow = {menu_x + i, menu_y + i, w, h};
                    SDL_RenderDrawRect(renderer, &shadow);
                }
                
                // 2. Background
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                SDL_RenderFillRect(renderer, &menu);
                SDL_SetRenderDrawColor(renderer, 220, 220, 220, 255);
                SDL_RenderDrawRect(renderer, &menu);

                // 3. Items
                int mouse_x, mouse_y;
                SDL_GetMouseState(&mouse_x, &mouse_y);

                for (int i = 0; i < 4; i++) {
                    int iy = menu_y + i * item_h;
                    SDL_Rect item_rect = {menu_x, iy, w, item_h};
                    
                    if (mouse_x >= menu_x && mouse_x <= menu_x + w && mouse_y >= iy && mouse_y <= iy + item_h) {
                        SDL_SetRenderDrawColor(renderer, 245, 245, 245, 255);
                        SDL_RenderFillRect(renderer, &item_rect);
                    }

                    int icon_x = menu_x + 12;
                    int text_x = menu_x + 40;
                    int center_y = iy + 20;

                    SDL_SetRenderDrawColor(renderer, 34, 43, 50, 255);
                    if (i == 0) { // Shortcut App
                        SDL_Rect link1 = {icon_x, center_y - 1, 12, 2}; SDL_RenderFillRect(renderer, &link1);
                        SDL_Rect link2 = {icon_x + 2, center_y - 5, 2, 10}; SDL_RenderFillRect(renderer, &link2);
                        font_small.draw_text(renderer, "Add Shortcut", text_x, center_y + 6, {34, 43, 50, 255});
                    } else if (i == 1) { // Shortcut Game
                        draw_filled_triangle(renderer, icon_x, center_y - 5, icon_x + 10, center_y, icon_x, center_y + 5);
                        font_small.draw_text(renderer, "Add Shortcut InGame", text_x, center_y + 6, {34, 43, 50, 255});
                    } else if (i == 2) { // Change Cover
                        SDL_Rect img = {icon_x, center_y - 5, 14, 10}; SDL_RenderDrawRect(renderer, &img);
                        draw_filled_circle(renderer, icon_x + 4, center_y - 2, 2);
                        font_small.draw_text(renderer, "Change Cover", text_x, center_y + 6, {34, 43, 50, 255});
                    } else if (i == 3) { // Delete
                        SDL_SetRenderDrawColor(renderer, 200, 80, 80, 255);
                        SDL_Rect bin = {icon_x + 2, center_y - 3, 10, 10}; SDL_RenderFillRect(renderer, &bin);
                        SDL_Rect lid = {icon_x, center_y - 5, 14, 2}; SDL_RenderFillRect(renderer, &lid);
                        font_small.draw_text(renderer, "Delete ROM", text_x, center_y + 6, {200, 80, 80, 255});
                    }

                    if (i < 3) {
                        SDL_SetRenderDrawColor(renderer, 240, 240, 240, 255);
                        SDL_RenderDrawLine(renderer, menu_x + 10, iy + item_h, menu_x + w - 10, iy + item_h);
                    }
                }
                SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
            }

            // --- POPUP ---
            if (showing_delete_popup) {
                // Overlay
                SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 150);
                SDL_Rect overlay = {0, 0, SCREEN_WIDTH * SCALE, SCREEN_HEIGHT * SCALE};
                SDL_RenderFillRect(renderer, &overlay);
                SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

                // Popup Box
                int cx = (SCREEN_WIDTH * SCALE) / 2;
                int cy = (SCREEN_HEIGHT * SCALE) / 2;
                int pw = 400; int ph = 200;
                SDL_Rect popup = {cx - pw/2, cy - ph/2, pw, ph};
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                SDL_RenderFillRect(renderer, &popup);
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
                SDL_RenderDrawRect(renderer, &popup);

                // Text
                std::string msg = "Delete " + slots[delete_candidate_index].name + "?";
                float tw = font_body.get_text_width(msg);
                font_body.draw_text(renderer, msg, cx - tw/2, cy - 40, {0, 0, 0, 255});

                // Buttons
                // Yes
                SDL_Rect btnYes = {cx - 110, cy + 20, 100, 40};
                SDL_SetRenderDrawColor(renderer, 200, 50, 50, 255); // Red
                SDL_RenderFillRect(renderer, &btnYes);
                font_body.draw_text(renderer, "Yes", cx - 110 + 35, cy + 20 + 28, {255, 255, 255, 255});

                // No
                SDL_Rect btnNo = {cx + 10, cy + 20, 100, 40};
                SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255); // Gray
                SDL_RenderFillRect(renderer, &btnNo);
                font_body.draw_text(renderer, "No", cx + 10 + 40, cy + 20 + 28, {255, 255, 255, 255});
            }

            // Render QuickBall for Home Navigation
            quickBall.render(renderer);

        } else if (current_scene == SCENE_LOBBY) {
            // === LOBBY SCENE ===
            SDL_SetRenderDrawColor(renderer, 245, 246, 247, 255);
            SDL_RenderClear(renderer);
            
            int cx = (SCREEN_WIDTH * SCALE) / 2;
            int cy = (SCREEN_HEIGHT * SCALE) / 2;
            
            // --- HEADER ---
            int header_h = 100;
            SDL_Rect header_rect = {0, 0, SCREEN_WIDTH * SCALE, header_h};
            SDL_SetRenderDrawColor(renderer, 34, 43, 50, 255);
            SDL_RenderFillRect(renderer, &header_rect);
            font_title.draw_text(renderer, "Game Enter NES", 20, 50, {255, 255, 255, 255});
            std::string status = connected_controllers.empty() ? "No devices connected, play by touch." : "Gamepad connected.";
            font_small.draw_text(renderer, status, 20, 80, {220, 220, 220, 255});

            // Lobby Info (Below Header)
            std::string title = lobby_is_host ? "Hosting: " + lobby_host_name : "Joining: " + lobby_host_name;
            font_title.draw_text(renderer, title, cx - font_title.get_text_width(title)/2, 140, {34, 43, 50, 255});
            
            // ROM Info
            std::string rom_info = "Game: " + lobby_rom_name;
            font_body.draw_text(renderer, rom_info, cx - font_body.get_text_width(rom_info)/2, 175, {100, 100, 100, 255});
            
            // Players Card (Premium Style)
            SDL_Rect players_box = {cx - 220, cy - 100, 440, 200};
            // Shadow
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 15);
            SDL_Rect shadow = {players_box.x + 4, players_box.y + 4, players_box.w, players_box.h};
            SDL_RenderFillRect(renderer, &shadow);
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            SDL_RenderFillRect(renderer, &players_box);
            SDL_SetRenderDrawColor(renderer, 230, 230, 230, 255);
            SDL_RenderDrawRect(renderer, &players_box);
            
            // Player 1 (Host)
            SDL_SetRenderDrawColor(renderer, 46, 204, 113, 255); // Emerald Green
            draw_filled_circle_aa(renderer, players_box.x + 40, players_box.y + 60, 8);
            std::string p1_text = lobby_is_host ? lobby_host_name + " (You)" : lobby_host_name;
            font_body.draw_text(renderer, "Player 1: " + p1_text, players_box.x + 65, players_box.y + 70, {34, 43, 50, 255});
            
            // Player 2 (Client)
            if (lobby_player2_connected) {
                SDL_SetRenderDrawColor(renderer, 46, 204, 113, 255);
                draw_filled_circle_aa(renderer, players_box.x + 40, players_box.y + 130, 8);
                std::string p2_text = lobby_is_host ? "Player 2" : "You";
                font_body.draw_text(renderer, "Player 2: " + p2_text, players_box.x + 65, players_box.y + 140, {34, 43, 50, 255});
            } else {
                SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
                draw_filled_circle_aa(renderer, players_box.x + 40, players_box.y + 130, 8);
                font_body.draw_text(renderer, "Waiting for Player 2...", players_box.x + 65, players_box.y + 140, {150, 150, 150, 255});
            }
            
            // Buttons
            if (lobby_is_host) {
                // Cancel Button
                SDL_Rect cancel_btn = {cx - 180, cy + 150, 120, 45};
                SDL_SetRenderDrawColor(renderer, 231, 76, 60, 255); // Alizarin Red
                SDL_RenderFillRect(renderer, &cancel_btn);
                font_body.draw_text(renderer, "Cancel", cancel_btn.x + (120 - font_body.get_text_width("Cancel"))/2, cancel_btn.y + 30, {255, 255, 255, 255});
                
                // Start Button
                SDL_Rect start_btn = {cx + 60, cy + 150, 120, 45};
                if (lobby_player2_connected) {
                    SDL_SetRenderDrawColor(renderer, 46, 204, 113, 255);
                } else {
                    SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
                }
                SDL_RenderFillRect(renderer, &start_btn);
                SDL_Color start_color = lobby_player2_connected ? SDL_Color{255, 255, 255, 255} : SDL_Color{220, 220, 220, 255};
                font_body.draw_text(renderer, "Start", start_btn.x + (120 - font_body.get_text_width("Start"))/2, start_btn.y + 30, start_color);
            } else {
                // Leave Button (Client)
                SDL_Rect leave_btn = {cx - 60, cy + 150, 120, 45};
                SDL_SetRenderDrawColor(renderer, 231, 76, 60, 255);
                SDL_RenderFillRect(renderer, &leave_btn);
                font_body.draw_text(renderer, "Leave", leave_btn.x + (120 - font_body.get_text_width("Leave"))/2, leave_btn.y + 30, {255, 255, 255, 255});
                
                // Status
                std::string wait_msg = "Waiting for host to start...";
                font_small.draw_text(renderer, wait_msg, cx - font_small.get_text_width(wait_msg)/2, cy + 125, {120, 120, 120, 255});
            }

        } else if (current_scene == SCENE_SETTINGS) {
            // === SETTINGS SCENE (Modern Redesign) ===
            SDL_SetRenderDrawColor(renderer, 245, 246, 247, 255); // Premium Light Gray BG
            SDL_RenderClear(renderer);
            
            int content_x = 40;
            int content_width = SCREEN_WIDTH * SCALE - 80;
            int start_y = 130;  // Lowered by 20px as requested
            
            // --- PROFILE SECTION ---
            // Section Title with Icon
            int section_y = start_y;
            // Use Duo "Create Host" icon for Profile Settings
            SDL_SetRenderDrawColor(renderer, 34, 43, 50, 255);
            draw_filled_circle_aa(renderer, content_x + 10, section_y + 12, 12);
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            draw_filled_circle_aa(renderer, content_x + 10, section_y + 8, 4); // Head
            SDL_Rect body_icon = {content_x + 6, section_y + 13, 8, 4}; SDL_RenderFillRect(renderer, &body_icon);
            
            font_title.draw_text(renderer, "PROFILE SETTINGS", content_x + 35, section_y + 22, {34, 43, 50, 255});
            
            // Profile Card
            SDL_Rect profile_card = {content_x, section_y + 40, content_width, 160};
            
            // Shadow
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 15);
            SDL_Rect shadow1 = {profile_card.x + 4, profile_card.y + 4, profile_card.w, profile_card.h};
            SDL_RenderFillRect(renderer, &shadow1);
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
            
            // Card BG
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            SDL_RenderFillRect(renderer, &profile_card);
            SDL_SetRenderDrawColor(renderer, 230, 230, 230, 255);
            SDL_RenderDrawRect(renderer, &profile_card);
            
            // 1. Avatar (Left)
            SDL_Rect avatar_box = {profile_card.x + 30, profile_card.y + 30, 100, 100};
            SDL_Texture* avatar_tex = nullptr;
            if (!settings_avatar_path.empty()) {
                avatar_tex = load_texture(renderer, settings_avatar_path);
            }
            
            if (avatar_tex) {
                // Display Avatar
                SDL_RenderCopy(renderer, avatar_tex, NULL, &avatar_box);
                SDL_DestroyTexture(avatar_tex);
                
                // Add border
                SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
                SDL_RenderDrawRect(renderer, &avatar_box);
            } else {
                // Placeholder
                SDL_SetRenderDrawColor(renderer, 240, 240, 240, 255);
                SDL_RenderFillRect(renderer, &avatar_box);
                SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
                SDL_RenderDrawRect(renderer, &avatar_box);
                
                font_small.draw_text(renderer, "No Avatar", avatar_box.x + 15, avatar_box.y + 45, {150, 150, 150, 255});
                font_small.draw_text(renderer, "Click to set", avatar_box.x + 12, avatar_box.y + 65, {100, 150, 200, 255});
            }
            
            // Edit Overlay on Hover (optional, simplified here as constant icon)
            SDL_SetRenderDrawColor(renderer, 34, 43, 50, 200);
            SDL_Rect edit_badge = {avatar_box.x + avatar_box.w - 24, avatar_box.y + avatar_box.h - 24, 24, 24};
            // SDL_RenderFillRect(renderer, &edit_badge); // Just a small icon
            
            // 2. Nickname Input (Right)
            int input_x = avatar_box.x + 130;
            int input_y = profile_card.y + 55; // Vertically center with avatar's visual weight (approx)
             // Label
            font_small.draw_text(renderer, "NICKNAME", input_x, input_y - 12, {120, 120, 120, 255});
            
            // Input Box
            SDL_Rect name_field = {input_x, input_y, profile_card.w - 190, 45};
            SDL_SetRenderDrawColor(renderer, 248, 249, 250, 255);
            SDL_RenderFillRect(renderer, &name_field);
            
            // Focus State Border
            if (active_input_field == 0) {
                 SDL_SetRenderDrawColor(renderer, 52, 152, 219, 255); // Blue focus
            } else {
                 SDL_SetRenderDrawColor(renderer, 220, 220, 220, 255); // Gray default
            }
            SDL_RenderDrawRect(renderer, &name_field);
            
            // Input Text
            // Calculate vertical center for text: y + (h - font_h)/2. Assuming font_h ~20px
            font_body.draw_text(renderer, settings_nickname, name_field.x + 15, name_field.y + 26, {34, 43, 50, 255});
            
            // Cursor
            if (active_input_field == 0) {
                 float tw = font_body.get_text_width(settings_nickname);
                 SDL_SetRenderDrawColor(renderer, 34, 43, 50, 255);
                 SDL_RenderDrawLine(renderer, name_field.x + 15 + (int)tw + 2, name_field.y + 10, name_field.x + 15 + (int)tw + 2, name_field.y + 35);
            }

            // --- SYSTEM SECTION ---
            section_y = profile_card.y + profile_card.h + 50; 
            
            // Icon - Use Duo "Available Hosts" icon (Radar) for System Preferences
            SDL_SetRenderDrawColor(renderer, 34, 43, 50, 255);
            draw_filled_circle_aa(renderer, content_x + 10, section_y + 12, 12);
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            draw_filled_circle_aa(renderer, content_x + 10, section_y + 12, 6);
            SDL_SetRenderDrawColor(renderer, 34, 43, 50, 255);
            draw_filled_circle_aa(renderer, content_x + 10, section_y + 12, 3);
            
            font_title.draw_text(renderer, "SYSTEM PREFERENCES", content_x + 35, section_y + 22, {34, 43, 50, 255});
            
            // System Card
            SDL_Rect sys_card = {content_x, section_y + 40, content_width, 80}; // Shorter card
            
            // Shadow
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 15);
            SDL_Rect shadow2 = {sys_card.x + 4, sys_card.y + 4, sys_card.w, sys_card.h};
            SDL_RenderFillRect(renderer, &shadow2);
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
            
            // Card BG
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            SDL_RenderFillRect(renderer, &sys_card);
            SDL_SetRenderDrawColor(renderer, 230, 230, 230, 255);
            SDL_RenderDrawRect(renderer, &sys_card);
            
            // Row 1: Gameplay Recorder
            int row_y = sys_card.y + 15;
            
            // Label
            font_body.draw_text(renderer, "Enable Gameplay Recorder", sys_card.x + 30, row_y + 32, {34, 43, 50, 255});
            font_small.draw_text(renderer, "Automatically save replays of your sessions", sys_card.x + 30, row_y + 55, {150, 150, 150, 255});
            
            // Toggle Switch (Right aligned)
            int toggle_w = 60;
            int toggle_h = 32;
            int toggle_x = sys_card.x + sys_card.w - toggle_w - 30;
            int toggle_y = row_y + 10;
            
            // Track
            SDL_Rect track_rect = {toggle_x, toggle_y, toggle_w, toggle_h};
            if (settings_recorder_enabled) {
                SDL_SetRenderDrawColor(renderer, 46, 204, 113, 255); // Green ON
            } else {
                SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255); // Gray OFF
            }
            SDL_RenderFillRect(renderer, &track_rect); // Currently rect, ideally rounded rect
            
            // Thumb
            int thumb_size = 26;
            int thumb_x = settings_recorder_enabled ? (toggle_x + toggle_w - thumb_size - 3) : (toggle_x + 3);
            int thumb_y = toggle_y + 3;
            
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            SDL_Rect thumb_rect = {thumb_x, thumb_y, thumb_size, thumb_size};
            SDL_RenderFillRect(renderer, &thumb_rect);
            
            // --- SAVE BUTTON ---
            // Floating or bottom right
            int btn_w = 140;
            int btn_h = 45;
            SDL_Rect save_btn = {content_x + content_width - btn_w, sys_card.y + sys_card.h + 20, btn_w, btn_h};
            
            SDL_SetRenderDrawColor(renderer, 46, 204, 113, 255); // Emerald Green
            SDL_RenderFillRect(renderer, &save_btn);
            
            // Centered Text
            float save_txt_w = font_body.get_text_width("Save Changes");
            font_body.draw_text(renderer, "Save Changes", save_btn.x + (btn_w - (int)save_txt_w)/2, save_btn.y + 28, {255, 255, 255, 255});

            // --- HEADER ---
            int header_h = 100;
            SDL_Rect header_rect = {0, 0, SCREEN_WIDTH * SCALE, header_h};
            SDL_SetRenderDrawColor(renderer, 34, 43, 50, 255);
            SDL_RenderFillRect(renderer, &header_rect);

            // Title
            font_title.draw_text(renderer, "Game Enter NES", 20, 50, {255, 255, 255, 255});
            // Status
            std::string status = connected_controllers.empty() ? "No devices connected, play by touch." : "Gamepad connected.";
            font_small.draw_text(renderer, status, 20, 80, {220, 220, 220, 255});

            // Render QuickBall
            quickBall.render(renderer);


        } else {
            // --- GAME SCENE ---
            
            // Check if replay has finished and return to Library
            if (replay_player.replay_finished) {
                std::cout << "🔙 Replay finished, returning to Library" << std::endl;
                current_scene = SCENE_HOME;
                home_active_panel = HOME_PANEL_LIBRARY;
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
                         // Get current P1 input state
                         uint8_t local_input = emu.get_controller_state(0);
                         
                         // Only host can control pause (to prevent conflicts)
                         if (lobby_is_host) {
                             // Check for Start button press (pause/resume)
                             static bool prev_start_pressed_host = false;
                             bool start_pressed_host = (local_input & (1 << Input::BUTTON_START)) != 0;
                             if (start_pressed_host && !prev_start_pressed_host) {
                                 // Start button just pressed - toggle pause
                                 multiplayer_paused = !multiplayer_paused;
                                 if (multiplayer_paused) {
                                     std::cout << "⏸️ Pausing game..." << std::endl;
                                     net_manager.send_input(MSG_PAUSE_GAME, 0);
                                 } else {
                                     std::cout << "▶️ Resuming game..." << std::endl;
                                     net_manager.send_input(MSG_RESUME_GAME, 0);
                                 }
                             }
                             prev_start_pressed_host = start_pressed_host;
                         } else {
                             // Client: Strip Select from local input (Start is allowed for menu/intro)
                             local_input &= ~(1 << Input::BUTTON_SELECT); // Remove Select
                             // Re-apply cleaned input to P1
                             emu.set_controller(0, local_input);
                         }
                         
                         // Check for pause/resume messages from remote
                         nes::NetworkManager::Packet remote_packet;
                         while (net_manager.pop_remote_input(remote_packet)) {
                             if (remote_packet.frame_id == MSG_PAUSE_GAME) {
                                 multiplayer_paused = true;
                                 std::cout << "⏸️ Remote player paused game" << std::endl;
                             } else if (remote_packet.frame_id == MSG_RESUME_GAME) {
                                 multiplayer_paused = false;
                                 std::cout << "▶️ Remote player resumed game" << std::endl;
                             } else {
                                 // Regular input - set as P2
                                 // Strip out Start and Select buttons (only host controls these)
                                 uint8_t p2_input = remote_packet.input_state;
                                 p2_input &= ~(1 << Input::BUTTON_START);  // Remove Start
                                 p2_input &= ~(1 << Input::BUTTON_SELECT); // Remove Select
                                 emu.set_controller(1, p2_input);
                             }
                         }
                         
                         // Only run frame if not paused
                         if (!multiplayer_paused) {
                             // Send local input
                             net_manager.send_input(multiplayer_frame_id, local_input);
                             
                             // Run frame
                             emu.run_frame();
                             emulator_ran = true;
                             multiplayer_frame_id++;
                         }
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
            
            // Update QuickBall Layout based on mode
            static bool was_replaying = false;
            if (is_replaying != was_replaying) {
                if (is_replaying) {
                    // Switch to Replay Layout (Far Right, Vertical)
                    // QuickBall takes the rightmost spot (R-50)
                    quickBall.set_pos((SCREEN_WIDTH * SCALE) - 50, bottom_y);
                    quickBall.set_layout_replay();
                } else {
                    // Switch to Normal Layout (Center, Fan out)
                    quickBall.set_pos(center_x, bottom_y);
                    quickBall.set_layout_normal();
                }
                was_replaying = is_replaying;
            }

            if (connected_controllers.empty() && !is_replaying) {
                joystick.render(renderer);
                // Render buttons, but hide Select for client in multiplayer
                for (auto& b : buttons) {
                    // Skip Select button if client in multiplayer (Start is allowed for menu/intro)
                    if (multiplayer_active && !lobby_is_host) {
                        if (b.nes_button_mapping == Input::BUTTON_SELECT) {
                            continue; // Skip rendering Select
                        }
                    }
                    b.render(renderer);
                }
            }
            quickBall.render(renderer);
            
            // --- REPLAY PLAYBACK UI ---
            if (replay_player.is_playing || replay_player.get_current_frame() > 0) {
                // Coordinates
                int btn_radius = 25;
                int bottom_y = SCREEN_HEIGHT * SCALE - 50;
                // QuickBall is at Right-50
                // Controls shift left: Play(R-110), Fast(R-170), Slow(R-230)
                int start_x = SCREEN_WIDTH * SCALE - 110; // Play/Pause
                int ff_x = start_x - 60; // Fast Forward
                int rw_x = ff_x - 60; // Slow Down

                // (Progress bar moved to bottom)
                
                // (Time display moved to bottom)
                
                // Replay name
                std::string status = replay_player.is_playing ? "Playing: " : "Paused: ";
                status += replay_player.replay_name;
                // Replay name position might need adjustment or removal if it overlaps. 
                // Let's keep it but maybe move it up? Or just leave it. 
                // Assuming bar_x/bar_y/bar_h are gone, we need new coords for text if we keep it.
                // Let's just comment it out or put it somewhere safe, e.g., top left.
                font_small.draw_text(renderer, status, 20, 20, {255, 255, 255, 255});

                // --- Playback Controls (Bottom Right) ---
                // (Coordinates already defined above)

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

                // (Speed Display moved to above progress bar)

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
                        SDL_Rect bg = {cx - (int)w/2 - 10, cy - 20, (int)w + 20, 40};
                        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 200);
                        SDL_RenderFillRect(renderer, &bg);
                        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

                        font_title.draw_text(renderer, time_str, cx - w/2, cy - 15, {255, 255, 255, 255});
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
