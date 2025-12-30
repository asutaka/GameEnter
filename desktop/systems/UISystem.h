#pragma once

#include <SDL2/SDL.h>
#include <string>
#include <vector>
#include <cmath>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <map>
#include <cstdlib>
#include <sstream>

#include "../stb_image.h"
#include "AppPath.h"

namespace fs = std::filesystem;

// --- Helper Functions Declaration ---

// Helper to load texture from file
// Note: Requires STB_IMAGE_IMPLEMENTATION to be defined in one compilation unit (main_sdl.cpp)
inline SDL_Texture* load_texture(SDL_Renderer* renderer, const std::string& path) {
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
inline std::string find_cover_image(const std::string& rom_path) {
    std::string base_path = rom_path.substr(0, rom_path.find_last_of("/\\") + 1);
    std::string full_filename = rom_path.substr(rom_path.find_last_of("/\\") + 1);
    std::string name_no_ext = full_filename.substr(0, full_filename.find_last_of("."));
    
    std::vector<std::string> search_paths;
    search_paths.push_back(base_path); // ROM folder
    search_paths.push_back((nes::get_app_dir() / "images/").string()); // Images folder
    
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

// Helper to open file dialog (Windows only for now, simple wrapper)
#ifdef _WIN32
#include <windows.h>
#include <commdlg.h>
inline std::string open_file_dialog(const char* filter = "NES ROMs\0*.nes\0All Files\0*.*\0") {
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
inline std::string open_file_dialog(const char* filter = "") { return ""; } // Not implemented for other OS
#endif

// Helper: Create Windows Shortcut (.lnk)
inline bool create_shortcut(const std::string& name, const std::string& target_args) {
#ifdef _WIN32
    char exe_path[MAX_PATH];
    GetModuleFileNameA(NULL, exe_path, MAX_PATH);
    std::string app_exe = exe_path;
    
    const char* userprofile = std::getenv("USERPROFILE");
    if (!userprofile) return false;
    
    std::filesystem::path link_path = std::filesystem::path(userprofile) / "Desktop" / (name + ".lnk");

    // Escape single quotes for PowerShell
    auto escape_ps = [](std::string s) {
        size_t pos = 0;
        while ((pos = s.find("'", pos)) != std::string::npos) {
            s.replace(pos, 1, "''");
            pos += 2;
        }
        return s;
    };

    std::string escaped_link = escape_ps(link_path.string());
    std::string escaped_exe = escape_ps(app_exe);
    
    std::stringstream ps_cmd;
    ps_cmd << "powershell -WindowStyle Hidden -Command \"$s=(New-Object -COM WScript.Shell).CreateShortcut('" << escaped_link << "');"
           << "$s.TargetPath='" << escaped_exe << "';";
    
    if (!target_args.empty()) {
        std::string escaped_args = escape_ps(target_args);
        // Note: we need to wrap the path in escaped double quotes for the command line argument
        ps_cmd << "$s.Arguments='\\\"" << escaped_args << "\\\"';";
    }
    
    ps_cmd << "$s.Save()\"";
    
    int result = system(ps_cmd.str().c_str());
    return result == 0;
#else
    return false;
#endif
}

// Helper: Import cover image to local storage
inline std::string import_cover_image(const std::string& source_path, const std::string& game_name) {
    try {
        // 1. Get Exe Directory
        fs::path covers_dir = nes::get_app_dir() / "images" / "covers";
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
        fs::copy_file(source_path, dest_path, fs::copy_options::overwrite_existing);
        
        std::cout << "📥 Imported cover to: " << dest_path.string() << std::endl;
        return dest_path.string();
    } catch (const std::exception& e) {
        std::cerr << "❌ Error importing cover: " << e.what() << std::endl;
        return source_path; // Fallback to original path
    }
}

// Helper: Import avatar image to local storage
inline std::string import_avatar_image(const std::string& source_path, const std::string& device_id) {
    try {
        fs::path avatars_dir = nes::get_app_dir() / "images" / "avatars";
        if (!fs::exists(avatars_dir)) {
            fs::create_directories(avatars_dir);
        }

        // 3. Generate Destination Path
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

// --- Drawing Helpers ---

// Helper to draw filled circle with Anti-Aliasing
inline void draw_filled_circle(SDL_Renderer* renderer, int cx, int cy, int radius) {
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
inline void draw_filled_circle_aa(SDL_Renderer* renderer, int cx, int cy, int radius) {
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
inline void draw_circle_outline_aa(SDL_Renderer* renderer, int cx, int cy, int radius) {
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
inline void draw_circle_outline(SDL_Renderer* renderer, int cx, int cy, int radius) {
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
inline void draw_filled_triangle(SDL_Renderer* renderer, int x1, int y1, int x2, int y2, int x3, int y3) {
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
inline void draw_nes_cartridge(SDL_Renderer* renderer, int x, int y, int scale) {
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

// --- UI Classes ---

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
