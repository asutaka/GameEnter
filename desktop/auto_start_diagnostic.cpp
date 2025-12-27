#include "../core/emulator.h"
#include <iostream>
#include <iomanip>
#include <vector>

using namespace nes;

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <rom_file>" << std::endl;
        return 1;
    }

    Emulator emu;
    if (!emu.load_rom(argv[1])) {
        std::cerr << "Failed to load ROM" << std::endl;
        return 1;
    }

    std::cout << "=== Auto-Start Diagnostic Tool ===" << std::endl;
    std::cout << "ROM loaded successfully!" << std::endl;
    std::cout << std::endl;

    emu.reset();

    // Run without input for 2 seconds (120 frames)
    std::cout << "Phase 1: Running without input for 2 seconds..." << std::endl;
    for (int frame = 0; frame < 120; frame++) {
        emu.set_controller(0, 0);  // No input
        emu.run_frame();
        
        if (frame % 30 == 0) {
            std::cout << "  Frame " << frame << "..." << std::endl;
        }
    }

    // Check framebuffer #1
    std::cout << std::endl << "Checking framebuffer (before START)..." << std::endl;
    const uint8_t* fb = emu.get_framebuffer();
    uint32_t first_color = (fb[0] << 16) | (fb[1] << 8) | fb[2];
    
    int different_pixels_before = 0;
    for (int i = 0; i < 256*240; i++) {
        uint32_t color = (fb[i*4] << 16) | (fb[i*4+1] << 8) | fb[i*4+2];
        if (color != first_color) different_pixels_before++;
    }
    
    std::cout << "  Background color: #" << std::hex << std::setw(6) << std::setfill('0') << first_color << std::dec << std::endl;
    std::cout << "  Different pixels: " << different_pixels_before << " / " << (256*240) << std::endl;

    // Press START button for 1 second (60 frames)
    std::cout << std::endl << "Phase 2: Pressing START for 1 second..." << std::endl;
    uint8_t start_button = (1 << Input::BUTTON_START);
    for (int frame = 0; frame < 60; frame++) {
        emu.set_controller(0, start_button);
        emu.run_frame();
        
        if (frame % 15 == 0) {
            std::cout << "  Frame " << frame << " (START pressed)..." << std::endl;
        }
    }

    // Release START and run for another 2 seconds
    std::cout << std::endl << "Phase 3: Released START, running for 2 more seconds..." << std::endl;
    for (int frame = 0; frame < 120; frame++) {
        emu.set_controller(0, 0);  // No input
        emu.run_frame();
        
        if (frame % 30 == 0) {
            std::cout << "  Frame " << frame << "..." << std::endl;
        }
    }

    // Check framebuffer #2
    std::cout << std::endl << "Checking framebuffer (after START)..." << std::endl;
    fb = emu.get_framebuffer();
    first_color = (fb[0] << 16) | (fb[1] << 8) | fb[2];
    
    int different_pixels_after = 0;
    std::vector<uint32_t> unique_colors;
    for (int i = 0; i < 256*240; i++) {
        uint32_t color = (fb[i*4] << 16) | (fb[i*4+1] << 8) | fb[i*4+2];
        if (color != first_color) different_pixels_after++;
        
        // Collect unique colors
        if (std::find(unique_colors.begin(), unique_colors.end(), color) == unique_colors.end()) {
            unique_colors.push_back(color);
        }
    }
    
    std::cout << "  Background color: #" << std::hex << std::setw(6) << std::setfill('0') << first_color << std::dec << std::endl;
    std::cout << "  Different pixels: " << different_pixels_after << " / " << (256*240) << std::endl;
    std::cout << "  Unique colors: " << unique_colors.size() << std::endl;

    // Show some unique colors
    if (unique_colors.size() > 1 && unique_colors.size() <= 10) {
        std::cout << std::endl << "  Color palette:" << std::endl;
        for (size_t i = 0; i < unique_colors.size(); i++) {
            std::cout << "    #" << std::hex << std::setw(6) << std::setfill('0') << unique_colors[i] << std::dec << std::endl;
        }
    }

    // Final verdict
    std::cout << std::endl << "=== VERDICT ===" << std::endl;
    if (different_pixels_after == 0) {
        std::cout << "❌ FAIL: Still no graphics after pressing START" << std::endl;
        std::cout << std::endl;
        std::cout << "POSSIBLE CAUSES:" << std::endl;
        std::cout << "  1. PPU rendering is broken" << std::endl;
        std::cout << "  2. Game needs more time to initialize" << std::endl;
        std::cout << "  3. Controller input not working" << std::endl;
        std::cout << "  4. CHR ROM/RAM not loading correctly" << std::endl;
    } else if (different_pixels_after < 1000) {
        std::cout << "⚠️  PARTIAL: Some pixels but not enough" << std::endl;
        std::cout << "    This might be extremely minimal graphics" << std::endl;
    } else {
        std::cout << "✅ SUCCESS: Graphics are rendering!" << std::endl;
        std::cout << "    Pixels changed: " << different_pixels_before << " → " << different_pixels_after << std::endl;
        std::cout << "    Unique colors: " << unique_colors.size() << std::endl;
    }

    return 0;
}
