#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include "../core/emulator.h"
#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <iomanip>
#include <cmath>
#include <vector>
#include <algorithm>
#include <fstream>

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

using namespace nes;

// Screen dimensions
const int SCREEN_WIDTH = 256;
const int SCREEN_HEIGHT = 240;
const int SCALE = 3; // 3x scale = 768x720

// --- Font System (stb_truetype) ---
struct FontSystem {
    stbtt_bakedchar cdata[96]; // ASCII 32..126 is 95 glyphs
    SDL_Texture* font_texture = nullptr;
    int texture_width = 512;
    int texture_height = 512;
    bool initialized = false;

    bool init(SDL_Renderer* renderer, const std::string& font_path, float pixel_height) {
        std::ifstream file(font_path, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            std::cerr << "Failed to open font file: " << font_path << std::endl;
            return false;
        }
        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);

        std::vector<unsigned char> ttf_buffer(size);
        if (!file.read((char*)ttf_buffer.data(), size)) return false;

        std::vector<unsigned char> temp_bitmap(texture_width * texture_height);
        
        // Bake font into a single-channel bitmap
        int result = stbtt_BakeFontBitmap(ttf_buffer.data(), 0, pixel_height, temp_bitmap.data(), texture_width, texture_height, 32, 96, cdata);
        if (result <= 0) {
             std::cerr << "Failed to bake font bitmap (bitmap too small?)" << std::endl;
             return false;
        }

        // Convert 1-channel bitmap to 4-channel RGBA for SDL
        std::vector<unsigned char> rgba_bitmap(texture_width * texture_height * 4);
        for (int i = 0; i < texture_width * texture_height; ++i) {
            rgba_bitmap[i*4 + 0] = 255; // R
            rgba_bitmap[i*4 + 1] = 255; // G
            rgba_bitmap[i*4 + 2] = 255; // B
            rgba_bitmap[i*4 + 3] = temp_bitmap[i]; // Alpha from font bitmap
        }

        font_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC, texture_width, texture_height);
        if (!font_texture) return false;

        SDL_UpdateTexture(font_texture, NULL, rgba_bitmap.data(), texture_width * 4);
        SDL_SetTextureBlendMode(font_texture, SDL_BLENDMODE_BLEND);
        
        initialized = true;
        return true;
    }

    void draw_text(SDL_Renderer* renderer, const std::string& text, float x, float y, SDL_Color color) {
        if (!initialized) return;

        SDL_SetTextureColorMod(font_texture, color.r, color.g, color.b);
        SDL_SetTextureAlphaMod(font_texture, color.a);

        for (char c : text) {
            if (c < 32 || c >= 128) c = '?';
            if (c >= 32 && c < 128) {
                stbtt_aligned_quad q;
                stbtt_GetBakedQuad(cdata, texture_width, texture_height, c - 32, &x, &y, &q, 0);

                SDL_Rect src = { (int)(q.s0 * texture_width), (int)(q.t0 * texture_height), (int)((q.s1 - q.s0) * texture_width), (int)((q.t1 - q.t0) * texture_height) };
                SDL_Rect dst = { (int)q.x0, (int)q.y0, (int)(q.x1 - q.x0), (int)(q.y1 - q.y0) };
                
                SDL_RenderCopy(renderer, font_texture, &src, &dst);
            }
        }
    }

    void cleanup() {
        if (font_texture) SDL_DestroyTexture(font_texture);
    }
};

// Helper to draw filled circle
void draw_filled_circle(SDL_Renderer* renderer, int cx, int cy, int radius) {
    for (int w = 0; w < radius * 2; w++) {
        for (int h = 0; h < radius * 2; h++) {
            int dx = radius - w; // horizontal offset
            int dy = radius - h; // vertical offset
            if ((dx*dx + dy*dy) <= (radius * radius)) {
                SDL_RenderDrawPoint(renderer, cx + dx, cy + dy);
            }
        }
    }
}

// Helper to draw circle outline
void draw_circle_outline(SDL_Renderer* renderer, int cx, int cy, int radius) {
    const int segments = 64;
    for (int i = 0; i < segments; i++) {
        float theta = 2.0f * 3.1415926f * float(i) / float(segments);
        float next_theta = 2.0f * 3.1415926f * float(i + 1) / float(segments);
        
        int x1 = cx + int(radius * cosf(theta));
        int y1 = cy + int(radius * sinf(theta));
        int x2 = cx + int(radius * cosf(next_theta));
        int y2 = cy + int(radius * sinf(next_theta));
        
        SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
    }
}

