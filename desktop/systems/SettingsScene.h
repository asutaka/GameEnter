#pragma once
#include <SDL2/SDL.h>
#include <string>
#include <vector>
#include "FontSystem.h"
#include "UISystem.h"

namespace nes {

class SettingsScene {
public:
    std::string toast_message = "";
    Uint32 toast_timer = 0;

    void render(SDL_Renderer* renderer,
                const std::string& settings_nickname,
                const std::string& settings_avatar_path,
                bool settings_recorder_enabled,
                int active_input_field,
                FontSystem& font_title,
                FontSystem& font_body,
                FontSystem& font_small,
                int SCREEN_WIDTH, int SCREEN_HEIGHT, int SCALE,
                SDL_Texture* (*load_texture_func)(SDL_Renderer*, const std::string&)) {
        
        SDL_SetRenderDrawColor(renderer, 245, 246, 247, 255);
        SDL_RenderClear(renderer);
        
        int content_x = 40;
        int content_width = SCREEN_WIDTH * SCALE - 80;
        int start_y = 130;
        
        // --- PROFILE SECTION ---
        int section_y = start_y;
        SDL_SetRenderDrawColor(renderer, 34, 43, 50, 255);
        draw_filled_circle_aa(renderer, content_x + 10, section_y + 12, 12);
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        draw_filled_circle_aa(renderer, content_x + 10, section_y + 8, 4);
        SDL_Rect body_icon = {content_x + 6, section_y + 13, 8, 4}; SDL_RenderFillRect(renderer, &body_icon);
        font_title.draw_text(renderer, "PROFILE SETTINGS", content_x + 35, section_y + 22, {34, 43, 50, 255});
        
        SDL_Rect profile_card = {content_x, section_y + 40, content_width, 160};
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 15);
        SDL_Rect shadow1 = {profile_card.x + 4, profile_card.y + 4, profile_card.w, profile_card.h};
        SDL_RenderFillRect(renderer, &shadow1);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
        
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderFillRect(renderer, &profile_card);
        SDL_SetRenderDrawColor(renderer, 230, 230, 230, 255);
        SDL_RenderDrawRect(renderer, &profile_card);
        
        SDL_Rect avatar_box = {profile_card.x + 30, profile_card.y + 30, 100, 100};
        SDL_Texture* avatar_tex = nullptr;
        if (!settings_avatar_path.empty() && load_texture_func) {
            avatar_tex = load_texture_func(renderer, settings_avatar_path);
        }
        
        if (avatar_tex) {
            SDL_RenderCopy(renderer, avatar_tex, NULL, &avatar_box);
            SDL_DestroyTexture(avatar_tex);
            SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
            SDL_RenderDrawRect(renderer, &avatar_box);
        } else {
            SDL_SetRenderDrawColor(renderer, 240, 240, 240, 255);
            SDL_RenderFillRect(renderer, &avatar_box);
            SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
            SDL_RenderDrawRect(renderer, &avatar_box);
            font_small.draw_text(renderer, "No Avatar", avatar_box.x + 15, avatar_box.y + 45, {150, 150, 150, 255});
            font_small.draw_text(renderer, "Click to set", avatar_box.x + 12, avatar_box.y + 65, {100, 150, 200, 255});
        }
        
        int input_x = avatar_box.x + 130;
        int input_y = profile_card.y + 55;
        font_small.draw_text(renderer, "NICKNAME", input_x, input_y - 12, {120, 120, 120, 255});
        SDL_Rect name_field = {input_x, input_y, profile_card.w - 190, 45};
        SDL_SetRenderDrawColor(renderer, 248, 249, 250, 255);
        SDL_RenderFillRect(renderer, &name_field);
        SDL_SetRenderDrawColor(renderer, active_input_field == 0 ? 52 : 220, active_input_field == 0 ? 152 : 220, active_input_field == 0 ? 219 : 220, 255);
        SDL_RenderDrawRect(renderer, &name_field);
        font_body.draw_text(renderer, settings_nickname, name_field.x + 15, name_field.y + 26, {34, 43, 50, 255});
        
