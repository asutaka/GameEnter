#ifndef NES_PPU_H
#define NES_PPU_H

#include <cstdint>
#include <array>

namespace nes {

// Forward declaration
class Cartridge;

/**
 * @brief Picture Processing Unit - Đơn vị xử lý đồ họa NES
 * 
 * Specs:
 * - Resolution: 256x240 pixels
 * - Refresh rate: 60 Hz (NTSC)
 * - Colors: 64 colors total, 25 on screen simultaneously
 * - Sprites: 64 sprites, 8 per scanline
 * - Background: 32x30 tiles (8x8 pixels each)
 */
class PPU {
public:
    PPU();
    ~PPU();
    
    void reset();
    void connect_cartridge(Cartridge* cartridge);
    
    /**
     * @brief Thực thi 1 PPU cycle
     * PPU chạy 3x nhanh hơn CPU
     * @return true nếu frame hoàn thành (trigger NMI)
     */
    bool step();
    
    /**
     * @brief Đọc/ghi PPU registers ($2000-$2007)
     */
    uint8_t read_register(uint16_t address);
    void write_register(uint16_t address, uint8_t value);
    
    /**
     * @brief OAM DMA - Copy 256 bytes vào OAM
     */
    void write_oam_dma(uint8_t index, uint8_t value);
    
    /**
     * @brief Lấy framebuffer để render (256x240x4 RGBA)
     */
    const uint8_t* get_framebuffer() const;
    
    /**
     * @brief Check nếu cần trigger NMI
     */
    bool nmi_occurred() const { return nmi_occurred_; }
    void clear_nmi() { nmi_occurred_ = false; }

private:
    // ==================
    // PPU Registers
    // ==================
    
    // $2000 PPUCTRL
    struct {
        uint8_t nametable_addr : 2;      // Base nametable address (0-3)
        uint8_t vram_increment : 1;      // 0=add 1, 1=add 32
        uint8_t sprite_pattern : 1;      // Sprite pattern table (0=$0000, 1=$1000)
        uint8_t bg_pattern : 1;          // Background pattern table
        uint8_t sprite_size : 1;         // 0=8x8, 1=8x16
        uint8_t ppu_master_slave : 1;    // PPU master/slave
        uint8_t nmi_enable : 1;          // Trigger NMI on VBlank
    } ctrl_;
    
    // $2001 PPUMASK
    struct {
        uint8_t grayscale : 1;           // 0=color, 1=grayscale
        uint8_t show_bg_left : 1;        // Show background in leftmost 8 pixels
        uint8_t show_sprites_left : 1;   // Show sprites in leftmost 8 pixels
        uint8_t show_bg : 1;             // Show background
        uint8_t show_sprites : 1;        // Show sprites
        uint8_t emphasize_red : 1;       // Emphasize red
        uint8_t emphasize_green : 1;     // Emphasize green
        uint8_t emphasize_blue : 1;      // Emphasize blue
    } mask_;
    
    // $2002 PPUSTATUS
    struct {
        uint8_t unused : 5;
        uint8_t sprite_overflow : 1;     // Sprite overflow flag
        uint8_t sprite_0_hit : 1;        // Sprite 0 hit flag
        uint8_t vblank : 1;              // VBlank flag
    } status_;
    
    // $2003 OAMADDR
    uint8_t oam_addr_;
    
    // $2004 OAMDATA (read/write)
    // $2005 PPUSCROLL (write 2x)
    // $2006 PPUADDR (write 2x)
    // $2007 PPUDATA (read/write)
    
    // Internal registers
    uint16_t v_;        // Current VRAM address (15 bits)
    uint16_t t_;        // Temporary VRAM address (15 bits)
    uint8_t x_;         // Fine X scroll (3 bits)
    uint8_t w_;         // Write toggle (1 bit)
    
    uint8_t read_buffer_;  // Data read buffer
    
    // ==================
    // Memory
    // ==================
    
    // VRAM (2KB internal, mirrored)
    std::array<uint8_t, 0x0800> vram_;  // $2000-$27FF (nametables)
    
    // OAM (Object Attribute Memory) - 256 bytes
    // 64 sprites × 4 bytes each
    std::array<uint8_t, 256> oam_;
    std::array<uint8_t, 32> secondary_oam_;  // 8 sprites × 4 bytes
    
    // Palette RAM - 32 bytes
    std::array<uint8_t, 32> palette_;
    
    // Connected cartridge (for CHR ROM/RAM)
    Cartridge* cartridge_;
    
    // ==================
    // Rendering State
    // ==================
    
    // Current scanline (0-261)
    // 0-239: Visible scanlines
    // 240: Post-render
    // 241-260: VBlank
    // 261: Pre-render
    int scanline_;
    
    // Current dot/cycle in scanline (0-340)
    int cycle_;
    
    // Frame counter
    uint64_t frame_;
    
    // NMI flag
    bool nmi_occurred_;
    
    // Framebuffer (256×240×4 RGBA)
    std::array<uint8_t, 256 * 240 * 4> framebuffer_;
    
    // ==================
    // Rendering helpers
    // ==================
    
    // Background rendering
    struct BackgroundShiftRegisters {
        uint16_t pattern_lo;   // Pattern table low bits
        uint16_t pattern_hi;   // Pattern table high bits
        uint16_t attribute_lo; // Attribute low bits
        uint16_t attribute_hi; // Attribute high bits
    } bg_shifters_;
    
    // Sprite rendering
    struct Sprite {
        uint8_t y;
        uint8_t tile_index;
        uint8_t attributes;
        uint8_t x;
        
        // Rendering data
        uint8_t pattern_lo;
        uint8_t pattern_hi;
    };
    
    std::array<Sprite, 8> sprite_shifters_;
    int sprite_count_;
    bool sprite_0_rendering_;
    
    // ==================
    // Internal functions
    // ==================
    
    // Memory access
    uint8_t ppu_read(uint16_t address);
    void ppu_write(uint16_t address, uint8_t value);
    
    // Rendering
    void render_pixel();
    void fetch_background_tile();
    void evaluate_sprites();
    void load_sprites();
    
    // Scrolling
    void increment_scroll_x();
    void increment_scroll_y();
    void copy_horizontal_position();
    void copy_vertical_position();
    
    // Palette
    uint32_t get_color_from_palette(uint8_t palette_index, uint8_t pixel);
    
    // NES color palette (NTSC)
    static const uint32_t PALETTE_COLORS[64];
};

} // namespace nes

#endif // NES_PPU_H
