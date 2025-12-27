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

using namespace nes;

// Screen dimensions
const int SCREEN_WIDTH = 256;
const int SCREEN_HEIGHT = 240;
const int SCALE = 3; // 3x scale = 768x720

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
    // Simple scanline or bounding box approach for small triangles
    int minx = std::min(x1, std::min(x2, x3));
    int maxx = std::max(x1, std::max(x2, x3));
    int miny = std::min(y1, std::min(y2, y3));
    int maxy = std::max(y1, std::max(y2, y3));

    for (int y = miny; y <= maxy; y++) {
        for (int x = minx; x <= maxx; x++) {
            // Barycentric coordinates
            float w1 = (x1*(y3-y1) + (y-y1)*(x3-x1) - x*(y3-y1)) / (float)((y2-y1)*(x3-x1) - (x2-x1)*(y3-y1));
            float w2 = (y - y1 - w1*(y2-y1)) / (float)(y3-y1);
            float w3 = 1.0f - w1 - w2;
            
            if (w1 >= 0 && w2 >= 0 && w3 >= 0) {
                 SDL_RenderDrawPoint(renderer, x, y);
            }
        }
    }
    
    // Outline
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
    
    // Current input state
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

        // Draw Directional Arrows
        SDL_SetRenderDrawColor(renderer, 150, 150, 150, 150);
        int arrow_dist = base_radius - 15;
        int arrow_size = 8;

        // Up Arrow
        draw_filled_triangle(renderer, 
            base_x, base_y - arrow_dist - arrow_size,
            base_x - arrow_size, base_y - arrow_dist,
            base_x + arrow_size, base_y - arrow_dist
        );
        // Down Arrow
        draw_filled_triangle(renderer, 
            base_x, base_y + arrow_dist + arrow_size,
            base_x - arrow_size, base_y + arrow_dist,
            base_x + arrow_size, base_y + arrow_dist
        );
        // Left Arrow
        draw_filled_triangle(renderer, 
            base_x - arrow_dist - arrow_size, base_y,
            base_x - arrow_dist, base_y - arrow_size,
            base_x - arrow_dist, base_y + arrow_size
        );
        // Right Arrow
        draw_filled_triangle(renderer, 
            base_x + arrow_dist + arrow_size, base_y,
            base_x + arrow_dist, base_y - arrow_size,
            base_x + arrow_dist, base_y + arrow_size
        );

        SDL_SetRenderDrawColor(renderer, 100, 150, 255, 180);
        draw_filled_circle(renderer, knob_x, knob_y, knob_radius);
    }
};

// Virtual Button Class
enum ButtonType { BTN_SQUARE, BTN_TRIANGLE, BTN_CIRCLE, BTN_CROSS, BTN_RECT, BTN_SMALL_TRIANGLE };

struct VirtualButton {
    int x, y, width, height; // Use width/height for rects, radius for circles (width=radius)
    ButtonType type;
    bool pressed;
    int nes_button_mapping; 

    void init(int cx, int cy, int size, ButtonType t, int mapping) {
        x = cx;
        y = cy;
        width = size; // For circles, this is radius
        height = size;
        type = t;
        pressed = false;
        nes_button_mapping = mapping;
    }
    
    // Overload for non-square/circle shapes if needed
    void init_rect(int cx, int cy, int w, int h, ButtonType t, int mapping) {
        x = cx;
        y = cy;
        width = w;
        height = h;
        type = t;
        pressed = false;
        nes_button_mapping = mapping;
    }

    void handle_event(const SDL_Event& e) {
        if (e.type == SDL_MOUSEBUTTONDOWN) {
            int mx = e.button.x;
            int my = e.button.y;
            bool hit = false;
            
            if (type == BTN_RECT) {
                // Centered rect
                if (mx >= x - width/2 && mx <= x + width/2 &&
                    my >= y - height/2 && my <= y + height/2) {
                    hit = true;
                }
            } else if (type == BTN_SMALL_TRIANGLE) {
                // Approximate hit box as circle or rect for simplicity
                int dx = mx - x;
                int dy = my - y;
                if (dx*dx + dy*dy < width * width) hit = true;
            } else {
                // Circle based
                int dx = mx - x;
                int dy = my - y;
                if (dx*dx + dy*dy < width * width) hit = true;
            }
            
            if (hit) pressed = true;
            
        } else if (e.type == SDL_MOUSEBUTTONUP) {
            pressed = false;
        }
    }

