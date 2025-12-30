# 🎨 PPU Implementation Plan

## 📋 Overview

**PPU (Picture Processing Unit)** - The NES graphics chip that renders 256x240 pixel display at 60 FPS.

**Status**: 5% (stub implementation exists)  
**Priority**: HIGH - Next major component  
**Complexity**: 8/10 (timing-critical, complex state machine)

---

## 🎯 Implementation Phases

### Phase 1: PPU Registers (Week 1) ⭐⭐⭐
**Goal**: Handle CPU ↔ PPU communication

**Registers to Implement**:
1. **$2000 - PPUCTRL** (Control Register)
   - Nametable selection
   - VRAM increment
   - Sprite/background pattern table
   - Sprite size
   - NMI enable

2. **$2001 - PPUMASK** (Mask Register)
   - Grayscale
   - Show background left 8 pixels
   - Show sprites left 8 pixels
   - Show background
   - Show sprites
   - Color emphasis

3. **$2002 - PPUSTATUS** (Status Register)
   - VBlank flag (bit 7)
   - Sprite 0 hit (bit 6)
   - Sprite overflow (bit 5)
   - Read clears VBlank

4. **$2003 - OAMADDR** (OAM Address)
5. **$2004 - OAMDATA** (OAM Data)
6. **$2005 - PPUSCROLL** (Scroll Position)
7. **$2006 - PPUADDR** (VRAM Address)
8. **$2007 - PPUDATA** (VRAM Data)

**Deliverable**: PPU responds to CPU reads/writes correctly

---

### Phase 2: Memory System (Week 1) ⭐⭐
**Goal**: PPU internal memory working

**Components**:
1. **VRAM** (2KB internal)
   - Nametables (4x1KB, mirrored)
   - Attribute tables

2. **Palette RAM** (32 bytes)
   - Background palettes (16 bytes)
   - Sprite palettes (16 bytes)

3. **OAM** (256 bytes)
   - 64 sprites × 4 bytes each
   - Y pos, tile, attributes, X pos

4. **CHR ROM/RAM** (from cartridge)
   - Pattern tables (8KB)

**Mirroring Modes**:
- Horizontal
- Vertical
- 4-screen
- Single-screen

**Deliverable**: Memory reads/writes work correctly

---

### Phase 3: Background Rendering (Week 2) ⭐⭐⭐
**Goal**: Display static background

**Steps**:
1. **Nametable Fetching**
   - Read tile indices from nametable
   - Each tile is 8x8 pixels

2. **Pattern Table Fetching**
   - Convert tile index to pattern data
   - 2 bitplanes per tile

3. **Attribute Table**
   - 2x2 tile groups
   - 2-bit palette selection

4. **Palette Lookup**
   - 4 colors per palette
   - NES color palette (64 colors total)

5. **Pixel Output**
   - Combine all data
   - Output to framebuffer (256x240 RGBA)

**Deliverable**: Can see background graphics!

---

### Phase 4: Timing & VBlank (Week 2) ⭐⭐⭐
**Goal**: Synchronize with CPU

**Timing**:
- **262 scanlines** per frame
  - 0-239: Visible scanlines
  - 240: Post-render
  - 241-260: VBlank
  - 261: Pre-render

- **341 PPU cycles** per scanline
- **PPU runs at 3x CPU speed**

**VBlank**:
- Set VBlank flag at scanline 241
- Trigger NMI if enabled
- Game updates during VBlank
- Clear flag when $2002 read

**Deliverable**: Games can sync with PPU

---

### Phase 5: Sprite Rendering (Week 3) ⭐⭐
**Goal**: Display sprites (moving objects)

**OAM (Object Attribute Memory)**:
- 64 sprites max
- 8 sprites per scanline max
- Sprite 0 hit detection

**Sprite Attributes**:
- Y position
- Tile index
- Attributes (palette, flip, priority)
- X position

**Priority**:
- Background vs sprite
- Sprite-sprite (lower index wins)

**Deliverable**: Can see sprites (Mario, enemies, etc.)

---

### Phase 6: Scrolling (Week 3) ⭐⭐
**Goal**: Smooth scrolling

**Scroll Registers**:
- X scroll (horizontal)
- Y scroll (vertical)
- Nametable selection

**Implementation**:
- Fine X scroll (0-7 pixels)
- Coarse X/Y scroll (tiles)
- Mid-frame scroll changes

**Deliverable**: Scrolling games work (Super Mario Bros)

---

### Phase 7: Advanced Features (Week 4) ⭐
**Goal**: Edge cases and optimizations

1. **Sprite 0 Hit**
   - Detect overlap with background
   - Used for split-screen effects

2. **OAM DMA** ($4014)
   - Fast sprite data transfer
   - 256 bytes in ~512 CPU cycles

3. **Fine Scrolling**
   - Mid-scanline changes
   - Status bar tricks

4. **Palette Updates**
   - Mid-frame palette changes

**Deliverable**: Advanced games work correctly

---

## 📊 Estimated Effort

