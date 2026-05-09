#pragma once
#include "core/Core.hpp"
#include "imgui.h"
#include <string>
#include <unordered_map>
#include <vector>

namespace explorer {
class Explorer {
public:
  static void render();
  static void render_embedded();
  static void initialize();
  static void cleanup();

private:
  static bool first_time;
  static Engine::Instance selected_instance;
  static std::string selected_instance_path;

  static void recursive_draw(Engine::Instance instance, std::string parent_path,
                             std::string display_path);
  static bool is_valid(const std::string &str);
};
} // namespace explorer