    void render(SDL_Renderer* renderer) {
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        
        // Color setup
        if (pressed) SDL_SetRenderDrawColor(renderer, 150, 150, 150, 180);
        else SDL_SetRenderDrawColor(renderer, 50, 50, 50, 150);

        if (type == BTN_RECT) {
            SDL_Rect r = {x - width/2, y - height/2, width, height};
            SDL_RenderFillRect(renderer, &r);
            SDL_SetRenderDrawColor(renderer, 200, 200, 200, 200);
            SDL_RenderDrawRect(renderer, &r);
            
            // Label "SELECT" (Simulated with lines or just shape)
            // Just drawing a small box inside
            SDL_Rect r2 = {x - width/4, y - height/4, width/2, height/2};
            SDL_RenderDrawRect(renderer, &r2);
            
        } else if (type == BTN_SMALL_TRIANGLE) {
            // Draw filled triangle? Hard with SDL_RenderGeometry, let's use lines for outline and points for fill or just simple shape
            // For simplicity, just draw the shape icon
            // Draw circle background first
            draw_filled_circle(renderer, x, y, width); // width is radius here
            SDL_SetRenderDrawColor(renderer, 200, 200, 200, 200);
            draw_circle_outline(renderer, x, y, width);
            
            // Draw Play/Start Triangle Icon
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            int s = width / 2;
            // Pointing Right
            int x1 = x - s/2, y1 = y - s;
            int x2 = x + s, y2 = y;
            int x3 = x - s/2, y3 = y + s;
            SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
            SDL_RenderDrawLine(renderer, x2, y2, x3, y3);
            SDL_RenderDrawLine(renderer, x3, y3, x1, y1);
            
        } else {
            // Standard Face Buttons
            draw_filled_circle(renderer, x, y, width);
            SDL_SetRenderDrawColor(renderer, 220, 220, 220, 180);
            draw_circle_outline(renderer, x, y, width);

            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 200);
            int s = width / 2; 
            
            switch (type) {
                case BTN_SQUARE: {
                    SDL_Rect r = {x - s/2, y - s/2, s, s};
                    SDL_RenderDrawRect(renderer, &r);
                    break;
                }
                case BTN_TRIANGLE: {
                    int x1 = x, y1 = y - s/2 - 2;
                    int x2 = x + s/2 + 2, y2 = y + s/2;
                    int x3 = x - s/2 - 2, y3 = y + s/2;
                    SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
                    SDL_RenderDrawLine(renderer, x2, y2, x3, y3);
                    SDL_RenderDrawLine(renderer, x3, y3, x1, y1);
                    break;
                }
                case BTN_CIRCLE: {
                    draw_circle_outline(renderer, x, y, s/2 + 2);
                    break;
                }
                case BTN_CROSS: {
                    int len = s / 2 + 2;
                    SDL_RenderDrawLine(renderer, x - len, y - len, x + len, y + len);
                    SDL_RenderDrawLine(renderer, x + len, y - len, x - len, y + len);
                    break;
                }
                default: break;
            }
        }
    }
};

