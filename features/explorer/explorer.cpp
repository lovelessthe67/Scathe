#include "explorer.h"
#include "core/storage/Globals.hpp"
#include "imgui.h"
#include <Windows.h>
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <unordered_map>

namespace explorer {

bool Explorer::first_time = true;
Engine::Instance Explorer::selected_instance{};
std::string Explorer::selected_instance_path = "";

void Explorer::render() {
  if (first_time) {
    initialize();
  }

  static bool first_open = true;
  static bool window_open = true;
  static bool was_enabled = false;

  if (storage::dex_explorer_enabled && !was_enabled) {
    window_open = true;
    first_open = true;
  }
  was_enabled = storage::dex_explorer_enabled;

  if (first_open && window_open) {
    ImVec2 main_viewport_size = ImGui::GetMainViewport()->Size;
    ImVec2 explorer_size = ImVec2(900, 600);
    float pos_x = (main_viewport_size.x - explorer_size.x) * 0.5f;
    float pos_y = (main_viewport_size.y - explorer_size.y) * 0.5f;
    if (pos_x < 10.0f)
      pos_x = 10.0f;
    if (pos_y < 10.0f)
      pos_y = 10.0f;
    ImVec2 explorer_pos = ImVec2(pos_x, pos_y);
    ImGui::SetNextWindowPos(explorer_pos, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(explorer_size, ImGuiCond_FirstUseEver);
    first_open = false;
  }

  ImGui::SetNextWindowSize(ImVec2(900, 600), ImGuiCond_Always);
  if (!window_open) {
    first_open = true;
  }
  ImGui::PushStyleColor(
      ImGuiCol_WindowBg,
      ImVec4(20.0f / 255.0f, 20.0f / 255.0f, 24.0f / 255.0f, 0.95f));
  ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(23.0f / 255.0f, 22.0f / 255.0f,
                                                 27.0f / 255.0f, 1.0f));
  ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(30.0f / 255.0f, 29.0f / 255.0f,
                                                37.0f / 255.0f, 1.0f));
  ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(255.0f / 255.0f, 255.0f / 255.0f,
                                              255.0f / 255.0f, 1.0f));
  ImGui::PushStyleColor(
      ImGuiCol_TextDisabled,
      ImVec4(114.0f / 255.0f, 113.0f / 255.0f, 138.0f / 255.0f, 1.0f));
  ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(23.0f / 255.0f, 22.0f / 255.0f,
                                                27.0f / 255.0f, 1.0f));
  ImGui::PushStyleColor(
      ImGuiCol_ButtonHovered,
      ImVec4(35.0f / 255.0f, 34.0f / 255.0f, 42.0f / 255.0f, 1.0f));
  ImGui::PushStyleColor(
      ImGuiCol_ButtonActive,
      ImVec4(52.0f / 255.0f, 52.0f / 255.0f, 74.0f / 255.0f, 1.0f));
  ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(35.0f / 255.0f, 34.0f / 255.0f,
                                                42.0f / 255.0f, 1.0f));
  ImGui::PushStyleColor(
      ImGuiCol_HeaderHovered,
      ImVec4(52.0f / 255.0f, 52.0f / 255.0f, 74.0f / 255.0f, 1.0f));
  ImGui::PushStyleColor(
      ImGuiCol_HeaderActive,
      ImVec4(255.0f / 255.0f, 215.0f / 255.0f, 0.0f / 255.0f, 0.3f));
  ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(23.0f / 255.0f, 22.0f / 255.0f,
                                                 27.0f / 255.0f, 1.0f));
  ImGui::PushStyleColor(
      ImGuiCol_FrameBgHovered,
      ImVec4(35.0f / 255.0f, 34.0f / 255.0f, 42.0f / 255.0f, 1.0f));
  ImGui::PushStyleColor(
      ImGuiCol_FrameBgActive,
      ImVec4(52.0f / 255.0f, 52.0f / 255.0f, 74.0f / 255.0f, 1.0f));
  ImGui::PushStyleColor(
      ImGuiCol_Separator,
      ImVec4(30.0f / 255.0f, 29.0f / 255.0f, 37.0f / 255.0f, 1.0f));
  ImGui::PushStyleColor(
      ImGuiCol_ScrollbarBg,
      ImVec4(20.0f / 255.0f, 20.0f / 255.0f, 24.0f / 255.0f, 1.0f));
  ImGui::PushStyleColor(
      ImGuiCol_ScrollbarGrab,
      ImVec4(35.0f / 255.0f, 34.0f / 255.0f, 42.0f / 255.0f, 1.0f));
  ImGui::PushStyleColor(
      ImGuiCol_ScrollbarGrabHovered,
      ImVec4(52.0f / 255.0f, 52.0f / 255.0f, 74.0f / 255.0f, 1.0f));
  ImGui::PushStyleColor(
      ImGuiCol_ScrollbarGrabActive,
      ImVec4(255.0f / 255.0f, 215.0f / 255.0f, 0.0f / 255.0f, 1.0f));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(15, 15));
  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10, 6));
  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 4));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 4.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 4.0f);

  if (ImGui::Begin("DEX Explorer", &window_open,
                   ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize)) {
    render_embedded();
  }
  ImGui::End();
  ImGui::PopStyleVar(7);
  ImGui::PopStyleColor(19);

  if (!window_open) {
    storage::dex_explorer_enabled = false;
  }
}

