#ifndef NES_CARTRIDGE_H
#define NES_CARTRIDGE_H

#include <cstdint>
#include <vector>
#include <string>

namespace nes {

class Mapper;

/**
 * @brief Nametable mirroring modes
 */
enum class MirrorMode {
    HORIZONTAL,   // Vertical arrangement (Mario, Donkey Kong)
    VERTICAL,     // Horizontal arrangement (some games)
    FOUR_SCREEN,  // 4KB VRAM (rare)
    SINGLE_SCREEN // One nametable (rare)
};

/**
 * @brief Cartridge (game ROM) loader và manager
 */
class Cartridge {
public:
    Cartridge();
    ~Cartridge();
    
    /**
     * @brief Load ROM file (.nes format)
     */
    bool load_from_file(const std::string& filename);
    
    /**
     * @brief Đọc từ cartridge space
     */
    uint8_t read(uint16_t address);
    
    /**
     * @brief Ghi vào cartridge space
     */
    void write(uint16_t address, uint8_t value);
    
    /**
     * @brief Reset cartridge
     */
    void reset();
    
    /**
     * @brief Get nametable mirroring mode
     */
    MirrorMode get_mirroring() const { return mirror_mode_; }

private:
    std::vector<uint8_t> prg_rom_;  // Program ROM
    std::vector<uint8_t> chr_rom_;  // Character ROM
    std::vector<uint8_t> prg_ram_;  // Program RAM (battery-backed)
    
    Mapper* mapper_;
    
    uint8_t mapper_number_;
    bool has_battery_;
    MirrorMode mirror_mode_;  // Nametable mirroring mode
    
    // Helper để tạo mapper phù hợp
    Mapper* create_mapper();
};

} // namespace nes

#endif // NES_CARTRIDGE_H