// Key mapping
void handle_input(Emulator& emu, const Uint8* keys, const VirtualJoystick& joystick, const std::vector<VirtualButton>& buttons) {
    uint8_t nes_buttons = 0;
    
    // Keyboard
    if (keys[SDL_SCANCODE_Z])      nes_buttons |= (1 << Input::BUTTON_A);
    if (keys[SDL_SCANCODE_X])      nes_buttons |= (1 << Input::BUTTON_B);
    if (keys[SDL_SCANCODE_A])      nes_buttons |= (1 << Input::BUTTON_SELECT); 
    if (keys[SDL_SCANCODE_S])      nes_buttons |= (1 << Input::BUTTON_START);
    if (keys[SDL_SCANCODE_UP])     nes_buttons |= (1 << Input::BUTTON_UP);
    if (keys[SDL_SCANCODE_DOWN])   nes_buttons |= (1 << Input::BUTTON_DOWN);
    if (keys[SDL_SCANCODE_LEFT])   nes_buttons |= (1 << Input::BUTTON_LEFT);
    if (keys[SDL_SCANCODE_RIGHT])  nes_buttons |= (1 << Input::BUTTON_RIGHT);
    
    // Joystick
    if (joystick.up)    nes_buttons |= (1 << Input::BUTTON_UP);
    if (joystick.down)  nes_buttons |= (1 << Input::BUTTON_DOWN);
    if (joystick.left)  nes_buttons |= (1 << Input::BUTTON_LEFT);
    if (joystick.right) nes_buttons |= (1 << Input::BUTTON_RIGHT);

    // Virtual Buttons
    for (const auto& btn : buttons) {
        if (btn.pressed) {
            nes_buttons |= (1 << btn.nes_button_mapping);
        }
    }
    
    emu.set_controller(0, nes_buttons);
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <rom_file>" << std::endl;
        return 1;
    }

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
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

    SDL_Window* window = SDL_CreateWindow("NES Emulator", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH * SCALE, SCREEN_HEIGHT * SCALE, SDL_WINDOW_SHOWN);
    if (!window) return 1;

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) return 1;

    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, SCREEN_WIDTH, SCREEN_HEIGHT);

    Emulator emu;
    if (!emu.load_rom(argv[1])) return 1;

    // --- UI SETUP ---
    VirtualJoystick joystick;
    joystick.init(100, (SCREEN_HEIGHT * SCALE) - 100, 60);

    std::vector<VirtualButton> buttons;
    int btn_radius = 35;
    int base_x = (SCREEN_WIDTH * SCALE) - 120;
    int base_y = (SCREEN_HEIGHT * SCALE) - 120;
    int offset = 55;

    // Face Buttons (Diamond)
    VirtualButton btn;
    
    // Cross (Bottom) -> B (Shoot)
    btn.init(base_x, base_y + offset, btn_radius, BTN_CROSS, Input::BUTTON_B);
    buttons.push_back(btn);

    // Square (Left) -> B (Shoot)
    btn.init(base_x - offset, base_y, btn_radius, BTN_SQUARE, Input::BUTTON_B);
    buttons.push_back(btn);

    // Triangle (Top) -> A (Jump)
    btn.init(base_x, base_y - offset, btn_radius, BTN_TRIANGLE, Input::BUTTON_A);
    buttons.push_back(btn);

    // Circle (Right) -> A (Jump)
    btn.init(base_x + offset, base_y, btn_radius, BTN_CIRCLE, Input::BUTTON_A);
    buttons.push_back(btn);

    // --- Center Buttons (Select/Start) ---
    int center_x = (SCREEN_WIDTH * SCALE) / 2;
    int bottom_y = (SCREEN_HEIGHT * SCALE) - 50;
    
    // Select (Rectangle)
    VirtualButton btnSelect;
    btnSelect.init_rect(center_x - 40, bottom_y, 50, 25, BTN_RECT, Input::BUTTON_SELECT);
    buttons.push_back(btnSelect);

    // Start (Small Triangle)
    VirtualButton btnStart;
    btnStart.init(center_x + 40, bottom_y, 20, BTN_SMALL_TRIANGLE, Input::BUTTON_START);
    buttons.push_back(btnStart);


    emu.reset();
    
    // Warmup
    for (int i = 0; i < 10; i++) emu.run_frame();
    
    // Palette hack
    emu.memory_.read(0x2002);
    emu.memory_.write(0x2006, 0x3F); emu.memory_.write(0x2006, 0x00);
    emu.memory_.write(0x2007, 0x0F); emu.memory_.write(0x2007, 0x30);
    emu.memory_.write(0x2007, 0x16); emu.memory_.write(0x2007, 0x27);
    
    bool quit = false;
    SDL_Event e;
    auto fps_timer = std::chrono::high_resolution_clock::now();
    int frame_count = 0;
    
    while (!quit) {
        auto frame_start = std::chrono::high_resolution_clock::now();

        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) quit = true;
            if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) quit = true;
            if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_r) emu.reset();
            
            joystick.handle_event(e);
            for (auto& b : buttons) b.handle_event(e);
        }

        const Uint8* currentKeyStates = SDL_GetKeyboardState(NULL);
        handle_input(emu, currentKeyStates, joystick, buttons);
        
        emu.run_frame();
        
        if (audio_device != 0) {
            const std::vector<float>& samples = emu.get_audio_samples();
            if (!samples.empty()) SDL_QueueAudio(audio_device, samples.data(), samples.size() * sizeof(float));
        }

        const uint8_t* framebuffer = emu.get_framebuffer();
        SDL_UpdateTexture(texture, NULL, framebuffer, SCREEN_WIDTH * 4);

        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        
        // Render UI
        joystick.render(renderer);
        for (auto& b : buttons) b.render(renderer);
        
        SDL_RenderPresent(renderer);
        
        auto current_time = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> elapsed_ms = current_time - frame_start;
        if (elapsed_ms.count() < 16.667) SDL_Delay((Uint32)(16.667 - elapsed_ms.count()));
        
        frame_count++;
        std::chrono::duration<double> elapsed_sec = current_time - fps_timer;
        if (elapsed_sec.count() >= 1.0) {
            std::string title = "NES Emulator - FPS: " + std::to_string(frame_count);
            SDL_SetWindowTitle(window, title.c_str());
            frame_count = 0;
            fps_timer = current_time;
        }
    }

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    if (audio_device != 0) SDL_CloseAudioDevice(audio_device);
    SDL_Quit();

    return 0;
}
