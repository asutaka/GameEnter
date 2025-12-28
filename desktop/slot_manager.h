#ifndef SLOT_MANAGER_H
#define SLOT_MANAGER_H

#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <filesystem>

/**
 * @brief Quản lý lưu/load danh sách ROM slots
 * 
 * Lưu các ROM đã thêm vào slots để khi mở lại app,
 * các slots vẫn còn đó.
 */
class SlotManager {
public:
    struct Slot {
        std::string rom_path;
        std::string name;
        std::string cover_path;  // Path to cover image
        bool occupied = false;
        
        Slot() = default;
        Slot(const std::string& path, const std::string& n, const std::string& cover = "") 
            : rom_path(path), name(n), cover_path(cover), occupied(true) {}
    };
    
    /**
     * @brief Lưu danh sách slots vào file
     */
    static bool save_slots(const std::string& filename, const std::vector<Slot>& slots) {
        std::ofstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Không thể tạo file slots: " << filename << std::endl;
            return false;
        }
        
        std::cout << "💾 Đang lưu slots vào: " << filename << std::endl;
        
        // Lưu từng slot
        int saved_count = 0;
        for (const auto& slot : slots) {
            if (slot.occupied) {
                std::cout << "   - Checking slot: " << slot.name << " (" << slot.rom_path << ")" << std::endl;
                // Check xem file ROM còn tồn tại không
                if (std::filesystem::exists(slot.rom_path)) {
                    file << slot.rom_path << "\n";
                    file << slot.name << "\n";
                    file << slot.cover_path << "\n";  // Lưu cover path
                    saved_count++;
                } else {
                    std::cout << "⚠️  ROM không tồn tại, bỏ qua: " << slot.rom_path << std::endl;
                }
            }
        }
        
        std::cout << "💾 Đã lưu " << saved_count << " slots vào: " << filename << std::endl;
        return true;
    }
    
    /**
     * @brief Load danh sách slots từ file
     */
    static bool load_slots(const std::string& filename, std::vector<Slot>& slots) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            // File không tồn tại - không phải lỗi, chỉ là lần đầu chạy
            return false;
        }
        
        slots.clear();
        std::string rom_path, name, cover_path;
        
        while (std::getline(file, rom_path)) {
            if (std::getline(file, name)) {
                std::getline(file, cover_path);  // Load cover path
                
                // Check xem ROM còn tồn tại không
                if (std::filesystem::exists(rom_path)) {
                    slots.push_back(Slot(rom_path, name, cover_path));
                    std::cout << "✅ Load slot: " << name << std::endl;
                } else {
                    std::cout << "⚠️  ROM không tồn tại, bỏ qua: " << rom_path << std::endl;
                }
            }
        }
        
        std::cout << "📂 Đã load " << slots.size() << " slots từ: " << filename << std::endl;
        return true;
    }
    
    /**
     * @brief Kiểm tra file slots có tồn tại không
     */
    static bool exists(const std::string& filename) {
        return std::filesystem::exists(filename);
    }
};

#endif // SLOT_MANAGER_H