void Explorer::render_embedded() {
  float split_total_w = ImGui::GetContentRegionAvail().x;
  float split_total_h = ImGui::GetContentRegionAvail().y;
  float gap = 15.0f;
  float avail_no_gap = split_total_w - gap;
  float left_w = floorf(avail_no_gap * 0.55f);
  float right_w = floorf(avail_no_gap - left_w);

  ImGui::BeginChild("Explorer Tree", ImVec2(left_w, split_total_h), true,
                    ImGuiWindowFlags_HorizontalScrollbar);
  {
    ImGui::Spacing();

    if (storage::datamodel.address != 0) {
      std::string dm_name = storage::datamodel.GetName();
      std::string dm_class = storage::datamodel.GetClass();
      std::string dm_display = dm_name;
      if (!dm_class.empty() && _stricmp(dm_class.c_str(), "unknown") != 0) {
        dm_display += " [" + dm_class + "]";
      }

      ImGui::PushID((void *)storage::datamodel.address);
      ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow |
                                 ImGuiTreeNodeFlags_SpanAvailWidth |
                                 ImGuiTreeNodeFlags_DefaultOpen;

      bool open = ImGui::TreeNodeEx(dm_display.c_str(), flags);
      if (ImGui::IsItemClicked()) {
        selected_instance = storage::datamodel;
        selected_instance_path = dm_name;
      }

      if (open) {
        auto children = storage::datamodel.GetChildren();
        std::string root_path = dm_name;
        for (const auto &child : children) {
          recursive_draw(child, "game:GetService(\"", root_path);
        }
        ImGui::TreePop();
      }
      ImGui::PopID();
    } else {
      ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f),
                         "Datamodel not found!");
    }
  }
  ImGui::EndChild();

  ImGui::SameLine(0.0f, gap);

  ImGui::BeginChild("Inspector", ImVec2(right_w, split_total_h), true,
                    ImGuiWindowFlags_HorizontalScrollbar);
  {
    ImGui::Spacing();

    if (selected_instance.address != 0) {
      std::string name = selected_instance.GetName();
      std::string class_name = selected_instance.GetClass();

      ImGui::Text("Name: %s", name.c_str());
      ImGui::Text("ClassName: %s", class_name.c_str());
      ImGui::Text("Address: 0x%llX",
                  static_cast<unsigned long long>(selected_instance.address));

      ImGui::Spacing();
      
      // Style buttons to match config list style
      ImDrawList* dl = ImGui::GetWindowDrawList();
      ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
      ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 6));
      ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
      ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
      ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 1.0f, 1.0f, 0.05f));
      ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1.0f, 1.0f, 1.0f, 0.1f));
      ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));
      
      if (ImGui::Button("Copy Address", ImVec2(120, 0))) {
        char buf[32];
        sprintf_s(buf, "0x%llX",
                  static_cast<unsigned long long>(selected_instance.address));
        ImGui::SetClipboardText(buf);
      }
      // Double-layered border
      ImVec2 item_min = ImGui::GetItemRectMin();
      ImVec2 item_max = ImGui::GetItemRectMax();
      dl->AddRect(item_min, item_max, IM_COL32(0, 0, 0, 255));
      dl->AddRect(item_min + ImVec2(1, 1), item_max - ImVec2(1, 1), IM_COL32(55, 55, 55, 255));
      
      ImGui::SameLine();
      if (ImGui::Button("Copy Path", ImVec2(120, 0))) {
        if (!selected_instance_path.empty()) {
          ImGui::SetClipboardText(selected_instance_path.c_str());
        }
      }
      // Double-layered border
      item_min = ImGui::GetItemRectMin();
      item_max = ImGui::GetItemRectMax();
      dl->AddRect(item_min, item_max, IM_COL32(0, 0, 0, 255));
      dl->AddRect(item_min + ImVec2(1, 1), item_max - ImVec2(1, 1), IM_COL32(55, 55, 55, 255));
      
      ImGui::PopStyleColor(4);
      ImGui::PopStyleVar(3);

      ImGui::Spacing();
      ImGui::Separator();
      ImGui::Spacing();

      std::string cname = class_name;
      bool is_int_value = (_stricmp(cname.c_str(), "IntValue") == 0);
      bool is_number_value = (_stricmp(cname.c_str(), "NumberValue") == 0);

      if (is_int_value || is_number_value) {
        ImGui::Text("Value Editor");
        ImGui::Spacing();

        static std::unordered_map<uint64_t, int> int_values;
        static std::unordered_map<uint64_t, float> float_values;
        static uint64_t last_selected_address = 0;

        if (last_selected_address != selected_instance.address) {
          last_selected_address = selected_instance.address;
          if (is_int_value) {
            int_values[selected_instance.address] =
                selected_instance.getIntFromValue();
          } else if (is_number_value) {
            float_values[selected_instance.address] =
                (float)selected_instance.DoubleValue();
          }
        }

        if (is_int_value) {
          int &int_val = int_values[selected_instance.address];

          ImGui::InputInt("Set Int Value", &int_val);
          ImGui::SameLine();
          
          // Style Apply button
          ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
          ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 6));
          ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
          ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
          ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 1.0f, 1.0f, 0.05f));
          ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1.0f, 1.0f, 1.0f, 0.1f));
          ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));
          
          if (ImGui::Button("Apply##int")) {
            selected_instance.SetIntValue(int_val);
          }
          item_min = ImGui::GetItemRectMin();
          item_max = ImGui::GetItemRectMax();
          dl->AddRect(item_min, item_max, IM_COL32(0, 0, 0, 255));
          dl->AddRect(item_min + ImVec2(1, 1), item_max - ImVec2(1, 1), IM_COL32(55, 55, 55, 255));
          
          ImGui::PopStyleColor(4);
          ImGui::PopStyleVar(3);
        } else if (is_number_value) {
          float &float_val = float_values[selected_instance.address];

          ImGui::InputFloat("Set Number Value", &float_val);
          ImGui::SameLine();
          
          // Style Apply button
          ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
          ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 6));
          ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
          ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
          ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 1.0f, 1.0f, 0.05f));
          ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1.0f, 1.0f, 1.0f, 0.1f));
          ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));
          
          if (ImGui::Button("Apply##float")) {
            selected_instance.writedoublevalue((double)float_val);
          }
          item_min = ImGui::GetItemRectMin();
          item_max = ImGui::GetItemRectMax();
          dl->AddRect(item_min, item_max, IM_COL32(0, 0, 0, 255));
          dl->AddRect(item_min + ImVec2(1, 1), item_max - ImVec2(1, 1), IM_COL32(55, 55, 55, 255));
          
          ImGui::PopStyleColor(4);
          ImGui::PopStyleVar(3);
        }
      } else {
        ImGui::TextDisabled("Select an IntValue or NumberValue to edit.");
      }
    } else {
      ImGui::TextDisabled("Select an instance on the left.");
    }
  }
  ImGui::EndChild();
}

