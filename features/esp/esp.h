#pragma once
#include <algorithm>
#include <string>
#include <unordered_map>
#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif
#include "core/Core.hpp"
#include "core/storage/Globals.hpp"

struct render_t {

#define vec_ref const ImVec2 &
#define decl static auto

  inline static ImFont *visitor{NULL};
  inline static ImFont *verdana_bold{NULL};
  inline static ImFont *weapon_icon_font{NULL};
  inline static std::unordered_map<float, float> health_text_positions{};

  decl rect_filled_gradient(vec_ref from, vec_ref to, ImU32 col1, ImU32 col2,
                            float rounding = 0.f) {
    ImDrawList *draw = ImGui::GetBackgroundDrawList();

    draw->AddRectFilledMultiColor(from, to, col1, col1, col2, col2);
  }

  decl rect_filled_blur(vec_ref from, vec_ref to, ImU32 col,
                        float rounding = 0.f, int layers = 5,
                        float offset = 0.5f) {
    auto draw = ImGui::GetBackgroundDrawList();
    for (int i = 0; i < layers; ++i) {
      ImVec2 offset_pos{offset * i, offset * i};
      draw->AddRectFilled(from - offset_pos, to + offset_pos, col, rounding);
    }
  }

  decl rect_outlined(ImVec2 from, ImVec2 to, ImU32 col, float rounding = 0.f,
                     float thickness = 1.0f) {
    ImGui::GetBackgroundDrawList()->AddRect(from, to, col, rounding, 0,
                                            thickness);
    if (storage::fill_box) {
      if (storage::fill_box_gradient) {
        ImU32 gradient_color1 = ImGui::ColorConvertFloat4ToU32(
            ImVec4(storage::fill_box_gradient_color1[0],
                   storage::fill_box_gradient_color1[1],
                   storage::fill_box_gradient_color1[2],
                   storage::fill_box_gradient_color1[3]));
        ImU32 gradient_color2 = ImGui::ColorConvertFloat4ToU32(
            ImVec4(storage::fill_box_gradient_color2[0],
                   storage::fill_box_gradient_color2[1],
                   storage::fill_box_gradient_color2[2],
                   storage::fill_box_gradient_color2[3]));
        rect_filled_gradient(from, to, gradient_color1, gradient_color2);
      } else {
        ImU32 box_color = ImGui::ColorConvertFloat4ToU32(
            ImVec4(storage::box_color1[0], storage::box_color1[1],
                   storage::box_color1[2], 0.3f));
        ImGui::GetBackgroundDrawList()->AddRectFilled(from, to, box_color);
      }
    }
  }

  decl rect_filled(vec_ref from, vec_ref to, ImU32 col, float rounding = 0.f) {
    ImGui::GetBackgroundDrawList()->AddRectFilled(from, to, col, rounding);
  }

  decl outlined_rect_blur(vec_ref pos, vec_ref size, ImU32 col,
                          float blur_intensity = 0.5f, int layers = 30) {
    auto draw = ImGui::GetBackgroundDrawList();

    for (int i = 1; i <= layers; ++i) {
      float alpha_factor = 1.0f - (float(i) / layers);
      ImU32 faded_col = ImGui::ColorConvertFloat4ToU32(
          ImVec4((col >> IM_COL32_R_SHIFT & 0xFF) / 255.0f,
                 (col >> IM_COL32_G_SHIFT & 0xFF) / 255.0f,
                 (col >> IM_COL32_B_SHIFT & 0xFF) / 255.0f,
                 alpha_factor * blur_intensity));

      ImVec2 offset(i * 0.5f, i * 0.5f);
      draw->AddRect(pos - offset, pos + size + offset, faded_col, 0.0f);
      draw->AddRect(pos + offset, pos + size - offset, faded_col, 0.0f);
    }

    draw->AddRect(pos, pos + size, col, 0.0f);
  }

  decl glow_box_rect(ImVec2 &pos, ImVec2 &size, ImU32 col,
                     float rounding = 0.f) {
    pos.x = std::round(pos.x);
    pos.y = std::round(pos.y);
    size.x = std::round(size.x);
    size.y = std::round(size.y);

    auto draw = ImGui::GetBackgroundDrawList();
    ImRect rect_bb(pos, ImVec2(pos.x + size.x, pos.y + size.y));

    ImU32 glow_color = ImGui::ColorConvertFloat4ToU32(
        ImVec4(storage::glow_box_color[0], storage::glow_box_color[1],
               storage::glow_box_color[2], storage::glow_box_color[3]));

    // Simple glow effect using multiple rect outlines with decreasing alpha
    for (int i = 5; i >= 1; i--) {
      float alpha = storage::glow_box_color[3] * (0.1f * i);
      ImU32 layer_color = ImGui::ColorConvertFloat4ToU32(
          ImVec4(storage::glow_box_color[0], storage::glow_box_color[1],
                 storage::glow_box_color[2], alpha));
      float offset = storage::glow_box_intensity * 0.2f * (6 - i);
      draw->AddRect(ImVec2(rect_bb.Min.x - offset, rect_bb.Min.y - offset),
                    ImVec2(rect_bb.Max.x + offset, rect_bb.Max.y + offset),
                    layer_color, rounding, 0, 1.0f);
    }

    draw->AddRect(rect_bb.Min, rect_bb.Max, col, rounding, 0, 1.0f);
  }