        if (active_input_field == 0) {
             float tw = font_body.get_text_width(settings_nickname);
             SDL_SetRenderDrawColor(renderer, 34, 43, 50, 255);
             SDL_RenderDrawLine(renderer, name_field.x + 15 + (int)tw + 2, name_field.y + 10, name_field.x + 15 + (int)tw + 2, name_field.y + 35);
        }

        // --- SYSTEM SECTION ---
        section_y = profile_card.y + profile_card.h + 50; 
        SDL_SetRenderDrawColor(renderer, 34, 43, 50, 255);
        draw_filled_circle_aa(renderer, content_x + 10, section_y + 12, 12);
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        draw_filled_circle_aa(renderer, content_x + 10, section_y + 12, 6);
        SDL_SetRenderDrawColor(renderer, 34, 43, 50, 255);
        draw_filled_circle_aa(renderer, content_x + 10, section_y + 12, 3);
        font_title.draw_text(renderer, "SYSTEM PREFERENCES", content_x + 35, section_y + 22, {34, 43, 50, 255});
        
        SDL_Rect sys_card = {content_x, section_y + 40, content_width, 80};
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 15);
        SDL_Rect shadow2 = {sys_card.x + 4, sys_card.y + 4, sys_card.w, sys_card.h};
        SDL_RenderFillRect(renderer, &shadow2);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
        
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderFillRect(renderer, &sys_card);
        SDL_SetRenderDrawColor(renderer, 230, 230, 230, 255);
        SDL_RenderDrawRect(renderer, &sys_card);
        
        int row_y = sys_card.y + 15;
        font_body.draw_text(renderer, "Enable Gameplay Recorder", sys_card.x + 30, row_y + 32, {34, 43, 50, 255});
        font_small.draw_text(renderer, "Automatically save replays of your sessions", sys_card.x + 30, row_y + 55, {150, 150, 150, 255});
        
        int toggle_w = 60, toggle_h = 32;
        int toggle_x = sys_card.x + sys_card.w - toggle_w - 30;
        int toggle_y = row_y + 10;
        SDL_Rect track_rect = {toggle_x, toggle_y, toggle_w, toggle_h};
        SDL_SetRenderDrawColor(renderer, settings_recorder_enabled ? 46 : 200, settings_recorder_enabled ? 204 : 200, settings_recorder_enabled ? 113 : 200, 255);
        SDL_RenderFillRect(renderer, &track_rect);
        
        int thumb_size = 26;
        int thumb_x = settings_recorder_enabled ? (toggle_x + toggle_w - thumb_size - 3) : (toggle_x + 3);
        int thumb_y = toggle_y + 3;
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_Rect thumb_rect = {thumb_x, thumb_y, thumb_size, thumb_size};
        SDL_RenderFillRect(renderer, &thumb_rect);
        
        int btn_w = 140, btn_h = 45;
        SDL_Rect save_btn = {content_x + content_width - btn_w, sys_card.y + sys_card.h + 20, btn_w, btn_h};
        SDL_SetRenderDrawColor(renderer, 46, 204, 113, 255);
        SDL_RenderFillRect(renderer, &save_btn);
        float save_txt_w = font_body.get_text_width("Save Changes");
        font_body.draw_text(renderer, "Save Changes", save_btn.x + (btn_w - (int)save_txt_w)/2, save_btn.y + 28, {255, 255, 255, 255});

