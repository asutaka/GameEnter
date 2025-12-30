#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <SDL2/SDL.h>

#ifdef STB_TRUETYPE_IMPLEMENTATION
#undef STB_TRUETYPE_IMPLEMENTATION
#endif
#include "../stb_truetype.h"

// --- Font System (stb_truetype) ---
struct FontSystem {
    stbtt_bakedchar cdata[96]; // ASCII 32..126 is 95 glyphs
    SDL_Texture* font_texture = nullptr;
    int texture_width = 512;
    int texture_height = 512;
    bool initialized = false;

    bool init(SDL_Renderer* renderer, const std::string& font_path, float pixel_height) {
        std::ifstream file(font_path, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            std::cerr << "Failed to open font file: " << font_path << std::endl;
            return false;
        }
        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);

        std::vector<unsigned char> ttf_buffer(size);
        if (!file.read((char*)ttf_buffer.data(), size)) return false;

        std::vector<unsigned char> temp_bitmap(texture_width * texture_height);
        
        // Bake font into a single-channel bitmap
        int result = stbtt_BakeFontBitmap(ttf_buffer.data(), 0, pixel_height, temp_bitmap.data(), texture_width, texture_height, 32, 96, cdata);
        if (result <= 0) {
             std::cerr << "Failed to bake font bitmap (bitmap too small?). Result: " << result << std::endl;
             // Try increasing texture size or reducing font size if this fails regularly
             return false;
        }

        // Convert 1-channel bitmap to 4-channel RGBA for SDL
        std::vector<unsigned char> rgba_bitmap(texture_width * texture_height * 4);
        for (int i = 0; i < texture_width * texture_height; ++i) {
            rgba_bitmap[i*4 + 0] = 255; // R
            rgba_bitmap[i*4 + 1] = 255; // G
            rgba_bitmap[i*4 + 2] = 255; // B
            rgba_bitmap[i*4 + 3] = temp_bitmap[i]; // Alpha from font bitmap
        }

        font_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC, texture_width, texture_height);
        if (!font_texture) return false;

        SDL_UpdateTexture(font_texture, NULL, rgba_bitmap.data(), texture_width * 4);
        SDL_SetTextureBlendMode(font_texture, SDL_BLENDMODE_BLEND);
        
        initialized = true;
        return true;
    }

    void draw_text(SDL_Renderer* renderer, const std::string& text, float x, float y, SDL_Color color) {
        if (!initialized) return;

        SDL_SetTextureColorMod(font_texture, color.r, color.g, color.b);
        SDL_SetTextureAlphaMod(font_texture, color.a);

        for (char c : text) {
            if (c < 32 || c >= 128) c = '?';
            if (c >= 32 && c < 128) {
                stbtt_aligned_quad q;
                stbtt_GetBakedQuad(cdata, texture_width, texture_height, c - 32, &x, &y, &q, 0);

                SDL_Rect src = { (int)(q.s0 * texture_width), (int)(q.t0 * texture_height), (int)((q.s1 - q.s0) * texture_width), (int)((q.t1 - q.t0) * texture_height) };
                SDL_Rect dst = { (int)q.x0, (int)q.y0, (int)(q.x1 - q.x0), (int)(q.y1 - q.y0) };
                
                SDL_RenderCopy(renderer, font_texture, &src, &dst);
            }
        }
    }

    float get_text_width(const std::string& text) {
        float x = 0;
        float y = 0;
        for (char c : text) {
            if (c < 32 || c >= 128) c = '?';
            if (c >= 32 && c < 128) {
                stbtt_aligned_quad q;
                stbtt_GetBakedQuad(cdata, texture_width, texture_height, c - 32, &x, &y, &q, 0);
            }
        }
        return x;
    }

    void cleanup() {
        if (font_texture) {
            SDL_DestroyTexture(font_texture);
            font_texture = nullptr;
        }
    }
};

// Extern declarations
extern FontSystem font_small;
extern FontSystem font_body;
extern FontSystem font_title;