void Explorer::initialize() { first_time = false; }

void Explorer::cleanup() {}

void Explorer::recursive_draw(Engine::Instance instance,
                              std::string parent_path,
                              std::string display_path) {
  if (instance.address == 0)
    return;

  std::string instance_path;
  std::string instance_name = instance.GetName();
  std::string instance_class = instance.GetClass();

  std::string current_display_path = display_path;
  if (!current_display_path.empty()) {
    current_display_path += " -> " + instance_name;
  } else {
    current_display_path = instance_name;
  }

  if (parent_path == "NULL") {
    instance_path = "game:GetService(\"";
  } else {
    instance_path = (parent_path == "game:GetService(\"")
                        ? parent_path + instance_name + "\")"
                    : (!is_valid(instance_name))
                        ? parent_path + "[\"" + instance_name + "\"]"
                        : parent_path + "." + instance_name;
  }

  auto children = instance.GetChildren();

  std::string display_text = instance_name;
  if (!instance_class.empty() &&
      _stricmp(instance_class.c_str(), "unknown") != 0) {
    display_text += " [" + instance_class + "]";
  }

  ImGui::PushID((void *)instance.address);

  if (!children.empty()) {
    ImGuiTreeNodeFlags flags =
        ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
    if (selected_instance.address == instance.address) {
    }
    bool open = ImGui::TreeNodeEx(display_text.c_str(), flags);
    if (ImGui::IsItemClicked()) {
      selected_instance = instance;
      selected_instance_path = current_display_path;
    }
    if (selected_instance.address == instance.address) {
    }
    if (open) {
      std::string popup_id =
          "Context Menu##" + std::to_string(instance.address);
      if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(1)) {
        ImGui::OpenPopup(popup_id.c_str());
      }

      if (ImGui::BeginPopup(popup_id.c_str())) {
        if (ImGui::MenuItem("Copy Path")) {
          ImGui::SetClipboardText(instance_path.c_str());
        }
        if (ImGui::MenuItem("Copy Address")) {
          char buf[32];
          sprintf_s(buf, "0x%llX",
                    static_cast<unsigned long long>(instance.address));
          ImGui::SetClipboardText(buf);
        }
        ImGui::EndPopup();
      }

      for (const auto &child : children) {
        recursive_draw(child, instance_path, current_display_path);
      }
      ImGui::TreePop();
    }
  } else {

    ImGui::Selectable(display_text.c_str(),
                      selected_instance.address == instance.address);
    if (ImGui::IsItemClicked()) {
      selected_instance = instance;
      selected_instance_path = current_display_path;
    }


    if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(1)) {
      std::string popup_id =
          "Context Menu##" + std::to_string(instance.address);
      ImGui::OpenPopup(popup_id.c_str());
    }

    std::string popup_id = "Context Menu##" + std::to_string(instance.address);
    if (ImGui::BeginPopup(popup_id.c_str())) {
      if (ImGui::MenuItem("Copy Path")) {
        ImGui::SetClipboardText(instance_path.c_str());
      }
      if (ImGui::MenuItem("Copy Address")) {
        char buf[32];
        sprintf_s(buf, "0x%llX",
                  static_cast<unsigned long long>(instance.address));
        ImGui::SetClipboardText(buf);
      }
      ImGui::EndPopup();
    }
  }

  ImGui::PopID();
}

bool Explorer::is_valid(const std::string &str) {
  if (str.empty())
    return false;

  bool starts_with_number = std::isdigit(str[0]);
  bool contains_symbol = false;

  for (char ch : str) {
    if (!std::isalnum(ch) && ch != '_') {
      contains_symbol = true;
      break;
    }
  }

  return starts_with_number || contains_symbol;
}

} // namespace explorer