// Helper to draw filled triangle
void draw_filled_triangle(SDL_Renderer* renderer, int x1, int y1, int x2, int y2, int x3, int y3) {
    int minx = std::min(x1, std::min(x2, x3));
    int maxx = std::max(x1, std::max(x2, x3));
    int miny = std::min(y1, std::min(y2, y3));
    int maxy = std::max(y1, std::max(y2, y3));

    for (int y = miny; y <= maxy; y++) {
        for (int x = minx; x <= maxx; x++) {
            float w1 = (x1*(y3-y1) + (y-y1)*(x3-x1) - x*(y3-y1)) / (float)((y2-y1)*(x3-x1) - (x2-x1)*(y3-y1));
            float w2 = (y - y1 - w1*(y2-y1)) / (float)(y3-y1);
            float w3 = 1.0f - w1 - w2;
            
            if (w1 >= 0 && w2 >= 0 && w3 >= 0) {
                 SDL_RenderDrawPoint(renderer, x, y);
            }
        }
    }
    SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
    SDL_RenderDrawLine(renderer, x2, y2, x3, y3);
    SDL_RenderDrawLine(renderer, x3, y3, x1, y1);
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
    uint8_t nes_buttons = 0;
    
    // 1. Keyboard
    if (keys[SDL_SCANCODE_Z])      nes_buttons |= (1 << Input::BUTTON_A);
    if (keys[SDL_SCANCODE_X])      nes_buttons |= (1 << Input::BUTTON_B);
    if (keys[SDL_SCANCODE_A])      nes_buttons |= (1 << Input::BUTTON_SELECT); 
    if (keys[SDL_SCANCODE_S])      nes_buttons |= (1 << Input::BUTTON_START);
    if (keys[SDL_SCANCODE_UP])     nes_buttons |= (1 << Input::BUTTON_UP);
    if (keys[SDL_SCANCODE_DOWN])   nes_buttons |= (1 << Input::BUTTON_DOWN);
    if (keys[SDL_SCANCODE_LEFT])   nes_buttons |= (1 << Input::BUTTON_LEFT);
    if (keys[SDL_SCANCODE_RIGHT])  nes_buttons |= (1 << Input::BUTTON_RIGHT);
    
    // 2. Virtual Controls (Only if no controller connected)
    if (controllers.empty()) {
        if (joystick.up)    nes_buttons |= (1 << Input::BUTTON_UP);
        if (joystick.down)  nes_buttons |= (1 << Input::BUTTON_DOWN);
        if (joystick.left)  nes_buttons |= (1 << Input::BUTTON_LEFT);
        if (joystick.right) nes_buttons |= (1 << Input::BUTTON_RIGHT);

        for (const auto& btn : buttons) {
            if (btn.pressed) nes_buttons |= (1 << btn.nes_button_mapping);
        }
    }

    // 3. Game Controller (Gamepad)
    if (!controllers.empty()) {
        SDL_GameController* ctrl = controllers[0]; // Use first controller
        
        // Buttons
        if (SDL_GameControllerGetButton(ctrl, SDL_CONTROLLER_BUTTON_A)) nes_buttons |= (1 << Input::BUTTON_A);
        if (SDL_GameControllerGetButton(ctrl, SDL_CONTROLLER_BUTTON_B)) nes_buttons |= (1 << Input::BUTTON_B);
        if (SDL_GameControllerGetButton(ctrl, SDL_CONTROLLER_BUTTON_X)) nes_buttons |= (1 << Input::BUTTON_B); // X also shoots
        if (SDL_GameControllerGetButton(ctrl, SDL_CONTROLLER_BUTTON_Y)) nes_buttons |= (1 << Input::BUTTON_A); // Y also jumps
        if (SDL_GameControllerGetButton(ctrl, SDL_CONTROLLER_BUTTON_START)) nes_buttons |= (1 << Input::BUTTON_START);
        if (SDL_GameControllerGetButton(ctrl, SDL_CONTROLLER_BUTTON_BACK))  nes_buttons |= (1 << Input::BUTTON_SELECT);
        
        // D-Pad
        if (SDL_GameControllerGetButton(ctrl, SDL_CONTROLLER_BUTTON_DPAD_UP))    nes_buttons |= (1 << Input::BUTTON_UP);
        if (SDL_GameControllerGetButton(ctrl, SDL_CONTROLLER_BUTTON_DPAD_DOWN))  nes_buttons |= (1 << Input::BUTTON_DOWN);
        if (SDL_GameControllerGetButton(ctrl, SDL_CONTROLLER_BUTTON_DPAD_LEFT))  nes_buttons |= (1 << Input::BUTTON_LEFT);
        if (SDL_GameControllerGetButton(ctrl, SDL_CONTROLLER_BUTTON_DPAD_RIGHT)) nes_buttons |= (1 << Input::BUTTON_RIGHT);

        // Analog Stick (Left Stick)
        int16_t axisX = SDL_GameControllerGetAxis(ctrl, SDL_CONTROLLER_AXIS_LEFTX);
        int16_t axisY = SDL_GameControllerGetAxis(ctrl, SDL_CONTROLLER_AXIS_LEFTY);
        const int DEADZONE = 8000;

        if (axisY < -DEADZONE) nes_buttons |= (1 << Input::BUTTON_UP);
        if (axisY > DEADZONE)  nes_buttons |= (1 << Input::BUTTON_DOWN);
        if (axisX < -DEADZONE) nes_buttons |= (1 << Input::BUTTON_LEFT);
        if (axisX > DEADZONE)  nes_buttons |= (1 << Input::BUTTON_RIGHT);
    }
    
    emu.set_controller(0, nes_buttons);
}

