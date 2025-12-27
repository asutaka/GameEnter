// Add this to the end of ppu.cpp before the closing brace

void PPU::dump_nametable_debug(const char* filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        return;
    }
    
    file << "=== PPU Nametable & Attribute Debug Dump ===" << std::endl;
    file << "Frame: " << frame_ << std::endl;
    file << "Scanline: " << scanline_ << ", Cycle: " << cycle_ << std::endl;
    file << "Scroll: v_=0x" << std::hex << v_ << ", t_=0x" << t_ << ", x_=" << std::dec << (int)x_ << std::endl;
    file << std::endl;
    
    // Dump all 4 nametables
    for (int nt = 0; nt < 4; nt++) {
        file << "=== Nametable " << nt << " ($" << std::hex << (0x2000 + nt * 0x400) << ") ===" << std::dec << std::endl;
        
        uint16_t nt_base = 0x2000 + (nt * 0x400);
        
        // Dump nametable tiles (30 rows x 32 cols)
        file << "Tiles:" << std::endl;
        for (int row = 0; row < 30; row++) {
            for (int col = 0; col < 32; col++) {
                uint16_t addr = nt_base + (row * 32) + col;
                uint8_t tile = ppu_read(addr);
                file << std::hex << std::setw(2) << std::setfill('0') << (int)tile << " ";
            }
            file << std::endl;
        }
        file << std::endl;
        
        // Dump attribute table (8 rows x 8 cols)
        file << "Attribute Table:" << std::endl;
        uint16_t attr_base = nt_base + 0x3C0;
        for (int row = 0; row < 8; row++) {
            for (int col = 0; col < 8; col++) {
                uint16_t addr = attr_base + (row * 8) + col;
                uint8_t attr = ppu_read(addr);
                
                // Decode attribute byte into 4 quadrants
                uint8_t tl = (attr >> 0) & 0x03;  // Top-left
                uint8_t tr = (attr >> 2) & 0x03;  // Top-right
                uint8_t bl = (attr >> 4) & 0x03;  // Bottom-left
                uint8_t br = (attr >> 6) & 0x03;  // Bottom-right
                
                file << "[" << (int)tl << (int)tr << (int)bl << (int)br << "] ";
            }
            file << std::endl;
        }
        file << std::endl;
    }
    
    // Dump palette RAM
    file << "=== Palette RAM ===" << std::endl;
    file << "Background Palettes:" << std::endl;
    for (int i = 0; i < 4; i++) {
        file << "Palette " << i << ": ";
        for (int j = 0; j < 4; j++) {
            uint8_t color = ppu_read(0x3F00 + i * 4 + j);
            file << std::hex << std::setw(2) << std::setfill('0') << (int)color << " ";
        }
        file << std::endl;
    }
    file << "Sprite Palettes:" << std::endl;
    for (int i = 0; i < 4; i++) {
        file << "Palette " << i << ": ";
        for (int j = 0; j < 4; j++) {
            uint8_t color = ppu_read(0x3F10 + i * 4 + j);
            file << std::hex << std::setw(2) << std::setfill('0') << (int)color << " ";
        }
        file << std::endl;
    }
    
    file << std::dec << std::endl;
    file << "=== End of dump ===" << std::endl;
    file.close();
}
