#include "ppu/ppu.h"
#include "cartridge/cartridge.h"
#include <cstring>

namespace nes {

// NES Color Palette (NTSC) - 64 màu
// Format: 0xAARRGGBB
const uint32_t PPU::PALETTE_COLORS[64] = {
    0xFF666666, 0xFF002A88, 0xFF1412A7, 0xFF3B00A4, 0xFF5C007E, 0xFF6E0040, 0xFF6C0600, 0xFF561D00,
    0xFF333500, 0xFF0B4800, 0xFF005200, 0xFF004F08, 0xFF00404D, 0xFF000000, 0xFF000000, 0xFF000000,
    0xFFADADAD, 0xFF155FD9, 0xFF4240FF, 0xFF7527FE, 0xFFA01ACC, 0xFFB71E7B, 0xFFB53120, 0xFF994E00,
    0xFF6B6D00, 0xFF388700, 0xFF0C9300, 0xFF008F32, 0xFF007C8D, 0xFF000000, 0xFF000000, 0xFF000000,
    0xFFFFFEFF, 0xFF64B0FF, 0xFF9290FF, 0xFFC676FF, 0xFFF36AFF, 0xFFFE6ECC, 0xFFFE8170, 0xFFEA9E22,
    0xFFBCBE00, 0xFF88D800, 0xFF5CE430, 0xFF45E082, 0xFF48CDDE, 0xFF4F4F4F, 0xFF000000, 0xFF000000,
    0xFFFFFEFF, 0xFFC0DFFF, 0xFFD3D2FF, 0xFFE8C8FF, 0xFFFBC2FF, 0xFFFEC4EA, 0xFFFECCC5, 0xFFF7D8A5,
    0xFFE4E594, 0xFFCFEF96, 0xFFBDF4AB, 0xFFB3F3CC, 0xFFB5EBF2, 0xFFB8B8B8, 0xFF000000, 0xFF000000,
};

PPU::PPU() 
    : scanline_(0), cycle_(0), frame_(0),
      nmi_occurred_(false), cartridge_(nullptr),
      oam_addr_(0), read_buffer_(0), data_bus_(0),
      v_(0), t_(0), x_(0), w_(0),
      sprite_count_(0), sprite_0_rendering_(false),
      odd_frame_(false) {
    
    // Initialize registers
    std::memset(&ctrl_, 0, sizeof(ctrl_));
    std::memset(&mask_, 0, sizeof(mask_));
    std::memset(&status_, 0, sizeof(status_));
    std::memset(&bg_shifters_, 0, sizeof(bg_shifters_));
    
    // Clear memory
    vram_.fill(0);
    oam_.fill(0);
    secondary_oam_.fill(0xFF);
    palette_.fill(0);
    framebuffer_.fill(0);
    sprite_shifters_.fill({0, 0, 0, 0, 0, 0});
}

PPU::~PPU() {
}

void PPU::reset() {
    scanline_ = 0;
    cycle_ = 0;
    frame_ = 0;
    nmi_occurred_ = false;
    
    std::memset(&ctrl_, 0, sizeof(ctrl_));
    std::memset(&mask_, 0, sizeof(mask_));
    std::memset(&status_, 0, sizeof(status_));
    
    oam_addr_ = 0;
    v_ = 0;
    t_ = 0;
    x_ = 0;
    w_ = 0;
    read_buffer_ = 0;
}

void PPU::connect_cartridge(Cartridge* cartridge) {
    cartridge_ = cartridge;
}

bool PPU::step() {
    bool frame_complete = false;
    
    // Visible scanlines (0-239) và pre-render (261)
    if (scanline_ < 240 || scanline_ == 261) {
        // Background rendering cycles
        if (cycle_ >= 1 && cycle_ <= 256) {
            render_pixel();
            
            // Fetch background tile data mỗi 8 cycles
            if (cycle_ % 8 == 0) {
                fetch_background_tile();
            }
        }
        
        // Sprite evaluation (cycle 65-256)
        if (cycle_ == 65) {
            secondary_oam_.fill(0xFF);
            sprite_count_ = 0;
        }
        
        if (cycle_ >= 65 && cycle_ <= 256) {
            if ((cycle_ - 65) % 32 == 0) {
                evaluate_sprites();
            }
        }
        
        // Load sprites for next scanline
        if (cycle_ == 257) {
            load_sprites();
        }
        
        // Increment scrolling
        if (cycle_ == 256) {
            increment_scroll_y();
        }
        if (cycle_ == 257) {
            copy_horizontal_position();
        }
        
        // Pre-render scanline: copy vertical position
        if (scanline_ == 261 && cycle_ >= 280 && cycle_ <= 304) {
            copy_vertical_position();
        }
    }
    
    // VBlank start (scanline 241, cycle 1)
    if (scanline_ == 241 && cycle_ == 1) {
        status_.vblank = 1;
        if (ctrl_.nmi_enable) {
            nmi_occurred_ = true;
        }
        frame_complete = true;
    }
    
    // Clear VBlank (scanline 261, cycle 1)
    if (scanline_ == 261 && cycle_ == 1) {
        status_.vblank = 0;
        status_.sprite_0_hit = 0;
        status_.sprite_overflow = 0;
        nmi_occurred_ = false;
    }
    
    // ==================
    // TIMING QUIRK: Odd Frame Skip
    // ==================
    // On odd frames, when rendering is enabled, skip from cycle 339 to 0
    // This is a real NES hardware quirk that affects timing
    if (scanline_ == 261 && cycle_ == 339 && odd_frame_ && rendering_enabled()) {
        // Skip cycle 340 on odd frames
        cycle_ = 0;
        scanline_ = 0;
        frame_++;
        odd_frame_ = !odd_frame_;  // Toggle odd/even frame
        return frame_complete;
    }
    
    // Advance cycle normally
    cycle_++;
    if (cycle_ > 340) {
        cycle_ = 0;
        scanline_++;
        
        if (scanline_ > 261) {
            scanline_ = 0;
            frame_++;
            odd_frame_ = !odd_frame_;  // Toggle odd/even frame
        }
    }
    
    return frame_complete;
}

uint8_t PPU::read_register(uint16_t address) {
    uint8_t value = data_bus_;  // Default: return last bus value (open bus)
    
    switch (address & 0x0007) {
        case 0: // $2000 PPUCTRL - Write only
            break;
            
        case 1: // $2001 PPUMASK - Write only
            break;
            
        case 2: // $2002 PPUSTATUS
            // Status bits (7-5) + open bus (4-0)
            value = (status_.vblank << 7) | 
                    (status_.sprite_0_hit << 6) |
                    (status_.sprite_overflow << 5) |
                    (data_bus_ & 0x1F);  // Lower 5 bits from bus
            status_.vblank = 0;  // Reading clears VBlank
            w_ = 0;              // Reset write toggle
            data_bus_ = value;   // Update bus
            break;
            
        case 3: // $2003 OAMADDR - Write only
            break;
            
        case 4: // $2004 OAMDATA
            value = oam_[oam_addr_];
            data_bus_ = value;  // Update bus
            break;
            
        case 5: // $2005 PPUSCROLL - Write only
            break;
            
        case 6: // $2006 PPUADDR - Write only
            break;
            
        case 7: // $2007 PPUDATA
            value = read_buffer_;
            read_buffer_ = ppu_read(v_);
            
            // Palette reads are not buffered
            if (v_ >= 0x3F00) {
                value = read_buffer_;
                // Fill buffer with mirrored nametable data
                read_buffer_ = ppu_read(v_ & 0x2FFF);
            }
            
            data_bus_ = value;  // Update bus
            
            // Increment address
            v_ += ctrl_.vram_increment ? 32 : 1;
            v_ &= 0x3FFF;
            break;
    }
    
    return value;
}

void PPU::write_register(uint16_t address, uint8_t value) {
    data_bus_ = value;  // All writes update the data bus
    
    switch (address & 0x0007) {
        case 0: // $2000 PPUCTRL
            *reinterpret_cast<uint8_t*>(&ctrl_) = value;
            t_ = (t_ & 0xF3FF) | ((value & 0x03) << 10);
            break;
            
        case 1: // $2001 PPUMASK
            *reinterpret_cast<uint8_t*>(&mask_) = value;
            break;
            
        case 2: // $2002 PPUSTATUS - Read only
            break;
            
        case 3: // $2003 OAMADDR
            oam_addr_ = value;
            break;
            
        case 4: // $2004 OAMDATA
            oam_[oam_addr_++] = value;
            break;
            
        case 5: // $2005 PPUSCROLL
            if (w_ == 0) {
                // First write: X scroll
                t_ = (t_ & 0xFFE0) | (value >> 3);
                x_ = value & 0x07;
                w_ = 1;
            } else {
                // Second write: Y scroll
                t_ = (t_ & 0x8FFF) | ((value & 0x07) << 12);
                t_ = (t_ & 0xFC1F) | ((value & 0xF8) << 2);
                w_ = 0;
            }
            break;
            
        case 6: // $2006 PPUADDR
            if (w_ == 0) {
                // First write: High byte
                t_ = (t_ & 0x00FF) | ((value & 0x3F) << 8);
                w_ = 1;
            } else {
                // Second write: Low byte
                t_ = (t_ & 0xFF00) | value;
                v_ = t_;
                w_ = 0;
            }
            break;
            
        case 7: // $2007 PPUDATA
            ppu_write(v_, value);
            v_ += ctrl_.vram_increment ? 32 : 1;
            v_ &= 0x3FFF;
            break;
    }
}

void PPU::write_oam_dma(uint8_t index, uint8_t value) {
    oam_[index] = value;
}

const uint8_t* PPU::get_framebuffer() const {
    return framebuffer_.data();
}

// ==================
// PPU Memory Access
// ==================

uint8_t PPU::ppu_read(uint16_t address) {
    address &= 0x3FFF;  // Mirror down to 14 bits
    
    if (address < 0x2000) {
        // Pattern tables (CHR ROM/RAM)
        if (cartridge_) {
            return cartridge_->read(address);
        }
    }
    else if (address < 0x3F00) {
        // Nametables with proper mirroring
        address &= 0x0FFF;
        
        if (!cartridge_) {
            // No cartridge: default horizontal mirroring
            if (address >= 0x0800) address -= 0x0800;
            return vram_[address & 0x07FF];
        }
        
        // Get mirroring mode from cartridge
        MirrorMode mirror = cartridge_->get_mirroring();
        
        switch (mirror) {
            case MirrorMode::HORIZONTAL:
                // Nametables 0,1 → same; 2,3 → same
                // $2000-$23FF and $2400-$27FF mirror each other
                // $2800-$2BFF and $2C00-$2FFF mirror each other
                if (address >= 0x0800) {
                    address -= 0x0800;
                }
                return vram_[address & 0x07FF];
                
            case MirrorMode::VERTICAL:
                // Nametables 0,2 → same; 1,3 → same
                // $2000-$23FF and $2800-$2BFF mirror each other
                // $2400-$27FF and $2C00-$2FFF mirror each other
                if (address >= 0x0800) {
                    // Map $2800-$2FFF to $2000-$27FF
                    address -= 0x0800;
                }
                // Map $2400-$27FF to $2000-$23FF + 0x0400
                if (address >= 0x0400 && address < 0x0800) {
                    address = (address - 0x0400) | 0x0400;
                }
                return vram_[address & 0x07FF];
                
            case MirrorMode::FOUR_SCREEN:
                // All four nametables are separate (needs 4KB VRAM)
                // For now, treat as horizontal (we only have 2KB)
                if (address >= 0x0800) {
                    address -= 0x0800;
                }
                return vram_[address & 0x07FF];
                
            case MirrorMode::SINGLE_SCREEN:
                // All nametables map to same memory
                return vram_[address & 0x03FF];
                
            default:
                return vram_[address & 0x07FF];
        }
    }
    else if (address < 0x4000) {
        // Palette RAM
        address &= 0x001F;
        
        // Mirror $3F10, $3F14, $3F18, $3F1C to $3F00, $3F04, $3F08, $3F0C
        if (address >= 16 && (address & 0x03) == 0) {
            address -= 16;
        }
        
        return palette_[address];
    }
    
    return 0;
}

void PPU::ppu_write(uint16_t address, uint8_t value) {
    address &= 0x3FFF;
    
    if (address < 0x2000) {
        // Pattern tables (CHR ROM read-only, or CHR RAM)
        if (cartridge_) {
            cartridge_->write(address, value);
        }
    }
    else if (address < 0x3F00) {
        // Nametables with proper mirroring
        address &= 0x0FFF;
        
        if (!cartridge_) {
            // No cartridge: default horizontal mirroring
            if (address >= 0x0800) address -= 0x0800;
            vram_[address & 0x07FF] = value;
            return;
        }
        
        // Get mirroring mode from cartridge
        MirrorMode mirror = cartridge_->get_mirroring();
        
        switch (mirror) {
            case MirrorMode::HORIZONTAL:
                if (address >= 0x0800) {
                    address -= 0x0800;
                }
                vram_[address & 0x07FF] = value;
                break;
                
            case MirrorMode::VERTICAL:
                if (address >= 0x0800) {
                    address -= 0x0800;
                }
                if (address >= 0x0400 && address < 0x0800) {
                    address = (address - 0x0400) | 0x0400;
                }
                vram_[address & 0x07FF] = value;
                break;
                
            case MirrorMode::FOUR_SCREEN:
                if (address >= 0x0800) {
                    address -= 0x0800;
                }
                vram_[address & 0x07FF] = value;
                break;
                
            case MirrorMode::SINGLE_SCREEN:
                vram_[address & 0x03FF] = value;
                break;
                
            default:
                vram_[address & 0x07FF] = value;
                break;
        }
    }
    else if (address < 0x4000) {
        // Palette RAM
        address &= 0x001F;
        
        if (address >= 16 && (address & 0x03) == 0) {
            address -= 16;
        }
        
        palette_[address] = value;
    }
}

// ==================
// Rendering
// ==================

void PPU::render_pixel() {
    if (scanline_ >= 240 || cycle_ < 1 || cycle_ > 256) {
        return;
    }
    
    uint8_t bg_pixel = 0;
    uint8_t bg_palette = 0;
    
    // Background rendering
    if (mask_.show_bg) {
        // Get pixel from shift registers
        uint16_t bit_mux = 0x8000 >> x_;
        
        uint8_t p0_pixel = (bg_shifters_.pattern_lo & bit_mux) > 0;
        uint8_t p1_pixel = (bg_shifters_.pattern_hi & bit_mux) > 0;
        bg_pixel = (p1_pixel << 1) | p0_pixel;
        
        uint8_t p0_palette = (bg_shifters_.attribute_lo & bit_mux) > 0;
        uint8_t p1_palette = (bg_shifters_.attribute_hi & bit_mux) > 0;
        bg_palette = (p1_palette << 1) | p0_palette;
    }
    
    // Sprite rendering
    uint8_t sprite_pixel = 0;
    uint8_t sprite_palette = 0;
    bool sprite_priority = false;
    
    if (mask_.show_sprites) {
    for (int i = 0; i < sprite_count_ && i < 8; i++) {
        int sprite_x_pos = cycle_ - 1 - sprite_shifters_[i].x;
        
        if (sprite_x_pos >= 0 && sprite_x_pos < 8) {
            uint8_t bit_mask = 0x80 >> sprite_x_pos;
            
            uint8_t p0 = (sprite_shifters_[i].pattern_lo & bit_mask) ? 1 : 0;
            uint8_t p1 = (sprite_shifters_[i].pattern_hi & bit_mask) ? 1 : 0;
            uint8_t pixel = (p1 << 1) | p0;
            
            if (pixel != 0) {
                sprite_palette = sprite_shifters_[i].attributes & 0x03;
                sprite_pixel = pixel;
                sprite_priority = (sprite_shifters_[i].attributes & 0x20) != 0;
                break;
            }
        }
    }
}
    
    // Priority and final pixel
    uint8_t final_pixel = 0;
    uint8_t final_palette = 0;
    
    if (bg_pixel == 0 && sprite_pixel == 0) {
        // Universal background color
        final_pixel = 0;
        final_palette = 0;
    }
    else if (bg_pixel == 0 && sprite_pixel > 0) {
        final_pixel = sprite_pixel;
        final_palette = sprite_palette + 4;
    }
    else if (bg_pixel > 0 && sprite_pixel == 0) {
        final_pixel = bg_pixel;
        final_palette = bg_palette;
    }
    else {
        // Both visible: check priority
        if (sprite_priority) {
            final_pixel = sprite_pixel;
            final_palette = sprite_palette + 4;
        } else {
            final_pixel = bg_pixel;
            final_palette = bg_palette;
        }
        
        // Sprite 0 hit detection
        if (sprite_0_rendering_ && mask_.show_bg && mask_.show_sprites) {
            if (cycle_ < 255) {  // Not rightmost column
                status_.sprite_0_hit = 1;
            }
        }
    }
    
    // Get color and write to framebuffer
    uint32_t color = get_color_from_palette(final_palette, final_pixel);
    
    int x = cycle_ - 1;
    int y = scanline_;
    int index = (y * 256 + x) * 4;
    
    framebuffer_[index + 0] = (color >> 16) & 0xFF;  // R
    framebuffer_[index + 1] = (color >> 8) & 0xFF;   // G
    framebuffer_[index + 2] = color & 0xFF;          // B
    framebuffer_[index + 3] = 0xFF;                  // A
    
    // Shift background registers
    bg_shifters_.pattern_lo <<= 1;
    bg_shifters_.pattern_hi <<= 1;
    bg_shifters_.attribute_lo <<= 1;
    bg_shifters_.attribute_hi <<= 1;
}

void PPU::fetch_background_tile() {
    if (!mask_.show_bg && !mask_.show_sprites) {
        return;
    }
    
    if (scanline_ >= 240 && scanline_ != 261) {
        return;
    }
    
    uint16_t nametable_addr = 0x2000 | (v_ & 0x0FFF);
    uint8_t nametable_byte = ppu_read(nametable_addr);
    
    uint16_t attribute_addr = 0x23C0 | (v_ & 0x0C00) |
                              ((v_ >> 4) & 0x38) | ((v_ >> 2) & 0x07);
    uint8_t attribute_byte = ppu_read(attribute_addr);
    
    uint8_t shift = ((v_ & 0x02) | ((v_ & 0x40) >> 4));
    uint8_t palette_bits = (attribute_byte >> shift) & 0x03;
    
    uint16_t pattern_addr = (ctrl_.bg_pattern ? 0x1000 : 0x0000);
    pattern_addr += (nametable_byte * 16) + ((v_ >> 12) & 0x07);
    
    uint8_t pattern_lo = ppu_read(pattern_addr);
    uint8_t pattern_hi = ppu_read(pattern_addr + 8);
    
    bg_shifters_.pattern_lo = (bg_shifters_.pattern_lo & 0xFF00) | pattern_lo;
    bg_shifters_.pattern_hi = (bg_shifters_.pattern_hi & 0xFF00) | pattern_hi;
    
    bg_shifters_.attribute_lo = (bg_shifters_.attribute_lo & 0xFF00) | 
                                 ((palette_bits & 0x01) ? 0xFF : 0x00);
    bg_shifters_.attribute_hi = (bg_shifters_.attribute_hi & 0xFF00) | 
                                 ((palette_bits & 0x02) ? 0xFF : 0x00);
    
    if (cycle_ >= 1 && cycle_ <= 256) {
        increment_scroll_x();
    }
}

void PPU::evaluate_sprites() {
    if (scanline_ >= 240) {
        return;
    }
    
    sprite_count_ = 0;
    sprite_0_rendering_ = false;
    
    for (int i = 0; i < 64; i++) {
        uint8_t sprite_y = oam_[i * 4 + 0];
        uint8_t tile = oam_[i * 4 + 1];
        uint8_t attr = oam_[i * 4 + 2];
        uint8_t sprite_x = oam_[i * 4 + 3];
        
        int sprite_height = ctrl_.sprite_size ? 16 : 8;
        int diff = scanline_ - sprite_y;
        
        if (diff >= 0 && diff < sprite_height) {
            if (sprite_count_ < 8) {
                secondary_oam_[sprite_count_ * 4 + 0] = sprite_y;
                secondary_oam_[sprite_count_ * 4 + 1] = tile;
                secondary_oam_[sprite_count_ * 4 + 2] = attr;
                secondary_oam_[sprite_count_ * 4 + 3] = sprite_x;
                
                if (i == 0) {
                    sprite_0_rendering_ = true;
                }
                
                sprite_count_++;
            } else {
                status_.sprite_overflow = 1;
                break;
            }
        }
    }
}

void PPU::load_sprites() {
    for (int i = 0; i < sprite_count_ && i < 8; i++) {
        uint8_t sprite_y = secondary_oam_[i * 4 + 0];
        uint8_t tile_index = secondary_oam_[i * 4 + 1];
        uint8_t attributes = secondary_oam_[i * 4 + 2];
        uint8_t sprite_x = secondary_oam_[i * 4 + 3];
        
        bool flip_horizontal = attributes & 0x40;
        bool flip_vertical = attributes & 0x80;
        
        int sprite_row = scanline_ - sprite_y;
        
        if (flip_vertical) {
            int sprite_height = ctrl_.sprite_size ? 16 : 8;
            sprite_row = sprite_height - 1 - sprite_row;
        }
        
        uint16_t pattern_addr;
        
        if (ctrl_.sprite_size == 0) {
            pattern_addr = (ctrl_.sprite_pattern ?  0x1000 : 0x0000);
            pattern_addr += tile_index * 16 + sprite_row;
        } else {
            pattern_addr = (tile_index & 0x01) ? 0x1000 : 0x0000;
            tile_index &= 0xFE;
            
            if (sprite_row < 8) {
                pattern_addr += tile_index * 16 + sprite_row;
            } else {
                pattern_addr += (tile_index + 1) * 16 + (sprite_row - 8);
            }
        }
        
        uint8_t pattern_lo = ppu_read(pattern_addr);
        uint8_t pattern_hi = ppu_read(pattern_addr + 8);
        
        if (flip_horizontal) {
            pattern_lo = ((pattern_lo & 0xF0) >> 4) | ((pattern_lo & 0x0F) << 4);
            pattern_lo = ((pattern_lo & 0xCC) >> 2) | ((pattern_lo & 0x33) << 2);
            pattern_lo = ((pattern_lo & 0xAA) >> 1) | ((pattern_lo & 0x55) << 1);
            
            pattern_hi = ((pattern_hi & 0xF0) >> 4) | ((pattern_hi & 0x0F) << 4);
            pattern_hi = ((pattern_hi & 0xCC) >> 2) | ((pattern_hi & 0x33) << 2);
            pattern_hi = ((pattern_hi & 0xAA) >> 1) | ((pattern_hi & 0x55) << 1);
        }
        
        sprite_shifters_[i].y = sprite_y;
        sprite_shifters_[i].tile_index = tile_index;
        sprite_shifters_[i].attributes = attributes;
        sprite_shifters_[i].x = sprite_x;
        sprite_shifters_[i].pattern_lo = pattern_lo;
        sprite_shifters_[i].pattern_hi = pattern_hi;
    }
}

// ==================
// Scrolling
// ==================

void PPU::increment_scroll_x() {
    if ((v_ & 0x001F) == 31) {
        v_ &= ~0x001F;
        v_ ^= 0x0400;  // Switch horizontal nametable
    } else {
        v_++;
    }
}

void PPU::increment_scroll_y() {
    if ((v_ & 0x7000) != 0x7000) {
        v_ += 0x1000;
    } else {
        v_ &= ~0x7000;
        int y = (v_ & 0x03E0) >> 5;
        if (y == 29) {
            y = 0;
            v_ ^= 0x0800;  // Switch vertical nametable
        } else if (y == 31) {
            y = 0;
        } else {
            y++;
        }
        v_ = (v_ & ~0x03E0) | (y << 5);
    }
}

void PPU::copy_horizontal_position() {
    v_ = (v_ & 0xFBE0) | (t_ & 0x041F);
}

void PPU::copy_vertical_position() {
    v_ = (v_ & 0x841F) | (t_ & 0x7BE0);
}

// ==================
// Palette
// ==================

uint32_t PPU::get_color_from_palette(uint8_t palette_index, uint8_t pixel) {
    uint8_t color_index = ppu_read(0x3F00 + (palette_index * 4) + pixel) & 0x3F;
    return PALETTE_COLORS[color_index];
}

} // namespace nes
