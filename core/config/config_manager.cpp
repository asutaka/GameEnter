#include "config_manager.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <random>
#include <iomanip>
#include <filesystem>

namespace nes {

#ifdef _WIN32
#include <windows.h> // For GetModuleFileNameA
#endif

ConfigManager::ConfigManager() : nickname_("player") {
    // Determine absolute path for config.ini based on executable location
#ifdef _WIN32
    char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);
    std::string::size_type pos = std::string(buffer).find_last_of("\\/");
    std::string exe_dir = std::string(buffer).substr(0, pos);
#else
    std::string exe_dir = ".";
#endif
    
    std::filesystem::path data_dir = std::filesystem::path(exe_dir) / "data";
    if (!std::filesystem::exists(data_dir)) {
        std::filesystem::create_directories(data_dir);
    }
    
    config_file_ = (data_dir / "config.ini").string();

    load();
}

ConfigManager::~ConfigManager() {
    save();
}

void ConfigManager::load() {
    std::ifstream file(config_file_);
    if (!file.is_open()) {
        // First run: Generate Device ID
        device_id_ = generate_uuid();
        save();
        return;
    }

    std::string line;
    while (std::getline(file, line)) {
        parse_line(line);
    }

    // If device_id is still empty (corrupt file?), generate one
    if (device_id_.empty()) {
        device_id_ = generate_uuid();
        save();
    }
    
    if (nickname_.empty()) {
        nickname_ = "player";
    }
}

void ConfigManager::save() {
    std::ofstream file(config_file_);
    if (file.is_open()) {
        file << "device_id=" << device_id_ << "\n";
        file << "nickname=" << nickname_ << "\n";
        file << "avatar_path=" << avatar_path_ << "\n";
        file << "gameplay_recorder_enabled=" << (gameplay_recorder_enabled_ ? "1" : "0") << "\n";
        std::cout << "[Config] Saved to " << config_file_ << ": " << nickname_ << ", " << avatar_path_ << ", Recorder: " << gameplay_recorder_enabled_ << std::endl;
    } else {
        std::cerr << "[Config] Failed to open file for writing: " << config_file_ << std::endl;
    }
}

void ConfigManager::parse_line(const std::string& line) {
    if (line.empty() || line[0] == '#' || line[0] == ';') return;

    size_t delimiter_pos = line.find('=');
    if (delimiter_pos != std::string::npos) {
        std::string key = line.substr(0, delimiter_pos);
        std::string value = line.substr(delimiter_pos + 1);

        // Trim whitespace and \r
        key.erase(0, key.find_first_not_of(" \t"));
        key.erase(key.find_last_not_of(" \t\r") + 1);
        value.erase(0, value.find_first_not_of(" \t"));
        value.erase(value.find_last_not_of(" \t\r") + 1);

        if (key == "device_id") device_id_ = value;
        else if (key == "nickname") nickname_ = value;
        else if (key == "avatar_path") avatar_path_ = value;
        else if (key == "gameplay_recorder_enabled") gameplay_recorder_enabled_ = (value == "1" || value == "true");
    }
}

std::string ConfigManager::generate_uuid() {
    // Simple random hex string generator (32 chars)
    // Not a true UUID v4, but sufficient for local ID
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);

    std::stringstream ss;
    for (int i = 0; i < 32; i++) {
        ss << std::hex << dis(gen);
    }
    return ss.str();
}

// Getters
std::string ConfigManager::get_device_id() const { return device_id_; }
std::string ConfigManager::get_nickname() const { return nickname_; }
std::string ConfigManager::get_avatar_path() const { return avatar_path_; }
bool ConfigManager::get_gameplay_recorder_enabled() const { return gameplay_recorder_enabled_; }

// Setters
void ConfigManager::set_device_id(const std::string& value) { device_id_ = value; }
void ConfigManager::set_nickname(const std::string& value) { nickname_ = value; }
void ConfigManager::set_avatar_path(const std::string& value) { avatar_path_ = value; }
void ConfigManager::set_gameplay_recorder_enabled(bool value) { gameplay_recorder_enabled_ = value; }

}