        // --- TOAST NOTIFICATION ---
        if (toast_timer > SDL_GetTicks()) {
            int tw = 280, th = btn_h;
            int tx = content_x;
            int ty = save_btn.y;
            
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            // Light green background with border for success feedback
            SDL_SetRenderDrawColor(renderer, 46, 204, 113, 30); 
            SDL_Rect r = {tx, ty, tw, th};
            SDL_RenderFillRect(renderer, &r);
            SDL_SetRenderDrawColor(renderer, 46, 204, 113, 100);
            SDL_RenderDrawRect(renderer, &r);
            
            font_body.draw_text(renderer, toast_message, tx + 15, ty + 28, {39, 174, 96, 255}); // Darker green text
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
        }
    }

    void handle_event(const SDL_Event& e, 
                      std::string& settings_nickname, 
                      std::string& settings_avatar_path,
                      bool& settings_recorder_enabled,
                      int& active_input_field,
                      ConfigManager& config,
                      int SCREEN_WIDTH, int SCREEN_HEIGHT, int SCALE,
                      std::function<std::string()> open_file_dialog_func,
                      std::function<std::string(const std::string&, const std::string&)> import_avatar_func) {
        
        if (e.type == SDL_TEXTINPUT) {
            if (active_input_field == 0) settings_nickname += e.text.text;
        } else if (e.type == SDL_KEYDOWN) {
            if (e.key.keysym.sym == SDLK_BACKSPACE) {
                if (active_input_field == 0 && !settings_nickname.empty()) {
                    settings_nickname.pop_back();
                }
            } else if (e.key.keysym.sym == SDLK_RETURN || e.key.keysym.sym == SDLK_ESCAPE) {
                active_input_field = -1;
                SDL_StopTextInput();
            }
        } else if (e.type == SDL_MOUSEBUTTONDOWN) {
            int mx = e.button.x;
            int my = e.button.y;
            
            int content_x = 40;
            int content_width = SCREEN_WIDTH * SCALE - 80;
            int start_y = 130;  
            int profile_card_y = start_y + 40;
            
            // Avatar Hitbox
            SDL_Rect avatar_box = {content_x + 30, profile_card_y + 30, 100, 100};
            if (mx >= avatar_box.x && mx <= avatar_box.x + avatar_box.w &&
                my >= avatar_box.y && my <= avatar_box.y + avatar_box.h) {
                if (open_file_dialog_func && import_avatar_func) {
                    std::string path = open_file_dialog_func();
                    if (!path.empty()) {
                        std::string imported = import_avatar_func(path, config.get_device_id());
                        if (!imported.empty()) settings_avatar_path = imported;
                    }
                }
            }
            
            // Nickname Input Hitbox
            SDL_Rect name_field = {avatar_box.x + 130, profile_card_y + 55, content_width - 190, 45};
            if (mx >= name_field.x && mx <= name_field.x + name_field.w &&
                my >= name_field.y && my <= name_field.y + name_field.h) {
                active_input_field = 0;
                SDL_StartTextInput();
            } else {
                // Check other clicks to blur? 
                // Simplified: if click outside, blur. But save button is outside.
            }
            
            // Recorder Toggle
            int section_y = profile_card_y + 160 + 50; 
            int sys_card_y = section_y + 40;
            int toggle_w = 60, toggle_h = 32;
            int toggle_x = content_x + content_width - toggle_w - 30;
            int toggle_y = sys_card_y + 15 + 10;
            SDL_Rect toggle_rect = {toggle_x, toggle_y, toggle_w, toggle_h};
            if (mx >= toggle_rect.x && mx <= toggle_rect.x + toggle_rect.w &&
                my >= toggle_rect.y && my <= toggle_rect.y + toggle_rect.h) {
                settings_recorder_enabled = !settings_recorder_enabled;
            }
            
            // Save Button
            int btn_w = 140, btn_h = 45;
            SDL_Rect save_btn = {content_x + content_width - btn_w, sys_card_y + 80 + 20, btn_w, btn_h};
            if (mx >= save_btn.x && mx <= save_btn.x + save_btn.w &&
                my >= save_btn.y && my <= save_btn.y + save_btn.h) {
                config.set_nickname(settings_nickname);
                config.set_avatar_path(settings_avatar_path);
                config.set_gameplay_recorder_enabled(settings_recorder_enabled);
                config.save();
                std::cout << "💾 Settings Saved!" << std::endl;
                toast_message = "Settings Saved Successfully!";
                toast_timer = SDL_GetTicks() + 3000; // Show for 3 seconds
            }
        }
    }
};

} // namespace nes
