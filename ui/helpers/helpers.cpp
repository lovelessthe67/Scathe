#include "helpers.h"
#include "utils/configs/configs.hpp"
#include "features/explorer/explorer.h"
#include "globals.h"
#include "core/storage/include.h"
#include "imgui_internal.h"
#include <string>
#include <vector>
#include "features/notifications/NotificationManager.hpp"

extern bool g_menuVisible;

void helpers::HotkeyButton(keybind &kb, const char *label) {
  ImGuiWindow *window = ImGui::GetCurrentWindow();
  if (window->SkipItems)
    return;

  ImGuiContext &g = *GImGui;
  ImGuiIO &io = g.IO;

  const ImU32 bg_one = IM_COL32(15, 15, 15, 255);
  const ImU32 bg_two = IM_COL32(17, 17, 17, 255);
  const ImU32 stroke = IM_COL32(55, 55, 55, 255);
  const ImU32 stroke_two = IM_COL32(0, 0, 0, 255);
  const ImU32 text_col = IM_COL32(255, 255, 255, 200);
  const ImU32 accent = IM_COL32(137, 207, 240, 255);
  const ImU32 outline = IM_COL32(0, 0, 0, 255);

  const ImVec2 key_size = ImVec2(35, 16);

  ImGui::PushID(kb.name);

  const ImGuiID id = window->GetID(label);
  const ImRect rect(window->DC.CursorPos, window->DC.CursorPos + key_size);

  ImGui::ItemSize(rect, 0.0f);
  if (!ImGui::ItemAdd(rect, id)) {
    ImGui::PopID();
    return;
  }

  bool hovered = ImGui::ItemHoverable(rect, id, 0);

  char buf_display[64] = "none";
  if (kb.key != 0 && g.ActiveId != id) {
    strcpy_s(buf_display, kb.get_key_name().c_str());
  } else if (g.ActiveId == id) {
    strcpy_s(buf_display, "-");
  }

  ImDrawList *dl = window->DrawList;
  ImVec2 rMin = rect.Min;
  ImVec2 rMax = rect.Max;

  dl->AddRectFilledMultiColor(rMin + ImVec2(2, 2), rMax - ImVec2(2, 2), bg_two,
                              bg_two, bg_one, bg_one);

  dl->AddRect(rMin + ImVec2(1, 1), rMax - ImVec2(1, 1), stroke, 0.0f, 0, 1.0f);

  dl->AddRect(rMin, rMax, hovered ? accent : stroke_two, 0.0f, 0, 1.0f);

  ImVec2 text_size = ImGui::CalcTextSize(buf_display);
  ImVec2 text_pos = ImVec2(rMin.x + (key_size.x - text_size.x) * 0.5f,
                           rMin.y + (key_size.y - text_size.y) * 0.5f - 1);

  dl->AddText(text_pos + ImVec2(-1, -1), outline, buf_display);
  dl->AddText(text_pos + ImVec2(-1, 1), outline, buf_display);
  dl->AddText(text_pos + ImVec2(1, -1), outline, buf_display);
  dl->AddText(text_pos + ImVec2(1, 1), outline, buf_display);
  dl->AddText(text_pos, text_col, buf_display);

  if (hovered && io.MouseClicked[0]) {
    if (g.ActiveId != id) {
      memset(io.MouseDown, 0, sizeof(io.MouseDown));
      kb.key = 0;
    }
    ImGui::SetActiveID(id, window);
    ImGui::FocusWindow(window);
  } else if (io.MouseClicked[0]) {
    if (g.ActiveId == id)
      ImGui::ClearActiveID();
  }

  if (g.ActiveId == id) {
    for (int i = 0; i < 5; i++) {
      if (io.MouseDown[i]) {
        switch (i) {
        case 0:
          kb.key = VK_LBUTTON;
          break;
        case 1:
          kb.key = VK_RBUTTON;
          break;
        case 2:
          kb.key = VK_MBUTTON;
          break;
        case 3:
          kb.key = VK_XBUTTON1;
          break;
        case 4:
          kb.key = VK_XBUTTON2;
          break;
        }
        ImGui::ClearActiveID();
        ImGui::PopID();
        return;
      }
    }

    for (int i = VK_BACK; i <= VK_RMENU; i++) {
      if (GetAsyncKeyState(i) & 0x8000) {
        if (i == VK_ESCAPE) {
          kb.key = 0;
        } else {
          kb.key = i;
        }
        ImGui::ClearActiveID();
        ImGui::PopID();
        return;
      }
    }
  }

  const ImGuiID popup_id = ImHashStr("##KeybindPopup", 0, id);
  bool popup_open = ImGui::IsPopupOpen(popup_id, ImGuiPopupFlags_None);

  if (hovered && io.MouseClicked[1] && !popup_open) {
    ImGui::OpenPopupEx(popup_id, ImGuiPopupFlags_None);
    popup_open = true;
  }

  if (popup_open) {
    ImGui::PushStyleVar(ImGuiStyleVar_PopupBorderSize, 1.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4, 4));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 4));
    ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(53, 53, 67, 255));
    ImGui::PushStyleColor(ImGuiCol_PopupBg, IM_COL32(36, 36, 47, 255));

    ImGui::SetNextWindowPos(ImVec2(rect.Min.x, rect.Max.y + 3));
    if (ImGui::BeginPopupEx(popup_id, ImGuiWindowFlags_NoSavedSettings |
                                           ImGuiWindowFlags_NoDecoration |
                                           ImGuiWindowFlags_AlwaysAutoResize)) {

      ImGui::PushStyleColor(ImGuiCol_Header, IM_COL32(137, 207, 240, 100));
      ImGui::PushStyleColor(ImGuiCol_HeaderHovered,
                            IM_COL32(137, 207, 240, 150));

      if (ImGui::Selectable("Hold", kb.type == keybind::HOLD, 0, ImVec2(50, 0)))
        kb.type = keybind::HOLD;
      if (ImGui::Selectable("Toggle", kb.type == keybind::TOGGLE, 0,
                            ImVec2(50, 0)))
        kb.type = keybind::TOGGLE;
      if (ImGui::Selectable("Always", kb.type == keybind::ALWAYS, 0,
                            ImVec2(50, 0)))
        kb.type = keybind::ALWAYS;

      ImGui::PopStyleColor(2);
      ImGui::EndPopup();
    }

    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(3);
  }

  ImGui::PopID();
}

void helpers::add_tab() {
  ImDrawList *draw_list = ImGui::GetWindowDrawList();
  ImVec2 window_pos = ImGui::GetWindowPos();
  ImVec2 window_size = ImGui::GetWindowSize();
  float tab_height = 18.0f;
  float x_padding = 8.0f;
  float tab_y = window_pos.y + 4.0f;
  float right_padding = 4.0f;
  float gradient_height = 20.0f;
  float line_width = 65;
  float spacing = 4.0f;
  const char *tabs[] = {"Legit", "Rage",     "Esp",      "Players",
                        "Misc",  "Exploits", "Explorer", "Settings"};
  int tab_count = 8;
  float total_width = (line_width * tab_count) + (spacing * (tab_count - 1));
  float tab_start_x =
      window_pos.x + window_size.x - total_width - right_padding;
  float current_x = tab_start_x;

  draw_list->AddLine(ImVec2(tab_start_x - 3, window_pos.y),
                      ImVec2(tab_start_x - 3, window_pos.y + 25),
                      IM_COL32(55, 55, 55, 255), 1.0f);

  for (int i = 0; i < tab_count; i++) {
    ImVec2 text_size = ImGui::CalcTextSize(tabs[i]);
    bool is_active = (helpers::active_tab == i);
    ImU32 text_color =
        is_active ? IM_COL32(255, 255, 255, 255) : IM_COL32(255, 255, 255, 128);
    ImU32 line_color = IM_COL32(137, 207, 240, 255);
    if (helpers::is_tab_clicked(i, ImVec2(current_x, tab_y),
                                 ImVec2(line_width, tab_height)))
      helpers::active_tab = i;
    ImVec2 text_pos = ImVec2(current_x + (line_width - text_size.x) * 0.5f,
                              tab_y + (tab_height - text_size.y) * 0.5f);
    float line_start_x = current_x;
    float line_end_x = current_x + line_width;
    float line_y = text_pos.y + text_size.y + 1.0f;
    if (is_active) {
      float gradient_start_y = line_y + 2 - gradient_height;
      float gradient_end_y = line_y + 2;
      draw_list->AddRectFilledMultiColor(
          ImVec2(line_start_x, gradient_start_y),
          ImVec2(line_end_x, gradient_end_y), IM_COL32(137, 207, 240, 0),
          IM_COL32(137, 207, 240, 0), IM_COL32(137, 207, 240, 128),
          IM_COL32(137, 207, 240, 128));
      draw_list->AddRectFilled(ImVec2(line_start_x, line_y + 2),
                                ImVec2(line_end_x, line_y + 3), line_color);
    } else {
      draw_list->AddRectFilled(ImVec2(line_start_x, line_y + 2),
                                ImVec2(line_end_x, line_y + 3),
                                IM_COL32(60, 60, 60, 255));
    }
    for (int x = -1; x <= 1; x++) {
      for (int y = -1; y <= 1; y++) {
        if (x == 0 && y == 0)
          continue;
        draw_list->AddText(ImVec2(text_pos.x + x, text_pos.y + y),
                            IM_COL32(0, 0, 0, 255), tabs[i]);
      }
    }
    draw_list->AddText(text_pos, text_color, tabs[i]);
    current_x += line_width + spacing;
  }
}

