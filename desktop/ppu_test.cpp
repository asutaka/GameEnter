#include "../core/emulator.h"
#include <iostream>
#include <fstream>
#include <iomanip>

using namespace nes;

// Save framebuffer as PPM image (simple format)
void save_framebuffer_ppm(const uint8_t* framebuffer, const std::string& filename) {
    std::ofstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Failed to create " << filename << std::endl;
        return;
    }
    
    // PPM header
    file << "P6\n";
    file << "256 240\n";
    file << "255\n";
    
    // Write pixel data (RGBA -> RGB)
    for (int i = 0; i < 256 * 240; i++) {
        file.put(framebuffer[i * 4 + 0]); // R
        file.put(framebuffer[i * 4 + 1]); // G
        file.put(framebuffer[i * 4 + 2]); // B
        // Skip alpha channel
    }
    
    file.close();
    std::cout << "Framebuffer saved to: " << filename << std::endl;
}

int main(int argc, char* argv[]) {
    std::cout << "=== NES PPU Rendering Test ===" << std::endl;
    std::cout << std::endl;
    
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <rom_file.nes> [frames]" << std::endl;
        std::cerr << std::endl;
        std::cerr << "Example:" << std::endl;
        std::cerr << "  " << argv[0] << " donkeykong.nes 10" << std::endl;
        return 1;
    }
    
    std::string rom_file = argv[1];
    int num_frames = 10; // Default
    
    if (argc >= 3) {
        num_frames = std::atoi(argv[2]);
    }
    
    // Create emulator
    Emulator emu;
    
    // Load ROM
    std::cout << "Loading ROM: " << rom_file << std::endl;
    if (!emu.load_rom(rom_file)) {
        std::cerr << "Failed to load ROM!" << std::endl;
        return 1;
    }
    std::cout << "ROM loaded successfully!" << std::endl;
    std::cout << std::endl;
    
    // Reset
    emu.reset();
    
    std::cout << "Running " << num_frames << " frames..." << std::endl;
    std::cout << "This will generate frame_00.ppm, frame_01.ppm, etc." << std::endl;
    std::cout << std::endl;
    
    // Run frames
    for (int frame = 0; frame < num_frames; frame++) {
        std::cout << "Frame " << frame << "..." << std::flush;
        
        // Run one frame
        emu.run_frame();
        
        // Get framebuffer via public API
        const uint8_t* fb = emu.get_framebuffer();
        
        // Save to file
        char filename[32];
        snprintf(filename, sizeof(filename), "frame_%02d.ppm", frame);
        save_framebuffer_ppm(fb, filename);
    }
    
    std::cout << std::endl;
    std::cout << "=== PPU Test Complete! ===" << std::endl;
    std::cout << std::endl;
    std::cout << "View the .ppm files with:" << std::endl;
    std::cout << "  - Windows: GIMP, IrfanView, or Paint.NET" << std::endl;
    std::cout << "  - Or convert to PNG: magick frame_00.ppm frame_00.png" << std::endl;
    std::cout << std::endl;
    std::cout << "You should see:" << std::endl;
    std::cout << "  - Background tiles rendered" << std::endl;
    std::cout << "  - Sprites visible (if game has started)" << std::endl;
    std::cout << "  - Correct NES colors" << std::endl;
    
    return 0;
}
