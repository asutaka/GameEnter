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

// --- Recorder System ---
struct ReplayFrame {
    uint8_t p1_buttons;
    uint8_t p2_buttons;
};

struct ReplayHeader {
    char signature[4] = {'N', 'E', 'S', 'R'}; // NES Replay
    uint32_t version = 1;
    uint32_t total_frames;
    char rom_hash[32]; // Placeholder for ROM hash
};

class Recorder {
public:
    bool is_recording = false;
    std::vector<ReplayFrame> frames;
    std::string current_rom_name;

    void start_recording(const std::string& rom_name) {
        is_recording = true;
        frames.clear();
        current_rom_name = rom_name;
        std::cout << "[Recorder] Started recording for: " << rom_name << std::endl;
    }

    void record_frame(uint8_t p1, uint8_t p2) {
        if (!is_recording) return;
        frames.push_back({p1, p2});
    }

    void stop_recording() {
        if (!is_recording) return;
        is_recording = false;
        save_to_file();
        std::cout << "[Recorder] Stopped recording. Total frames: " << frames.size() << std::endl;
    }

    void save_to_file() {
        if (frames.empty()) return;

        // 1. Get Exe Directory
        char buffer[MAX_PATH];
        GetModuleFileNameA(NULL, buffer, MAX_PATH);
        std::string::size_type pos = std::string(buffer).find_last_of("\\/");
        std::string exe_dir = std::string(buffer).substr(0, pos);

        // 2. Ensure 'saves' directory exists
        fs::path saves_dir = fs::path(exe_dir) / "saves";
        if (!fs::exists(saves_dir)) {
            fs::create_directory(saves_dir);
        }

        auto now = std::chrono::system_clock::now();
        auto in_time_t = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss_name;
        
        // Sanitize ROM name
        std::string safe_name = current_rom_name;
        std::replace(safe_name.begin(), safe_name.end(), ' ', '_');
        std::replace(safe_name.begin(), safe_name.end(), '.', '_');
        std::replace(safe_name.begin(), safe_name.end(), '/', '_');
        std::replace(safe_name.begin(), safe_name.end(), '\\', '_');

        ss_name << "replay_" << safe_name << "_" << std::put_time(std::localtime(&in_time_t), "%Y%m%d_%H%M%S") << ".rpl";
        fs::path full_path = saves_dir / ss_name.str();
        std::string filename = full_path.string();

        std::ofstream outfile(filename, std::ios::binary);
        if (!outfile) {
            std::cerr << "[Recorder] Failed to open file for saving: " << filename << std::endl;
            return;
        }

        ReplayHeader header;
        header.total_frames = (uint32_t)frames.size();
        // TODO: Calculate actual ROM hash
        memset(header.rom_hash, 0, 32); 
        
        outfile.write((char*)&header, sizeof(header));
        outfile.write((char*)frames.data(), frames.size() * sizeof(ReplayFrame));
        outfile.close();
        
        std::cout << "[Recorder] Saved replay to: " << filename << std::endl;
    }
};

Recorder recorder;

// --- Replay Player System ---
class ReplayPlayer {
public:
    bool is_playing = false;
    bool replay_finished = false;  // Flag to indicate replay has finished
    std::vector<ReplayFrame> frames;
    size_t current_frame_index = 0;
    std::string replay_name;
    
    bool load_replay(const std::string& filepath) {
        try {
            std::ifstream file(filepath, std::ios::binary);
            if (!file) {
                std::cerr << "[ReplayPlayer] Failed to open file: " << filepath << std::endl;
                return false;
            }
            
            // Read header
            ReplayHeader header;
            file.read((char*)&header, sizeof(header));
            
            // Validate signature
            if (header.signature[0] != 'N' || header.signature[1] != 'E' || 
                header.signature[2] != 'S' || header.signature[3] != 'R') {
                std::cerr << "[ReplayPlayer] Invalid replay file signature" << std::endl;
                return false;
            }
            
            // Read frames
            frames.resize(header.total_frames);
            file.read((char*)frames.data(), header.total_frames * sizeof(ReplayFrame));
            file.close();
            
            // Extract replay name from filepath
            size_t last_slash = filepath.find_last_of("/\\");
            replay_name = (last_slash == std::string::npos) ? filepath : filepath.substr(last_slash + 1);
            
            std::cout << "[ReplayPlayer] Loaded replay: " << replay_name << std::endl;
            std::cout << "               Total frames: " << frames.size() << std::endl;
            
            return true;
        } catch (const std::exception& e) {
            std::cerr << "[ReplayPlayer] Error loading replay: " << e.what() << std::endl;
            return false;
        }
    }
    
    float playback_speed = 1.0f;
    float speed_accumulator = 0.0f;

    void set_speed(float speed) {
        playback_speed = speed;
        if (playback_speed < 0.5f) playback_speed = 0.5f;
        if (playback_speed > 4.0f) playback_speed = 4.0f;
        std::cout << "[ReplayPlayer] Speed set to: " << playback_speed << "x" << std::endl;
    }

    void start_playback() {
        if (frames.empty()) {
            std::cerr << "[ReplayPlayer] No replay loaded" << std::endl;
            return;
        }
        
        is_playing = true;
        replay_finished = false;
        current_frame_index = 0;
        playback_speed = 1.0f;
        speed_accumulator = 0.0f;
        std::cout << "[ReplayPlayer] Started playback" << std::endl;
    }
    
    void stop_playback() {
        is_playing = false;
        current_frame_index = 0;
        playback_speed = 1.0f;
        speed_accumulator = 0.0f;
        std::cout << "[ReplayPlayer] Stopped playback" << std::endl;
    }

    void unload_replay() {
        stop_playback();
        frames.clear();
        replay_name = "";
        std::cout << "[ReplayPlayer] Unloaded replay" << std::endl;
    }
    
    void pause_playback() {
        is_playing = false;
        std::cout << "[ReplayPlayer] Paused playback at frame " << current_frame_index << std::endl;
    }
    
    void resume_playback() {
        if (!frames.empty() && current_frame_index < frames.size()) {
            is_playing = true;
            std::cout << "[ReplayPlayer] Resumed playback from frame " << current_frame_index << std::endl;
        }
    }
    
    // Get current frame inputs and advance to next frame
    bool get_next_frame(uint8_t& p1_buttons, uint8_t& p2_buttons) {
        if (!is_playing || current_frame_index >= frames.size()) {
            if (current_frame_index >= frames.size() && is_playing) {
                // Replay finished
                std::cout << "[ReplayPlayer] Replay finished" << std::endl;
                is_playing = false;
                replay_finished = true;  // Set flag to trigger return to Library
            }
            return false;
        }
        
        p1_buttons = frames[current_frame_index].p1_buttons;
        p2_buttons = frames[current_frame_index].p2_buttons;
        current_frame_index++;
        
        return true;
    }
    