enum Scene { SCENE_HOME, SCENE_GAME };

// Helper to open file dialog (Windows only for now, simple wrapper)
#ifdef _WIN32
#include <windows.h>
#include <commdlg.h>
std::string open_file_dialog() {
    OPENFILENAMEA ofn;
    char szFile[260] = {0};
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = "NES ROMs\0*.nes\0All Files\0*.*\0";
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
std::string open_file_dialog() { return ""; } // Not implemented for other OS
#endif

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

    Emulator emu;
    Scene current_scene = SCENE_HOME;
    
    // Slots
    struct Slot {
        std::string rom_path;
        std::string name;
        bool occupied = false;
    };
    std::vector<Slot> slots(12); // Default 12 slots
    
    // Pre-load if arg provided (into slot 0)
    if (argc > 1) {
        slots[0].rom_path = argv[1];
        slots[0].name = "Game 1";
        slots[0].occupied = true;
    }

    // Scroll State
    int scroll_y = 0;
    int scroll_speed = 30;

    // --- UI SETUP ---
    VirtualJoystick joystick;
    joystick.init(100, (SCREEN_HEIGHT * SCALE) - 100, 60);

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

    int center_x = (SCREEN_WIDTH * SCALE) / 2;
    int bottom_y = (SCREEN_HEIGHT * SCALE) - 50;
    VirtualButton btnSelect; btnSelect.init_rect(center_x - 40, bottom_y, 50, 25, BTN_RECT, Input::BUTTON_SELECT); buttons.push_back(btnSelect);
    VirtualButton btnStart; btnStart.init(center_x + 40, bottom_y, 20, BTN_SMALL_TRIANGLE, Input::BUTTON_START); buttons.push_back(btnStart);

    std::vector<SDL_GameController*> connected_controllers;

    bool quit = false;
    SDL_Event e;
    auto fps_timer = std::chrono::high_resolution_clock::now();
    int frame_count = 0;
    
    while (!quit) {
        auto frame_start = std::chrono::high_resolution_clock::now();

        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) quit = true;
            if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) {
                if (current_scene == SCENE_GAME) current_scene = SCENE_HOME;
                else quit = true;
            }
            if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_r && current_scene == SCENE_GAME) emu.reset();
            
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

            if (current_scene == SCENE_GAME && connected_controllers.empty()) {
                joystick.handle_event(e);
                for (auto& b : buttons) b.handle_event(e);
            }

            // Home Screen Interactions
            if (current_scene == SCENE_HOME) {
                // Scrolling
                if (e.type == SDL_MOUSEWHEEL) {
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
                }

                if (e.type == SDL_MOUSEBUTTONDOWN) {
                    int mx = e.button.x;
                    int my = e.button.y;
                    int adj_my = my + scroll_y;

                    int slot_w = 200;
                    int slot_h = 250;
                    int gap = 20;
                    int cols = 3;
                    int start_x = (SCREEN_WIDTH * SCALE - (cols*slot_w + (cols-1)*gap)) / 2;
                    int start_y = 150;

                    // Find first empty slot for Add Button
                    int add_btn_index = -1;
                    for (size_t k = 0; k < slots.size(); ++k) {
                        if (!slots[k].occupied) {
                            add_btn_index = (int)k;
                            break;
                        }
                    }
                    // If full, expand
                    if (add_btn_index == -1) {
                        slots.resize(slots.size() + 3);
                        add_btn_index = (int)slots.size() - 3;
                    }

                    for (size_t i=0; i<slots.size(); i++) {
                        int col = i % cols;
                        int row = (int)i / cols;
                        
                        int sx = start_x + col * (slot_w + gap);
                        int sy = start_y + row * (slot_h + gap);

                        if (mx >= sx && mx <= sx+slot_w && adj_my >= sy && adj_my <= sy+slot_h) {
                            if (i == add_btn_index) {
                                // Add ROM
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
                                }
                            } else if (slots[i].occupied) {
                                // Load Game
                                if (emu.load_rom(slots[i].rom_path.c_str())) {
                                    emu.reset();
                                    for (int k = 0; k < 10; k++) emu.run_frame();
                                    emu.memory_.read(0x2002);
                                    emu.memory_.write(0x2006, 0x3F); emu.memory_.write(0x2006, 0x00);
                                    emu.memory_.write(0x2007, 0x0F); emu.memory_.write(0x2007, 0x30);
                                    emu.memory_.write(0x2007, 0x16); emu.memory_.write(0x2007, 0x27);
                                    current_scene = SCENE_GAME;
                                }
                            }
                        }
                    }
                }
            }
        }

        SDL_SetRenderDrawColor(renderer, 240, 240, 240, 255); // White BG
        SDL_RenderClear(renderer);

        if (current_scene == SCENE_HOME) {
            // --- GRID ---
            int slot_w = 200;
            int slot_h = 250;
            int gap = 20;
            int cols = 3;
            int start_x = (SCREEN_WIDTH * SCALE - (cols*slot_w + (cols-1)*gap)) / 2;
            int start_y = 150;

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
                    draw_filled_circle(renderer, cx, cy, 40);
                    
                    // Plus Sign
                    SDL_SetRenderDrawColor(renderer, 180, 20, 20, 255);
                    SDL_Rect rv = {cx - 3, cy - 20, 6, 40};
                    SDL_Rect rh = {cx - 20, cy - 3, 40, 6};
                    SDL_RenderFillRect(renderer, &rv);
                    SDL_RenderFillRect(renderer, &rh);

                    font_body.draw_text(renderer, "Add ROM", sx + 60, sy + slot_h - 40, {180, 20, 20, 255});

                } else if (slots[i].occupied) {
                    // --- Occupied Slot ---
                    // Draw Name at bottom (like Add ROM)
                    // Truncate name if too long
                    std::string display_name = slots[i].name;
                    if (display_name.length() > 18) display_name = display_name.substr(0, 15) + "...";
                    
                    // Center text roughly
                    int text_len = display_name.length() * 10; // Approx width
                    int tx = sx + (slot_w - text_len) / 2; 
                    if (tx < sx + 10) tx = sx + 10;

                    font_body.draw_text(renderer, display_name, tx, sy + slot_h - 40, {0, 0, 0, 255});
                } else {
                    // --- Empty Slot ---
                    // Blank
                }
            }

            // --- HEADER (Draw last to be on top of scrolling content) ---
            SDL_SetRenderDrawColor(renderer, 180, 20, 20, 255); // Red
            SDL_Rect header = {0, 0, SCREEN_WIDTH * SCALE, 100};
            SDL_RenderFillRect(renderer, &header);

            // Title
            font_title.draw_text(renderer, "Game Enter NES", 20, 50, {255, 255, 255, 255});
            // Status
            std::string status = connected_controllers.empty() ? "No devices connected, play by touch." : "Gamepad connected.";
            font_small.draw_text(renderer, status, 20, 80, {220, 220, 220, 255});

            // Menu Dots (Top Right)
            int mx = SCREEN_WIDTH * SCALE - 40;
            int my = 50;
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            draw_filled_circle(renderer, mx, my - 15, 4);
            draw_filled_circle(renderer, mx, my, 4);
            draw_filled_circle(renderer, mx, my + 15, 4);

        } else {
            // --- GAME SCENE ---
            const Uint8* currentKeyStates = SDL_GetKeyboardState(NULL);
            handle_input(emu, currentKeyStates, joystick, buttons, connected_controllers);
            
            emu.run_frame();
            
            const uint8_t* framebuffer = emu.get_framebuffer();
            SDL_UpdateTexture(texture, NULL, framebuffer, SCREEN_WIDTH * 4);
            SDL_RenderCopy(renderer, texture, NULL, NULL);
            
            if (connected_controllers.empty()) {
                joystick.render(renderer);
                for (auto& b : buttons) b.render(renderer);
            }
            
            if (audio_device != 0) {
                const std::vector<float>& samples = emu.get_audio_samples();
                if (!samples.empty()) SDL_QueueAudio(audio_device, samples.data(), samples.size() * sizeof(float));
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

    font_title.cleanup();
    font_body.cleanup();
    font_small.cleanup();
    for (auto c : connected_controllers) SDL_GameControllerClose(c);
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    if (audio_device != 0) SDL_CloseAudioDevice(audio_device);
    SDL_Quit();

    return 0;
}
