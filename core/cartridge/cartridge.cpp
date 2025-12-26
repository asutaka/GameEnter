#include "cartridge/cartridge.h"
#include "mappers/mapper.h"
#include <fstream>
#include <iostream>

// Include mapper implementations
#include "mappers/mapper0.cpp"

namespace nes {

Cartridge::Cartridge() 
    : mapper_(nullptr), mapper_number_(0), has_battery_(false), 
      mirror_mode_(MirrorMode::HORIZONTAL) {
}

Cartridge::~Cartridge() {
    delete mapper_;
}

bool Cartridge::load_from_file(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Không thể mở ROM file: " << filename << std::endl;
        return false;
    }
    
    // Đọc iNES header (16 bytes)
    uint8_t header[16];
    file.read(reinterpret_cast<char*>(header), 16);
    
    // Kiểm tra magic number "NES\x1A"
    if (header[0] != 'N' || header[1] != 'E' || 
        header[2] != 'S' || header[3] != 0x1A) {
        std::cerr << "File không phải iNES format!" << std::endl;
        return false;
    }
    
    // Parse header
    uint8_t prg_rom_size = header[4];  // Số blocks 16KB
    uint8_t chr_rom_size = header[5];  // Số blocks 8KB
    uint8_t flags6 = header[6];
    uint8_t flags7 = header[7];
    uint8_t prg_ram_size = header[8];  // Số blocks 8KB
    
    // Mapper number
    mapper_number_ = (flags7 & 0xF0) | (flags6 >> 4);
    
    // Flags
    bool has_trainer = (flags6 & 0x04) != 0;
    has_battery_ = (flags6 & 0x02) != 0;
    bool four_screen = (flags6 & 0x08) != 0;
    bool vertical_mirror = (flags6 & 0x01) != 0;
    
    // Parse mirroring mode
    if (four_screen) {
        mirror_mode_ = MirrorMode::FOUR_SCREEN;
    } else if (vertical_mirror) {
        mirror_mode_ = MirrorMode::VERTICAL;
    } else {
        mirror_mode_ = MirrorMode::HORIZONTAL;
    }
    
    std::cout << "=== iNES ROM Info ===" << std::endl;
    std::cout << "PRG ROM: " << (int)prg_rom_size << " x 16KB" << std::endl;
    std::cout << "CHR ROM: " << (int)chr_rom_size << " x 8KB" << std::endl;
    std::cout << "Mapper: " << (int)mapper_number_ << std::endl;
    std::cout << "Battery: " << (has_battery_ ? "Yes" : "No") << std::endl;
    std::cout << "Trainer: " << (has_trainer ? "Yes" : "No") << std::endl;
    
    // Print mirroring mode
    std::cout << "Mirroring: ";
    switch (mirror_mode_) {
        case MirrorMode::HORIZONTAL: std::cout << "Horizontal"; break;
        case MirrorMode::VERTICAL: std::cout << "Vertical"; break;
        case MirrorMode::FOUR_SCREEN: std::cout << "Four-Screen"; break;
        case MirrorMode::SINGLE_SCREEN: std::cout << "Single-Screen"; break;
    }
    std::cout << std::endl;
    
    // Skip trainer nếu có (512 bytes)
    if (has_trainer) {
        file.seekg(512, std::ios::cur);
    }
    
    // Đọc PRG ROM
    size_t prg_size = prg_rom_size * 16384;  // 16KB per block
    prg_rom_.resize(prg_size);
    file.read(reinterpret_cast<char*>(prg_rom_.data()), prg_size);
    
    // Đọc CHR ROM
    size_t chr_size = chr_rom_size * 8192;  // 8KB per block
    if (chr_size > 0) {
        chr_rom_.resize(chr_size);
        file.read(reinterpret_cast<char*>(chr_rom_.data()), chr_size);
    } else {
        // CHR RAM (8KB)
        chr_rom_.resize(8192);
        std::fill(chr_rom_.begin(), chr_rom_.end(), 0);
    }
    
    // PRG RAM
    if (prg_ram_size == 0) prg_ram_size = 1;  // Default 8KB
    prg_ram_.resize(prg_ram_size * 8192);
    std::fill(prg_ram_.begin(), prg_ram_.end(), 0);
    
    file.close();
    
    // Tạo mapper
    delete mapper_;
    mapper_ = create_mapper();
    
    if (!mapper_) {
        std::cerr << "Mapper " << (int)mapper_number_ 
                  << " chưa được implement!" << std::endl;
        return false;
    }
    
    std::cout << "ROM loaded successfully!" << std::endl;
    return true;
}

Mapper* Cartridge::create_mapper() {
    switch (mapper_number_) {
        case 0:
            // Mapper 0 (NROM)
            return new Mapper0(prg_rom_.data(), prg_rom_.size(),
                              chr_rom_.data(), chr_rom_.size());
        
        case 1:
            // TODO: Mapper 1 (MMC1)
            return nullptr;
        
        case 4:
            // TODO: Mapper 4 (MMC3) - Cho Contra
            return nullptr;
        
        default:
            return nullptr;
    }
}

uint8_t Cartridge::read(uint16_t address) {
    if (mapper_) {
        return mapper_->read(address);
    }
    
    // Fallback: Direct mapping cho Mapper 0
    if (address >= 0x8000) {
        // PRG ROM
        uint16_t index = address - 0x8000;
        if (prg_rom_.size() == 16384) {
            // 16KB: Mirror
            index %= 16384;
        }
        return prg_rom_[index];
    }
    
    return 0;
}

void Cartridge::write(uint16_t address, uint8_t value) {
    if (mapper_) {
        mapper_->write(address, value);
    }
    
    // PRG RAM ($6000-$7FFF)
    if (address >= 0x6000 && address < 0x8000) {
        prg_ram_[address - 0x6000] = value;
    }
}

void Cartridge::reset() {
    if (mapper_) {
        mapper_->reset();
    }
}

} // namespace nes