    float get_progress() const {
        if (frames.empty()) return 0.0f;
        return (float)current_frame_index / (float)frames.size();
    }
    
    size_t get_current_frame() const {
        return current_frame_index;
    }
    
    size_t get_total_frames() const {
        return frames.size();
    }
};

ReplayPlayer replay_player;

// --- Replay Library System ---
struct ReplayFileInfo {
    std::string filename;        // Full filename (e.g., "replay_Contra_20251228_170530.rpl")
    std::string display_name;    // Display name (e.g., "Contra - 2025/12/28 17:05:30")
    std::string full_path;       // Full path to file
    uint32_t total_frames;       // Number of frames in replay
    std::string date_time;       // Formatted date/time
    size_t file_size;            // File size in bytes
    
    ReplayFileInfo() : total_frames(0), file_size(0) {}
};

// Function to scan saves folder and get list of replay files
std::vector<ReplayFileInfo> scan_replay_files() {
    std::vector<ReplayFileInfo> replays;
    
    try {
        // Get exe directory
        char buffer[MAX_PATH];
        GetModuleFileNameA(NULL, buffer, MAX_PATH);
        std::string::size_type pos = std::string(buffer).find_last_of("\\/");
        std::string exe_dir = std::string(buffer).substr(0, pos);
        
        fs::path saves_dir = fs::path(exe_dir) / "saves";
        
        // Check if saves directory exists
        if (!fs::exists(saves_dir)) {
            return replays; // Return empty list
        }
        
        // Iterate through all .rpl files
        for (const auto& entry : fs::directory_iterator(saves_dir)) {
            if (entry.is_regular_file() && entry.path().extension() == ".rpl") {
                ReplayFileInfo info;
                info.filename = entry.path().filename().string();
                info.full_path = entry.path().string();
                info.file_size = fs::file_size(entry.path());
                
                // Try to read header to get frame count
                std::ifstream file(info.full_path, std::ios::binary);
                if (file) {
                    ReplayHeader header;
                    file.read((char*)&header, sizeof(header));
                    if (file && header.signature[0] == 'N' && header.signature[1] == 'E' && 
                        header.signature[2] == 'S' && header.signature[3] == 'R') {
                        info.total_frames = header.total_frames;
                    }
                    file.close();
                }
                
                // Parse filename to extract game name and date/time
                // Format: replay_GameName_YYYYMMDD_HHMMSS.rpl
                std::string name = info.filename;
                if (name.substr(0, 7) == "replay_" && name.size() > 23) {
                    // Remove "replay_" prefix and ".rpl" suffix
                    name = name.substr(7, name.size() - 11);
                    
                    // Find last two underscores (date and time)
                    size_t last_underscore = name.rfind('_');
                    size_t second_last_underscore = name.rfind('_', last_underscore - 1);
                    
                    if (last_underscore != std::string::npos && second_last_underscore != std::string::npos) {
                        std::string game_name = name.substr(0, second_last_underscore);
                        std::string date_str = name.substr(second_last_underscore + 1, 8);
                        std::string time_str = name.substr(last_underscore + 1, 6);
                        
                        // Replace underscores with spaces in game name
                        std::replace(game_name.begin(), game_name.end(), '_', ' ');
                        
                        // Format date: YYYYMMDD -> YYYY/MM/DD
                        if (date_str.size() == 8) {
                            info.date_time = date_str.substr(0, 4) + "/" + 
                                           date_str.substr(4, 2) + "/" + 
                                           date_str.substr(6, 2);
                        }
                        
                        // Format time: HHMMSS -> HH:MM:SS
                        if (time_str.size() == 6) {
                            info.date_time += " " + time_str.substr(0, 2) + ":" + 
                                            time_str.substr(2, 2) + ":" + 
                                            time_str.substr(4, 2);
                        }
                        
                        info.display_name = game_name + " - " + info.date_time;
                    } else {
                        info.display_name = name;
                    }
                } else {
                    info.display_name = info.filename;
                }
                
                replays.push_back(info);
            }
        }
        
        // Sort by date/time (most recent first)
        // Extract YYYYMMDD_HHMMSS from filename and compare
        std::sort(replays.begin(), replays.end(), [](const ReplayFileInfo& a, const ReplayFileInfo& b) {
            // Extract date/time portion: replay_GameName_YYYYMMDD_HHMMSS.rpl
            auto extract_datetime = [](const std::string& filename) -> std::string {
                // Find last two underscores
                size_t last_dot = filename.rfind('.');
                if (last_dot == std::string::npos) return "";
                
                std::string name_part = filename.substr(0, last_dot);
                size_t last_underscore = name_part.rfind('_');
                if (last_underscore == std::string::npos) return "";
                
                size_t second_last_underscore = name_part.rfind('_', last_underscore - 1);
                if (second_last_underscore == std::string::npos) return "";
                
                // Return YYYYMMDD_HHMMSS
                return name_part.substr(second_last_underscore + 1);
            };
            
            std::string datetime_a = extract_datetime(a.filename);
            std::string datetime_b = extract_datetime(b.filename);
            
            return datetime_a > datetime_b; // Most recent first
        });
        
    } catch (const std::exception& e) {
        std::cerr << "Error scanning replay files: " << e.what() << std::endl;
    }
    
    return replays;
}

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

enum Scene { SCENE_HOME, SCENE_GAME, SCENE_SETTINGS, SCENE_MULTIPLAYER_LOBBY, SCENE_LOBBY };

// Home Screen Panels
enum HomePanel { 
    HOME_PANEL_ROM_GRID = 0,  // Default panel showing ROM grid
    HOME_PANEL_LIBRARY = 1,   // PlayBack (formerly Library)
    HOME_PANEL_FAVORITES = 2, // Duo (formerly Favorites)
    HOME_PANEL_TOURNAMENT = 3 // Tournament
};