  decl outlined_rect(ImVec2 &pos, ImVec2 &size, ImU32 col,
                     float rounding = 0.f) {
    auto draw = ImGui::GetBackgroundDrawList();
    
    // Strict integer snapping for pixel-perfection
    ImVec2 p = { std::round(pos.x), std::round(pos.y) };
    ImVec2 s = { std::round(size.x), std::round(size.y) };
    ImRect rect_bb(p, ImVec2(p.x + s.x, p.y + s.y));

    if (storage::fill_box) {
      if (storage::fill_box_gradient) {
        ImU32 gradient_color1 = ImGui::ColorConvertFloat4ToU32(
            ImVec4(storage::fill_box_gradient_color1[0],
                   storage::fill_box_gradient_color1[1],
                   storage::fill_box_gradient_color1[2],
                   storage::fill_box_gradient_color1[3]));
        ImU32 gradient_color2 = ImGui::ColorConvertFloat4ToU32(
            ImVec4(storage::fill_box_gradient_color2[0],
                   storage::fill_box_gradient_color2[1],
                   storage::fill_box_gradient_color2[2],
                   storage::fill_box_gradient_color2[3]));
        rect_filled_gradient(rect_bb.Min, rect_bb.Max, gradient_color1,
                             gradient_color2);
      } else {
        ImU32 box_color = ImGui::ColorConvertFloat4ToU32(
            ImVec4(storage::box_color1[0], storage::box_color1[1],
                   storage::box_color1[2], 0.3f));
        draw->AddRectFilled(rect_bb.Min, rect_bb.Max, box_color, rounding);
      }
    }

    // High-contrast 3-pass outline system
    ImU32 black = IM_COL32(0, 0, 0, col >> 24);
    
    // Outer shadow
    draw->AddRect(ImVec2(rect_bb.Min.x - 2.0f, rect_bb.Min.y - 2.0f),
                  ImVec2(rect_bb.Max.x + 2.0f, rect_bb.Max.y + 2.0f),
                  black, rounding);
    // Inner shadow
    draw->AddRect(rect_bb.Min, rect_bb.Max, black, rounding);
    // Main colored line
    draw->AddRect(ImVec2(rect_bb.Min.x - 1.0f, rect_bb.Min.y - 1.0f),
                  ImVec2(rect_bb.Max.x + 1.0f, rect_bb.Max.y + 1.0f), 
                  col, rounding);
  }
  void draw_health_bar(ImDrawList *draw, float max, float current, ImVec2 pos,
                       ImVec2 size, float alpha_factor = 1.0f,
                       bool show_text = true) {
    if (max <= 0.0f)
      max = 100.0f;

    float clamped_current = std::clamp(current, 0.0f, max);
    float health_percent = (max > 0.0f) ? (clamped_current / max) : 0.0f;
    health_percent = std::clamp(health_percent, 0.0f, 1.0f);

    float bar_width = 1.0f;
    float bar_height = std::round(size.y);
    float bar_x = std::round(pos.x);
    float bar_y = std::round(pos.y);

    // High-contrast background with integer snapping
    draw->AddRectFilled(ImVec2(bar_x - 1, bar_y - 1),
                        ImVec2(bar_x + bar_width + 1, bar_y + bar_height + 1),
                        IM_COL32(0, 0, 0, 255));

    float fill_height = bar_height * health_percent;

    ImU32 health_color =
        IM_COL32(static_cast<int>(globals::health_bar_color_main[0] * 255.f),
                 static_cast<int>(globals::health_bar_color_main[1] * 255.f),
                 static_cast<int>(globals::health_bar_color_main[2] * 255.f),
                 static_cast<int>(globals::health_bar_color_main[3] * 255.f));

    draw->AddRectFilled(ImVec2(bar_x, bar_y),
                        ImVec2(bar_x + bar_width, bar_y + bar_height),
                        IM_COL32(40, 40, 40, 255));

    if (fill_height > 0.1f) {
      float fill_start_y = bar_y + bar_height - fill_height;
      draw->AddRectFilled(
          ImVec2(bar_x, std::round(fill_start_y)),
          ImVec2(bar_x + bar_width, bar_y + bar_height),
          health_color);
    }

    if (storage::health_bar_text && health_percent < 1.0f) {
      char buffer[16];
      sprintf_s(buffer, "%.0f", current);

      ImFont *font = visitor;
      float font_size = 11.0f; // Upgraded font size for clarity

      float target_fill_top_y = bar_y + bar_height - fill_height;
      float lerp_speed = 8.0f;
      float delta_time = ImGui::GetIO().DeltaTime;
      float bar_key = bar_x * 10000.0f + bar_y;

      float &current_y = health_text_positions[bar_key];
      if (health_text_positions.find(bar_key) == health_text_positions.end()) {
        current_y = target_fill_top_y;
      }

      if (std::abs(current_y - target_fill_top_y) > bar_height) {
        current_y = target_fill_top_y;
      } else {
        current_y = current_y + (target_fill_top_y - current_y) * std::clamp(lerp_speed * delta_time, 0.0f, 1.0f);
      }

      ImVec2 text_size = font ? font->CalcTextSizeA(font_size, FLT_MAX, 0.0f, buffer) : ImGui::CalcTextSize(buffer);
      ImVec2 text_pos = ImVec2(std::round(bar_x - (text_size.x / 2.0f) + (bar_width / 2.0f)), std::round(current_y));

      // Use the high-quality 8-direction stroke for health text
      DrawTextWithSpacingAndOutline(draw, font, font_size, text_pos, 
                                     IM_COL32(255, 255, 255, 255), IM_COL32(0, 0, 0, 255), 
                                     buffer, 1.0f);
    }
  }
  decl text(vec_ref pos, const std::string &text, ImU32 col, ImFont *font) {

    ImGui::GetBackgroundDrawList()->AddText(font, 15.0f, pos, col,
                                            text.c_str());
  }

