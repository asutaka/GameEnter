#pragma once
#include <vector>
#include <string>
#include <cstdint>
#include <iostream>
#include <fstream>
#include <sstream>
#include <ctime>
#include <iomanip>
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;

// --- Replay Data Structures ---
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

struct ReplayFileInfo {
    std::string filename;     // Full filename (e.g., "replay_Contra_20251228_170530.rpl")
    std::string display_name; // Display name (e.g., "Contra - 2025/12/28 17:05:30")
    std::string full_path;    // Absolute path
    uint32_t total_frames;    // Number of frames in replay
    std::string date_time;    // Formatted date/time
    size_t file_size;         // File size in bytes

    ReplayFileInfo() : total_frames(0), file_size(0) {}
};

// --- Recorder Class ---
// Handles recording of gameplay inputs
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
        std::cout << "[Recorder] Stopped recording. Total frames: " << frames.size() << std::endl;
        save_to_file();
    }

    void save_to_file() {
        if (frames.empty()) return;

        // Create saves directory if not exists
        if (!fs::exists("saves")) {
            fs::create_directory("saves");
        }

        // Generate filename with timestamp
        auto t = std::time(nullptr);
        auto tm = *std::localtime(&t);
        std::ostringstream oss;
        oss << "saves/replay_" << current_rom_name << "_" 
            << std::put_time(&tm, "%Y%m%d_%H%M%S") << ".rpl";
        std::string filename = oss.str();

        std::ofstream outfile(filename, std::ios::binary);
        if (!outfile) {
            std::cerr << "[Recorder] Failed to open file for writing: " << filename << std::endl;
            return;
        }

        // Write Header
        ReplayHeader header;
        header.total_frames = (uint32_t)frames.size();
        outfile.write((char*)&header, sizeof(header));

        // Write Frames
        outfile.write((char*)frames.data(), frames.size() * sizeof(ReplayFrame));
        outfile.close();
        
        std::cout << "[Recorder] Saved replay to: " << filename << std::endl;
    }
};

// --- Replay Player Class ---
// Handles playback of recorded inputs
class ReplayPlayer {
public:
    bool is_playing = false;
    bool replay_finished = false; // Flag to indicate replay has finished
    std::vector<ReplayFrame> frames;
    size_t current_frame_index = 0;
    std::string replay_name;
    
    // Playback control
    float playback_speed = 1.0f; // 1.0 = Normal, 2.0 = 2x, 0.5 = 0.5x
    float speed_accumulator = 0.0f;

    bool load_replay(const std::string& filepath) {
        std::ifstream infile(filepath, std::ios::binary);
        if (!infile) {
            std::cerr << "[ReplayPlayer] Failed to open replay file: " << filepath << std::endl;
            return false;
        }

        ReplayHeader header;
        infile.read((char*)&header, sizeof(header));

        if (strncmp(header.signature, "NESR", 4) != 0) {
            std::cerr << "[ReplayPlayer] Invalid replay file format." << std::endl;
            return false;
        }

        frames.resize(header.total_frames);
        infile.read((char*)frames.data(), header.total_frames * sizeof(ReplayFrame));
        infile.close();

        current_frame_index = 0;
        replay_name = fs::path(filepath).filename().string();
        replay_finished = false;
        
        std::cout << "[ReplayPlayer] Loaded replay: " << replay_name << " (" << frames.size() << " frames)" << std::endl;
        return true;
    }

    void set_speed(float speed) {
        playback_speed = speed;
        if (playback_speed < 0.25f) playback_speed = 0.25f;
        if (playback_speed > 8.0f) playback_speed = 8.0f;
    }

    void start_playback() {
        if (frames.empty()) return;
        is_playing = true;
        replay_finished = false;
        playback_speed = 1.0f;
        speed_accumulator = 0.0f;
        std::cout << "[ReplayPlayer] Playback started." << std::endl;
    }

    void stop_playback() {
        is_playing = false;
        current_frame_index = 0;
        replay_finished = false;
        std::cout << "[ReplayPlayer] Playback stopped." << std::endl;
    }
    
    void unload_replay() {
        if (is_playing) stop_playback();
        frames.clear();
        replay_name = "";
    }