int home_active_panel = HOME_PANEL_ROM_GRID; // Track current active panel

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
        set_pos(start_x, start_y);
        set_layout_normal();
    }

    void set_pos(int start_x, int start_y) {
        x = start_x; y = start_y; r = 25;
    }

    void set_layout_normal() {
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
        // 5. Timer (Top) - Replaces Settings
        items.push_back({x, y - 70, 20, 4});
    }

    void set_layout_replay() {
        items.clear();
        // Expand Upwards (Vertical Stack)
        // 1. Snapshot (id 1) - Bottom
        items.push_back({x, y - 60, 20, 1});
        // 2. Home (id 3) - Above Snapshot
        items.push_back({x, y - 110, 20, 3});
        
        // Others (Hidden in Replay, but available in Game)
        // 3. Reset (id 2)
        items.push_back({x, y - 160, 20, 2});
        // 4. Timer (id 4)
        items.push_back({x, y - 210, 20, 4});
        // 5. Share (id 0)
        items.push_back({x, y - 260, 20, 0});
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
                    // Skip Reset, Timer, and Share during replay
                    bool is_replaying = replay_player.is_playing || replay_player.get_current_frame() > 0;
                    if (is_replaying && (item.id == 2 || item.id == 4 || item.id == 0)) continue;

                    int idx = mx - item.x; int idy = my - item.y;
                    if (idx*idx + idy*idy <= item.r*item.r) {
                        if (item.id == 0) { // Share
                             std::cout << "[QuickBall] Share: Not Implemented" << std::endl;
                        } else if (item.id == 1) { // Snapshot
                             const uint8_t* fb = emu.get_framebuffer();
                             // Copy framebuffer to temp buffer to ensure safety
                             std::vector<uint8_t> temp_fb(256 * 240 * 4);
                             std::memcpy(temp_fb.data(), fb, temp_fb.size());
                             
                             SDL_Surface* ss = SDL_CreateRGBSurfaceWithFormatFrom(temp_fb.data(), 256, 240, 32, 256*4, SDL_PIXELFORMAT_RGBA32);
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
                            recorder.stop_recording(); // Stop previous recording
                            emu.reset();
                            // Restart recording if enabled
                            if (config.get_gameplay_recorder_enabled()) {
                                recorder.start_recording(recorder.current_rom_name);
                            }
                        } else if (item.id == 3) { // Home
                            recorder.stop_recording();
                            scene = SCENE_HOME;
                        } else if (item.id == 4) { // Timer
                            if (!timer_running) {
                                // Start Timer
                                timer_running = true;
                                timer_start_time = std::chrono::high_resolution_clock::now();
                                timer_show_final = false;
                            } else {
                                // Stop Timer
                                timer_running = false;
                                auto now = std::chrono::high_resolution_clock::now();
                                std::chrono::duration<double> elapsed = now - timer_start_time;
                                timer_final_value = elapsed.count();
                                timer_show_final = true;
                                timer_final_display_start = now;
                            }
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
                // Skip Reset, Timer, and Share during replay
                bool is_replaying = replay_player.is_playing || replay_player.get_current_frame() > 0;
                if (is_replaying && (item.id == 2 || item.id == 4 || item.id == 0)) continue;

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
                } else if (item.id == 4) { // Timer (Clock)
                    int ix = item.x, iy = item.y;
                    draw_circle_outline(renderer, ix, iy, 12); // Clock face
                    SDL_RenderDrawLine(renderer, ix, iy, ix, iy - 8); // Minute hand
                    SDL_RenderDrawLine(renderer, ix, iy, ix + 6, iy); // Hour hand
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
    
    // Pre-load if arg provided (into slot 0)
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--id" && i + 1 < argc) {
            config.set_device_id(argv[++i]);
            config.set_nickname("Player 2"); // Auto-set nickname for convenience
        } else if (arg.find(".nes") != std::string::npos) {
            slots[0].rom_path = arg;
            slots[0].name = "Game 1";
            slots[0].occupied = true;
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
    std::string duo_host_name = "";
    bool duo_rom_selector_open = false;
    int duo_active_input_field = -1; // -1: none, 0: host name
    
    // Lobby State
    bool lobby_is_host = false;
    bool lobby_player2_connected = false;
    std::string lobby_rom_path = "";
    std::string lobby_rom_name = "";
    std::string lobby_host_name = "";
    
    // --- UI SETUP ---
    VirtualJoystick joystick;
    joystick.init(100, (SCREEN_HEIGHT * SCALE) - 100, 60);

    int center_x = (SCREEN_WIDTH * SCALE) / 2;
    int bottom_y = (SCREEN_HEIGHT * SCALE) - 50;

    QuickBall quickBall;
    quickBall.init(center_x, bottom_y); // Default to Normal Layout (Center)

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
            if (e.type == SDL_QUIT) {
                recorder.stop_recording();
                quit = true;
            }
            if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) {
                if (current_scene == SCENE_GAME) {
                    recorder.stop_recording();
                    current_scene = SCENE_HOME;
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

                    // Multiplayer Button Click
                    if (mx >= 15 && mx <= 115 && my >= 15 && my <= 45) {
                        current_scene = SCENE_MULTIPLAYER_LOBBY;
                        discovery.start_advertising(config.get_device_id(), config.get_nickname(), "Menu", "", 6502);
                    }

                    // Settings Button Click (Top Right Dots)
                    int dots_x = SCREEN_WIDTH * SCALE - 40;
                    int dots_y = 50;
                    if (mx >= dots_x - 20 && mx <= dots_x + 20 && my >= dots_y - 20 && my <= dots_y + 20) {
                        current_scene = SCENE_SETTINGS;
                        settings_nickname = config.get_nickname();
                        settings_avatar_path = config.get_avatar_path();
                        settings_recorder_enabled = config.get_gameplay_recorder_enabled();
                        settings_loaded = true;
                    }

                    // Panel Tab Clicks
                    int tab_y = 100;
                    int tab_h = 40;
                    int tab_w = 150;
                    int tab_gap = 10;
                    int tab_start_x = 20;
                    
                    // Check if clicked on any tab
                    if (my >= tab_y && my <= tab_y + tab_h) {
                        if (mx >= tab_start_x && mx <= tab_start_x + tab_w) {
                            // ROM Grid tab
                            home_active_panel = HOME_PANEL_ROM_GRID;
                        } else if (mx >= tab_start_x + tab_w + tab_gap && mx <= tab_start_x + 2 * tab_w + tab_gap) {
                            // PlayBack tab
                            home_active_panel = HOME_PANEL_LIBRARY;
                        } else if (mx >= tab_start_x + 2 * (tab_w + tab_gap) && mx <= tab_start_x + 3 * tab_w + 2 * tab_gap) {
                            // Duo tab
                            home_active_panel = HOME_PANEL_FAVORITES;
                        } else if (mx >= tab_start_x + 3 * (tab_w + tab_gap) && mx <= tab_start_x + 4 * tab_w + 3 * tab_gap) {
                            // Tournament tab
                            home_active_panel = HOME_PANEL_TOURNAMENT;
                        }
                    }

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
                        int w = 150; int h = 100;
                        // Add Shortcut
                        if (mx >= menu_x && mx <= menu_x + w && my >= menu_y && my <= menu_y + 35) {
                            // Create Shortcut (.lnk) using PowerShell
                            if (context_menu_slot >= 0 && context_menu_slot < (int)slots.size()) {
                                std::string desktop_path = getenv("USERPROFILE");
                                desktop_path += "\\Desktop\\";
                                std::string shortcut_path = desktop_path + slots[context_menu_slot].name + ".lnk";
                                
                                char buffer[MAX_PATH];
                                GetModuleFileNameA(NULL, buffer, MAX_PATH);
                                std::string exe_path = buffer;
                                std::string rom_path = slots[context_menu_slot].rom_path;
                                std::string icon_path = slots[context_menu_slot].cover_path;

                                // Build PowerShell command to create shortcut
                                // Note: We use single quotes for PowerShell strings to avoid escaping hell
                                std::string ps_cmd = "powershell.exe -ExecutionPolicy Bypass -NoProfile -Command \"";
                                ps_cmd += "$ws = New-Object -ComObject WScript.Shell; ";
                                ps_cmd += "$s = $ws.CreateShortcut('" + shortcut_path + "'); ";
                                ps_cmd += "$s.TargetPath = '" + exe_path + "'; ";
                                // Argument needs double quotes inside single quotes to handle spaces in ROM path
                                ps_cmd += "$s.Arguments = '\"" + rom_path + "\"'; "; 
                                
                                // Set Icon if available
                                if (!icon_path.empty()) {
                                    ps_cmd += "$s.IconLocation = '" + icon_path + "'; ";
                                }
                                
                                ps_cmd += "$s.Save()\"";

                                // Execute command
                                system(ps_cmd.c_str());
                                std::cout << "✅ Created shortcut: " << shortcut_path << std::endl;
                            }
                            showing_context_menu = false;
                        }
                        // Change Cover
                        else if (mx >= menu_x && mx <= menu_x + w && my >= menu_y + 35 && my <= menu_y + 65) {
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
                                    // Import cover image to local storage
                                    std::string new_cover_path = import_cover_image(szFile, slots[context_menu_slot].name);
                                    slots[context_menu_slot].cover_path = new_cover_path;
                                    
                                    // Destroy texture cũ nếu có
                                    if (slots[context_menu_slot].cover_texture) {
                                        SDL_DestroyTexture(slots[context_menu_slot].cover_texture);
                                    }
                                    
                                    // Load texture mới
                                    slots[context_menu_slot].cover_texture = load_texture(renderer, new_cover_path);
                                    
                                    std::cout << "✅ Đã thay đổi cover: " << szFile << std::endl;
                                    
                                    // Auto-save
                                    std::vector<SlotManager::Slot> slots_to_save;
                                    for (const auto& slot : slots) {
                                        if (slot.occupied) {
                                            slots_to_save.push_back(SlotManager::Slot(slot.rom_path, slot.name, slot.cover_path));
                                        }
                                    }
                                    SlotManager::save_slots(slots_file, slots_to_save);
                                }
                            }
                            #endif
                            showing_context_menu = false;
                        }
                        // Delete
                        else if (mx >= menu_x && mx <= menu_x + w && my >= menu_y + 65 && my <= menu_y + 100) {
                            delete_candidate_index = context_menu_slot;
                            showing_delete_popup = true;
                            showing_context_menu = false;
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
                            // ROM Selector Dialog Clicks
                            int dialog_w = 600;
                            int dialog_h = 500;
                            int dialog_x = (SCREEN_WIDTH * SCALE - dialog_w) / 2;
                            int dialog_y = (SCREEN_HEIGHT * SCALE - dialog_h) / 2;
                            
                            // Close button
                            SDL_Rect close_btn = {dialog_x + dialog_w - 80, dialog_y + 20, 60, 30};
                            if (mx >= close_btn.x && mx <= close_btn.x + close_btn.w &&
                                my >= close_btn.y && my <= close_btn.y + close_btn.h) {
                                duo_rom_selector_open = false;
                            } else {
                                // ROM item clicks
                                int list_y = dialog_y + 70;
                                int item_h = 60;
                                int item_count = 0;
                                
                                for (size_t i = 0; i < slots.size() && i < 6; i++) {
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
                                    
                                    list_y += item_h + 10;
                                    item_count++;
                                }
                            }
                        } else {
                            // Main Duo Panel Clicks
                            int section_y = panel_content_y + 70; // After "CREATE HOST" title
                            int box_padding = 20;
                            int row_y = section_y + box_padding + 25; // First row after "Selected ROM:" label
                            
                            // Browse Button
                            SDL_Rect browse_btn = {content_x + content_width - 90, row_y, 70, 35};
                            if (mx >= browse_btn.x && mx <= browse_btn.x + browse_btn.w &&
                                my >= browse_btn.y && my <= browse_btn.y + browse_btn.h) {
                                duo_rom_selector_open = true;
                                std::cout << "📁 Opening ROM selector..." << std::endl;
                            }
                            
                            row_y += 75; // Move to host name row
                            
                            // Host Name Input Box
                            SDL_Rect name_box = {content_x + box_padding, row_y, 300, 35};
                            if (mx >= name_box.x && mx <= name_box.x + name_box.w &&
                                my >= name_box.y && my <= name_box.y + name_box.h) {
                                duo_active_input_field = 0;
                                std::cout << "✏️ Activated host name input" << std::endl;
                            } else {
                                // Click outside - deactivate
                                if (duo_active_input_field == 0) {
                                    duo_active_input_field = -1;
                                }
                            }
                            
                            // Create Host Button
                            bool can_create = !duo_selected_rom_path.empty() && !duo_host_name.empty();
                            SDL_Rect create_btn = {content_x + content_width - 150, row_y, 130, 35};
                            if (can_create && mx >= create_btn.x && mx <= create_btn.x + create_btn.w &&
                                my >= create_btn.y && my <= create_btn.y + create_btn.h) {
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
                                
                                // Ensure replay is unloaded so we don't get stuck in "Paused Replay" mode
                                replay_player.unload_replay();
                                
                                current_scene = SCENE_GAME;
                                
                                // Start Recording if enabled
                                if (config.get_gameplay_recorder_enabled()) {
                                    // Use filename as ROM name
                                    std::string rom_name = fs::path(slots[i].rom_path).filename().string();
                                    recorder.start_recording(rom_name);
                                }
                            }
                        }
                        mouse_down_slot = -1;
                    }
                }
            }


            // Settings Scene Interactions
            if (current_scene == SCENE_SETTINGS) {
                if (e.type == SDL_TEXTINPUT) {
                    if (active_input_field == 0) settings_nickname += e.text.text;
                } else if (e.type == SDL_KEYDOWN) {
                    if (e.key.keysym.sym == SDLK_BACKSPACE) {
                        std::string* target = nullptr;
                        if (active_input_field == 0) target = &settings_nickname;
                        
                        if (target && !target->empty()) {
                            target->pop_back();
                        }
                    }
                } else if (e.type == SDL_MOUSEBUTTONDOWN) {
                    int mx = e.button.x;
                    int my = e.button.y;
                    
                    // Input Fields Hitbox (Approximate)
                    int start_y = 100;
                    if (mx > 50 && mx < 400 && my > start_y && my < start_y + 30) { active_input_field = 0; SDL_StartTextInput(); }
                    else { active_input_field = -1; SDL_StopTextInput(); }

                    // Save Button
                    if (mx > 50 && mx < 150 && my > 300 && my < 340) {
                        config.set_nickname(settings_nickname);
                        config.set_avatar_path(settings_avatar_path);
                        config.set_gameplay_recorder_enabled(settings_recorder_enabled);
                        config.save();
                        current_scene = SCENE_HOME; // Return to Home
                    }
                    
                    // Recorder Checkbox
                    // Let's say it's at y=220
                    if (mx > 50 && mx < 400 && my > 220 && my < 250) {
                        settings_recorder_enabled = !settings_recorder_enabled;
                    }

                    // Back Button (Top Left)
                    if (mx >= 20 && mx <= 80 && my >= 20 && my <= 50) {
                        current_scene = SCENE_HOME; // Return without saving (or maybe prompt?)
                    }
                    
                    // Avatar Button
                    if (mx > 450 && mx < 550 && my > 100 && my < 200) {
                        std::string path = open_file_dialog("Images\0*.png;*.jpg;*.jpeg;*.bmp\0All Files\0*.*\0");
                        if (!path.empty()) {
                             std::string imported_path = import_avatar_image(path, config.get_device_id());
                             if (!imported_path.empty()) {
                                 settings_avatar_path = imported_path;
                             }
                        }
                    }
                }
            }

            // Multiplayer Lobby Interactions
            if (current_scene == SCENE_MULTIPLAYER_LOBBY) {
                if (e.type == SDL_MOUSEBUTTONDOWN) {
                    int mx = e.button.x;
                    int my = e.button.y;
                    // Back Button
                    if (mx >= 20 && mx <= 80 && my >= 20 && my <= 50) {
                        discovery.stop_advertising();
                        current_scene = SCENE_HOME;
                    }
                }
            }
        }

        SDL_SetRenderDrawColor(renderer, 240, 240, 240, 255); // White BG
        SDL_RenderClear(renderer);

        if (current_scene == SCENE_HOME) {
            // --- PANEL CONTAINER ---
            // Panel tabs (below header)
            int tab_y = 100;
            int tab_h = 40;
            int tab_w = 150;
            int tab_gap = 10;
            int tab_start_x = 20;
            
            // Define tabs
            struct PanelTab {
                std::string label;
                HomePanel panel_id;
            };
            std::vector<PanelTab> tabs = {
                {"ROM Grid", HOME_PANEL_ROM_GRID},
                {"PlayBack", HOME_PANEL_LIBRARY},
                {"Duo", HOME_PANEL_FAVORITES},
                {"Tournament", HOME_PANEL_TOURNAMENT}
            };
            
            // Draw tabs
            for (size_t i = 0; i < tabs.size(); i++) {
                int tx = tab_start_x + i * (tab_w + tab_gap);
                SDL_Rect tab_rect = {tx, tab_y, tab_w, tab_h};
                
                // Tab background
                if (home_active_panel == tabs[i].panel_id) {
                    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); // Active: White
                } else {
                    SDL_SetRenderDrawColor(renderer, 220, 220, 220, 255); // Inactive: Light gray
                }
                SDL_RenderFillRect(renderer, &tab_rect);
                
                // Tab border
                SDL_SetRenderDrawColor(renderer, 180, 180, 180, 255);
                SDL_RenderDrawRect(renderer, &tab_rect);
                
                // Tab label
                float label_w = font_body.get_text_width(tabs[i].label);
                int label_x = tx + (tab_w - (int)label_w) / 2;
                int label_y = tab_y + (tab_h - 18) / 2;
                
                SDL_Color text_color = (home_active_panel == tabs[i].panel_id) 
                    ? SDL_Color{34, 43, 50, 255}    // Active: Dark
                    : SDL_Color{120, 120, 120, 255}; // Inactive: Gray
                font_body.draw_text(renderer, tabs[i].label, label_x, label_y, text_color);
            }
            
            // Panel content area
            int panel_content_y = tab_y + tab_h;
            
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
                // --- DUO PANEL ---
                int panel_content_y = 140; // Below tabs
                int content_x = 40;
                int content_width = SCREEN_WIDTH * SCALE - 80;
                
                // === CREATE HOST SECTION ===
                int section_y = panel_content_y + 20;
                
                // Section Title
                font_title.draw_text(renderer, "CREATE HOST", content_x, section_y, {50, 50, 50, 255});
                section_y += 50;
                
                // Container Box
                SDL_Rect create_box = {content_x, section_y, content_width, 150};
                SDL_SetRenderDrawColor(renderer, 250, 250, 250, 255);
                SDL_RenderFillRect(renderer, &create_box);
                SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
                SDL_RenderDrawRect(renderer, &create_box);
                
                int box_padding = 20;
                int row_y = section_y + box_padding;
                
                // Row 1: ROM Selector
                font_body.draw_text(renderer, "Selected ROM:", content_x + box_padding, row_y, {80, 80, 80, 255});
                row_y += 25;
                
                // ROM Display Box
                SDL_Rect rom_box = {content_x + box_padding, row_y, content_width - 120, 35};
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                SDL_RenderFillRect(renderer, &rom_box);
                SDL_SetRenderDrawColor(renderer, 180, 180, 180, 255);
                SDL_RenderDrawRect(renderer, &rom_box);
                
                // ROM Name Text
                std::string rom_display = duo_selected_rom_name.empty() ? "No ROM selected" : duo_selected_rom_name;
                SDL_Color rom_text_color = duo_selected_rom_name.empty() ? SDL_Color{150, 150, 150, 255} : SDL_Color{50, 50, 50, 255};
                font_body.draw_text(renderer, rom_display, content_x + box_padding + 10, row_y + 8, rom_text_color);
                
                // Browse Button (📁)
                SDL_Rect browse_btn = {content_x + content_width - 90, row_y, 70, 35};
                SDL_SetRenderDrawColor(renderer, 100, 150, 250, 255);
                SDL_RenderFillRect(renderer, &browse_btn);
                SDL_SetRenderDrawColor(renderer, 80, 130, 230, 255);
                SDL_RenderDrawRect(renderer, &browse_btn);
                font_body.draw_text(renderer, "Browse", browse_btn.x + 8, browse_btn.y + 8, {255, 255, 255, 255});
                
                row_y += 50;
                
                // Row 2: Host Name Input
                font_body.draw_text(renderer, "Host Name:", content_x + box_padding, row_y, {80, 80, 80, 255});
                row_y += 25;
                
                SDL_Rect name_box = {content_x + box_padding, row_y, 300, 35};
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                SDL_RenderFillRect(renderer, &name_box);
                
                // Highlight if active
                if (duo_active_input_field == 0) {
                    SDL_SetRenderDrawColor(renderer, 100, 150, 250, 255);
                } else {
                    SDL_SetRenderDrawColor(renderer, 180, 180, 180, 255);
                }
                SDL_RenderDrawRect(renderer, &name_box);
                
                // Host Name Text
                std::string name_display = duo_host_name.empty() ? "" : duo_host_name;
                font_body.draw_text(renderer, name_display, name_box.x + 10, name_box.y + 8, {50, 50, 50, 255});
                
                // Cursor if active
                if (duo_active_input_field == 0) {
                    float text_w = font_body.get_text_width(name_display);
                    int cursor_x = name_box.x + 10 + (int)text_w;
                    SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
                    SDL_RenderDrawLine(renderer, cursor_x, name_box.y + 8, cursor_x, name_box.y + 27);
                }
                
                // Create Host Button
                bool can_create = !duo_selected_rom_path.empty() && !duo_host_name.empty();
                SDL_Rect create_btn = {content_x + content_width - 150, row_y, 130, 35};
                
                if (can_create) {
                    SDL_SetRenderDrawColor(renderer, 50, 200, 100, 255);
                } else {
                    SDL_SetRenderDrawColor(renderer, 180, 180, 180, 255);
                }
                SDL_RenderFillRect(renderer, &create_btn);
                
                SDL_Color btn_text_color = can_create ? SDL_Color{255, 255, 255, 255} : SDL_Color{220, 220, 220, 255};
                font_body.draw_text(renderer, "Create Host", create_btn.x + 15, create_btn.y + 8, btn_text_color);
                
                section_y += 170;
                
                // === AVAILABLE HOSTS SECTION ===
                font_title.draw_text(renderer, "AVAILABLE HOSTS", content_x, section_y, {50, 50, 50, 255});
                section_y += 50;
                
                // Get hosts from discovery
                auto available_hosts = discovery.get_peers();
                
                // Host list container
                SDL_Rect hosts_container = {content_x, section_y, content_width, 250};
                SDL_SetRenderDrawColor(renderer, 250, 250, 250, 255);
                SDL_RenderFillRect(renderer, &hosts_container);
                SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
                SDL_RenderDrawRect(renderer, &hosts_container);
                
                if (available_hosts.empty()) {
                    // Empty state message
                    int empty_cx = content_x + content_width / 2;
                    int empty_cy = section_y + 125;
                    font_body.draw_text(renderer, "No hosts found", empty_cx - 60, empty_cy - 10, {150, 150, 150, 255});
                    font_small.draw_text(renderer, "Searching for hosts on local network...", empty_cx - 110, empty_cy + 20, {180, 180, 180, 255});
                } else {
                    // Render host list
                    int item_y = section_y + 10;
                    int item_h = 70;
                    int item_margin = 10;
                    
                    for (size_t i = 0; i < available_hosts.size() && i < 3; i++) {
                        const auto& host = available_hosts[i];
                        
                        SDL_Rect item = {content_x + 10, item_y, content_width - 20, item_h};
                        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                        SDL_RenderFillRect(renderer, &item);
                        SDL_SetRenderDrawColor(renderer, 220, 220, 220, 255);
                        SDL_RenderDrawRect(renderer, &item);
                        
                        // Host icon (green circle)
                        SDL_SetRenderDrawColor(renderer, 50, 200, 100, 255);
                        draw_filled_circle(renderer, item.x + 25, item.y + item_h / 2, 8);
                        
                        // Host name
                        font_body.draw_text(renderer, host.username, item.x + 45, item.y + 10, {50, 50, 50, 255});
                        
                        // ROM name
                        std::string rom_info = "ROM: " + host.game_name;
                        font_small.draw_text(renderer, rom_info, item.x + 45, item.y + 35, {120, 120, 120, 255});
                        
                        // Check if we have this ROM
                        bool has_rom = false;
                        for (const auto& slot : slots) {
                            if (slot.occupied && slot.rom_path == host.rom_path) {
                                has_rom = true;
                                break;
                            }
                        }
                        
                        // Connect button
                        SDL_Rect connect_btn = {item.x + item.w - 110, item.y + 20, 90, 30};
                        if (has_rom) {
                            SDL_SetRenderDrawColor(renderer, 50, 150, 250, 255);
                        } else {
                            SDL_SetRenderDrawColor(renderer, 180, 180, 180, 255);
                        }
                        SDL_RenderFillRect(renderer, &connect_btn);
                        
                        SDL_Color btn_text = has_rom ? SDL_Color{255, 255, 255, 255} : SDL_Color{220, 220, 220, 255};
                        font_body.draw_text(renderer, "Connect", connect_btn.x + 15, connect_btn.y + 5, btn_text);
                        
                        // ROM not found indicator
                        if (!has_rom) {
                            font_small.draw_text(renderer, "❌ ROM not found", item.x + 45, item.y + 50, {200, 80, 80, 255});
                        }
                        
                        item_y += item_h + item_margin;
                    }
                }
                
                // ROM Selector Overlay
                if (duo_rom_selector_open) {
                    // Dim background
                    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 150);
                    SDL_Rect dim = {0, 0, SCREEN_WIDTH * SCALE, SCREEN_HEIGHT * SCALE};
                    SDL_RenderFillRect(renderer, &dim);
                    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
                    
                    // Selector Dialog
                    int dialog_w = 600;
                    int dialog_h = 500;
                    int dialog_x = (SCREEN_WIDTH * SCALE - dialog_w) / 2;
                    int dialog_y = (SCREEN_HEIGHT * SCALE - dialog_h) / 2;
                    
                    SDL_Rect dialog = {dialog_x, dialog_y, dialog_w, dialog_h};
                    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                    SDL_RenderFillRect(renderer, &dialog);
                    SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
                    SDL_RenderDrawRect(renderer, &dialog);
                    
                    // Title
                    font_title.draw_text(renderer, "Select ROM", dialog_x + 20, dialog_y + 20, {50, 50, 50, 255});
                    
                    // Close button
                    SDL_Rect close_btn = {dialog_x + dialog_w - 80, dialog_y + 20, 60, 30};
                    SDL_SetRenderDrawColor(renderer, 200, 80, 80, 255);
                    SDL_RenderFillRect(renderer, &close_btn);
                    font_body.draw_text(renderer, "Close", close_btn.x + 10, close_btn.y + 5, {255, 255, 255, 255});
                    
                    // ROM List (from slots)
                    int list_y = dialog_y + 70;
                    int item_h = 60;
                    
                    for (size_t i = 0; i < slots.size() && i < 6; i++) {
                        if (!slots[i].occupied) continue;
                        
                        SDL_Rect item = {dialog_x + 20, list_y, dialog_w - 40, item_h};
                        SDL_SetRenderDrawColor(renderer, 245, 245, 245, 255);
                        SDL_RenderFillRect(renderer, &item);
                        SDL_SetRenderDrawColor(renderer, 220, 220, 220, 255);
                        SDL_RenderDrawRect(renderer, &item);
                        
                        // ROM Name
                        font_body.draw_text(renderer, slots[i].name, item.x + 10, item.y + 10, {50, 50, 50, 255});
                        
                        // Path (truncated)
                        std::string path_display = slots[i].rom_path;
                        if (path_display.length() > 50) {
                            path_display = "..." + path_display.substr(path_display.length() - 47);
                        }
                        font_small.draw_text(renderer, path_display, item.x + 10, item.y + 35, {120, 120, 120, 255});
                        
                        list_y += item_h + 10;
                    }
                }

            } else if (home_active_panel == HOME_PANEL_TOURNAMENT) {
                // --- TOURNAMENT PANEL (Placeholder) ---
                int cx = (SCREEN_WIDTH * SCALE) / 2;
                int cy = (SCREEN_HEIGHT * SCALE) / 2;
                
                font_title.draw_text(renderer, "Tournament", cx - 90, cy - 50, {100, 100, 100, 255});
                font_body.draw_text(renderer, "Tournament brackets coming soon...", cx - 130, cy, {150, 150, 150, 255});
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

            // Multiplayer Button (Top Left)
            font_small.draw_text(renderer, "Multiplayer", 20, 20, {255, 255, 255, 255});
            SDL_Rect mp_btn = {15, 15, 100, 30};
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 50);
            SDL_RenderDrawRect(renderer, &mp_btn);

            // Menu Dots (Top Right) -> Settings Icon
            int dots_x = SCREEN_WIDTH * SCALE - 40;
            int dots_y = 50;
            
            // Draw Gear Icon
            SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
            draw_circle_outline(renderer, dots_x, dots_y, 12);
            draw_circle_outline(renderer, dots_x, dots_y, 6);
            for(int k=0; k<8; k++) {
                float angle = k * (3.14159f * 2 / 8);
                int tx1 = dots_x + (int)(cos(angle) * 12);
                int ty1 = dots_y + (int)(sin(angle) * 12);
                int tx2 = dots_x + (int)(cos(angle) * 16);
                int ty2 = dots_y + (int)(sin(angle) * 16);
                SDL_RenderDrawLine(renderer, tx1, ty1, tx2, ty2);
            }

            // --- CONTEXT MENU ---
            if (showing_context_menu) {
                int w = 150; int h = 100;
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
                font_small.draw_text(renderer, "Add Shortcut", menu_x + 10, menu_y + 15, {0, 0, 0, 255});
                
                // Separator 1
                SDL_SetRenderDrawColor(renderer, 230, 230, 230, 255);
                SDL_RenderDrawLine(renderer, menu_x, menu_y + 35, menu_x + w, menu_y + 35);

                // Change Cover Item
                font_small.draw_text(renderer, "Change Cover", menu_x + 10, menu_y + 45, {0, 100, 200, 255});
                
                // Separator 2
                SDL_SetRenderDrawColor(renderer, 230, 230, 230, 255);
                SDL_RenderDrawLine(renderer, menu_x, menu_y + 65, menu_x + w, menu_y + 65);

                // Delete Item
                font_small.draw_text(renderer, "Delete", menu_x + 10, menu_y + 75, {200, 50, 50, 255});
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

        } else if (current_scene == SCENE_LOBBY) {
            // === LOBBY SCENE ===
            SDL_SetRenderDrawColor(renderer, 240, 240, 240, 255);
            SDL_RenderClear(renderer);
            
            int cx = (SCREEN_WIDTH * SCALE) / 2;
            int cy = (SCREEN_HEIGHT * SCALE) / 2;
            
            // Title
            std::string title = lobby_is_host ? "Hosting: " + lobby_host_name : "Joining: " + lobby_host_name;
            font_title.draw_text(renderer, title, cx - 150, 100, {50, 50, 50, 255});
            
            // ROM Info
            std::string rom_info = "ROM: " + lobby_rom_name;
            font_body.draw_text(renderer, rom_info, cx - 100, 150, {100, 100, 100, 255});
            
            // Players Box
            SDL_Rect players_box = {cx - 200, cy - 100, 400, 200};
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            SDL_RenderFillRect(renderer, &players_box);
            SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
            SDL_RenderDrawRect(renderer, &players_box);
            
            // Player 1 (Host)
            SDL_SetRenderDrawColor(renderer, 50, 200, 100, 255);
            draw_filled_circle(renderer, players_box.x + 30, players_box.y + 50, 10);
            std::string p1_text = lobby_is_host ? lobby_host_name + " (You)" : lobby_host_name;
            font_body.draw_text(renderer, "🎮 Player 1: " + p1_text, players_box.x + 50, players_box.y + 40, {50, 50, 50, 255});
            
            // Player 2 (Client)
            if (lobby_player2_connected) {
                SDL_SetRenderDrawColor(renderer, 50, 200, 100, 255);
                draw_filled_circle(renderer, players_box.x + 30, players_box.y + 120, 10);
                std::string p2_text = lobby_is_host ? "Player 2" : "You";
                font_body.draw_text(renderer, "🎮 Player 2: " + p2_text, players_box.x + 50, players_box.y + 110, {50, 50, 50, 255});
            } else {
                SDL_SetRenderDrawColor(renderer, 180, 180, 180, 255);
                draw_filled_circle(renderer, players_box.x + 30, players_box.y + 120, 10);
                font_body.draw_text(renderer, "⏳ Waiting for Player 2...", players_box.x + 50, players_box.y + 110, {150, 150, 150, 255});
            }
            
            // Buttons
            if (lobby_is_host) {
                // Cancel Button
                SDL_Rect cancel_btn = {cx - 200, cy + 150, 100, 40};
                SDL_SetRenderDrawColor(renderer, 200, 80, 80, 255);
                SDL_RenderFillRect(renderer, &cancel_btn);
                font_body.draw_text(renderer, "Cancel", cancel_btn.x + 25, cancel_btn.y + 10, {255, 255, 255, 255});
                
                // Start Button (enabled only if P2 connected)
                SDL_Rect start_btn = {cx + 100, cy + 150, 100, 40};
                if (lobby_player2_connected) {
                    SDL_SetRenderDrawColor(renderer, 50, 200, 100, 255);
                } else {
                    SDL_SetRenderDrawColor(renderer, 180, 180, 180, 255);
                }
                SDL_RenderFillRect(renderer, &start_btn);
                SDL_Color start_color = lobby_player2_connected ? SDL_Color{255, 255, 255, 255} : SDL_Color{220, 220, 220, 255};
                font_body.draw_text(renderer, "Start", start_btn.x + 30, start_btn.y + 10, start_color);
            } else {
                // Leave Button (Client)
                SDL_Rect leave_btn = {cx - 50, cy + 150, 100, 40};
                SDL_SetRenderDrawColor(renderer, 200, 80, 80, 255);
                SDL_RenderFillRect(renderer, &leave_btn);
                font_body.draw_text(renderer, "Leave", leave_btn.x + 30, leave_btn.y + 10, {255, 255, 255, 255});
                
                // Status
                font_small.draw_text(renderer, "Waiting for host to start...", cx - 100, cy + 120, {120, 120, 120, 255});
            }

        } else if (current_scene == SCENE_SETTINGS) {
            // BG
            SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
            SDL_RenderClear(renderer);
            
            // Back Button
            SDL_Rect back_btn = {20, 20, 60, 30};
            SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
            SDL_RenderFillRect(renderer, &back_btn);
            font_small.draw_text(renderer, "Back", 30, 25, {255, 255, 255, 255});

            font_title.draw_text(renderer, "Settings", 100, 30, {255, 255, 255, 255});
            
            int start_y = 100;
            
            // Nickname
            font_body.draw_text(renderer, "Nickname:", 50, start_y - 25, {200, 200, 200, 255});
            SDL_Rect box1 = {50, start_y, 300, 30};
            SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
            SDL_RenderFillRect(renderer, &box1);
            if (active_input_field == 0) {
                SDL_SetRenderDrawColor(renderer, 100, 100, 200, 255);
                SDL_RenderDrawRect(renderer, &box1);
            }
            font_body.draw_text(renderer, settings_nickname, 55, start_y + 5, {255, 255, 255, 255});

            // Avatar
            font_body.draw_text(renderer, "Avatar:", 450, start_y - 25, {200, 200, 200, 255});
            SDL_Rect avatar_box = {450, start_y, 100, 100};
            
            // Render Avatar Image if available
            SDL_Texture* avatar_tex = nullptr;
            if (!settings_avatar_path.empty()) {
                avatar_tex = load_texture(renderer, settings_avatar_path);
            }

            if (avatar_tex) {
                SDL_RenderCopy(renderer, avatar_tex, NULL, &avatar_box);
                SDL_DestroyTexture(avatar_tex); // Clean up immediately as we reload every frame (inefficient but simple for now)
            } else {
                SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
                SDL_RenderFillRect(renderer, &avatar_box);
                font_small.draw_text(renderer, "Click to change", 455, start_y + 40, {150, 150, 150, 255});
            }
            SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255); // Border
            SDL_RenderDrawRect(renderer, &avatar_box);
            
            // Gameplay Recorder Checkbox
            int cb_y = 220;
            SDL_Rect cb_box = {50, cb_y, 20, 20};
            SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
            SDL_RenderDrawRect(renderer, &cb_box);
            if (settings_recorder_enabled) {
                SDL_SetRenderDrawColor(renderer, 0, 200, 0, 255);
                SDL_RenderFillRect(renderer, &cb_box);
                // Checkmark
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                SDL_RenderDrawLine(renderer, 50 + 4, cb_y + 10, 50 + 8, cb_y + 16);
                SDL_RenderDrawLine(renderer, 50 + 8, cb_y + 16, 50 + 16, cb_y + 4);
            }
            font_body.draw_text(renderer, "Enable Gameplay Recorder", 80, cb_y, {255, 255, 255, 255});

            // Save Button
            SDL_Rect save_btn = {50, 300, 100, 40};
            SDL_SetRenderDrawColor(renderer, 0, 150, 0, 255);
            SDL_RenderFillRect(renderer, &save_btn);
            font_body.draw_text(renderer, "Save", 75, 310, {255, 255, 255, 255});

        } else if (current_scene == SCENE_MULTIPLAYER_LOBBY) {
             SDL_SetRenderDrawColor(renderer, 20, 20, 40, 255);
             SDL_RenderClear(renderer);
             font_title.draw_text(renderer, "Lobby", 100, 30, {255, 255, 255, 255});
             
             // Back Button
             SDL_Rect back_btn = {20, 20, 60, 30};
             SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
             SDL_RenderFillRect(renderer, &back_btn);
             font_small.draw_text(renderer, "Back", 30, 25, {255, 255, 255, 255});

             // List Peers
             auto peers = discovery.get_peers();
             int y = 100;
             if (peers.empty()) {
                 font_body.draw_text(renderer, "Scanning for players...", 50, y, {150, 150, 150, 255});
             } else {
                 for (const auto& peer : peers) {
                     std::string info = peer.username + " (" + peer.game_name + ")";
                     font_body.draw_text(renderer, info, 50, y, {255, 255, 255, 255});
                     y += 30;
                 }
             }

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
                     // Normal Game: Handle Input & Run Frame
                     handle_input(emu, currentKeyStates, joystick, buttons, connected_controllers);
                     emu.run_frame();
                     emulator_ran = true;
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
                for (auto& b : buttons) b.render(renderer);
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
                draw_filled_circle(renderer, start_x, bottom_y, btn_radius);
                SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
                draw_circle_outline(renderer, start_x, bottom_y, btn_radius);
                
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
                draw_filled_circle(renderer, ff_x, bottom_y, btn_radius);
                SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
                draw_circle_outline(renderer, ff_x, bottom_y, btn_radius);
                
                // Icon (>>)
                SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
                draw_filled_triangle(renderer, ff_x - 6, bottom_y - 6, ff_x + 2, bottom_y, ff_x - 6, bottom_y + 6);
                draw_filled_triangle(renderer, ff_x + 2, bottom_y - 6, ff_x + 10, bottom_y, ff_x + 2, bottom_y + 6);

                // 3. Slow Down (<<)
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 200);
                draw_filled_circle(renderer, rw_x, bottom_y, btn_radius);
                SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
                draw_circle_outline(renderer, rw_x, bottom_y, btn_radius);

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
