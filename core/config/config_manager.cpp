#include "config_manager.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <random>
#include <iomanip>
#include <filesystem>

namespace nes {

ConfigManager::ConfigManager() {
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
}

void ConfigManager::save() {
    std::ofstream file(config_file_);
    if (file.is_open()) {
        file << "device_id=" << device_id_ << "\n";
        file << "nickname=" << nickname_ << "\n";
        file << "avatar_path=" << avatar_path_ << "\n";
    }
}

void ConfigManager::parse_line(const std::string& line) {
    if (line.empty() || line[0] == '#' || line[0] == ';') return;

    size_t delimiter_pos = line.find('=');
    if (delimiter_pos != std::string::npos) {
        std::string key = line.substr(0, delimiter_pos);
        std::string value = line.substr(delimiter_pos + 1);

        if (key == "device_id") device_id_ = value;
        else if (key == "nickname") nickname_ = value;
        else if (key == "avatar_path") avatar_path_ = value;
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

// Setters
void ConfigManager::set_device_id(const std::string& value) { device_id_ = value; }
void ConfigManager::set_nickname(const std::string& value) { nickname_ = value; }
void ConfigManager::set_avatar_path(const std::string& value) { avatar_path_ = value; }

}
