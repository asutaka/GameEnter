#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <string>
#include <map>

namespace nes {

class ConfigManager {
public:
    ConfigManager();
    ~ConfigManager();

    // Load config from file (or create default)
    void load();
    
    // Save config to file
    void save();

    // Getters
    std::string get_device_id() const;
    std::string get_nickname() const;
    std::string get_avatar_path() const;
    bool get_gameplay_recorder_enabled() const;

    // Setters
    void set_device_id(const std::string& value);
    void set_nickname(const std::string& value);
    void set_avatar_path(const std::string& value);
    void set_gameplay_recorder_enabled(bool value);

private:
    std::string generate_uuid();
    void parse_line(const std::string& line);

    std::string config_file_ = "config.ini";
    
    // Stored values
    std::string device_id_;
    std::string nickname_;
    std::string avatar_path_;
    bool gameplay_recorder_enabled_ = false;
};

}

#endif // CONFIG_MANAGER_H