    void pause_playback() {
        is_playing = false;
    }

    void resume_playback() {
        if (!frames.empty() && !replay_finished) {
            is_playing = true;
        }
    }

    // Get current frame inputs and advance to next frame
    bool get_next_frame(uint8_t& p1_buttons, uint8_t& p2_buttons) {
        if (!is_playing || frames.empty()) return false;

        if (current_frame_index >= frames.size()) {
            is_playing = false;
            replay_finished = true;
            std::cout << "[ReplayPlayer] Replay finished." << std::endl;
            return false;
        }

        const auto& frame = frames[current_frame_index];
        p1_buttons = frame.p1_buttons;
        p2_buttons = frame.p2_buttons;

        // Handle Speed
        speed_accumulator += playback_speed;
        while (speed_accumulator >= 1.0f) {
             current_frame_index++;
             speed_accumulator -= 1.0f;
             if (current_frame_index >= frames.size()) {
                 is_playing = false;
                 replay_finished = true;
                 break;
             }
        }

        return true;
    }

    float get_progress() {
        if (frames.empty()) return 0.0f;
        return (float)current_frame_index / frames.size();
    }
    
    size_t get_current_frame() { return current_frame_index; }
    size_t get_total_frames() { return frames.size(); }
};

// --- Replay Library Helper ---
// Helper to scan for replay files
inline std::vector<ReplayFileInfo> scan_replay_files() {
    std::vector<ReplayFileInfo> files;
    std::string saves_dir = "saves";
    
    if (!fs::exists(saves_dir)) {
        fs::create_directory(saves_dir);
        return files;
    }

    for (const auto& entry : fs::directory_iterator(saves_dir)) {
        if (entry.path().extension() == ".rpl") {
            ReplayFileInfo info;
            info.full_path = entry.path().string();
            info.filename = entry.path().filename().string();
            info.file_size = entry.file_size();
            
            // Parse filename for easy display: "replay_GameName_YYYYMMDD_HHMMSS.rpl"
            std::string name = info.filename;
            if (name.rfind("replay_", 0) == 0) name = name.substr(7); // Remove "replay_"
            size_t ext_pos = name.find(".rpl");
            if (ext_pos != std::string::npos) name = name.substr(0, ext_pos); // Remove ".rpl"
            
            // Find last underscore for time
            size_t last_us = name.find_last_of('_');
            if (last_us != std::string::npos) {
                // Check if the part before is also a date format
                 size_t second_last_us = name.find_last_of('_', last_us - 1);
                 if (second_last_us != std::string::npos) {
                     std::string date_part = name.substr(second_last_us + 1, last_us - second_last_us - 1); // YYYYMMDD
                     std::string time_part = name.substr(last_us + 1); // HHMMSS
                     std::string game_name = name.substr(0, second_last_us);
                     
                     // Format Date Time
                     if (date_part.length() == 8 && time_part.length() == 6) {
                         info.date_time = date_part.substr(0, 4) + "/" + date_part.substr(4, 2) + "/" + date_part.substr(6, 2) + " " +
                                          time_part.substr(0, 2) + ":" + time_part.substr(2, 2) + ":" + time_part.substr(4, 2);
                         info.display_name = game_name; 
                     } else {
                         info.display_name = name;
                         info.date_time = "Unknown Date";
                     }
                 } else {
                     info.display_name = name;
                     info.date_time = "Unknown Date";
                 }
            } else {
                info.display_name = name;
                info.date_time = "Unknown Date";
            }
            
            // Read header for duration
            std::ifstream infile(info.full_path, std::ios::binary);
            if (infile) {
                ReplayHeader header;
                infile.read((char*)&header, sizeof(header));
                if (strncmp(header.signature, "NESR", 4) == 0) {
                    info.total_frames = header.total_frames;
                }
                infile.close();
            }

            files.push_back(info);
        }
    }
    
    // Sort by Date (Reverse - Newest first)
    // We can rely on filename sort since it contains YYYYMMDD_HHMMSS
    std::sort(files.begin(), files.end(), [](const ReplayFileInfo& a, const ReplayFileInfo& b) {
        return a.filename > b.filename; 
    });

    return files;
}