| Phase | Days | Complexity | Priority |
|-------|------|------------|----------|
| Phase 1: Registers | 2-3 | Medium | Critical |
| Phase 2: Memory | 1-2 | Low | Critical |
| Phase 3: Background | 3-5 | High | Critical |
| Phase 4: Timing | 2-3 | High | Critical |
| Phase 5: Sprites | 3-4 | Medium | High |
| Phase 6: Scrolling | 2-3 | Medium | High |
| Phase 7: Advanced | 2-4 | High | Medium |

**Total**: 3-4 weeks for complete PPU

**Minimum Viable**: Phases 1-4 = ~1 week for basic graphics

---

## 🛠️ Implementation Strategy

### Start Simple:
1. Stub all 8 registers
2. Implement basic read/write
3. Add VBlank timing
4. Render single frame background
5. Optimize scanline-by-scanline

### Testing:
Use test ROMs:
- `ppu_vbl_nmi.nes` - VBlank timing
- `sprite_hit_tests_2005.10.05` - Sprite 0
- `blargg_ppu_tests_2005.09.15b` - General PPU

### Reference Implementation:
- Study FCEUX source code
- Check NESDev wiki for details
- Use PPU visualization tools

---

## 🎯 Quick Start (Next Session)

### Step 1: PPU Class Structure
```cpp
class PPU {
public:
    // Registers
    uint8_t PPUCTRL;    // $2000
    uint8_t PPUMASK;    // $2001
    uint8_t PPUSTATUS;  // $2002
    uint8_t OAMADDR;    // $2003
    // ... etc
    
    // Internal state
    uint16_t v;         // Current VRAM address
    uint16_t t;         // Temp VRAM address
    uint8_t x;          // Fine X scroll
    bool w;             // Write toggle
    
    // Memory
    uint8_t vram[2048];      // Nametables
    uint8_t palette[32];     // Palettes
    uint8_t oam[256];        // Sprite memory
    
    // Output
    uint8_t framebuffer[256 * 240 * 4]; // RGBA
    
    // Timing
    int scanline;
    int cycle;
    
    // Methods
    void step();                    // Execute 1 PPU cycle
    uint8_t read_register(uint16_t addr);
    void write_register(uint16_t addr, uint8_t value);
    void render_scanline();
};
```

### Step 2: Connect to CPU
```cpp
// In Memory::read()
if (address >= 0x2000 && address < 0x4000) {
    return ppu->read_register(address);
}

// In Memory::write()
if (address >= 0x2000 && address < 0x4000) {
    ppu->write_register(address, value);
}
```

### Step 3: Basic Timing Loop
```cpp
void Emulator::run_frame() {
    // 262 scanlines per frame
    for (int scanline = 0; scanline < 262; scanline++) {
        // 341 PPU cycles per scanline
        for (int cycle = 0; cycle < 341; cycle++) {
            ppu.step();
            
            // CPU runs at 1/3 PPU speed
            if (cycle % 3 == 0) {
                cpu.step();
            }
        }
    }
}
```

---

## 📚 Essential Resources

### Documentation:
1. **NESDev Wiki PPU**
   - https://www.nesdev.org/wiki/PPU
   - Complete reference

2. **PPU Rendering**
   - https://www.nesdev.org/wiki/PPU_rendering
   - Scanline rendering details

3. **PPU Registers**
   - https://www.nesdev.org/wiki/PPU_registers
   - Register specifications

### Test ROMs:
1. **blargg's PPU tests**
2. **sprite_hit_tests**
3. **ppu_vbl_nmi**

### Visualization:
1. **FCEUX Debugger** - See PPU state
2. **Mesen** - Advanced debugging

---

## 🎨 Expected Output

**After Phase 3** (Background rendering):
```
You'll see:
- Title screens
- Static backgrounds
- No sprites yet
- No scrolling yet
```

**After Phase 5** (Sprites):
```
You'll see:
- Full game graphics!
- Characters moving
- Still basic scrolling
```

**After Phase 7** (Complete):
```
You'll see:
- Perfect graphics
- Smooth scrolling
- All visual effects
- 99% game compatibility
```

---

## 💡 Pro Tips

1. **Start with one game**: Focus on making Donkey Kong work
2. **Visualize**: Add debug output to see nametables/patterns
3. **Test incrementally**: Don't wait for full implementation
4. **Use existing tools**: Compare with FCEUX/Mesen
5. **Palette first**: Get colors working before rendering

---

## 🎯 Success Criteria

### Minimum (1 week):
- ✅ VBlank timing working
- ✅ Can see background graphics
- ✅ At least one game visible

### Complete (3-4 weeks):
- ✅ All 8 registers working
- ✅ Background + sprites
- ✅ Scrolling functional
- ✅ 90%+ games render correctly

---

**Ready to start?** Begin with Phase 1 - PPU Registers! 🎨

**Next file to create**: `core/ppu/ppu_detailed.cpp`

---

Generated: 2025-12-25 21:51  
**Status**: Ready for PPU implementation!