  decl text_Voided(vec_ref pos, const std::string &text, ImU32 col,
                   ImFont *font, float font_size = 15.0f) {
    auto alpha = col >> 24;

    ImGui::GetBackgroundDrawList()->AddText(font, font_size, pos, col,
                                            text.c_str());
  }

  static void DrawTextWithSpacing(ImDrawList *draw, ImFont *font,
                                  float font_size, ImVec2 pos, ImU32 col,
                                  const std::string &text,
                                  float char_spacing = 1.0f) {
    if (!font || text.empty())
      return;

    float x_offset = 0.0f;
    float start_x = std::round(pos.x);
    float start_y = std::round(pos.y);

    for (size_t i = 0; i < text.length(); i++) {
      char c = text[i];
      char char_str[2] = {c, '\0'};

      // Absolute integer placement for every character
      ImVec2 char_pos = ImVec2(std::round(start_x + x_offset), start_y);
      draw->AddText(font, font_size, char_pos, col, char_str);

      ImVec2 char_size =
          font->CalcTextSizeA(font_size, FLT_MAX, 0.0f, char_str);
      x_offset += char_size.x + char_spacing;
    }
  }

  static void DrawTextWithSpacingAndOutline(ImDrawList *draw, ImFont *font,
                                            float font_size, ImVec2 pos,
                                            ImU32 col, ImU32 outline_col,
                                            const std::string &text,
                                            float char_spacing = 1.0f) {
    if (!font || text.empty())
      return;

    // Strict 8-direction stroke with 1px integer offsets
    for (int dx = -1; dx <= 1; dx++) {
      for (int dy = -1; dy <= 1; dy++) {
        if (dx != 0 || dy != 0) {
          DrawTextWithSpacing(draw, font, font_size,
                              ImVec2(std::round(pos.x + (float)dx), std::round(pos.y + (float)dy)), 
                              outline_col, text, char_spacing);
        }
      }
    }
    DrawTextWithSpacing(draw, font, font_size, ImVec2(std::round(pos.x), std::round(pos.y)), col, text, char_spacing);
  }
};
inline render_t Visualize{};

bool IsPlayerDeadOrKO(const Engine::PlayerIns &player);
void DrawTracerESP(const Engine::PlayerIns &player, const ImVec2 &topLeft,
                   const ImVec2 &bottomRight, ImDrawList *draw,
                   const Engine::Vector2 &dimensions,
                   const Engine::Matrix4x4 &viewMatrix);
void DrawHitboxVisualization(const Engine::PlayerIns &player,
                             const Engine::Matrix4x4 &viewMatrix,
                             const Engine::Vector2 &dimensions,
                             ImDrawList *draw);
void DrawSkeletonESP(const Engine::PlayerIns &player,
                     const Engine::Matrix4x4 &viewMatrix,
                     const Engine::Vector2 &dimensions, ImDrawList *draw);
void render_player_chams(Engine::PlayerIns &player, ImDrawList *draw,
                         Engine::Vector2 dimensions,
                         Engine::Matrix4x4 viewMatrix);