bool helpers::is_tab_clicked(int tab_index, ImVec2 tab_pos, ImVec2 tab_size) {
  ImVec2 mouse_pos = ImGui::GetMousePos();
  bool is_hovered =
      mouse_pos.x >= tab_pos.x && mouse_pos.x <= tab_pos.x + tab_size.x &&
      mouse_pos.y >= tab_pos.y && mouse_pos.y <= tab_pos.y + tab_size.y;
  return is_hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left);
}

void helpers::render() {
  ImDrawList *draw_list = ImGui::GetWindowDrawList();
  ImVec2 window_pos = ImGui::GetWindowPos();
  ImVec2 window_size = ImGui::GetWindowSize();

  switch (helpers::active_tab) {
  case 0: {
    ImGui::SetCursorPos(ImVec2(17, 44));
    ImGui::BeginChild("Aimbot", ImVec2(324, 437), true);

    if (ImGui::Checkbox("Enable Aimbot", &storage::aimbot)) {
      NotificationManager::getInstance().push_notif(storage::aimbot ? "Enabled Aimbot" : "Disabled Aimbot");
    }
    ImGui::SameLine(ImGui::GetWindowWidth() - 50);
    helpers::HotkeyButton(storage::aimbotkeybind, "Aimbot");

    const char *aimbot_types[] = {"Mouse Aim", "Camera"};
    ImGui::Combo("Aimbot Type", &storage::aimbot_type, aimbot_types,
                 IM_ARRAYSIZE(aimbot_types));

    if (ImGui::Checkbox("Sticky Aim", &storage::sticky_aim)) {
      NotificationManager::getInstance().push_notif(storage::sticky_aim ? "Enabled Sticky Aim" : "Disabled Sticky Aim");
    }
    ImGui::SliderFloat("FOV", &storage::aimbot_fov, 1.0f, 800.0f);
    if (storage::aimbot_type == 0) {
      ImGui::SliderFloat("Smoothness", &storage::smoothness, 1.0f, 100.0f);
    } else {
      ImGui::SliderFloat("Smoothness X", &storage::smoothness_x, 1.0f, 100.0f);
      ImGui::SliderFloat("Smoothness Y", &storage::smoothness_y, 1.0f, 100.0f);
    }

    const char *aim_hitboxes[] = {"Head", "Torso", "Random"};
    ImGui::Combo("Aim Hitbox", &storage::aim_hitbox, aim_hitboxes,
                 IM_ARRAYSIZE(aim_hitboxes));

    if (ImGui::Checkbox("Show FOV", &storage::show_fov)) {
      NotificationManager::getInstance().push_notif(storage::show_fov ? "Shown FOV" : "Hidden FOV");
    }
    ImGui::SameLine();
    ImGui::ColorEdit4("##FOVColor", (float *)&storage::fov_color,
                      ImGuiColorEditFlags_NoInputs |
                          ImGuiColorEditFlags_NoLabel);

    if (ImGui::Checkbox("Use FOV", &storage::use_fov)) {
      NotificationManager::getInstance().push_notif(storage::use_fov ? "Enabled Use FOV" : "Disabled Use FOV");
    }

    ImGui::Spacing();

    ImGui::Text("Pred");
    ImGui::Separator();

    if (ImGui::Checkbox("Prediction", &storage::camera_prediction)) {
      NotificationManager::getInstance().push_notif(storage::camera_prediction ? "Enabled Prediction" : "Disabled Prediction");
    }
    if (storage::camera_prediction) {
      ImGui::SliderFloat("Pred X", &storage::camera_prediction_x, 0.1f, 5.0f);
      ImGui::SliderFloat("Pred Y", &storage::camera_prediction_y, 0.1f, 5.0f);
    }

    ImGui::Spacing();

    ImGui::Text("Checks");
    ImGui::Separator();

    if (ImGui::Checkbox("Team Check", &storage::team_prediction)) {
      NotificationManager::getInstance().push_notif(storage::team_prediction ? "Enabled Team Check" : "Disabled Team Check");
    }
    ImGui::SameLine();
    if (ImGui::Checkbox("Bot Check", &globals::botcheck)) {
      NotificationManager::getInstance().push_notif(globals::botcheck ? "Enabled Bot Check" : "Disabled Bot Check");
    }
    if (globals::botcheck) {
      const char* botmodes[] = { "Players Only", "Bots/NPCs Only", "Both" };
      ImGui::Combo("Target Mode", &globals::botmode, botmodes, 3);
    }
    if (ImGui::Checkbox("KO Check", &storage::ko_check)) {
      NotificationManager::getInstance().push_notif(storage::ko_check ? "Enabled KO Check" : "Disabled KO Check");
    }
    if (ImGui::Checkbox("Wall Check", &storage::wall_check)) {
      NotificationManager::getInstance().push_notif(storage::wall_check ? "Enabled Wall Check" : "Disabled Wall Check");
    }

    ImGui::EndChild();

    ImGui::SetCursorPos(ImVec2(348, 44));
    ImGui::BeginChild("TriggerBot", ImVec2(324, 437), true);

    if (ImGui::Checkbox("Enable Triggerbot", &storage::triggerbot)) {
      NotificationManager::getInstance().push_notif(storage::triggerbot ? "Enabled Triggerbot" : "Disabled Triggerbot");
    }
    ImGui::SameLine(ImGui::GetWindowWidth() - 50);
    helpers::HotkeyButton(storage::triggerbotkeybind, "Triggerbot");
    ImGui::SliderFloat("Delay (ms)", &storage::triggerbot_delay, 0.0f, 500.0f,
                       "%.0f ms");
    ImGui::SliderFloat("Range", &storage::triggerbot_range, 1.0f, 100.0f,
                       "%.0f px");

    ImGui::EndChild();
    break;
  }
  case 1: {
    ImGui::SetCursorPos(ImVec2(17, 44));
    ImGui::BeginChild("Silent Aim", ImVec2(324, 437), true);

    if (ImGui::Checkbox("Enable Silent Aim", &storage::silent_aim)) {
      NotificationManager::getInstance().push_notif(storage::silent_aim ? "Enabled Silent Aim" : "Disabled Silent Aim");
    }
    ImGui::SameLine(ImGui::GetWindowWidth() - 50);
    helpers::HotkeyButton(storage::silentaimkeybind, "Silent Aim");

    if (ImGui::Checkbox("Spoof Mouse", &storage::silentaim_spoof)) {
      NotificationManager::getInstance().push_notif(storage::silentaim_spoof ? "Enabled Spoof Mouse" : "Disabled Spoof Mouse");
    }

    ImGui::SliderFloat("FOV", &storage::silent_fov, 1.0f, 800.0f);
    ImGui::SliderFloat("Hit Chance", &storage::hitchance, 1.0f, 100.0f);

    const char *silent_hitboxes[] = {"Head", "Torso", "Random"};
    ImGui::Combo("Silent Hitbox", &storage::silent_hitbox, silent_hitboxes,
                 IM_ARRAYSIZE(silent_hitboxes));

    if (ImGui::Checkbox("Closest Part", &storage::silentaim_closestpart)) {
      NotificationManager::getInstance().push_notif(storage::silentaim_closestpart ? "Enabled Closest Part" : "Disabled Closest Part");
    }
    ImGui::SameLine();
    if (ImGui::Checkbox("Closest to Mouse", &storage::silent_closest_to_mouse)) {
      NotificationManager::getInstance().push_notif(storage::silent_closest_to_mouse ? "Enabled Closest to Mouse" : "Disabled Closest to Mouse");
    }
    if (ImGui::Checkbox("Use FOV", &storage::silent_use_fov)) {
      NotificationManager::getInstance().push_notif(storage::silent_use_fov ? "Enabled Use FOV" : "Disabled Use FOV");
    }
    if (ImGui::Checkbox("Show FOV", &storage::show_silent_fov)) {
      NotificationManager::getInstance().push_notif(storage::show_silent_fov ? "Shown Silent FOV" : "Hidden Silent FOV");
    }
    ImGui::SameLine();
    ImGui::ColorEdit4("##SilentFOVColor", (float *)&storage::silent_fov_color,
                      ImGuiColorEditFlags_NoInputs |
                          ImGuiColorEditFlags_NoLabel);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Text("Pred");

    if (ImGui::Checkbox("Prediction", &storage::silent_prediction)) {
      NotificationManager::getInstance().push_notif(storage::silent_prediction ? "Enabled Silent Prediction" : "Disabled Silent Prediction");
    }
    if (storage::silent_prediction) {
      ImGui::SliderFloat("Prediction X", &storage::silent_prediction_x, 0.1f,
                         5.0f);
      ImGui::SliderFloat("Prediction Y", &storage::silent_prediction_y, 0.1f,
                         5.0f);
    }

    ImGui::EndChild();

    ImGui::SetCursorPos(ImVec2(348, 44));
    ImGui::BeginChild("Misc", ImVec2(324, 437), true);

    if (ImGui::Checkbox("Sticky Aim", &storage::sticky_aim)) {
      NotificationManager::getInstance().push_notif(storage::sticky_aim ? "Enabled Sticky Aim" : "Disabled Sticky Aim");
    }
    if (ImGui::Checkbox("Team Check", &storage::silent_teamcheck)) {
      NotificationManager::getInstance().push_notif(storage::silent_teamcheck ? "Enabled Team Check" : "Disabled Team Check");
    }
    ImGui::SameLine();
    if (ImGui::Checkbox("Bot Check", &globals::botcheck)) {
      NotificationManager::getInstance().push_notif(globals::botcheck ? "Enabled Bot Check" : "Disabled Bot Check");
    }
    if (globals::botcheck) {
      const char* botmodes[] = { "Players Only", "Bots/NPCs Only", "Both" };
      ImGui::Combo("Target Mode", &globals::botmode, botmodes, 3);
    }
    if (ImGui::Checkbox("KO Check", &storage::knockhecksilent)) {
      NotificationManager::getInstance().push_notif(storage::knockhecksilent ? "Enabled KO Check" : "Disabled KO Check");
    }
    if (ImGui::Checkbox("Wall Check", &storage::silent_wallcheck)) {
      NotificationManager::getInstance().push_notif(storage::silent_wallcheck ? "Enabled Wall Check" : "Disabled Wall Check");
    }
    if (ImGui::Checkbox("Grabbed Check", &storage::silent_grabbedcheck)) {
      NotificationManager::getInstance().push_notif(storage::silent_grabbedcheck ? "Enabled Grabbed Check" : "Disabled Grabbed Check");
    }
    ImGui::EndChild();
    break;
  }
  case 2: {
    ImGui::SetCursorPos(ImVec2(17, 44));
    ImGui::BeginChild("Main ESP", ImVec2(324, 437), true);

    if (ImGui::Checkbox("Enable Visuals", &storage::esp)) {
      NotificationManager::getInstance().push_notif(storage::esp ? "Enabled Visuals" : "Disabled Visuals");
    }
    if (ImGui::Checkbox("Box ESP", &storage::visuals::box)) {
      NotificationManager::getInstance().push_notif(storage::visuals::box ? "Enabled Box ESP" : "Disabled Box ESP");
    }
    ImGui::SameLine();
    ImGui::ColorEdit4("##BoxColor", (float *)&storage::box_color,
                      ImGuiColorEditFlags_NoInputs |
                          ImGuiColorEditFlags_NoLabel);

    if (ImGui::Checkbox("Name ESP", &storage::visuals::name)) {
      NotificationManager::getInstance().push_notif(storage::visuals::name ? "Enabled Name ESP" : "Disabled Name ESP");
    }
    ImGui::SameLine();
    ImGui::ColorEdit4("##NameColor", (float *)&storage::name_color,
                      ImGuiColorEditFlags_NoInputs |
                          ImGuiColorEditFlags_NoLabel);

    if (ImGui::Checkbox("Distance ESP", &storage::visuals::distance)) {
      NotificationManager::getInstance().push_notif(storage::visuals::distance ? "Enabled Distance ESP" : "Disabled Distance ESP");
    }
    ImGui::SameLine();
    ImGui::ColorEdit4("##DistanceColor", (float *)&storage::distance_color,
                      ImGuiColorEditFlags_NoInputs |
                          ImGuiColorEditFlags_NoLabel);

    if (ImGui::Checkbox("Tool ESP", &storage::visuals::tool)) {
      NotificationManager::getInstance().push_notif(storage::visuals::tool ? "Enabled Tool ESP" : "Disabled Tool ESP");
    }
    ImGui::SameLine();
    ImGui::ColorEdit4("##ToolColor", (float *)&storage::tool_esp_color,
                      ImGuiColorEditFlags_NoInputs |
                          ImGuiColorEditFlags_NoLabel);

    if (ImGui::Checkbox("Skeleton ESP", &storage::visuals::skeleton)) {
      NotificationManager::getInstance().push_notif(storage::visuals::skeleton ? "Enabled Skeleton ESP" : "Disabled Skeleton ESP");
    }
    ImGui::SameLine();
    ImGui::ColorEdit4(
        "##SkeletonColor", (float *)&storage::visuals::skeleton_color,
        ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);

    if (ImGui::Checkbox("Tracer ESP", &storage::visuals::tracers)) {
      NotificationManager::getInstance().push_notif(storage::visuals::tracers ? "Enabled Tracer ESP" : "Disabled Tracer ESP");
    }
    ImGui::SameLine();
    ImGui::ColorEdit4("##TracerColor", (float *)&storage::tracers_color,
                      ImGuiColorEditFlags_NoInputs |
                          ImGuiColorEditFlags_NoLabel);
    if (storage::visuals::tracers) {
      const char *tracerTypes[] = {"Bottom", "Top", "Mouse"};
      ImGui::SetNextItemWidth(308.0f);
      ImGui::Combo("##TracerOrigin", &storage::tracer_type, tracerTypes,
                   IM_ARRAYSIZE(tracerTypes));
    }

    if (ImGui::Checkbox("Health Bar", &storage::visuals::health_bar)) {
      NotificationManager::getInstance().push_notif(storage::visuals::health_bar ? "Enabled Health Bar" : "Disabled Health Bar");
    }
    ImGui::SameLine();
    ImGui::ColorEdit4("##HealthColor", (float *)&globals::health_bar_color_main,
                      ImGuiColorEditFlags_NoInputs |
                          ImGuiColorEditFlags_NoLabel);

    if (ImGui::Checkbox("Weapon Icon", &storage::visuals::weapon_icon_esp)) {
      NotificationManager::getInstance().push_notif(storage::visuals::weapon_icon_esp ? "Enabled Weapon Icon" : "Disabled Weapon Icon");
    }
    ImGui::SameLine();
    ImGui::ColorEdit4(
        "##WeaponIconColor", (float *)&storage::visuals::weapon_icon_color,
        ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);

    if (ImGui::Checkbox("Filled Box", &storage::fill_box)) {
      NotificationManager::getInstance().push_notif(storage::fill_box ? "Enabled Filled Box" : "Disabled Filled Box");
    }
    ImGui::SameLine();
    ImGui::ColorEdit4("##FillBoxColor", (float *)&storage::box_color1,
                      ImGuiColorEditFlags_NoInputs |
                          ImGuiColorEditFlags_NoLabel);

    if (storage::fill_box) {
      if (ImGui::Checkbox("Gradient 1", &storage::fill_box_gradient)) {
        NotificationManager::getInstance().push_notif(storage::fill_box_gradient ? "Enabled Gradient 1" : "Disabled Gradient 1");
      }
      if (storage::fill_box_gradient) {
        ImGui::SameLine();
        ImGui::ColorEdit4(
            "##GradientColor1", (float *)&storage::fill_box_gradient_color1,
            ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
        ImGui::SameLine();
        ImGui::ColorEdit4(
            "##GradientColor2", (float *)&storage::fill_box_gradient_color2,
            ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
      }
      if (ImGui::Checkbox("Gradient 2", &storage::fill_box_gradient_2)) {
        NotificationManager::getInstance().push_notif(storage::fill_box_gradient_2 ? "Enabled Gradient 2" : "Disabled Gradient 2");
      }
      if (storage::fill_box_gradient_2) {
        ImGui::SameLine();
        ImGui::ColorEdit4(
            "##Gradient2Color1", (float *)&storage::fill_box_gradient_2_color1,
            ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
        ImGui::SameLine();
        ImGui::ColorEdit4(
            "##Gradient2Color2", (float *)&storage::fill_box_gradient_2_color2,
            ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
      }
    }
    if (ImGui::Checkbox("Chams", &storage::visuals::chams)) {
      NotificationManager::getInstance().push_notif(storage::visuals::chams ? "Enabled Chams" : "Disabled Chams");
    }
    if (ImGui::Checkbox("Self Forcefield Chams", &storage::visuals::forcefield_chams_self)) {
        NotificationManager::getInstance().push_notif(storage::visuals::forcefield_chams_self ? "Enabled Self Forcefield Chams" : "Disabled Self Forcefield Chams");
    }
    if (ImGui::Checkbox("Forcefield Chams", &storage::visuals::forcefield_chams_global)) {
        NotificationManager::getInstance().push_notif(storage::visuals::forcefield_chams_global ? "Enabled Forcefield Chams" : "Disabled Forcefield Chams");
    }

    ImGui::SameLine();
    ImGui::ColorEdit4("##ChamsColor", (float *)&storage::chams_color,
                      ImGuiColorEditFlags_NoInputs |
                          ImGuiColorEditFlags_NoLabel);

    if (ImGui::Checkbox("Sonar ESP", &globals::sonar_enabled)) {
      NotificationManager::getInstance().push_notif(globals::sonar_enabled ? "Enabled Sonar ESP" : "Disabled Sonar ESP");
    }
    if (globals::sonar_enabled) {
      ImGui::SliderFloat("Sonar Range", &globals::sonar_range, 10.0f, 500.0f);
      ImGui::SliderFloat("Sonar Thickness", &globals::sonar_thickness, 0.1f,
                         2.0f);
      ImGui::ColorEdit4("Sonar Color", globals::sonar_color);
      ImGui::ColorEdit4("Sonar Dot Color", globals::sonar_dot_color);
      if (ImGui::Checkbox("Show Sonar Distance", &globals::sonar_show_distance)) {
        NotificationManager::getInstance().push_notif(globals::sonar_show_distance ? "Shown Sonar Distance" : "Hidden Sonar Distance");
      }
    }

    if (ImGui::Checkbox("Crosshair", &storage::visuals::crosshair_enabled)) {
      NotificationManager::getInstance().push_notif(storage::visuals::crosshair_enabled ? "Enabled Crosshair" : "Disabled Crosshair");
    }
    ImGui::SameLine();
    ImGui::ColorEdit4(
        "##CrosshairColor", (float *)&storage::visuals::crosshair_color,
        ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);

    if (storage::visuals::crosshair_enabled) {
      ImGui::SliderFloat("Size", &storage::visuals::crosshair_size, 1.0f,
                         50.0f);
      ImGui::SliderFloat("Gap", &storage::visuals::crosshair_gap, 0.0f, 20.0f);
      ImGui::SliderFloat("Thickness", &storage::visuals::crosshair_thickness,
                         0.5f, 5.0f);
      const char *styles[] = {"Cross", "Circle", "Dot"};
      ImGui::Combo("Style", &storage::visuals::crosshair_styleIdx, styles,
                   IM_ARRAYSIZE(styles));
    }

    if (ImGui::Checkbox("Fog Control", &storage::visuals::fog)) {
      NotificationManager::getInstance().push_notif(storage::visuals::fog ? "Enabled Fog Control" : "Disabled Fog Control");
    }
    ImGui::SameLine();
    ImGui::ColorEdit4("##FogColor", (float *)&storage::visuals::fog_color,
                      ImGuiColorEditFlags_NoInputs |
                          ImGuiColorEditFlags_NoLabel);
    if (storage::visuals::fog) {
      ImGui::SliderFloat("Fog Start", &storage::visuals::fog_start, 0.0f,
                         1000.0f);
      ImGui::SliderFloat("Fog End", &storage::visuals::fog_end, 0.0f, 10000.0f);
    }

    ImGui::Spacing();
    ImGui::Separator();

    ImGui::Text("Bullet Tracers");
    if (ImGui::Checkbox("Bullet Tracers", &storage::visuals::bullet_tracers_enabled)) {
      NotificationManager::getInstance().push_notif(storage::visuals::bullet_tracers_enabled ? "Enabled Bullet Tracers" : "Disabled Bullet Tracers");
    }
    ImGui::SameLine();
    ImGui::ColorEdit4("##BulletTracerColor", (float *)&storage::visuals::bullet_tracer_color,
                      ImGuiColorEditFlags_NoInputs |
                          ImGuiColorEditFlags_NoLabel);
    
    if (storage::visuals::bullet_tracers_enabled) {
      const char *tracerStyles[] = {"Beam", "Swirl", "Lightning"};
      ImGui::SetNextItemWidth(308.0f);
      ImGui::Combo("##TracerStyle", &storage::visuals::bullet_tracer_style, tracerStyles,
                   IM_ARRAYSIZE(tracerStyles));
      
      ImGui::SliderFloat("Tracer Thickness", &storage::visuals::bullet_tracer_thickness, 0.5f, 10.0f, "%.1f");
      ImGui::SliderFloat("Tracer Opacity", &storage::visuals::bullet_tracer_color[3], 0.0f, 1.0f, "%.2f");
      ImGui::SliderFloat("Lifetime (ms)", &storage::visuals::bullet_tracer_lifetime, 500.0f, 5000.0f, "%.0f");
      ImGui::SliderFloat("Fade Start (ms)", &storage::visuals::bullet_tracer_fade_start, 0.0f, storage::visuals::bullet_tracer_lifetime, "%.0f");
      ImGui::Checkbox("Draw Outline", &storage::visuals::bullet_tracer_outline);
    }

    ImGui::EndChild();

    ImGui::SetCursorPos(ImVec2(348, 44));
    ImGui::BeginChild("Misc", ImVec2(324, 437), true);

    ImGui::Text("Global Checks");
    if (ImGui::Checkbox("Team Check", &storage::visuals::teamcheck)) {
      NotificationManager::getInstance().push_notif(storage::visuals::teamcheck ? "Enabled Team Check" : "Disabled Team Check");
    }
    ImGui::SameLine();
    if (ImGui::Checkbox("Bot Check", &globals::botcheck)) {
      NotificationManager::getInstance().push_notif(globals::botcheck ? "Enabled Bot Check" : "Disabled Bot Check");
    }
    if (globals::botcheck) {
      const char* botmodes[] = { "Players Only", "Bots/NPCs Only", "Both" };
      ImGui::Combo("Target Mode", &globals::botmode, botmodes, 3);
    }
    if (ImGui::Checkbox("Death Check", &storage::visuals::death_check)) {
      NotificationManager::getInstance().push_notif(storage::visuals::death_check ? "Enabled Death Check" : "Disabled Death Check");
    }
    if (ImGui::Checkbox("Allow Local", &storage::visuals::allow_local_player)) {
      NotificationManager::getInstance().push_notif(storage::visuals::allow_local_player ? "Enabled Allow Local" : "Disabled Allow Local");
    }

    ImGui::EndChild();
    break;
  }
  case 3: {
    ImGui::SetCursorPos(ImVec2(17, 44));
    ImGui::BeginChild("Player List", ImVec2(324, 437), true);

    for (const auto &player : storage::player_cache) {
      ImGui::PushID((int)player.address);
      bool is_selected = storage::selected_player == (int)player.address;

      int status = globals::GetPlayerStatus(player.name);
      if (status == 1) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 1.0f, 0.4f, 1.0f));
      } else if (status == 2) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.4f, 0.4f, 1.0f));
      } else {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
      }

      if (ImGui::Selectable(player.name.c_str(), is_selected)) {
        storage::selected_player = (int)player.address;
      }
      ImGui::PopStyleColor();
      ImGui::PopID();
    }

    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild("Player Actions", ImVec2(324, 437), true);
    ImGui::Text("Player Actions");
    ImGui::Separator();

    Engine::PlayerIns *selected = nullptr;
    for (auto &p : storage::player_cache) {
      if ((int)p.address == storage::selected_player) {
        selected = &p;
        break;
      }
    }

    if (selected && storage::selected_player != 0) {
      ImGui::Text("Selected: %s", selected->name.c_str());
      ImGui::Spacing();

      ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
      ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
      ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
      ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                            ImVec4(1.0f, 1.0f, 1.0f, 0.05f));
      ImGui::PushStyleColor(ImGuiCol_ButtonActive,
                            ImVec4(1.0f, 1.0f, 1.0f, 0.1f));
      ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));

      if (ImGui::Button("Teleport To", ImVec2(-1, 28))) {
        if (selected->rootPart.address) {
          Engine::Vector3 pos = selected->rootPart.GetPartPos();
          Engine::TeleportTo(pos);
        }
      }
      {
        ImVec2 item_min = ImGui::GetItemRectMin();
        ImVec2 item_max = ImGui::GetItemRectMax();
        ImDrawList *dl = ImGui::GetWindowDrawList();
        dl->AddRect(item_min, item_max, IM_COL32(0, 0, 0, 255));
        dl->AddRect(item_min + ImVec2(1, 1), item_max - ImVec2(1, 1),
                    IM_COL32(55, 55, 55, 255));
      }

      ImGui::Spacing();

      int current_status = globals::GetPlayerStatus(selected->name);

      if (current_status == 1) {
        if (ImGui::Button("Remove Whitelist", ImVec2(-1, 28))) {
          globals::SetPlayerStatus(selected->name, 0);
        }
      } else {
        if (ImGui::Button("Add Whitelist", ImVec2(-1, 28))) {
          globals::SetPlayerStatus(selected->name, 1);
        }
      }
      {
        ImVec2 item_min = ImGui::GetItemRectMin();
        ImVec2 item_max = ImGui::GetItemRectMax();
        ImDrawList *dl = ImGui::GetWindowDrawList();
        dl->AddRect(item_min, item_max, IM_COL32(0, 0, 0, 255));
        dl->AddRect(item_min + ImVec2(1, 1), item_max - ImVec2(1, 1),
                    IM_COL32(55, 55, 55, 255));
      }

      if (current_status == 2) {
        if (ImGui::Button("Remove Blacklist", ImVec2(-1, 28))) {
          globals::SetPlayerStatus(selected->name, 0);
        }
      } else {
        if (ImGui::Button("Add Blacklist", ImVec2(-1, 28))) {
          globals::SetPlayerStatus(selected->name, 2);
        }
      }
      {
        ImVec2 item_min = ImGui::GetItemRectMin();
        ImVec2 item_max = ImGui::GetItemRectMax();
        ImDrawList *dl = ImGui::GetWindowDrawList();
        dl->AddRect(item_min, item_max, IM_COL32(0, 0, 0, 255));
        dl->AddRect(item_min + ImVec2(1, 1), item_max - ImVec2(1, 1),
                    IM_COL32(55, 55, 55, 255));
      }

      ImGui::PopStyleColor(4);
      ImGui::PopStyleVar(2);

      ImGui::Spacing();

      ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
      ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
      ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
      ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                            ImVec4(1.0f, 1.0f, 1.0f, 0.05f));
      ImGui::PushStyleColor(ImGuiCol_ButtonActive,
                            ImVec4(1.0f, 1.0f, 1.0f, 0.1f));
      ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));

      if (ImGui::Button("Copy Name", ImVec2(-1, 28))) {
        ImGui::SetClipboardText(selected->name.c_str());
      }
      {
        ImVec2 item_min = ImGui::GetItemRectMin();
        ImVec2 item_max = ImGui::GetItemRectMax();
        ImDrawList *dl = ImGui::GetWindowDrawList();
        dl->AddRect(item_min, item_max, IM_COL32(0, 0, 0, 255));
        dl->AddRect(item_min + ImVec2(1, 1), item_max - ImVec2(1, 1),
                    IM_COL32(55, 55, 55, 255));
      }

      ImGui::PopStyleColor(4);
      ImGui::PopStyleVar(2);
    } else {
      ImGui::TextWrapped("Select a player from the list to perform actions.");
    }

    ImGui::EndChild();
    break;
  }

  case 4: {
    ImGui::SetCursorPos(ImVec2(17, 44));
    ImGui::BeginChild("Movement", ImVec2(324, 437), true);

    if (ImGui::Checkbox("Walkspeed", &globals::rage::speed_enabled)) {
      NotificationManager::getInstance().push_notif(globals::rage::speed_enabled ? "Enabled Walkspeed" : "Disabled Walkspeed");
    }
    ImGui::SameLine(ImGui::GetWindowWidth() - 50);
    helpers::HotkeyButton(globals::rage::walkspeed_bind, "Walkspeed");

    if (globals::rage::speed_enabled) {
      ImGui::SliderFloat("Speed Value", &globals::rage::walkspeed_amount, 1.0f, 100.0f);
    }

    if (ImGui::Checkbox("Flight", &storage::fly_enabled)) {
      NotificationManager::getInstance().push_notif(storage::fly_enabled ? "Enabled Flight" : "Disabled Flight");
    }
    ImGui::SameLine(ImGui::GetWindowWidth() - 50);
    helpers::HotkeyButton(storage::flightkeybind, "Flight");

    if (storage::fly_enabled) {
      ImGui::SliderFloat("Flight Speed", &storage::fly_speed, 1.0f, 500.0f);
    }

    if (ImGui::Checkbox("Jump Power", &storage::jump_power_enabled)) {
      NotificationManager::getInstance().push_notif(storage::jump_power_enabled ? "Enabled Jump Power" : "Disabled Jump Power");
    }
    ImGui::SameLine(ImGui::GetWindowWidth() - 50);
    helpers::HotkeyButton(storage::jumppowerkeybind, "Jump Power");

    if (storage::jump_power_enabled) {
      ImGui::SliderFloat("Jump Value", &storage::jumppower, 1.0f, 200.0f);
    }

    if (ImGui::Checkbox("Noclip", &storage::noclip)) {
      NotificationManager::getInstance().push_notif(storage::noclip ? "Enabled Noclip" : "Disabled Noclip");
    }
    ImGui::SameLine(ImGui::GetWindowWidth() - 50);
    helpers::HotkeyButton(storage::noclipkeybind, "Noclip");

    if (ImGui::Checkbox("No Jump Cooldown", &storage::nojumpcooldown)) {
      NotificationManager::getInstance().push_notif(storage::nojumpcooldown ? "Enabled No Jump Cooldown" : "Disabled No Jump Cooldown");
    }
    ImGui::SameLine(ImGui::GetWindowWidth() - 50);
    helpers::HotkeyButton(storage::no_jump_cooldown_keybind,
                          "No Jump Cooldown");

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    if (ImGui::Checkbox("Hitbox Expander", &storage::hitbox_expander::enabled)) {
      NotificationManager::getInstance().push_notif(storage::hitbox_expander::enabled ? "Enabled Hitbox Expander" : "Disabled Hitbox Expander");
    }
    if (storage::hitbox_expander::enabled) {
      ImGui::SliderFloat("Size X", &storage::hitbox_expander::size_x, 1.0f,
                         50.0f);
      ImGui::SliderFloat("Size Y", &storage::hitbox_expander::size_y, 1.0f,
                         50.0f);
      ImGui::SliderFloat("Size Z", &storage::hitbox_expander::size_z, 1.0f,
                         50.0f);

      if (ImGui::Checkbox("Visualize", &storage::hitbox_expander::visualize)) {
        NotificationManager::getInstance().push_notif(storage::hitbox_expander::visualize ? "Enabled Visualize" : "Disabled Visualize");
      }
      if (storage::hitbox_expander::visualize) {
        ImGui::SameLine();
        ImGui::ColorEdit4("##HitboxVisColor",
                          (float *)&storage::hitbox_expander::visualize_color,
                          ImGuiColorEditFlags_NoInputs |
                              ImGuiColorEditFlags_NoLabel);
      }
      if (ImGui::Checkbox("Can Collide", &storage::hitbox_expander::cancollide)) {
        NotificationManager::getInstance().push_notif(storage::hitbox_expander::cancollide ? "Enabled Can Collide" : "Disabled Can Collide");
      }
      if (ImGui::Checkbox("Custom Models",
                      &storage::hitbox_expander::custom_models)) {
        NotificationManager::getInstance().push_notif(storage::hitbox_expander::custom_models ? "Enabled Custom Models" : "Disabled Custom Models");
      }
    }

    ImGui::Spacing();

    if (ImGui::Checkbox("Spinbot", &storage::spinbot_enabled)) {
      NotificationManager::getInstance().push_notif(storage::spinbot_enabled ? "Enabled Spinbot" : "Disabled Spinbot");
    }
    ImGui::SameLine(ImGui::GetWindowWidth() - 50);
    helpers::HotkeyButton(storage::spinbotkeybind, "Spinbot");

    if (storage::spinbot_enabled) {
      ImGui::SliderFloat("Spin Speed", &storage::spin_speed, 1.0f, 100.0f);
    }

    if (ImGui::Checkbox("Hit Sound", &storage::hitsound)) {
      NotificationManager::getInstance().push_notif(storage::hitsound ? "Enabled Hit Sound" : "Disabled Hit Sound");
    }
    if (storage::hitsound) {
      const char *hit_sounds[] = {"Among Us", "Skeet",    "Neverlose", "Bameware",
                                  "Beep",     "Bonk",     "Bubble",    "COD",
                                  "CS:GO",    "Fairy",    "Fatality",  "OSU",
                                  "Rust"};
      ImGui::Combo("Sound Type", &storage::hitsound_type, hit_sounds,
                   IM_ARRAYSIZE(hit_sounds));
    }

    ImGui::EndChild();
    break;
  }
  case 5: {
    ImGui::SetCursorPos(ImVec2(17, 44));
    ImGui::BeginChild("Exploits", ImVec2(324, 437), true);

    if (storage::gameid == 1008451066 || storage::placeid == 2788229376) {
      if (ImGui::Checkbox("Skin Changer", &storage::skin_changer_enabled)) {
        NotificationManager::getInstance().push_notif(storage::skin_changer_enabled ? "Enabled Skin Changer" : "Disabled Skin Changer");
      }
      if (storage::skin_changer_enabled) {
        const char *skin_names[] = {
          "Valentine", "Galaxy", "Inferno", "Matrix", "Red Death",
          "Gold Glory", "Rainbow", "Battleworn Green", "Battleworn Red",
          "Danger", "Icey", "Luck", "Snow Wrap", "Christmas Wrap",
          "Biohazard", "Future", "Black & White", "Chief Keef", "Juice WRLD", "Tokyo Ghoul", "Mr Beast", "Galaxy V2", "Galaxy V3", "Scathe"
        };
        
        // Searchable combo for Double-Barrel SG
        static char search_db[128] = "";
        if (ImGui::BeginCombo("[DB-SG]", skin_names[storage::skin_changer_doublebarrel])) {
          ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
          ImGui::InputText("##db_search", search_db, IM_ARRAYSIZE(search_db));
          ImGui::PopStyleColor();
          for (int i = 0; i < IM_ARRAYSIZE(skin_names); i++) {
            if (search_db[0] != '\0') {
              std::string name_lower = skin_names[i];
              std::string search_lower = search_db;
              std::transform(name_lower.begin(), name_lower.end(), name_lower.begin(), ::tolower);
              std::transform(search_lower.begin(), search_lower.end(), search_lower.begin(), ::tolower);
              if (name_lower.find(search_lower) == std::string::npos) continue;
            }
            bool is_selected = (storage::skin_changer_doublebarrel == i);
            if (ImGui::Selectable(skin_names[i], is_selected)) {
              storage::skin_changer_doublebarrel = i;
              search_db[0] = '\0';
            }
            if (is_selected) ImGui::SetItemDefaultFocus();
          }
          ImGui::EndCombo();
        }
        
        // Searchable combo for Revolver
        static char search_rev[128] = "";
        if (ImGui::BeginCombo("[Revolver]", skin_names[storage::skin_changer_revolver])) {
          ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
          ImGui::InputText("##rev_search", search_rev, IM_ARRAYSIZE(search_rev));
          ImGui::PopStyleColor();
          for (int i = 0; i < IM_ARRAYSIZE(skin_names); i++) {
            if (search_rev[0] != '\0') {
              std::string name_lower = skin_names[i];
              std::string search_lower = search_rev;
              std::transform(name_lower.begin(), name_lower.end(), name_lower.begin(), ::tolower);
              std::transform(search_lower.begin(), search_lower.end(), search_lower.begin(), ::tolower);
              if (name_lower.find(search_lower) == std::string::npos) continue;
            }
            bool is_selected = (storage::skin_changer_revolver == i);
            if (ImGui::Selectable(skin_names[i], is_selected)) {
              storage::skin_changer_revolver = i;
              search_rev[0] = '\0';
            }
            if (is_selected) ImGui::SetItemDefaultFocus();
          }
          ImGui::EndCombo();
        }
        
        // Searchable combo for Tactical Shotgun
        static char search_tac[128] = "";
        if (ImGui::BeginCombo("[Tac-SG]", skin_names[storage::skin_changer_tacticalshotgun])) {
          ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
          ImGui::InputText("##tac_search", search_tac, IM_ARRAYSIZE(search_tac));
          ImGui::PopStyleColor();
          for (int i = 0; i < IM_ARRAYSIZE(skin_names); i++) {
            if (search_tac[0] != '\0') {
              std::string name_lower = skin_names[i];
              std::string search_lower = search_tac;
              std::transform(name_lower.begin(), name_lower.end(), name_lower.begin(), ::tolower);
              std::transform(search_lower.begin(), search_lower.end(), search_lower.begin(), ::tolower);
              if (name_lower.find(search_lower) == std::string::npos) continue;
            }
            bool is_selected = (storage::skin_changer_tacticalshotgun == i);
            if (ImGui::Selectable(skin_names[i], is_selected)) {
              storage::skin_changer_tacticalshotgun = i;
              search_tac[0] = '\0';
            }
            if (is_selected) ImGui::SetItemDefaultFocus();
          }
          ImGui::EndCombo();
        }
        
        // Searchable combo for AK47
        static char search_ak[128] = "";
        if (ImGui::BeginCombo("[AK47]", skin_names[storage::skin_changer_ak47])) {
          ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
          ImGui::InputText("##ak_search", search_ak, IM_ARRAYSIZE(search_ak));
          ImGui::PopStyleColor();
          for (int i = 0; i < IM_ARRAYSIZE(skin_names); i++) {
            if (search_ak[0] != '\0') {
              std::string name_lower = skin_names[i];
              std::string search_lower = search_ak;
              std::transform(name_lower.begin(), name_lower.end(), name_lower.begin(), ::tolower);
              std::transform(search_lower.begin(), search_lower.end(), search_lower.begin(), ::tolower);
              if (name_lower.find(search_lower) == std::string::npos) continue;
            }
            bool is_selected = (storage::skin_changer_ak47 == i);
            if (ImGui::Selectable(skin_names[i], is_selected)) {
              storage::skin_changer_ak47 = i;
              search_ak[0] = '\0';
            }
            if (is_selected) ImGui::SetItemDefaultFocus();
          }
          ImGui::EndCombo();
        }
        
        // Searchable combo for SMG
        static char search_smg[128] = "";
        if (ImGui::BeginCombo("[SMG]", skin_names[storage::skin_changer_smg])) {
          ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
          ImGui::InputText("##smg_search", search_smg, IM_ARRAYSIZE(search_smg));
          ImGui::PopStyleColor();
          for (int i = 0; i < IM_ARRAYSIZE(skin_names); i++) {
            if (search_smg[0] != '\0') {
              std::string name_lower = skin_names[i];
              std::string search_lower = search_smg;
              std::transform(name_lower.begin(), name_lower.end(), name_lower.begin(), ::tolower);
              std::transform(search_lower.begin(), search_lower.end(), search_lower.begin(), ::tolower);
              if (name_lower.find(search_lower) == std::string::npos) continue;
            }
            bool is_selected = (storage::skin_changer_smg == i);
            if (ImGui::Selectable(skin_names[i], is_selected)) {
              storage::skin_changer_smg = i;
              search_smg[0] = '\0';
            }
            if (is_selected) ImGui::SetItemDefaultFocus();
          }
          ImGui::EndCombo();
        }
        
        // Searchable combo for LMG
        static char search_lmg[128] = "";
        if (ImGui::BeginCombo("[LMG]", skin_names[storage::skin_changer_lmg])) {
          ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
          ImGui::InputText("##lmg_search", search_lmg, IM_ARRAYSIZE(search_lmg));
          ImGui::PopStyleColor();
          for (int i = 0; i < IM_ARRAYSIZE(skin_names); i++) {
            if (search_lmg[0] != '\0') {
              std::string name_lower = skin_names[i];
              std::string search_lower = search_lmg;
              std::transform(name_lower.begin(), name_lower.end(), name_lower.begin(), ::tolower);
              std::transform(search_lower.begin(), search_lower.end(), search_lower.begin(), ::tolower);
              if (name_lower.find(search_lower) == std::string::npos) continue;
            }
            bool is_selected = (storage::skin_changer_lmg == i);
            if (ImGui::Selectable(skin_names[i], is_selected)) {
              storage::skin_changer_lmg = i;
              search_lmg[0] = '\0';
            }
            if (is_selected) ImGui::SetItemDefaultFocus();
          }
          ImGui::EndCombo();
        }
      }
      ImGui::Separator();
    }

    if (storage::placeid == 286090429 || storage::gameid == 111958650) { // Arsenal
      if (ImGui::Checkbox("Arsenal Melee Changer", &storage::visuals::arsenal_skin_changer)) {
        NotificationManager::getInstance().push_notif(storage::visuals::arsenal_skin_changer ? "Enabled Arsenal Melee Changer" : "Disabled Arsenal Melee Changer");
      }
      if (storage::visuals::arsenal_skin_changer) {
        static char arsenal_search[128] = "";
        
        // Get current selected name
        const char* current_name = storage::visuals::arsenal_melees[storage::visuals::selected_melee_index].c_str();
        
        if (ImGui::BeginCombo("Melee Skin", current_name)) {
          ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
          ImGui::InputText("##arsenal_search", arsenal_search, IM_ARRAYSIZE(arsenal_search));
          ImGui::PopStyleColor();
          
          for (int i = 0; i < storage::visuals::arsenal_melees.size(); i++) {
            if (arsenal_search[0] != '\0') {
              std::string name_lower = storage::visuals::arsenal_melees[i];
              std::string search_lower = arsenal_search;
              std::transform(name_lower.begin(), name_lower.end(), name_lower.begin(), ::tolower);
              std::transform(search_lower.begin(), search_lower.end(), search_lower.begin(), ::tolower);
              if (name_lower.find(search_lower) == std::string::npos) continue;
            }
            bool is_selected = (storage::visuals::selected_melee_index == i);
            if (ImGui::Selectable(storage::visuals::arsenal_melees[i].c_str(), is_selected)) {
              storage::visuals::selected_melee_index = i;
              arsenal_search[0] = '\0';
            }
            if (is_selected) ImGui::SetItemDefaultFocus();
          }
          ImGui::EndCombo();
        }
      }
      ImGui::Separator();
    }

    if (ImGui::Checkbox("Headless", &storage::headless)) {
      NotificationManager::getInstance().push_notif(storage::headless ? "Enabled Headless" : "Disabled Headless");
    }
    if (ImGui::Checkbox("Korblox", &storage::korblox)) {
      NotificationManager::getInstance().push_notif(storage::korblox ? "Enabled Korblox" : "Disabled Korblox");
    }
    if (ImGui::Checkbox("Rapid Fire", &storage::rapidfire_enabled)) {
      NotificationManager::getInstance().push_notif(storage::rapidfire_enabled ? "Enabled Rapid Fire - Reset for rapid fire [BUGGY]" : "Disabled Rapid Fire");
    }

    ImGui::Separator();

    const char *animation_styles[] = {
        "Default",  "Zombie",     "Ninja", "Robot", "Levitation", "Stylish",
        "Cartoony", "Super Hero", "Elder", "Toy",   "Old School"};

    if (ImGui::Checkbox("Animation Changer", &storage::animation_changer)) {
      NotificationManager::getInstance().push_notif(storage::animation_changer ? "Enabled Animation Changer" : "Disabled Animation Changer");
    }
    if (storage::animation_changer) {
      ImGui::Combo("Idle", &storage::idle_animation, animation_styles,
                   IM_ARRAYSIZE(animation_styles));
      ImGui::Combo("Run", &storage::run_animation, animation_styles,
                   IM_ARRAYSIZE(animation_styles));
      ImGui::Combo("Walk", &storage::walk_animation, animation_styles,
                   IM_ARRAYSIZE(animation_styles));
      ImGui::Combo("Jump", &storage::jump_animation, animation_styles,
                   IM_ARRAYSIZE(animation_styles));
      ImGui::Combo("Fall", &storage::fall_animation, animation_styles,
                   IM_ARRAYSIZE(animation_styles));
    }

    if (ImGui::Checkbox("Emote Changer", &storage::emote_changer)) {
      NotificationManager::getInstance().push_notif(storage::emote_changer ? "Enabled Emote Changer" : "Disabled Emote Changer");
    }
    if (storage::emote_changer) {
      const char *emotes[] = {"Default", "Monkey", "Jolly",         "Griddy",
                              "Floss",   "Dab",    "T-Pose",        "Smooth",
                              "Robot",   "Hype",   "Orange Justice"};
      ImGui::Combo("Emote", &storage::emote_id, emotes, IM_ARRAYSIZE(emotes));
    }

    ImGui::EndChild();
    break;
  }
  case 6: {
    ImGui::SetCursorPos(ImVec2(17, 44));
    ImGui::BeginChild("Explorer Tab", ImVec2(665, 437), true);
    explorer::Explorer::render_embedded();
    ImGui::EndChild();
    break;
  }
  case 7: {
    ImGui::SetCursorPos(ImVec2(17, 44));
    ImGui::BeginChild("Settings", ImVec2(324, 437), true);
    {
      ImGui::Text("Other");
      ImGui::Separator();
      ImGui::Spacing();
      
      ImGui::Checkbox("Enable Notifications", &storage::notifications_enabled);
      ImGui::Checkbox("Show Keybind List", &storage::show_keybind_list);

      ImGui::Text("Menu Key");
      ImGui::SameLine(ImGui::GetWindowWidth() - 50);
      helpers::HotkeyButton(storage::menu_key_bind, "Menu Key");

      ImGui::Separator();
      ImGui::Spacing();

      if (ImGui::Checkbox("VSync", &storage::vsync)) {
        NotificationManager::getInstance().push_notif(storage::vsync ? "Enabled VSync" : "Disabled VSync");
      }
      if (ImGui::Checkbox("Streamproof", &storage::streamproof)) {
        NotificationManager::getInstance().push_notif(storage::streamproof ? "Enabled Streamproof" : "Disabled Streamproof");
      }
      if (ImGui::Checkbox("Watermark", &storage::watermark_enabled)) {
        NotificationManager::getInstance().push_notif(storage::watermark_enabled ? "Enabled Watermark" : "Disabled Watermark");
      }

      ImGui::Spacing();
      ImGui::Text("Discord RPC");
      ImGui::Separator();
      if (ImGui::Checkbox("Custom RPC", &storage::discord_rpc::enabled)) {
        NotificationManager::getInstance().push_notif(storage::discord_rpc::enabled ? "Enabled Custom RPC" : "Disabled Custom RPC");
      }

      ImGui::EndChild();
    }

    ImGui::SetCursorPos(ImVec2(349, 44));
    ImGui::BeginChild("Manager", ImVec2(324, 437), true);
    {
      float avail_width = ImGui::GetContentRegionAvail().x;
      float margin = 14.0f;

      ImGui::SetCursorPosX(margin);
      ImGui::Text("Configuration");
      ImGui::Separator();
      ImGui::Spacing();

      float input_w = 230.0f;
      float btn_create_w = 70.0f;
      float group_w = input_w + 4.0f + btn_create_w;
      float group_offset = (avail_width - group_w) * 0.5f;

      ImGui::SetCursorPosX(group_offset);
      static char config_name[64] = "";

      ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
      ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 6));
      ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
      ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
      ImGui::PushStyleColor(ImGuiCol_FrameBgHovered,
                             ImVec4(1.0f, 1.0f, 1.0f, 0.05f));
      ImGui::PushStyleColor(ImGuiCol_FrameBgActive,
                             ImVec4(1.0f, 1.0f, 1.0f, 0.1f));
      ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));

      ImGui::SetNextItemWidth(input_w);
      ImGui::InputText("##ConfigName", config_name, IM_ARRAYSIZE(config_name));

      ImVec2 item_min = ImGui::GetItemRectMin();
      ImVec2 item_max = ImGui::GetItemRectMax();
      ImDrawList *dl = ImGui::GetWindowDrawList();
      dl->AddRect(item_min, item_max, IM_COL32(0, 0, 0, 255));
      dl->AddRect(item_min + ImVec2(1, 1), item_max - ImVec2(1, 1),
                   IM_COL32(55, 55, 55, 255));

      ImGui::PopStyleColor(4);
      ImGui::PopStyleVar(3);

      ImGui::SameLine(0.0f, 4.0f);

      ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
      ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 6));
      ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
      ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
      ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                             ImVec4(1.0f, 1.0f, 1.0f, 0.05f));
      ImGui::PushStyleColor(ImGuiCol_ButtonActive,
                             ImVec4(1.0f, 1.0f, 1.0f, 0.1f));
      ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));

      if (ImGui::Button("Create", ImVec2(btn_create_w, 0))) {
        if (strlen(config_name) > 0) {
          storage::config_name = config_name;
          storage::save_config();
          storage::update_config_list();
        }
      }

      item_min = ImGui::GetItemRectMin();
      item_max = ImGui::GetItemRectMax();
      dl->AddRect(item_min, item_max, IM_COL32(0, 0, 0, 255));
      dl->AddRect(item_min + ImVec2(1, 1), item_max - ImVec2(1, 1),
                   IM_COL32(55, 55, 55, 255));

      ImGui::PopStyleColor(4);
      ImGui::PopStyleVar(3);

      ImGui::Spacing();
      ImGui::Separator();
      ImGui::Spacing();

      ImGui::SetCursorPosX(margin);
      ImGui::Text("Saved Profiles:");

      ImGui::PushStyleColor(ImGuiCol_ChildBg,
                             ImVec4(0.06f, 0.06f, 0.08f, 1.0f));
      ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));
      ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0.0f);
      ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 0.0f);

      ImGui::SetCursorPosX(margin);
      ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(137 / 255.f, 207 / 255.f, 240 / 255.f, 0.2f));
      ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(137 / 255.f, 207 / 255.f, 240 / 255.f, 0.4f));
      ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(137 / 255.f, 207 / 255.f, 240 / 255.f, 0.6f));
      
      if (ImGui::BeginChild("Configslist", ImVec2(avail_width - (margin * 2.0f), 180), false)) {
          if (ImGui::BeginTable("##ConfigsTable", 2, ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg | ImGuiTableFlags_NoBordersInBody)) {
              ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
              ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_WidthFixed, 80.0f);
              ImGui::TableHeadersRow();

              for (const auto &cfg : storage::config_list) {
                  ImGui::TableNextRow();
                  ImGui::TableNextColumn();
                  bool is_selected = (storage::config_name == cfg);
                  if (ImGui::Selectable(cfg.c_str(), is_selected, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap)) {
                      storage::config_name = cfg;
                  }
                  
                  ImGui::TableNextColumn();
                  if (is_selected) {
                      ImGui::TextColored(ImVec4(137/255.f, 207/255.f, 240/255.f, 1.0f), "Active");
                  } else {
                      ImGui::TextDisabled("Idle");
                  }
              }
              ImGui::EndTable();
          }
      }
      ImGui::EndChild();
      ImGui::PopStyleColor(3);

      item_min = ImGui::GetItemRectMin();
      item_max = ImGui::GetItemRectMax();
      dl->AddRect(item_min, item_max, IM_COL32(0, 0, 0, 255));
      dl->AddRect(item_min + ImVec2(1, 1), item_max - ImVec2(1, 1),
                   IM_COL32(55, 55, 55, 255));

      ImGui::PopStyleVar(2);
      ImGui::PopStyleColor(2);

      ImGui::Spacing();

      ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
      ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 6));
      ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
      ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
      ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                             ImVec4(1.0f, 1.0f, 1.0f, 0.05f));
      ImGui::PushStyleColor(ImGuiCol_ButtonActive,
                             ImVec4(1.0f, 1.0f, 1.0f, 0.1f));
      ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));

      float full_width_avail = ImGui::GetContentRegionAvail().x;
      float action_btn_group_w = 300.0f;
      float action_btn_w = (action_btn_group_w - 8.0f) / 3.0f;
      float action_offset = (avail_width - action_btn_group_w) * 0.5f;

      ImGui::SetCursorPosX(action_offset);

      if (ImGui::Button("Load", ImVec2(action_btn_w, 0))) {
        storage::load_config();
      }
      item_min = ImGui::GetItemRectMin();
      item_max = ImGui::GetItemRectMax();
      dl->AddRect(item_min, item_max, IM_COL32(0, 0, 0, 255));
      dl->AddRect(item_min + ImVec2(1, 1), item_max - ImVec2(1, 1),
                   IM_COL32(55, 55, 55, 255));

      ImGui::SameLine(0.0f, 4.0f);
      if (ImGui::Button("Save", ImVec2(action_btn_w, 0))) {
        storage::save_config();
      }
      item_min = ImGui::GetItemRectMin();
      item_max = ImGui::GetItemRectMax();
      dl->AddRect(item_min, item_max, IM_COL32(0, 0, 0, 255));
      dl->AddRect(item_min + ImVec2(1, 1), item_max - ImVec2(1, 1),
                   IM_COL32(55, 55, 55, 255));

      ImGui::SameLine(0.0f, 4.0f);
      if (ImGui::Button("Delete", ImVec2(action_btn_w, 0))) {
        storage::delete_config();
      }
      item_min = ImGui::GetItemRectMin();
      item_max = ImGui::GetItemRectMax();
      dl->AddRect(item_min, item_max, IM_COL32(0, 0, 0, 255));
      dl->AddRect(item_min + ImVec2(1, 1), item_max - ImVec2(1, 1),
                   IM_COL32(55, 55, 55, 255));

      float refresh_w = 160.0f;
      ImGui::SetCursorPosX((avail_width - refresh_w) * 0.5f);
      if (ImGui::Button("Refresh Files", ImVec2(refresh_w, 0))) {
        storage::update_config_list();
      }
      item_min = ImGui::GetItemRectMin();
      item_max = ImGui::GetItemRectMax();
      dl->AddRect(item_min, item_max, IM_COL32(0, 0, 0, 255));
      dl->AddRect(item_min + ImVec2(1, 1), item_max - ImVec2(1, 1),
                   IM_COL32(55, 55, 55, 255));

      ImGui::Dummy(ImVec2(0, 25.0f));

      ImGui::SetCursorPosX((avail_width - refresh_w) * 0.5f);
      if (ImGui::Button("Eject", ImVec2(refresh_w, 0))) {
        shouldExit = true;
      }
      item_min = ImGui::GetItemRectMin();
      item_max = ImGui::GetItemRectMax();
      dl->AddRect(item_min, item_max, IM_COL32(0, 0, 0, 255));
      dl->AddRect(item_min + ImVec2(1, 1), item_max - ImVec2(1, 1),
                   IM_COL32(55, 55, 55, 255));

      ImGui::PopStyleVar(3);
      ImGui::PopStyleColor(4);

      ImGui::EndChild();
    }
    break;
  }
  }
}
