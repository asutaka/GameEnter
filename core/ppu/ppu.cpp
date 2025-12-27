#include "ppu/ppu.h"
#include "cartridge/cartridge.h"
#include <cstring>
#include <iostream>

namespace nes {

// NES Color Palette (NTSC) - 64 colors
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
    bool rendering = rendering_enabled();
    
    // Visible scanlines (0-239) and pre-render (261)
    if (scanline_ < 240 || scanline_ == 261) {
        // Render pixel during visible cycles (MUST be before shifting)
        if (cycle_ >= 1 && cycle_ <= 256) {
            render_pixel();
        }

        if (rendering) {
            // Background processing: Visible cycles and Pre-fetch (321-336)
            if ((cycle_ >= 1 && cycle_ <= 256) || (cycle_ >= 321 && cycle_ <= 336)) {
                // Shift registers every cycle
                update_shifters();
                
                // Fetch tile every 8 cycles (at cycles 8, 16, 24, ...)
                // This allows the shifters to fully shift the previous tile into the high byte
                // before loading the new tile into the low byte.
                if (cycle_ % 8 == 0) {
                    fetch_background_tile();
                }
            }
        }
        
        if (rendering) {
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
    
    // TIMING QUIRK: Odd Frame Skip
    if (scanline_ == 261 && cycle_ == 339 && odd_frame_ && rendering_enabled()) {
        cycle_ = 0;
        scanline_ = 0;
        frame_++;
        odd_frame_ = !odd_frame_;
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
            odd_frame_ = !odd_frame_;
        }
    }
    
    return frame_complete;
}

uint8_t PPU::read_register(uint16_t address) {
    uint8_t value = data_bus_;
    
    switch (address & 0x0007) {
        case 2: // $2002 PPUSTATUS
            value = (status_.vblank << 7) | 
                    (status_.sprite_0_hit << 6) |
                    (status_.sprite_overflow << 5) |
                    (data_bus_ & 0x1F);
            status_.vblank = 0;
            w_ = 0;
            data_bus_ = value;
            break;
            
        case 4: // $2004 OAMDATA
            value = oam_[oam_addr_];
            data_bus_ = value;
            break;
            
        case 7: // $2007 PPUDATA
            value = read_buffer_;
            read_buffer_ = ppu_read(v_);
            if (v_ >= 0x3F00) {
                value = read_buffer_;
                read_buffer_ = ppu_read(v_ & 0x2FFF);
            }
            data_bus_ = value;
            v_ += ctrl_.vram_increment ? 32 : 1;
            v_ &= 0x3FFF;
            break;
    }
    
    return value;
}

void PPU::write_register(uint16_t address, uint8_t value) {
    data_bus_ = value;
    
    switch (address & 0x0007) {
        case 0: // $2000 PPUCTRL
            *reinterpret_cast<uint8_t*>(&ctrl_) = value;
            t_ = (t_ & 0xF3FF) | ((value & 0x03) << 10);
            break;
            
        case 1: // $2001 PPUMASK
            *reinterpret_cast<uint8_t*>(&mask_) = value;
            break;
            
        case 3: // $2003 OAMADDR
            oam_addr_ = value;
            break;
            
        case 4: // $2004 OAMDATA
            oam_[oam_addr_++] = value;
            break;
            
        case 5: // $2005 PPUSCROLL
            if (w_ == 0) {
                t_ = (t_ & 0xFFE0) | (value >> 3);
                x_ = value & 0x07;
                w_ = 1;
            } else {
                t_ = (t_ & 0x8FFF) | ((value & 0x07) << 12);
                t_ = (t_ & 0xFC1F) | ((value & 0xF8) << 2);
                w_ = 0;
            }
            break;
            
        case 6: // $2006 PPUADDR
            if (w_ == 0) {
                t_ = (t_ & 0x00FF) | ((value & 0x3F) << 8);
                w_ = 1;
            } else {
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
    return reinterpret_cast<const uint8_t*>(framebuffer_.data());
}

uint8_t PPU::ppu_read(uint16_t address) {
    address &= 0x3FFF;
    
    if (address < 0x2000) {
        if (cartridge_) return cartridge_->read(address);
    }
    else if (address < 0x3F00) {
        address &= 0x0FFF;
        if (!cartridge_) {
            if (address >= 0x0800) address -= 0x0800;
            return vram_[address & 0x07FF];
        }
        MirrorMode mirror = cartridge_->get_mirroring();
        switch (mirror) {
            case MirrorMode::HORIZONTAL:
                {
                    uint16_t masked = address & 0x03FF;
                    if (address & 0x0800) masked |= 0x0400;
                    return vram_[masked];
                }
            case MirrorMode::VERTICAL:
                return vram_[address & 0x07FF];
            case MirrorMode::SINGLE_SCREEN:
                return vram_[address & 0x03FF];
            default:
                return vram_[address & 0x07FF];
        }
    }
    else if (address < 0x4000) {
        address &= 0x001F;
        if (address >= 16 && (address & 0x03) == 0) address -= 16;
        return palette_[address];
    }
    return 0;
}

void PPU::ppu_write(uint16_t address, uint8_t value) {
    address &= 0x3FFF;
    if (address < 0x2000) {
        if (cartridge_) cartridge_->write(address, value);
    }
    else if (address < 0x3F00) {
        address &= 0x0FFF;
        if (!cartridge_) {
            if (address >= 0x0800) address -= 0x0800;
            vram_[address & 0x07FF] = value;
            return;
        }
        MirrorMode mirror = cartridge_->get_mirroring();
        switch (mirror) {
            case MirrorMode::HORIZONTAL:
                {
                    uint16_t masked = address & 0x03FF;
                    if (address & 0x0800) masked |= 0x0400;
                    vram_[masked] = value;
                }
                break;
            case MirrorMode::VERTICAL:
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
        address &= 0x001F;
        if (address >= 16 && (address & 0x03) == 0) address -= 16;
        palette_[address] = value;
    }
}

void PPU::render_pixel() {
    if (scanline_ >= 240 || cycle_ < 1 || cycle_ > 256) return;
    
    uint8_t bg_pixel = 0;
    uint8_t bg_palette = 0;
    if (mask_.show_bg) {
        if (!mask_.show_bg_left && cycle_ <= 8) {
            bg_pixel = 0;
        } else {
            uint16_t bit_mux = 0x8000 >> x_;
            uint8_t p0 = (bg_shifters_.pattern_lo & bit_mux) ? 1 : 0;
            uint8_t p1 = (bg_shifters_.pattern_hi & bit_mux) ? 1 : 0;
            bg_pixel = (p1 << 1) | p0;
            uint8_t a0 = (bg_shifters_.attribute_lo & bit_mux) ? 1 : 0;
            uint8_t a1 = (bg_shifters_.attribute_hi & bit_mux) ? 1 : 0;
            bg_palette = (a1 << 1) | a0;
        }
    }
    
    uint8_t sprite_pixel = 0;
    uint8_t sprite_palette = 0;
    bool sprite_priority = false;
    if (mask_.show_sprites) {
        if (!mask_.show_sprites_left && cycle_ <= 8) {
            sprite_pixel = 0;
        } else {
            for (int i = 0; i < sprite_count_; i++) {
                // Coordinate-based sprite rendering (more stable than shifting)
                int diff = cycle_ - 1 - sprite_shifters_[i].x;
                if (diff >= 0 && diff < 8) {
                    uint8_t bit_mask = 0x80 >> diff;
                    uint8_t p0 = (sprite_shifters_[i].pattern_lo & bit_mask) ? 1 : 0;
                    uint8_t p1 = (sprite_shifters_[i].pattern_hi & bit_mask) ? 1 : 0;
                    uint8_t pixel = (p1 << 1) | p0;
                    
                    if (pixel != 0) {
                        sprite_palette = sprite_shifters_[i].attributes & 0x03;
                        sprite_pixel = pixel;
                        sprite_priority = (sprite_shifters_[i].attributes & 0x20) != 0;
                        
                        if (sprite_shifters_[i].is_sprite_0 && bg_pixel != 0 && cycle_ < 256) {
                            status_.sprite_0_hit = 1;
                        }
                        break;
                    }
                }
            }
        }
    }
    
    uint8_t final_pixel = 0;
    uint8_t final_palette = 0;
    if (bg_pixel == 0 && sprite_pixel == 0) {
        final_pixel = 0; final_palette = 0;
    } else if (bg_pixel == 0 && sprite_pixel > 0) {
        final_pixel = sprite_pixel; final_palette = sprite_palette + 4;
    } else if (bg_pixel > 0 && sprite_pixel == 0) {
        final_pixel = bg_pixel; final_palette = bg_palette;
    } else {
        if (sprite_priority) {
            final_pixel = bg_pixel; final_palette = bg_palette;
        } else {
            final_pixel = sprite_pixel; final_palette = sprite_palette + 4;
        }
    }
    
    uint32_t color = get_color_from_palette(final_palette, final_pixel);
    int index = (scanline_ * 256 + (cycle_ - 1)) * 4;
    
    // Store as RGBA (SDL_PIXELFORMAT_RGBA32)
    // Palette is 0xAARRGGBB
    framebuffer_[index + 0] = (color >> 16) & 0xFF; // R
    framebuffer_[index + 1] = (color >> 8) & 0xFF;  // G
    framebuffer_[index + 2] = color & 0xFF;         // B
    framebuffer_[index + 3] = (color >> 24) & 0xFF; // A
}

void PPU::fetch_background_tile() {
    if (!rendering_enabled()) return;
    uint16_t nt_addr = 0x2000 | (v_ & 0x0FFF);
    uint8_t nt_byte = ppu_read(nt_addr);
    uint16_t attr_addr = 0x23C0 | (v_ & 0x0C00) | ((v_ >> 4) & 0x38) | ((v_ >> 2) & 0x07);
    uint8_t attr_byte = ppu_read(attr_addr);
    uint8_t shift = ((v_ & 0x02) | ((v_ & 0x40) >> 4));
    uint8_t pal = (attr_byte >> shift) & 0x03;
    uint16_t pat_addr = (ctrl_.bg_pattern ? 0x1000 : 0x0000) + (nt_byte * 16) + ((v_ >> 12) & 0x07);
    uint8_t pat_lo = ppu_read(pat_addr);
    uint8_t pat_hi = ppu_read(pat_addr + 8);
    
    bg_shifters_.pattern_lo = (bg_shifters_.pattern_lo & 0xFF00) | pat_lo;
    bg_shifters_.pattern_hi = (bg_shifters_.pattern_hi & 0xFF00) | pat_hi;
    bg_shifters_.attribute_lo = (bg_shifters_.attribute_lo & 0xFF00) | ((pal & 0x01) ? 0xFF : 0x00);
    bg_shifters_.attribute_hi = (bg_shifters_.attribute_hi & 0xFF00) | ((pal & 0x02) ? 0xFF : 0x00);
    
    // Increment scroll X during visible lines AND pre-fetch (321-336)
    if ((cycle_ >= 1 && cycle_ <= 256) || (cycle_ >= 321 && cycle_ <= 336)) {
        increment_scroll_x();
    }
}

void PPU::evaluate_sprites() {
    if (scanline_ >= 240) return;
    sprite_count_ = 0;
    sprite_0_rendering_ = false;
    for (int i = 0; i < 64; i++) {
        uint8_t y = oam_[i * 4 + 0];
        int h = ctrl_.sprite_size ? 16 : 8;
        int diff = scanline_ - y;
        if (diff >= 0 && diff < h) {
            if (sprite_count_ < 8) {
                secondary_oam_[sprite_count_ * 4 + 0] = y;
                secondary_oam_[sprite_count_ * 4 + 1] = oam_[i * 4 + 1];
                secondary_oam_[sprite_count_ * 4 + 2] = oam_[i * 4 + 2];
                secondary_oam_[sprite_count_ * 4 + 3] = oam_[i * 4 + 3];
                if (i == 0) sprite_0_rendering_ = true;
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
        uint8_t y = secondary_oam_[i * 4 + 0];
        uint8_t tile = secondary_oam_[i * 4 + 1];
        uint8_t attr = secondary_oam_[i * 4 + 2];
        uint8_t x = secondary_oam_[i * 4 + 3];
        bool flip_h = attr & 0x40;
        bool flip_v = attr & 0x80;
        int row = scanline_ - y;
        if (flip_v) row = (ctrl_.sprite_size ? 16 : 8) - 1 - row;
        uint16_t addr;
        if (!ctrl_.sprite_size) {
            addr = (ctrl_.sprite_pattern ? 0x1000 : 0x0000) + tile * 16 + row;
        } else {
            addr = ((tile & 0x01) ? 0x1000 : 0x0000) + (tile & 0xFE) * 16 + (row < 8 ? row : row + 8);
        }
        uint8_t lo = ppu_read(addr);
        uint8_t hi = ppu_read(addr + 8);
        if (flip_h) {
            auto rev = [](uint8_t b) {
                b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
                b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
                b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
                return b;
            };
            lo = rev(lo); hi = rev(hi);
        }
        sprite_shifters_[i] = {y, tile, attr, x, lo, hi, (i == 0 && sprite_0_rendering_)};
    }
}

void PPU::increment_scroll_x() {
    if ((v_ & 0x001F) == 31) { v_ &= ~0x001F; v_ ^= 0x0400; } else v_++;
}

void PPU::increment_scroll_y() {
    if ((v_ & 0x7000) != 0x7000) v_ += 0x1000;
    else {
        v_ &= ~0x7000;
        int y = (v_ & 0x03E0) >> 5;
        if (y == 29) { y = 0; v_ ^= 0x0800; }
        else if (y == 31) y = 0;
        else y++;
        v_ = (v_ & ~0x03E0) | (y << 5);
    }
}

void PPU::copy_horizontal_position() { v_ = (v_ & 0xFBE0) | (t_ & 0x041F); }
void PPU::copy_vertical_position() { v_ = (v_ & 0x841F) | (t_ & 0x7BE0); }

uint32_t PPU::get_color_from_palette(uint8_t palette_index, uint8_t pixel) {
    uint8_t color_index = ppu_read(0x3F00 + (palette_index * 4) + pixel) & 0x3F;
    return PALETTE_COLORS[color_index];
}

void PPU::update_shifters() {
    if (mask_.show_bg) {
        bg_shifters_.pattern_lo <<= 1;
        bg_shifters_.pattern_hi <<= 1;
        bg_shifters_.attribute_lo <<= 1;
        bg_shifters_.attribute_hi <<= 1;
    }
    
    // Sprite shifting removed - using coordinate based rendering
}

} // namespace nes
