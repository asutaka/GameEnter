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
#include <sstream>
#include <filesystem> // Added for directory creation

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

using namespace nes;
namespace fs = std::filesystem; // Alias for convenience

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

    float get_text_width(const std::string& text) {
        float x = 0;
        float y = 0;
        for (char c : text) {
            if (c < 32 || c >= 128) c = '?';
            if (c >= 32 && c < 128) {
                stbtt_aligned_quad q;
                stbtt_GetBakedQuad(cdata, texture_width, texture_height, c - 32, &x, &y, &q, 0);
            }
        }
        return x;
    }

    void cleanup() {
        if (font_texture) SDL_DestroyTexture(font_texture);
    }
};

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

// QuickBall Class
struct QuickBall {
    int x, y, r;
    bool expanded;
    
    struct Item {
        int x, y, r;
        int id; // 0: Share, 1: Snapshot, 2: Reset, 3: Home
    };
    std::vector<Item> items;

    void init(int start_x, int start_y) {
        x = start_x; y = start_y; r = 25;
        expanded = false;
        items.clear();
        // Expand Upwards (Fan out)
        // 1. Share (Left)
        items.push_back({x - 60, y - 10, 20, 0});
        // 2. Snapshot (Up-Left)
        items.push_back({x - 40, y - 50, 20, 1});
        // 3. Reset (Up-Right)
        items.push_back({x + 40, y - 50, 20, 2});
        // 4. Home (Right)
        items.push_back({x + 60, y - 10, 20, 3});
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
                    int idx = mx - item.x; int idy = my - item.y;
                    if (idx*idx + idy*idy <= item.r*item.r) {
                        if (item.id == 0) { // Share
                             std::cout << "[QuickBall] Share: Not Implemented" << std::endl;
                        } else if (item.id == 1) { // Snapshot
                             const uint8_t* fb = emu.get_framebuffer();
                             SDL_Surface* ss = SDL_CreateRGBSurfaceWithFormatFrom((void*)fb, 256, 240, 32, 256*4, SDL_PIXELFORMAT_RGBA32);
                             if (ss) {
                                 // Ensure directory exists
                                 if (!fs::exists("snapshots")) {
                                     fs::create_directory("snapshots");
                                 }

                                 auto now = std::chrono::system_clock::now();
                                 auto in_time_t = std::chrono::system_clock::to_time_t(now);
                                 std::stringstream ss_name;
                                 ss_name << "snapshots/snapshot_" << std::put_time(std::localtime(&in_time_t), "%Y%m%d_%H%M%S") << ".bmp";
                                 SDL_SaveBMP(ss, ss_name.str().c_str());
                                 SDL_FreeSurface(ss);
                                 std::cout << "[QuickBall] Snapshot saved: " << ss_name.str() << std::endl;
                             }
                        } else if (item.id == 2) { // Reset
                            emu.reset();
                        } else if (item.id == 3) { // Home
                            scene = SCENE_HOME;
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
            for (const auto& item : items) {
                SDL_SetRenderDrawColor(renderer, 240, 240, 240, 255);
                draw_filled_circle(renderer, item.x, item.y, item.r);
                SDL_SetRenderDrawColor(renderer, 180, 180, 180, 255);
                draw_circle_outline(renderer, item.x, item.y, item.r);
                
                SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
                if (item.id == 0) { // Share (Share Icon)
                    int ix = item.x, iy = item.y;
                    draw_filled_circle(renderer, ix - 5, iy, 3);
                    draw_filled_circle(renderer, ix + 5, iy - 5, 3);
                    draw_filled_circle(renderer, ix + 5, iy + 5, 3);
                    SDL_RenderDrawLine(renderer, ix - 5, iy, ix + 5, iy - 5);
                    SDL_RenderDrawLine(renderer, ix - 5, iy, ix + 5, iy + 5);
                } else if (item.id == 1) { // Snapshot (Camera)
                    SDL_Rect box = {item.x - 7, item.y - 5, 14, 10};
                    SDL_RenderDrawRect(renderer, &box);
                    draw_circle_outline(renderer, item.x, item.y, 3);
                    SDL_RenderDrawPoint(renderer, item.x + 5, item.y - 7);
                } else if (item.id == 2) { // Reset (R)
                    int ix = item.x, iy = item.y;
                    SDL_RenderDrawLine(renderer, ix - 4, iy - 6, ix - 4, iy + 6);
                    SDL_RenderDrawLine(renderer, ix - 4, iy - 6, ix + 2, iy - 6);
                    SDL_RenderDrawLine(renderer, ix + 2, iy - 6, ix + 2, iy);
                    SDL_RenderDrawLine(renderer, ix + 2, iy, ix - 4, iy);
                    SDL_RenderDrawLine(renderer, ix - 4, iy, ix + 4, iy + 6);
                } else if (item.id == 3) { // Home (House)
                    int ix = item.x, iy = item.y;
                    SDL_RenderDrawLine(renderer, ix - 8, iy + 2, ix, iy - 8);
                    SDL_RenderDrawLine(renderer, ix, iy - 8, ix + 8, iy + 2);
                    SDL_Rect box = {ix - 6, iy + 2, 12, 8};
                    SDL_RenderDrawRect(renderer, &box);
                }
            }
        }
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 200);
        draw_filled_circle(renderer, x, y, r);
        SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
        draw_circle_outline(renderer, x, y, r);
        SDL_SetRenderDrawColor(renderer, 100, 100, 100, 200);
        draw_filled_circle(renderer, x, y, r - 10);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    }
};

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
        SDL_Texture* cover_texture = nullptr; // Add texture support
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

    // Popup & Context Menu State
    int mouse_down_slot = -1;
    bool showing_delete_popup = false;
    int delete_candidate_index = -1;
    
    bool showing_context_menu = false;
    int context_menu_slot = -1;
    int menu_x = 0;
    int menu_y = 0;

    // --- UI SETUP ---
    VirtualJoystick joystick;
    joystick.init(100, (SCREEN_HEIGHT * SCALE) - 100, 60);

    int center_x = (SCREEN_WIDTH * SCALE) / 2;
    int bottom_y = (SCREEN_HEIGHT * SCALE) - 50;

    QuickBall quickBall;
    quickBall.init(center_x, bottom_y);

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

            if (current_scene == SCENE_GAME) {
                // Check QuickBall first
                if (quickBall.handle_event(e, current_scene, emu)) {
                    // Event consumed by QuickBall, do nothing else
                } else if (connected_controllers.empty()) {
                    joystick.handle_event(e);
                    for (auto& b : buttons) b.handle_event(e);
                }
            }

            // Home Screen Interactions
            if (current_scene == SCENE_HOME) {
                // Scrolling
                if (e.type == SDL_MOUSEWHEEL && !showing_delete_popup && !showing_context_menu) {
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
                            }
                            showing_delete_popup = false;
                            delete_candidate_index = -1;
                        }
                        // No Button
                        else if (mx >= cx + 10 && mx <= cx + 110 && my >= cy + 20 && my <= cy + 60) {
                            showing_delete_popup = false;
                            delete_candidate_index = -1;
                        }
                    } else if (showing_context_menu) {
                        // Handle Context Menu Clicks
                        int w = 150; int h = 80;
                        // Add Shortcut
                        if (mx >= menu_x && mx <= menu_x + w && my >= menu_y && my <= menu_y + 40) {
                            // Create Shortcut (Batch file on Desktop)
                            if (context_menu_slot >= 0 && context_menu_slot < (int)slots.size()) {
                                std::string desktop_path = getenv("USERPROFILE");
                                desktop_path += "\\Desktop\\";
                                std::string bat_path = desktop_path + slots[context_menu_slot].name + ".bat";
                                std::ofstream bat_file(bat_path);
                                if (bat_file.is_open()) {
                                    char buffer[MAX_PATH];
                                    GetModuleFileNameA(NULL, buffer, MAX_PATH);
                                    std::string exe_path = buffer;
                                    std::string rom_path = slots[context_menu_slot].rom_path;
                                    
                                    bat_file << "@echo off" << std::endl;
                                    bat_file << "start \"\" \"" << exe_path << "\" \"" << rom_path << "\"" << std::endl;
                                    bat_file.close();
                                }
                            }
                            showing_context_menu = false;
                        }
                        // Delete
                        else if (mx >= menu_x && mx <= menu_x + w && my >= menu_y + 40 && my <= menu_y + 80) {
                            delete_candidate_index = context_menu_slot;
                            showing_delete_popup = true;
                            showing_context_menu = false;
                        }
                        // Click outside -> Close
                        else {
                            showing_context_menu = false;
                        }
                    } else {
                        // Grid Clicks
                        int adj_my = my + scroll_y;
                        int slot_w = 200;
                        int slot_h = 250;
                        int gap = 20;
                        int cols = 3;
                        int start_x = (SCREEN_WIDTH * SCALE - (cols*slot_w + (cols-1)*gap)) / 2;
                        int start_y = 150;

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
                    }
                }
                
                if (e.type == SDL_MOUSEBUTTONUP) {
                    if (!showing_delete_popup && !showing_context_menu && mouse_down_slot != -1) {
                        // Short Click -> Load Game
                        int i = mouse_down_slot;
                        if (i >= 0 && i < (int)slots.size() && slots[i].occupied) {
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
                        mouse_down_slot = -1;
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
                    SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
                    draw_filled_circle(renderer, dx, dy - 6, 3);
                    draw_filled_circle(renderer, dx, dy, 3);
                    draw_filled_circle(renderer, dx, dy + 6, 3);
                }
            }

            // --- HEADER (Draw last to be on top of scrolling content) ---
            // Gradient Header: Deep Dark
            // Start: #222B32 (34, 43, 50)
            // End:   #263238 (38, 50, 56)
            int header_h = 100;
            for (int y = 0; y < header_h; y++) {
                float t = (float)y / (float)header_h;
                Uint8 r = (Uint8)(34 + t * (38 - 34));
                Uint8 g = (Uint8)(43 + t * (50 - 43));
                Uint8 b = (Uint8)(50 + t * (56 - 50));
                SDL_SetRenderDrawColor(renderer, r, g, b, 255);
                SDL_RenderDrawLine(renderer, 0, y, SCREEN_WIDTH * SCALE, y);
            }

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

            // --- CONTEXT MENU ---
            if (showing_context_menu) {
                int w = 150; int h = 80;
                SDL_Rect menu = {menu_x, menu_y, w, h};
                
                // Shadow
                SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 50);
                SDL_Rect shadow = {menu_x + 5, menu_y + 5, w, h};
                SDL_RenderFillRect(renderer, &shadow);
                SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

                // Menu BG
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                SDL_RenderFillRect(renderer, &menu);
                SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
                SDL_RenderDrawRect(renderer, &menu);

                // Add Shortcut Item
                font_small.draw_text(renderer, "Add Shortcut", menu_x + 10, menu_y + 25, {0, 0, 0, 255});
                
                // Separator
                SDL_SetRenderDrawColor(renderer, 230, 230, 230, 255);
                SDL_RenderDrawLine(renderer, menu_x, menu_y + 40, menu_x + w, menu_y + 40);

                // Delete Item
                font_small.draw_text(renderer, "Delete", menu_x + 10, menu_y + 65, {200, 50, 50, 255});
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
            quickBall.render(renderer);
            
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
