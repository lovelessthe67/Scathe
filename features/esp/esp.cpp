#include "esp.h"
#include "engine/Players/offsets.hpp"
#include "engine/mem_module/memory.hpp"
#include "core/storage/include.h"
#include "GetWeaponIcon.h"
#include "crosshair.h"
#include "bullet_tracers.h"
#include <Windows.h>
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <map>
#include <mutex>
#include <unordered_map>
#include <vector>

#include <clipper2/clipper.core.h>
#include <clipper2/clipper.h>

#define M_PI 3.14159265358979323846

bool IsPlayerDeadOrKO(const Engine::PlayerIns &player) {
  if (player.health <= 0.0f)
    return true;

  if (player.knockedOut.address && player.knockedOut.getBoolFromValue())
    return true;

  return false;
}

void DrawPolygonalFOV(ImDrawList *draw, ImVec2 center, float radius,
                      ImU32 color, float rotation = 0.0f) {
  const int segments = 12;
  const float angle_step = 2.0f * M_PI / segments;

  ImVec2 vertices[segments + 1];
  for (int i = 0; i < segments; i++) {
    float angle = i * angle_step + rotation;
    vertices[i] = ImVec2(std::round(center.x + radius * cosf(angle)),
                         std::round(center.y + radius * sinf(angle)));
  }
  vertices[segments] = vertices[0];

  for (int i = 0; i < segments; i++) {
    draw->AddLine(vertices[i], vertices[i + 1], IM_COL32(0, 0, 0, 255), 4.0f);
    draw->AddLine(vertices[i], vertices[i + 1], color, 2.0f);
  }
}

namespace MathUtils {
inline Engine::Vector3 CrossProduct(const Engine::Vector3 &a,
                                    const Engine::Vector3 &b) {
  return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

inline Engine::Vector3 Normalize(const Engine::Vector3 &v) {
  float len = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
  return len > 0.0f ? Engine::Vector3{v.x / len, v.y / len, v.z / len} : v;
}

inline Engine::Vector3 Add(const Engine::Vector3 &a, const Engine::Vector3 &b) {
  return {a.x + b.x, a.y + b.y, a.z + b.z};
}

inline Engine::Vector3 Sub(const Engine::Vector3 &a, const Engine::Vector3 &b) {
  return {a.x - b.x, a.y - b.y, a.z - b.z};
}

inline float Magnitude(const Engine::Vector3 &v) {
  return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
}

inline bool IsOnScreen(const Engine::Vector2 &point,
                       const Engine::Vector2 &screenSize) {
  return point.x >= 0.0f && point.x <= screenSize.x && point.y >= 0.0f &&
         point.y <= screenSize.y && point.x != -1.0f && point.y != -1.0f;
}
} // namespace MathUtils

static std::vector<Engine::Vector3>
GetCorners(const Engine::Vector3 &partCF, const Engine::Vector3 &partSize) {
  std::vector<Engine::Vector3> corners;
  for (int X = -1; X <= 1; X += 2) {
    for (int Y = -1; Y <= 1; Y += 2) {
      for (int Z = -1; Z <= 1; Z += 2) {
        Engine::Vector3 cornerPosition = {partCF.x + partSize.x * X,
                                          partCF.y + partSize.y * Y,
                                          partCF.z + partSize.z * Z};
        corners.push_back(cornerPosition);
      }
    }
  }
  return corners;
}

void draw_sonar_esp(ImDrawList *draw) {
  if (!globals::sonar_enabled)
    return;

  if (!storage::localplayer.address)
    return;

  ImDrawList *draw_list = draw;
  if (!draw_list)
    return;

  Engine::Vector2 dimensions = storage::visualengine.GetDimensions();
  if (dimensions.x <= 0 || dimensions.y <= 0)
    return;

  Engine::Vector3 humanoid_pos = storage::localplayer.rootPart.GetPartPos();

  Engine::Instance camera = storage::camera;
  Engine::Matrix3x3 camera_rotation = camera.GetCameraRotation();
  Engine::Vector3 camera_forward = camera_rotation.GetForwardVector();
  camera_forward = camera_forward * -1.0f;
  float camera_yaw = atan2(camera_forward.z, camera_forward.x);

  static auto start_time = std::chrono::steady_clock::now();
  auto current_time = std::chrono::steady_clock::now();
  auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                     current_time - start_time)
                     .count();
  float time_seconds = elapsed / 1000.0f;

  static std::map<std::string, float> player_dot_times;
  static float last_scan_cycle = 0.0f;

  float scan_cycle = fmod(time_seconds * 0.2f, 1.0f);

  if (scan_cycle < last_scan_cycle && scan_cycle < 0.1f) {
    player_dot_times.clear();
  }

  last_scan_cycle = scan_cycle;

  float expansion_progress = fmod(time_seconds * 0.2f, 1.0f);
  float current_radius = expansion_progress * globals::sonar_range;

  float circle_alpha = 0.9f * (1.0f - expansion_progress);

  ImU32 circle_color =
      IM_COL32((int)(globals::sonar_color[0] * 255),
               (int)(globals::sonar_color[1] * 255),
               (int)(globals::sonar_color[2] * 255),
               (int)(globals::sonar_color[3] * circle_alpha * 255));

  const int num_segments = 64;
  std::vector<ImVec2> screen_points;
  screen_points.reserve(num_segments + 1);

  Engine::Vector2 center_screen = Engine::WorldToScreen(
      humanoid_pos, dimensions, storage::visualengine.GetViewMatrix());
  if (!std::isfinite(center_screen.x) || !std::isfinite(center_screen.y)) {
    return;
  }

  for (int i = 0; i <= num_segments; i++) {
    float angle = (2.0f * M_PI * i) / num_segments;

    Engine::Vector3 circle_point_3d(
        humanoid_pos.x + cos(angle) * current_radius, humanoid_pos.y,
        humanoid_pos.z + sin(angle) * current_radius);

    Engine::Vector2 screen_point = Engine::WorldToScreen(
        circle_point_3d, dimensions, storage::visualengine.GetViewMatrix());

    if (screen_point.x == -1 && screen_point.y == -1) {
      continue;
    }

    if (std::isfinite(screen_point.x) && std::isfinite(screen_point.y) &&
        screen_point.x > -1000 && screen_point.x < dimensions.x + 1000 &&
        screen_point.y > -1000 && screen_point.y < dimensions.y + 1000) {
      screen_points.push_back(ImVec2(screen_point.x, screen_point.y));
    }
  }
  if (screen_points.size() >= 8) {
    for (size_t i = 0; i < screen_points.size() - 1; i++) {
      float dx = screen_points[i + 1].x - screen_points[i].x;
      float dy = screen_points[i + 1].y - screen_points[i].y;
      float distance = sqrtf(dx * dx + dy * dy);

      if (distance < 500.0f) {
        draw_list->AddLine(screen_points[i], screen_points[i + 1], circle_color,
                           globals::sonar_thickness);
      }
    }
    if (screen_points.size() > 2) {
      float dx = screen_points.front().x - screen_points.back().x;
      float dy = screen_points.front().y - screen_points.back().y;
      float distance = sqrtf(dx * dx + dy * dy);

      if (distance < 500.0f) {
        draw_list->AddLine(screen_points.back(), screen_points.front(),
                           circle_color, globals::sonar_thickness);
      }
    }
  }
  std::vector<Engine::PlayerIns> players_snapshot;
  std::vector<Engine::PlayerIns> custom_snapshot;
  {
    std::lock_guard<std::mutex> lg(storage::cached_players_mutex);
    players_snapshot = storage::player_cache;
  }
  {
    std::lock_guard<std::mutex> lg(storage::custom_players_mutex);
    custom_snapshot = storage::custom_player_cache;
  }

  for (auto &player : players_snapshot) {
    if (!player.address || player.address == storage::localplayer.address)
      continue;

    Engine::Vector3 player_pos = player.rootPart.GetPartPos();
    Engine::Vector3 delta = player_pos - humanoid_pos;
    float distance = MathUtils::Magnitude(delta);

    if (distance > globals::sonar_range)
      continue;
    float wave_distance = current_radius;
    float distance_to_wave = abs(distance - wave_distance);

    if (distance_to_wave <= 25.0f) {
      Engine::Vector2 screen_pos = Engine::WorldToScreen(
          player_pos, dimensions, storage::visualengine.GetViewMatrix());
      if (screen_pos.x > 0 && screen_pos.x < dimensions.x && screen_pos.y > 0 &&
          screen_pos.y < dimensions.y) {

        if (player_dot_times.find(player.name) == player_dot_times.end()) {
          player_dot_times[player.name] = time_seconds;
        }

        float fade_factor = 1.0f - (distance_to_wave / 25.0f);
        fade_factor = std::max(0.0f, std::min(1.0f, fade_factor));

        float dot_alpha = 0.9f * fade_factor;

        ImVec2 s_pos = ImVec2(std::round(screen_pos.x), std::round(screen_pos.y));

        ImU32 glow_color = IM_COL32(
            (int)(globals::sonar_dot_color[0] * 255),
            (int)(globals::sonar_dot_color[1] * 255),
            (int)(globals::sonar_dot_color[2] * 255),
            (int)((globals::sonar_dot_color[3] * dot_alpha) * 0.2f * 255));
        draw_list->AddCircleFilled(s_pos, 4.0f, glow_color);

        ImU32 middle_glow = IM_COL32(
            (int)(globals::sonar_dot_color[0] * 255),
            (int)(globals::sonar_dot_color[1] * 255),
            (int)(globals::sonar_dot_color[2] * 255),
            (int)((globals::sonar_dot_color[3] * dot_alpha) * 0.4f * 255));
        draw_list->AddCircleFilled(s_pos, 2.0f, middle_glow);

        ImU32 core_dot =
            IM_COL32((int)(globals::sonar_dot_color[0] * 255),
                     (int)(globals::sonar_dot_color[1] * 255),
                     (int)(globals::sonar_dot_color[2] * 255),
                     (int)(globals::sonar_dot_color[3] * dot_alpha * 255));
        draw_list->AddCircleFilled(s_pos, 1.0f, core_dot);

        if (globals::sonar_show_distance) {
          char dist_str[16];
          snprintf(dist_str, sizeof(dist_str), "%.0fm", distance);
          ImVec2 text_pos(s_pos.x + 10, s_pos.y - 10);
          Visualize.DrawTextWithSpacingAndOutline(draw_list, Visualize.visitor, 13.0f, text_pos,
                                                  IM_COL32(255, 255, 255, 255), IM_COL32(0, 0, 0, 255),
                                                  dist_str, 0.0f);
        }
      }
    }
  }
}

void draw_radar(ImDrawList *draw_unused) {
  if (!globals::radar_enabled)
    return;

  const float radar_size = globals::radar_size;
  const float half_size = radar_size / 2.0f;

  float radar_pos_x = globals::radar_pos_x;
  float radar_pos_y = globals::radar_pos_y;

  ImVec2 radar_origin = ImVec2(radar_pos_x, radar_pos_y);
  ImVec2 radar_center = radar_origin + ImVec2(half_size, half_size);

  ImDrawList *draw = ImGui::GetBackgroundDrawList();

  ImGui::SetNextWindowBgAlpha(0.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
  ImGui::Begin("RadarDragZone", nullptr,
               ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                   ImGuiWindowFlags_NoScrollbar |
                   ImGuiWindowFlags_NoSavedSettings |
                   ImGuiWindowFlags_NoInputs);

  ImGui::SetWindowPos(radar_origin);
  ImGui::SetWindowSize(ImVec2(radar_size, radar_size));
  ImGui::InvisibleButton("RadarMove", ImVec2(radar_size, radar_size));

  if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
    ImVec2 drag_delta = ImGui::GetIO().MouseDelta;
    radar_pos_x += drag_delta.x;
    radar_pos_y += drag_delta.y;
    radar_pos_x = std::clamp(radar_pos_x, 0.0f,
                             ImGui::GetIO().DisplaySize.x - radar_size);
    radar_pos_y = std::clamp(radar_pos_y, 0.0f,
                             ImGui::GetIO().DisplaySize.y - radar_size);

    globals::radar_pos_x = radar_pos_x;
    globals::radar_pos_y = radar_pos_y;
  }

  ImGui::End();
  ImGui::PopStyleVar(2);

  draw->AddRectFilled(radar_origin,
                      radar_origin + ImVec2(radar_size, radar_size),
                      IM_COL32(25, 25, 25, 180));

  draw->AddRect(radar_origin, radar_origin + ImVec2(radar_size, radar_size),
                IM_COL32(255, 255, 255, 255), 0.0f, 0, 3.0f);

  draw->AddLine(ImVec2(radar_center.x, radar_origin.y),
                ImVec2(radar_center.x, radar_origin.y + radar_size),
                IM_COL32(180, 180, 180, 180), 1.0f);
  draw->AddLine(ImVec2(radar_origin.x, radar_center.y),
                ImVec2(radar_origin.x + radar_size, radar_center.y),
                IM_COL32(180, 180, 180, 180), 1.0f);

  Engine::Vector3 local_pos = storage::localplayer.head.GetPartPos();
  float local_yaw = 0.0f;
  if (storage::camera.address) {
    auto cam_rot = storage::camera.GetCameraRotation();
    auto cam_euler = cam_rot.MatrixToEulerAngles();
    local_yaw = cam_euler.y * (M_PI / 180.0f);
  }

  draw->AddCircleFilled(radar_center, 5.0f, IM_COL32(0, 255, 0, 255));

  for (auto &player : storage::player_cache) {
    if (!player.address || player.address == storage::localplayer.address)
      continue;
    Engine::Vector3 pos = player.head.GetPartPos();
    float dx = pos.x - local_pos.x;
    float dz = pos.z - local_pos.z;

    float angle = -local_yaw;
    float rotated_x = dx * cos(angle) - dz * sin(angle);
    float rotated_z = dx * sin(angle) + dz * cos(angle);

    float max_dist = 200.f;

    float dist = sqrtf(rotated_x * rotated_x + rotated_z * rotated_z);
    if (dist > max_dist) {
      rotated_x *= max_dist / dist;
      rotated_z *= max_dist / dist;
      dist = max_dist;
    }

    float px = std::round(radar_center.x + (rotated_x / max_dist) * half_size);
    float py = std::round(radar_center.y + (rotated_z / max_dist) * half_size);

    ImVec2 player_pos = ImVec2(px, py);

    // High-contrast radar dots
    draw->AddCircleFilled(player_pos, 4.0f, IM_COL32(0, 0, 0, 255));
    draw->AddCircleFilled(player_pos, 2.0f, IM_COL32(255, 60, 60, 220));

    if (globals::radar_show_distance) {
      char dist_str[16];
      snprintf(dist_str, sizeof(dist_str), "%dm", static_cast<int>(dist));
      Visualize.DrawTextWithSpacingAndOutline(draw, Visualize.visitor, 13.0f, 
                                              ImVec2(px + 6, py - 6), 
                                              IM_COL32(255, 255, 255, 255), IM_COL32(0, 0, 0, 255),
                                              dist_str, 0.0f);
    }
  }
}

void DrawCornerBox(const ImVec2 &_topLeft, const ImVec2 &_bottomRight,
                   ImU32 color, float thickness = 2.0f,
                   float maxCornerSize = 20.0f) {
  ImVec2 topLeft(std::round(_topLeft.x), std::round(_topLeft.y));
  ImVec2 bottomRight(std::round(_bottomRight.x), std::round(_bottomRight.y));
  ImDrawList *draw = ImGui::GetBackgroundDrawList();
  float width = bottomRight.x - topLeft.x;
  float height = bottomRight.y - topLeft.y;
  float maxSizeX = width / 2;
  float maxSizeY = height / 2;
  float scaleX = width / 100.0f;
  float scaleY = height / 100.0f;
  scaleX = std::clamp(scaleX, 0.1f, 1.0f);
  scaleY = std::clamp(scaleY, 0.1f, 1.0f);
  float sizeX = maxCornerSize * scaleX;
  float sizeY = maxCornerSize * scaleY;
  sizeX = (sizeX > maxSizeX) ? maxSizeX : sizeX;
  sizeY = (sizeY > maxSizeY) ? maxSizeY : sizeY;

  // Ensure thickness is rounded for pixel-perfection
  float t = std::floor(thickness + 0.5f);
  if (t < 1.0f) t = 1.0f;
  ImU32 shadow = IM_COL32(0, 0, 0, 255);

  auto drawLineWithShadow = [&](ImVec2 p1, ImVec2 p2) {
    p1 = ImVec2(std::round(p1.x), std::round(p1.y));
    p2 = ImVec2(std::round(p2.x), std::round(p2.y));
    
    // 3-pass shadow for corners
    draw->AddLine(p1, p2, shadow, t + 2.0f); // Outer
    draw->AddLine(p1, p2, color, t);         // Main
  };

  if (storage::fill_box) {
    // ... (rest of filling logic stays same, it's already using rounded topLeft/bottomRight)
    if (storage::fill_box_gradient) {
      ImU32 gradient_color1 = IM_COL32((int)(storage::fill_box_gradient_color1[0] * 255), (int)(storage::fill_box_gradient_color1[1] * 255), (int)(storage::fill_box_gradient_color1[2] * 255), (int)(storage::fill_box_gradient_color1[3] * 255));
      ImU32 gradient_color2 = IM_COL32((int)(storage::fill_box_gradient_color2[0] * 255), (int)(storage::fill_box_gradient_color2[1] * 255), (int)(storage::fill_box_gradient_color2[2] * 255), (int)(storage::fill_box_gradient_color2[3] * 255));
      Visualize.rect_filled_gradient(topLeft, bottomRight, gradient_color1, gradient_color2);
    }
    if (storage::fill_box_gradient_2) {
      ImU32 gradient2_color1 = IM_COL32((int)(storage::fill_box_gradient_2_color1[0] * 255), (int)(storage::fill_box_gradient_2_color1[1] * 255), (int)(storage::fill_box_gradient_2_color1[2] * 255), (int)(storage::fill_box_gradient_2_color1[3] * 255));
      ImU32 gradient2_color2 = IM_COL32((int)(storage::fill_box_gradient_2_color2[0] * 255), (int)(storage::fill_box_gradient_2_color2[1] * 255), (int)(storage::fill_box_gradient_2_color2[2] * 255), (int)(storage::fill_box_gradient_2_color2[3] * 255));
      Visualize.rect_filled_gradient(topLeft, bottomRight, gradient2_color1, gradient2_color2);
    }
    if (!storage::fill_box_gradient && !storage::fill_box_gradient_2) {
      ImU32 fillColor = IM_COL32((int)(storage::box_color1[0] * 255), (int)(storage::box_color1[1] * 255), (int)(storage::box_color1[2] * 255), 80);
      draw->AddRectFilled(topLeft, bottomRight, fillColor);
    }
  }

  // Draw lines with high-contrast shadows
  drawLineWithShadow(topLeft, ImVec2(topLeft.x + sizeX, topLeft.y));
  drawLineWithShadow(topLeft, ImVec2(topLeft.x, topLeft.y + sizeY));
  drawLineWithShadow(ImVec2(bottomRight.x - sizeX, topLeft.y), ImVec2(bottomRight.x, topLeft.y));
  drawLineWithShadow(ImVec2(bottomRight.x, topLeft.y), ImVec2(bottomRight.x, topLeft.y + sizeY));
  drawLineWithShadow(ImVec2(topLeft.x, bottomRight.y - sizeY), ImVec2(topLeft.x, bottomRight.y));
  drawLineWithShadow(ImVec2(topLeft.x, bottomRight.y), ImVec2(topLeft.x + sizeX, bottomRight.y));
  drawLineWithShadow(ImVec2(bottomRight.x - sizeX, bottomRight.y), bottomRight);
  drawLineWithShadow(ImVec2(bottomRight.x, bottomRight.y - sizeY), bottomRight);
}

void DrawHitboxVisualization(const Engine::PlayerIns &player,
                             const Engine::Matrix4x4 &viewMatrix,
                             const Engine::Vector2 &dimensions,
                             ImDrawList *draw) {
  if (!storage::hitbox_expander::visualize)
    return;
  auto character = player.character;
  if (!character.address)
    return;

  auto humanoid_root_part = character.FindFirstChild("HumanoidRootPart");
  if (!humanoid_root_part.address)
    return;

  Engine::Vector3 hrpPos = humanoid_root_part.GetPartPos();

  Engine::Vector3 hitboxSize = {storage::hitbox_expander::size_x,
                                storage::hitbox_expander::size_y,
                                storage::hitbox_expander::size_z};

  float halfX = hitboxSize.x * 0.5f;
  float halfY = hitboxSize.y * 0.5f;
  float halfZ = hitboxSize.z * 0.5f;

  std::vector<Engine::Vector3> corners3D = {
      {hrpPos.x + halfX, hrpPos.y + halfY, hrpPos.z + halfZ},
      {hrpPos.x - halfX, hrpPos.y + halfY, hrpPos.z + halfZ},
      {hrpPos.x + halfX, hrpPos.y - halfY, hrpPos.z + halfZ},
      {hrpPos.x - halfX, hrpPos.y - halfY, hrpPos.z + halfZ},
      {hrpPos.x + halfX, hrpPos.y + halfY, hrpPos.z - halfZ},
      {hrpPos.x - halfX, hrpPos.y + halfY, hrpPos.z - halfZ},
      {hrpPos.x + halfX, hrpPos.y - halfY, hrpPos.z - halfZ},
      {hrpPos.x - halfX, hrpPos.y - halfY, hrpPos.z - halfZ}};

  struct FaceData {
    std::vector<ImVec2> screenCorners;
    float avgDepth;
    ImU32 color;
  };

  std::vector<FaceData> faces;
  Engine::Vector3 cameraPos = storage::camera.GetCameraPos();

  std::vector<std::vector<int>> faceIndices = {{0, 1, 3, 2}, {4, 5, 7, 6},
                                               {0, 1, 5, 4}, {2, 3, 7, 6},
                                               {1, 3, 7, 5}, {0, 2, 6, 4}};

  for (const auto &face : faceIndices) {
    FaceData faceData;
    float totalDepth = 0.0f;
    bool validFace = true;

    for (int cornerIdx : face) {
      Engine::Vector2 screenPos =
          Engine::WorldToScreen(corners3D[cornerIdx], dimensions, viewMatrix);
      if (screenPos.x == -1 && screenPos.y == -1) {
        validFace = false;
        break;
      }
      faceData.screenCorners.push_back(ImVec2(screenPos.x, screenPos.y));

      float depth =
          MathUtils::Magnitude(MathUtils::Sub(corners3D[cornerIdx], cameraPos));
      totalDepth += depth;
    }

    if (validFace && faceData.screenCorners.size() == 4) {
      faceData.avgDepth = totalDepth / 4.0f;
      float depthFactor =
          std::clamp(1.0f - (faceData.avgDepth / 1000.0f), 0.3f, 1.0f);
      faceData.color = IM_COL32(
          static_cast<int>(storage::hitbox_expander::visualize_color[0] * 255 *
                           depthFactor),
          static_cast<int>(storage::hitbox_expander::visualize_color[1] * 255 *
                           depthFactor),
          static_cast<int>(storage::hitbox_expander::visualize_color[2] * 255 *
                           depthFactor),
          static_cast<int>(storage::hitbox_expander::visualize_color[3] * 255 *
                           0.15f));

      // Snap face corners
      for (auto &corner : faceData.screenCorners) {
        corner.x = std::round(corner.x);
        corner.y = std::round(corner.y);
      }

      faces.push_back(faceData);
    }
  }
  std::sort(faces.begin(), faces.end(),
            [](const FaceData &a, const FaceData &b) {
              return a.avgDepth > b.avgDepth;
            });

  for (const auto &face : faces) {
    if (face.screenCorners.size() == 4) {
      draw->AddQuadFilled(face.screenCorners[0], face.screenCorners[1],
                          face.screenCorners[2], face.screenCorners[3],
                          face.color);
    }
  }

  ImU32 outlineColor = IM_COL32(
      static_cast<int>(storage::hitbox_expander::visualize_color[0] * 255),
      static_cast<int>(storage::hitbox_expander::visualize_color[1] * 255),
      static_cast<int>(storage::hitbox_expander::visualize_color[2] * 255),
      255);
  ImU32 shadow = IM_COL32(0, 0, 0, 255);

  std::vector<ImVec2> corners2D;
  for (const auto &corner : corners3D) {
    Engine::Vector2 screenPos =
        Engine::WorldToScreen(corner, dimensions, viewMatrix);
    if (screenPos.x != -1 && screenPos.y != -1) {
      corners2D.push_back(ImVec2(std::round(screenPos.x), std::round(screenPos.y)));
    }
  }

  if (corners2D.size() == 8) {
    auto drawSnappedLine = [&](ImVec2 p1, ImVec2 p2) {
      draw->AddLine(p1, p2, shadow, 3.0f); // Wide shadow for hitboxes
      draw->AddLine(p1, p2, outlineColor, 1.0f);
    };

    drawSnappedLine(corners2D[0], corners2D[1]);
    drawSnappedLine(corners2D[1], corners2D[3]);
    drawSnappedLine(corners2D[3], corners2D[2]);
    drawSnappedLine(corners2D[2], corners2D[0]);

    drawSnappedLine(corners2D[4], corners2D[5]);
    drawSnappedLine(corners2D[5], corners2D[7]);
    drawSnappedLine(corners2D[7], corners2D[6]);
    drawSnappedLine(corners2D[6], corners2D[4]);

    drawSnappedLine(corners2D[0], corners2D[4]);
    drawSnappedLine(corners2D[1], corners2D[5]);
    drawSnappedLine(corners2D[2], corners2D[6]);
    drawSnappedLine(corners2D[3], corners2D[7]);
  }
}

void DrawSkeletonESP(const Engine::PlayerIns &player,
                     const Engine::Matrix4x4 &viewMatrix,
                     const Engine::Vector2 &dimensions, ImDrawList *draw) {
  if (!storage::visuals::skeleton)
    return;

  // Enable anti-aliased lines for smooth skeleton
  ImDrawListFlags originalFlags = draw->Flags;
  draw->Flags |= ImDrawListFlags_AntiAliasedLines;

  ImU32 skeletonColor =
      IM_COL32(static_cast<int>(storage::visuals::skeleton_color[0] * 255),
               static_cast<int>(storage::visuals::skeleton_color[1] * 255),
               static_cast<int>(storage::visuals::skeleton_color[2] * 255),
               static_cast<int>(storage::visuals::skeleton_color[3] * 255));

  float thickness = storage::visuals::skeleton_thickness;
  
  // 3-pass shadow colors for depth
  ImU32 shadowOuter = IM_COL32(0, 0, 0, 180);
  ImU32 shadowInner = IM_COL32(0, 0, 0, 255);

  auto drawBone = [&](const Engine::Instance &part1,
                      const Engine::Instance &part2) {
    if (!part1.address || !part2.address)
      return;

    Engine::Vector3 pos1 = part1.GetPartPos();
    Engine::Vector3 pos2 = part2.GetPartPos();

    Engine::Vector2 screen1 =
        Engine::WorldToScreen(pos1, dimensions, viewMatrix);
    Engine::Vector2 screen2 =
        Engine::WorldToScreen(pos2, dimensions, viewMatrix);

    if (screen1.x != -1 && screen1.y != -1 && screen2.x != -1 &&
        screen2.y != -1) {
      // Pixel-snap for crisp rendering
      ImVec2 p1(std::round(screen1.x), std::round(screen1.y));
      ImVec2 p2(std::round(screen2.x), std::round(screen2.y));
      
      // Strict integer math for absolute clarity when zoomed
      float t = std::floor(thickness + 0.5f);
      if (t < 1.0f) t = 1.0f;

      // Extreme 3-pass contrast system (Outer/Inner/Main)
      // All thicknesses are integers to prevent sub-pixel blurring
      draw->AddLine(p1, p2, IM_COL32(0, 0, 0, 200), t + 3.0f);  // Outer glow
      draw->AddLine(p1, p2, IM_COL32(0, 0, 0, 255), t + 1.0f);  // Inner shadow
      draw->AddLine(p1, p2, skeletonColor, t);                // Main color
    }
  };

  bool isR6 = (player.r15 == 0);

  if (isR6) {
    if (player.upperTorso.address && player.head.address) {
      drawBone(player.head, player.upperTorso);
      if (player.leftArm.address)
        drawBone(player.upperTorso, player.leftArm);
      if (player.rightArm.address)
        drawBone(player.upperTorso, player.rightArm);
      if (player.leftLeg.address)
        drawBone(player.upperTorso, player.leftLeg);
      if (player.rightLeg.address)
        drawBone(player.upperTorso, player.rightLeg);
    }
  } else {
    drawBone(player.head, player.upperTorso);
    drawBone(player.upperTorso, player.lowerTorso);

    drawBone(player.upperTorso, player.leftUpperArm);
    drawBone(player.leftUpperArm, player.leftLowerArm);
    drawBone(player.leftLowerArm, player.leftHand);

    drawBone(player.upperTorso, player.rightUpperArm);
    drawBone(player.rightUpperArm, player.rightLowerArm);
    drawBone(player.rightLowerArm, player.rightHand);

    drawBone(player.lowerTorso, player.leftUpperLeg);
    drawBone(player.leftUpperLeg, player.leftLowerLeg);
    drawBone(player.leftLowerLeg, player.leftFoot);

    drawBone(player.lowerTorso, player.rightUpperLeg);
    drawBone(player.rightUpperLeg, player.rightLowerLeg);
    drawBone(player.rightLowerLeg, player.rightFoot);
  }

  // Restore original flags
  draw->Flags = originalFlags;
}


struct visualize {

  static void outlined(ImVec2 &pos, ImVec2 &size, ImU32 col,
                       float rounding = 0.f) {
    pos.x = std::round(pos.x);
    pos.y = std::round(pos.y);
    size.x = std::round(size.x);
    size.y = std::round(size.y);
    auto draw = ImGui::GetBackgroundDrawList();
    draw->Flags |= ImDrawListFlags_AntiAliasedLines;
    ImRect rect_bb(pos.x, pos.y, pos.x + size.x, pos.y + size.y);
    ImVec2 shadow_offset = {cosf(0.f) * 2.f, sinf(0.f) * 2.f};
    draw->AddRect({rect_bb.Min.x - 2.f, rect_bb.Min.y - 2.f},
                  {rect_bb.Max.x + 2.f, rect_bb.Max.y + 2.f},
                  IM_COL32(0, 0, 0, col >> 24), rounding);
    draw->AddRect({rect_bb.Min.x - 1.f, rect_bb.Min.y - 1.f},
                  {rect_bb.Max.x + 1.f, rect_bb.Max.y + 1.f}, col, rounding);
  }

  static void healthbar(const ImVec2 &box_pos, const ImVec2 &box_size,
                        float health, float max_health, ImU32 col) {
    auto draw = ImGui::GetBackgroundDrawList();
    draw->Flags |= ImDrawListFlags_AntiAliasedLines;
    float ratio = (max_health > 0.f) ? health / max_health : 0.f;
    ratio = std::fmax(0.f, std::fmin(ratio, 1.f));
    float box_top = std::round(box_pos.y);
    float box_bottom = std::round(box_pos.y + box_size.y);
    float box_left = std::round(box_pos.x);
    float bar_x = (box_left - 2.f) - 3.f;
    ImVec2 outline_min(bar_x - 1.f, box_top - 2.f);
    ImVec2 outline_max(bar_x + 2.f, box_bottom + 2.f);
    draw->AddRectFilled(outline_min, outline_max, IM_COL32(0, 0, 0, 255));
    float bar_height = (box_bottom - box_top) * ratio;
    ImVec2 fill_min(bar_x, (box_bottom - bar_height) - 1.f);
    ImVec2 fill_max(bar_x + 1.f, box_bottom + 1.f);
    draw->AddRectFilled(fill_min, fill_max, col);
  }

  static void outlined_line(const ImVec2 &from, const ImVec2 &to, ImU32 color,
                            float thickness = 1.0f,
                            ImU32 outline_color = IM_COL32(0, 0, 0, 255),
                            float outline_thickness = 2.0f) {
    ImDrawList *draw = ImGui::GetForegroundDrawList();
    draw->Flags |= ImDrawListFlags_AntiAliasedLines;
    draw->AddLine(from, to, outline_color, thickness + outline_thickness);
    draw->AddLine(from, to, color, thickness);
  }
};

void hooks::esp() {
  if (globals::performance_mode == 1) {
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
  }

  ImDrawList *draw = ImGui::GetBackgroundDrawList();
  draw->Flags &= ~ImDrawListFlags_AntiAliasedLines;
  draw->Flags &= ~ImDrawListFlags_AntiAliasedFill;
  if (!draw)
    return;

  POINT cursor_pos;
  GetCursorPos(&cursor_pos);
  ScreenToClient(FindWindowA(nullptr, "roblox"), &cursor_pos);

  Engine::Vector2 dimensions = storage::visualengine.GetDimensions();
  if (dimensions.x <= 0 || dimensions.y <= 0)
    return;

  Engine::Matrix4x4 viewMatrix = storage::visualengine.GetViewMatrix();
  Engine::PlayerIns localplayer = storage::localplayer;
  Engine::Vector3 cameraPos = storage::camera.GetCameraPos();

  if (storage::visuals::forcefield_chams_self) {
      auto localPlrRaw = storage::players.GetLocalPlayer();
      if (localPlrRaw.address) {
          Engine::Instance localPlrInst;
          localPlrInst.address = localPlrRaw.address;
          Engine::Instance character = localPlrInst.GetModelInstance();
      if (character.address) {
          for (auto child : character.GetChildren()) {
              if (child.address) {
                 std::string className = child.GetClass();
                 if (className == "Part" || className == "MeshPart") {
                     // Skip HumanoidRootPart
                     std::string partName = child.GetName();
                     if (partName == "HumanoidRootPart") {
                         continue;
                     }
                     
                     if (child.GetMaterial() != 1584) {
                         child.SetMaterial(1584);
                     }
                     child.SetTransparency(-1.0f);
                 }
              }
          }
      }
      }
  }

  if (cameraPos.x == 0 && cameraPos.y == 0 && cameraPos.z == 0) {
    cameraPos = localplayer.rootPart.GetPartPos();
  }
  if (storage::visuals::fog && storage::datamodel.address) {
    try {
      Engine::Instance lighting =
          storage::datamodel.FindFirstChildOfClass("Lighting");
      if (lighting.address) {
        lighting.SetFogStart(storage::visuals::fog_start);
        lighting.SetFogEnd(storage::visuals::fog_end);

        Engine::Vector3 fog_color_vec = Engine::Vector3(
            storage::visuals::fog_color[0], storage::visuals::fog_color[1],
            storage::visuals::fog_color[2]);
        lighting.FogColorSet(fog_color_vec);
      }
    } catch (...) {
    }
  }



  if (storage::watermark_enabled) {
    ImGui::PushStyleColor(
        ImGuiCol_WindowBg,
        ImVec4(15.0f / 255.0f, 15.0f / 255.0f, 15.0f / 255.0f, 0.9f));
    ImGui::PushStyleColor(
        ImGuiCol_Border,
        ImVec4(0.0f, 0.0f, 0.0f, 0.0f)); // Handled by manual draw
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 6));

    ImGui::SetNextWindowPos(ImVec2(storage::watermark_x, storage::watermark_y), ImGuiCond_Always);
    ImGui::Begin("Watermark", nullptr,
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                     ImGuiWindowFlags_AlwaysAutoResize |
                     ImGuiWindowFlags_NoFocusOnAppearing |
                     ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove);

    // Watermark Content

    ImVec2 win_pos = ImGui::GetWindowPos();
    ImVec2 win_size = ImGui::GetWindowSize();
    ImDrawList *dl = ImGui::GetWindowDrawList();

    // Double layered border like the menu
    dl->AddRect(win_pos, win_pos + win_size, IM_COL32(0, 0, 0, 255));
    dl->AddRect(win_pos + ImVec2(1, 1), win_pos + win_size - ImVec2(1, 1),
                IM_COL32(55, 55, 55, 255));
    
    // Baby Blue Top Outline
    dl->AddLine(
        ImVec2(win_pos.x, win_pos.y),
        ImVec2(win_pos.x + win_size.x, win_pos.y),
        IM_COL32(137, 207, 240, 255), 1.0f);

    ImVec4 accent =
        ImVec4(137.0f / 255.0f, 207.0f / 255.0f, 240.0f / 255.0f, 1.0f);
    ImU32 separator_col = IM_COL32(55, 55, 55, 255);

    // Vertical line dividers like the menu title
    auto DrawDivider = [&](float x_offset) {
      ImVec2 cursor = ImGui::GetCursorScreenPos();
      dl->AddLine(ImVec2(cursor.x + x_offset, win_pos.y + 1),
                  ImVec2(cursor.x + x_offset, win_pos.y + win_size.y - 1),
                  separator_col);
    };

    DrawDivider(-6.0f); // Initial separator

    ImGui::TextColored(accent, "##");
    ImGui::SameLine(0.0f, 0.0f); // No space
    ImGui::Text("SCATHE");

    ImGui::SameLine(0.0f, 12.0f);
    DrawDivider(-6.0f);
    ImGui::Text("v1.0");

    ImGui::SameLine(0.0f, 12.0f);
    DrawDivider(-6.0f);
    ImGui::Text("%d FPS", (int)ImGui::GetIO().Framerate);

    // Manual Dragging Logic (Robust)
    static bool is_dragging_watermark = false;
    if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(0)) {
        is_dragging_watermark = true;
    }
    if (!ImGui::IsMouseDown(0)) {
        is_dragging_watermark = false;
    }

    if (is_dragging_watermark) {
      ImVec2 delta = ImGui::GetIO().MouseDelta;
      storage::watermark_x += delta.x;
      storage::watermark_y += delta.y;
    }

    ImGui::End();
    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor(2);
  }

  if (storage::show_fov || storage::show_silent_fov) {
    ImVec2 center(cursor_pos.x, cursor_pos.y);

    if (storage::show_fov) {
      ImU32 fovColor = ImGui::ColorConvertFloat4ToU32(
          ImVec4(storage::fov_color[0], storage::fov_color[1],
                 storage::fov_color[2], storage::fov_color[3]));
      DrawPolygonalFOV(draw, center, storage::aimbot_fov, fovColor, 0.0f);
    }
    if (storage::show_silent_fov) {
      ImU32 silentFovColor = ImGui::ColorConvertFloat4ToU32(
          ImVec4(storage::silent_fov_color[0], storage::silent_fov_color[1],
                 storage::silent_fov_color[2], storage::silent_fov_color[3]));
      DrawPolygonalFOV(draw, center, storage::silent_fov, silentFovColor, 0.0f);
    }
  }



  crosshair::render();
  
  // Check for mouse clicks and register bullet tracers
  BulletTracers::CheckAndRegisterTracerOnClick();
  
  // Render bullet tracers
  BulletTracers::RenderBulletTracers(draw);

  std::vector<Engine::PlayerIns> players_snapshot;
  std::vector<Engine::PlayerIns> custom_snapshot;
  {
    std::lock_guard<std::mutex> lg(storage::cached_players_mutex);
    players_snapshot = storage::player_cache;
  }
  {
    std::lock_guard<std::mutex> lg(storage::custom_players_mutex);
    custom_snapshot = storage::custom_player_cache;
  }

  for (auto &player : players_snapshot) {

    if (player.address == localplayer.address) {
      if (!storage::visuals::allow_local_player)
        continue;
    } else {
      bool sameTeam =
          (player.team.address != 0 && localplayer.team.address != 0 &&
           player.team.address == localplayer.team.address);
      if (storage::visuals::teamcheck && sameTeam)
        continue;
    }

    if (storage::visuals::death_check && IsPlayerDeadOrKO(player))
      continue;

    if (globals::skipFriendlyESP && globals::IsPlayerFriendly(player.name)) {
      continue;
    }

    DrawHitboxVisualization(player, viewMatrix, dimensions, draw);
    Engine::Vector3 enemyPos = player.rootPart.GetPartPos();
    float distance = MathUtils::Magnitude(MathUtils::Sub(cameraPos, enemyPos));

    if (distance > 10000.0f || distance < 0.0f || !std::isfinite(distance)) {
      distance = 0.0f;
    }

    if (globals::radar_enabled) {
      draw_radar(draw);
    }

    ImVec2 startPos;

    switch (storage::mode) {
    case 0:
      startPos = ImVec2(cursor_pos.x, cursor_pos.y);
      break;
    case 1:
      startPos = ImVec2(dimensions.x / 2.0f, dimensions.y);
      break;
    case 2:
      startPos = ImVec2(dimensions.x / 2.0f, 0.0f);
      break;
    default:
      startPos = ImVec2(cursor_pos.x, cursor_pos.y);
      break;
    }

    std::vector<Engine::Instance> parts = {
        player.head,          player.upperTorso,    player.lowerTorso,
        player.leftFoot,      player.rightFoot,     player.leftUpperLeg,
        player.rightUpperLeg, player.rightLowerLeg, player.leftLowerLeg,
        player.leftHand,      player.rightHand,     player.leftUpperArm,
        player.rightUpperArm, player.leftLowerArm,  player.rightLowerArm,
        player.pfLimbs1,      player.pfLimbs2,      player.pfLimbs3,
        player.pfLimbs4,      player.pfLimbs5};

    static const Engine::Vector3 local_corners[8] = {
        {-1, -1, -1}, {1, -1, -1}, {-1, 1, -1}, {1, 1, -1},
        {-1, -1, 1},  {1, -1, 1},  {-1, 1, 1},  {1, 1, 1}};

    float left = FLT_MAX, top = FLT_MAX;
    float right = -FLT_MAX, bottom = -FLT_MAX;
    bool valid = false;

    for (const auto &part : parts) {
      if (!part.address)
        continue;

      auto size = part.GetSize();
      auto pos = part.GetPartPos();
      auto rot = part.GetRotation();

      if (storage::gameid == 113491250) {
        if (part == player.upperTorso) {
          size = {2.f, 2.f, 1.f};
        } else if (part == player.head) {
          size = {1.f, 1.f, 1.f};
        } else {
          size = {1.f, 2.f, 1.f};
        }
      }

      for (const auto &lc : local_corners) {
        Engine::Vector3 world =
            pos + rot * Engine::Vector3{lc.x * size.x * 0.5f,
                                        lc.y * size.y * 0.5f,
                                        lc.z * size.z * 0.5f};
        auto screen = Engine::WorldToScreen(world, dimensions, viewMatrix);
        if (screen.x < 0.f || screen.y < 0.f)
          continue;

        valid = true;
        left = min(left, screen.x);
        top = min(top, screen.y);
        right = max(right, screen.x);
        bottom = max(bottom, screen.y);
      }
    }

    if (!valid || left >= right || top >= bottom)
      continue;

    ImVec2 boxPos(left, top);
    ImVec2 boxSize(right - left, bottom - top);
    ImVec2 boxBR(boxPos.x + boxSize.x, boxPos.y + boxSize.y);

    if (storage::visuals::box) {
      switch (storage::box_type) {
      case 0: {
        ImU32 boxColor = ImGui::ColorConvertFloat4ToU32(
            ImVec4(storage::box_color[0], storage::box_color[1],
                   storage::box_color[2], storage::box_color[3]));

        float myThickness = storage::Boxthickness;
        visualize::outlined(boxPos, boxSize, boxColor, 0.f);
        break;
      }

      case 1: {
        ImU32 cornerColor = ImGui::ColorConvertFloat4ToU32(
            ImVec4(storage::box_color[0], storage::box_color[1],
                   storage::box_color[2], storage::box_color[3]));
        DrawCornerBox(boxPos, boxBR, cornerColor, storage::Boxthickness, 20.f);
        break;
      }

      default:
        break;
      }
    }

    if (storage::fill_box) {
      if (storage::fill_box_gradient) {
        ImU32 gradient_color1, gradient_color2;

        gradient_color1 =
            IM_COL32((int)(storage::fill_box_gradient_color1[0] * 255),
                     (int)(storage::fill_box_gradient_color1[1] * 255),
                     (int)(storage::fill_box_gradient_color1[2] * 255),
                     (int)(storage::fill_box_gradient_color1[3] * 255));
        gradient_color2 =
            IM_COL32((int)(storage::fill_box_gradient_color2[0] * 255),
                     (int)(storage::fill_box_gradient_color2[1] * 255),
                     (int)(storage::fill_box_gradient_color2[2] * 255),
                     (int)(storage::fill_box_gradient_color2[3] * 255));
        Visualize.rect_filled_gradient(boxPos, boxBR, gradient_color1,
                                       gradient_color2);
      } else {
        ImU32 fillColor = IM_COL32((int)(storage::box_color1[0] * 255),
                                   (int)(storage::box_color1[1] * 255),
                                   (int)(storage::box_color1[2] * 255), 80);
        draw->AddRectFilled(boxPos, boxBR, fillColor);
      }
    }
    if (storage::visuals::chams) {

      ImDrawList *draw = ImGui::GetBackgroundDrawList();

      // Draw standard chams (standard parts)
      const std::vector<Engine::Instance> &active_parts = parts;
      
      // Save original flags and enable anti-aliasing for smooth chams
      ImDrawListFlags originalFlags = draw->Flags;
      draw->Flags |= ImDrawListFlags_AntiAliasedLines;
      draw->Flags |= ImDrawListFlags_AntiAliasedFill;

      auto project_part = [&](Engine::Instance &part) -> std::vector<ImVec2> {
        std::vector<ImVec2> projected;
        if (!part.address)
          return projected;

        auto prim = part.primitive();
        auto size = part.GetSize();

        if (storage::gameid == 113491250) {
          if (part == player.upperTorso) {
            size = {2.f, 2.f, 1.f};
          } else if (part == player.head) {
            size = {1.f, 1.f, 1.f};
          } else {
            size = {1.f, 2.f, 1.f};
          }
        }

        auto pos = part.GetPartPos();
        auto rot = part.GetRotation();

        for (const auto &lc : local_corners) {
          Engine::Vector3 world =
              pos + rot * Engine::Vector3{lc.x * size.x * 0.5f,
                                          lc.y * size.y * 0.5f,
                                          lc.z * size.z * 0.5f};
          auto screen = Engine::WorldToScreen(world, dimensions, viewMatrix);
          if (screen.x >= 0.f && screen.y >= 0.f)
            projected.push_back(ImVec2(std::round(screen.x), std::round(screen.y)));
        }

        if (projected.size() < 3)
          return {};

        std::sort(projected.begin(), projected.end(),
                  [](const ImVec2 &a, const ImVec2 &b) {
                    return a.x < b.x || (a.x == b.x && a.y < b.y);
                  });

        std::vector<ImVec2> hull;
        auto cross = [](const ImVec2 &O, const ImVec2 &A, const ImVec2 &B) {
          return (A.x - O.x) * (B.y - O.y) - (A.y - O.y) * (B.x - O.x);
        };

        for (auto &p : projected) {
          while (hull.size() >= 2 &&
                 cross(hull[hull.size() - 2], hull[hull.size() - 1], p) <= 0)
            hull.pop_back();
          hull.push_back(p);
        }

        size_t t = hull.size() + 1;
        for (int i = (int)projected.size() - 1; i >= 0; --i) {
          auto &p = projected[i];
          while (hull.size() >= t &&
                 cross(hull[hull.size() - 2], hull[hull.size() - 1], p) <= 0)
            hull.pop_back();
          hull.push_back(p);
        }

        hull.pop_back();
        return hull;
      };

      Clipper2Lib::Paths64 all_parts;
      for (auto &part : parts) {
        auto hull = project_part(part);
        if (hull.size() < 3)
          continue;

        Clipper2Lib::Path64 path;
        for (auto &pt : hull)
          path.push_back({static_cast<int64_t>(pt.x * 1000.0),
                          static_cast<int64_t>(pt.y * 1000.0)});
        all_parts.push_back(path);
      }

      if (all_parts.empty())
        continue;

      auto unified_solution =
          Clipper2Lib::Union(all_parts, Clipper2Lib::FillRule::NonZero);

      std::vector<std::vector<ImVec2>> all_polys;
      for (auto &sp : unified_solution) {
        std::vector<ImVec2> poly;
        for (auto &pt : sp)
          poly.push_back(ImVec2(std::round(pt.x / 1000.0f), std::round(pt.y / 1000.0f)));
        if (poly.size() >= 3)
          all_polys.push_back(poly);
      }

      ImU32 fill_color = ImGui::ColorConvertFloat4ToU32(
          {storage::chams_color[0], storage::chams_color[1],
           storage::chams_color[2], storage::chams_color[3]});

      // Draw filled polygons with AA
      for (auto &poly : all_polys) {
        draw->AddConcavePolyFilled(poly.data(), poly.size(), fill_color);
      }

      // Draw smooth anti-aliased outlines with high-contrast integer shadow
      ImU32 outlineShadow = IM_COL32(0, 0, 0, 210);
      for (auto &poly : all_polys) {
        draw->AddPolyline(poly.data(), poly.size(), outlineShadow, true, 3.0f); // Strict integer shadow (1px each side)
        draw->AddPolyline(poly.data(), poly.size(), IM_COL32_WHITE, true, 1.0f);
      }

      // Restore original flags
      draw->Flags = originalFlags;
    }


    if (globals::r3dbox) {
      ImU32 esp3dColor =
          IM_COL32(static_cast<int>(globals::r3dboxcolor[0] * 255.f),
                   static_cast<int>(globals::r3dboxcolor[1] * 255.f),
                   static_cast<int>(globals::r3dboxcolor[2] * 255.f), 255);

      Engine::Vector3 hrpPos = player.rootPart.GetPartPos();
      Engine::Matrix3x3 hrpCFrame = player.rootPart.GetCframeLOl();

      Engine::Vector3 rightVec = {hrpCFrame.data[0], hrpCFrame.data[3],
                                  hrpCFrame.data[6]};
      Engine::Vector3 upVec = {hrpCFrame.data[1], hrpCFrame.data[4],
                               hrpCFrame.data[7]};
      Engine::Vector3 lookVec = {hrpCFrame.data[2], hrpCFrame.data[5],
                                 hrpCFrame.data[8]};

      Engine::Vector3 headPos = player.head.GetPartPos();
      Engine::Vector3 footPos = player.leftFoot.GetPartPos();

      float fullHeight = fabs(headPos.y - footPos.y);

      float scale = 1.0f;
      float height = fullHeight * 0.5f * scale;
      float width = fullHeight * 0.3f * scale;
      float depth = fullHeight * 0.25f * scale;

      std::vector<Engine::Vector3> corners3D = {
          hrpPos + rightVec * width + upVec * height + lookVec * depth,
          hrpPos - rightVec * width + upVec * height + lookVec * depth,
          hrpPos + rightVec * width - upVec * height + lookVec * depth,
          hrpPos - rightVec * width - upVec * height + lookVec * depth,
          hrpPos + rightVec * width + upVec * height - lookVec * depth,
          hrpPos - rightVec * width + upVec * height - lookVec * depth,
          hrpPos + rightVec * width - upVec * height - lookVec * depth,
          hrpPos - rightVec * width - upVec * height - lookVec * depth};

      std::vector<ImVec2> corners2D;
      for (const auto &corner : corners3D) {
        Engine::Vector2 screenPos =
            Engine::WorldToScreen(corner, dimensions, viewMatrix);
        if (screenPos.x != -1 && screenPos.y != -1)
          corners2D.push_back(ImVec2(screenPos.x, screenPos.y));
      }

      if (corners2D.size() == 8 && !storage::hitbox_expander::visualize) {
        ImU32 shadow = IM_COL32(0, 0, 0, 255);
        float t = 1.0f; // Thin, sharp lines for 3D boxes
        
        auto drawSnappedLine = [&](ImVec2 p1, ImVec2 p2) {
          p1 = ImVec2(std::round(p1.x), std::round(p1.y));
          p2 = ImVec2(std::round(p2.x), std::round(p2.y));
          
          // Outer shadow for depth
          draw->AddLine(p1, p2, shadow, t + 2.0f);
          draw->AddLine(p1, p2, esp3dColor, t);
        };

        drawSnappedLine(corners2D[0], corners2D[1]);
        drawSnappedLine(corners2D[1], corners2D[3]);
        drawSnappedLine(corners2D[3], corners2D[2]);
        drawSnappedLine(corners2D[2], corners2D[0]);

        drawSnappedLine(corners2D[4], corners2D[5]);
        drawSnappedLine(corners2D[5], corners2D[7]);
        drawSnappedLine(corners2D[7], corners2D[6]);
        drawSnappedLine(corners2D[6], corners2D[4]);

        drawSnappedLine(corners2D[0], corners2D[4]);
        drawSnappedLine(corners2D[1], corners2D[5]);
        drawSnappedLine(corners2D[2], corners2D[6]);
        drawSnappedLine(corners2D[3], corners2D[7]);
      }
    }


    DrawHitboxVisualization(player, viewMatrix, dimensions, draw);

    if (storage::visuals::skeleton) {
      DrawSkeletonESP(player, viewMatrix, dimensions, draw);
    }

    if (storage::visuals::health_bar) {
      constexpr float healthBarWidth = 1.0f;
      float boxHeight = boxSize.y;

      ImVec2 barPos;
      ImVec2 barSize;

      if (globals::TypeHealthBar == 2) {
        barPos = ImVec2(boxPos.x - 5.0f, boxPos.y - 1.0f);
        barSize = ImVec2(1.0f, boxHeight + 2.0f);
      } else {
        barPos = ImVec2(boxPos.x + boxSize.x + 3.0f, boxPos.y - 1.0f);
        barSize = ImVec2(1.0f, boxHeight + 2.0f);
      }

      Visualize.draw_health_bar(draw, player.maxhealth, player.health, barPos,
                                barSize, 1.0f, true);
    }



    std::vector<std::string> field_order = globals::ESPFieldOrder;
    std::unordered_map<std::string, int> field_sides = {
        {"name", globals::TypeTextName},
        {"distance", globals::TypeTextDistance},
        {"tool", globals::TypeTextTool}};

    auto calculateFieldPosition = [&](const std::string &field_name,
                                      ImVec2 text_size, int side, ImVec2 boxPos,
                                      ImVec2 boxBR) -> ImVec2 {
      float base_y;
      if (side == 1) {
        base_y = boxPos.y - text_size.y - 1.0f;
      } else {
        base_y = boxBR.y + 1.0f;
      }

      auto it = std::find(field_order.begin(), field_order.end(), field_name);
      if (it != field_order.end()) {
        int field_index = std::distance(field_order.begin(), it);

        int fields_before = 0;
        for (int i = 0; i < field_index; i++) {
          std::string other_field = field_order[i];
          bool other_enabled = false;
          if (other_field == "tool" && storage::visuals::tool)
            other_enabled = true;

          if (other_enabled && field_sides[other_field] == side) {
            fields_before++;
          }
        }

        ImFont *font = Visualize.visitor;
        float font_size = 13.0f;
        ImVec2 test_size =
            font ? font->CalcTextSizeA(font_size, FLT_MAX, 0.0f, "A")
                 : ImVec2(0, font_size);
        float field_height = test_size.y;
        float spacing = 1.0f;
        if (side == 1) {
          base_y -= fields_before * (field_height + spacing);
        } else {
          base_y += fields_before * (field_height + spacing);
        }
      }

      float centerX = (boxPos.x + boxBR.x) * 0.5f;
      float textX = centerX - (text_size.x * 0.5f);
      textX = std::floor(textX + 0.5f);
      base_y = std::floor(base_y + 0.5f);

      return ImVec2(textX, base_y);
    };

    for (const std::string &field_name : field_order) {
      if (field_name == "name" && storage::visuals::name) {
        std::string toDisplay = (storage::visuals::name_display_mode == 0)
                                    ? player.name
                                    : player.displayname;

        ImFont *font = Visualize.visitor;
        float font_size = 13.0f;
        float char_spacing = 1.0f;

        float total_width = 0.0f;
        float max_height = 0.0f;
        for (size_t i = 0; i < toDisplay.length(); i++) {
          char c = toDisplay[i];
          char char_str[2] = {c, '\0'};
          ImVec2 char_size =
              font ? font->CalcTextSizeA(font_size, FLT_MAX, 0.0f, char_str)
                   : ImGui::CalcTextSize(char_str);
          total_width +=
              char_size.x + (i < toDisplay.length() - 1 ? char_spacing : 0.0f);
          max_height = std::max(max_height, char_size.y);
        }
        ImVec2 textSize(total_width, max_height);

        ImVec2 textPos = calculateFieldPosition(
            "name", textSize, globals::TypeTextName, boxPos, boxBR);
        if (globals::TypeTextName == 1) { // Top
          textPos.y -= 1.0f;
        }

        ImU32 nameColor = ImGui::ColorConvertFloat4ToU32(
            ImVec4(storage::name_color[0], storage::name_color[1],
                   storage::name_color[2], storage::name_color[3]));

        ImDrawList *draw = ImGui::GetBackgroundDrawList();
        if (draw) {
          ImU32 outlineColor = IM_COL32(0, 0, 0, 255);
          Visualize.DrawTextWithSpacingAndOutline(
              draw, font, font_size, textPos, nameColor, outlineColor,
              toDisplay, char_spacing);
        }
      } else if (field_name == "distance" && storage::visuals::distance) {
        char distanceStr[32];
        if (distance > 0.0f && distance < 10000.0f && std::isfinite(distance)) {
          snprintf(distanceStr, sizeof(distanceStr), "[%.1fM]", distance);
        } else {
          snprintf(distanceStr, sizeof(distanceStr), "[0.0M]");
        }

        ImFont *font = Visualize.visitor;
        float font_size = 13.0f;
        float char_spacing = 1.0f;

        float total_width = 0.0f;
        float max_height = 0.0f;
        for (size_t i = 0; i < strlen(distanceStr); i++) {
          char c = distanceStr[i];
          char char_str[2] = {c, '\0'};
          ImVec2 char_size =
              font ? font->CalcTextSizeA(font_size, FLT_MAX, 0.0f, char_str)
                   : ImGui::CalcTextSize(char_str);
          total_width +=
              char_size.x + (i < strlen(distanceStr) - 1 ? char_spacing : 0.0f);
          max_height = std::max(max_height, char_size.y);
        }
        ImVec2 textSize(total_width, max_height);

        ImVec2 textPos = calculateFieldPosition(
            "distance", textSize, globals::TypeTextDistance, boxPos, boxBR);

        ImU32 textColor = ImGui::ColorConvertFloat4ToU32(
            ImVec4(storage::distance_color[0], storage::distance_color[1],
                   storage::distance_color[2], storage::distance_color[3]));
        ImDrawList *draw = ImGui::GetBackgroundDrawList();
        if (draw) {
          ImU32 outlineColor = IM_COL32(0, 0, 0, 255);
          Visualize.DrawTextWithSpacingAndOutline(
              draw, font, font_size, textPos, textColor, outlineColor,
              std::string(distanceStr), char_spacing);
        }
      } else if (field_name == "tool" && storage::visuals::tool) {
        std::string toolName = "None";
        if (player.currentToolName.length() > 0) {
          toolName = player.currentToolName;
        } else if (player.tool.address) {
          toolName = player.tool.GetName();
        }

        if (toolName != "None" && !toolName.empty()) {
          if (toolName.front() == '[' && toolName.back() == ']') {
            toolName = toolName.substr(1, toolName.length() - 2);
          }
          toolName = "[" + toolName + "]";

          ImFont *font = Visualize.visitor;
          float font_size = 13.0f;
          float char_spacing = 1.0f;

          float total_width = 0.0f;
          for (size_t i = 0; i < toolName.length(); i++) {
            char char_str[2] = {toolName[i], '\0'};
            ImVec2 char_size =
                font ? font->CalcTextSizeA(font_size, FLT_MAX, 0.0f, char_str)
                     : ImGui::CalcTextSize(char_str);
            total_width +=
                char_size.x + (i < toolName.length() - 1 ? char_spacing : 0.0f);
          }

          ImVec2 textPos =
              calculateFieldPosition("tool", ImVec2(total_width, font_size),
                                     globals::TypeTextTool, boxPos, boxBR);

        // Global Forcefield Chams
        if (storage::visuals::forcefield_chams_global) {
             static const std::vector<Engine::Instance Engine::PlayerIns::*> bodyParts = {
                &Engine::PlayerIns::head,
                &Engine::PlayerIns::upperTorso,
                &Engine::PlayerIns::lowerTorso,
                &Engine::PlayerIns::leftUpperArm,
                &Engine::PlayerIns::leftLowerArm,
                &Engine::PlayerIns::leftHand,
                &Engine::PlayerIns::rightUpperArm,
                &Engine::PlayerIns::rightLowerArm,
                &Engine::PlayerIns::rightHand,
                &Engine::PlayerIns::leftUpperLeg,
                &Engine::PlayerIns::leftLowerLeg,
                &Engine::PlayerIns::leftFoot,
                &Engine::PlayerIns::rightUpperLeg,
                &Engine::PlayerIns::rightLowerLeg,
                &Engine::PlayerIns::rightFoot
            };
            for (const auto& partPtr : bodyParts) {
                 Engine::Instance part = player.*partPtr;
                 if (part.address) {
                     if (part.GetMaterial() != 1584) {
                         part.SetMaterial(1584);
                     }
                 }
            }
        }

          ImU32 toolColor = ImGui::ColorConvertFloat4ToU32(

              ImVec4(storage::tool_esp_color[0], storage::tool_esp_color[1],
                     storage::tool_esp_color[2], storage::tool_esp_color[3]));
          ImDrawList *draw = ImGui::GetBackgroundDrawList();
          if (draw) {
            Visualize.DrawTextWithSpacingAndOutline(
                draw, font, font_size, textPos, toolColor,
                IM_COL32(0, 0, 0, 255), toolName, char_spacing);
          }
        }
      }
    }

    if (storage::visuals::weapon_icon_esp && Visualize.weapon_icon_font) {
      std::string toolName = "";

      if (player.currentToolName.length() > 0) {
        toolName = player.currentToolName;
      } else if (player.tool.address) {
        toolName = player.tool.GetName();
      }

      if (!toolName.empty() && toolName != "None") {
        const char *weaponIcon = GetWeaponIcon(toolName);

        if (weaponIcon && strlen(weaponIcon) > 0) {
          float iconFontSize = 18.0f;  // Larger for clarity
          ImVec2 iconTextSize = Visualize.weapon_icon_font->CalcTextSizeA(
              iconFontSize, FLT_MAX, 0.0f, weaponIcon);

          // Center the icon below the box
          float centerX = (boxPos.x + boxBR.x) * 0.5f;
          float iconX = std::round(centerX - (iconTextSize.x * 0.5f));

          float iconY = boxBR.y + 2.0f;
          if (storage::visuals::tool) {
            iconY += 14.0f;
          }
          if (storage::visuals::distance && globals::TypeTextDistance == 0) {
            iconY += 12.0f;
          }

          ImVec2 iconPos(std::round(iconX), std::round(iconY));

          ImU32 iconColor = ImGui::ColorConvertFloat4ToU32(
              ImVec4(storage::visuals::weapon_icon_color[0],
                     storage::visuals::weapon_icon_color[1],
                     storage::visuals::weapon_icon_color[2],
                     storage::visuals::weapon_icon_color[3]));

          ImDrawList *draw = ImGui::GetBackgroundDrawList();
          if (draw) {
            // Strict 8-direction stroke for zero-blur icons
            const ImVec2 offsets[8] = {{-1, -1}, {0, -1}, {1, -1}, {-1, 0},
                                       {1, 0}, {-1, 1}, {0, 1}, {1, 1}};
            for (const auto &off : offsets) {
              ImVec2 pos = {iconPos.x + off.x, iconPos.y + off.y};
              draw->AddText(Visualize.weapon_icon_font, iconFontSize, pos,
                            IM_COL32(0, 0, 0, 255), weaponIcon);
            }
            draw->AddText(Visualize.weapon_icon_font, iconFontSize, iconPos,
                          iconColor, weaponIcon);
          }
        }
      }
    }
    if (storage::visuals::tracers) {
      DrawTracerESP(player, boxPos, boxBR, draw, dimensions, viewMatrix);
    }
  }

  draw = ImGui::GetBackgroundDrawList();

  for (auto &player : custom_snapshot) {
    if (player.address == localplayer.address) {
      if (!storage::visuals::allow_local_player)
        continue;
    } else {
      bool sameTeam =
          (player.team.address != 0 && localplayer.team.address != 0 &&
           player.team.address == localplayer.team.address);
      if (storage::visuals::teamcheck && sameTeam)
        continue;
    }

    if (storage::visuals::death_check && IsPlayerDeadOrKO(player))
      continue;
    if (globals::skipFriendlyESP && globals::IsPlayerFriendly(player.name))
      continue;
    Engine::Vector3 enemyPos = player.rootPart.GetPartPos();
    float distance = MathUtils::Magnitude(MathUtils::Sub(cameraPos, enemyPos));

    if (distance > 10000.0f || distance < 0.0f || !std::isfinite(distance)) {
      distance = 0.0f;
    }

    if (globals::radar_enabled)
      draw_radar(draw);
    ImVec2 startPos;
    switch (storage::mode) {
    case 0:
      startPos = ImVec2(cursor_pos.x, cursor_pos.y);
      break;
    case 1:
      startPos = ImVec2(dimensions.x / 2.0f, dimensions.y);
      break;
    case 2:
      startPos = ImVec2(dimensions.x / 2.0f, 0.0f);
      break;
    default:
      startPos = ImVec2(cursor_pos.x, cursor_pos.y);
      break;
    }
    std::vector<Engine::Instance> parts = {
        player.head,          player.upperTorso,    player.lowerTorso,
        player.leftFoot,      player.rightFoot,     player.leftUpperLeg,
        player.rightUpperLeg, player.rightLowerLeg, player.leftLowerLeg,
        player.leftHand,      player.rightHand,     player.leftUpperArm,
        player.rightUpperArm, player.leftLowerArm,  player.rightLowerArm};

    static const Engine::Vector3 local_corners[8] = {
        {-1, -1, -1}, {1, -1, -1}, {-1, 1, -1}, {1, 1, -1},
        {-1, -1, 1},  {1, -1, 1},  {-1, 1, 1},  {-1, 1, 1}};

    float left = FLT_MAX, top = FLT_MAX;
    float right = -FLT_MAX, bottom = -FLT_MAX;
    bool valid = false;

    for (const auto &part : parts) {
      if (!part.address)
        continue;

      auto size = part.GetSize();
      auto pos = part.GetPartPos();
      auto rot = part.GetRotation();

      for (const auto &lc : local_corners) {
        Engine::Vector3 world =
            pos + rot * Engine::Vector3{lc.x * size.x * 0.5f,
                                        lc.y * size.y * 0.5f,
                                        lc.z * size.z * 0.5f};
        auto screen = Engine::WorldToScreen(world, dimensions, viewMatrix);
        if (screen.x < 0.f || screen.y < 0.f)
          continue;

        valid = true;
        left = min(left, screen.x);
        top = min(top, screen.y);
        right = max(right, screen.x);
        bottom = max(bottom, screen.y);
      }
    }

    if (!valid || left >= right || top >= bottom)
      continue;

    ImVec2 boxPos(left, top);
    ImVec2 boxSize(right - left, bottom - top);
    ImVec2 boxBR(boxPos.x + boxSize.x, boxPos.y + boxSize.y);
    if (storage::visuals::box) {
      switch (storage::box_type) {
      case 0: {
        ImU32 boxColor = ImGui::ColorConvertFloat4ToU32(
            ImVec4(storage::box_color[0], storage::box_color[1],
                   storage::box_color[2], storage::box_color[3]));
        float myThickness = storage::Boxthickness;
        visualize::outlined(boxPos, boxSize, boxColor, 0.f);
        break;
      }
      case 1: {
        ImU32 cornerColor = ImGui::ColorConvertFloat4ToU32(
            ImVec4(storage::box_color[0], storage::box_color[1],
                   storage::box_color[2], storage::box_color[3]));
        DrawCornerBox(boxPos, boxBR, cornerColor, storage::Boxthickness, 20.f);
        break;
      }
      default:
        break;
      }
    }
    if (storage::fill_box) {
      if (storage::fill_box_gradient) {
        ImU32 gradient_color1 =
            IM_COL32((int)(storage::fill_box_gradient_color1[0] * 255),
                     (int)(storage::fill_box_gradient_color1[1] * 255),
                     (int)(storage::fill_box_gradient_color1[2] * 255),
                     (int)(storage::fill_box_gradient_color1[3] * 255));
        ImU32 gradient_color2 =
            IM_COL32((int)(storage::fill_box_gradient_color2[0] * 255),
                     (int)(storage::fill_box_gradient_color2[1] * 255),
                     (int)(storage::fill_box_gradient_color2[2] * 255),
                     (int)(storage::fill_box_gradient_color2[3] * 255));
        Visualize.rect_filled_gradient(boxPos, boxBR, gradient_color1,
                                       gradient_color2);
      } else {
        ImU32 fillColor = IM_COL32((int)(storage::box_color1[0] * 255),
                                   (int)(storage::box_color1[1] * 255),
                                   (int)(storage::box_color1[2] * 255),
                                   (int)(storage::box_color1[3] * 255));
        draw->AddRectFilled(boxPos, boxBR, fillColor);
      }
    }

    if (storage::visuals::skeleton) {
      DrawSkeletonESP(player, viewMatrix, dimensions, draw);
    }

    if (storage::visuals::health_bar) {
      float boxHeight = boxSize.y;

      ImVec2 barPos;
      ImVec2 barSize;

      if (globals::TypeHealthBar == 2) {
        barPos = ImVec2(boxPos.x - 5.0f, boxPos.y - 1.0f);
        barSize = ImVec2(1.0f, boxHeight + 2.0f);
      } else {
        barPos = ImVec2(boxPos.x + boxSize.x + 3.0f, boxPos.y - 1.0f);
        barSize = ImVec2(1.0f, boxHeight + 2.0f);
      }

      Visualize.draw_health_bar(draw, player.maxhealth, player.health, barPos,
                                barSize, 1.0f, true);
    }

    if (storage::visuals::tracers) {
      DrawTracerESP(player, boxPos, boxBR, draw, dimensions, viewMatrix);
    }

    std::vector<std::string> field_order_2 = globals::ESPFieldOrder;
    std::unordered_map<std::string, int> field_sides_2 = {
        {"name", globals::TypeTextName},
        {"distance", globals::TypeTextDistance},
        {"tool", globals::TypeTextTool}};

    auto calculateFieldPosition2 = [&](const std::string &field_name,
                                       ImVec2 text_size, int side,
                                       ImVec2 boxPos, ImVec2 boxBR) -> ImVec2 {
      float base_y;
      if (side == 1) {
        base_y = boxPos.y - text_size.y - 1.0f;
      } else {
        base_y = boxBR.y + 1.0f;
      }

      auto it =
          std::find(field_order_2.begin(), field_order_2.end(), field_name);
      if (it != field_order_2.end()) {
        int field_index = std::distance(field_order_2.begin(), it);

        int fields_before = 0;
        for (int i = 0; i < field_index; i++) {
          std::string other_field = field_order_2[i];
          bool other_enabled = false;
          if (other_field == "name" && storage::visuals::name)
            other_enabled = true;
          else if (other_field == "distance" && storage::visuals::distance)
            other_enabled = true;
          else if (other_field == "tool" && storage::visuals::tool)
            other_enabled = true;

          if (other_enabled && field_sides_2[other_field] == side) {
            fields_before++;
          }
        }

        ImFont *font = Visualize.visitor;
        float font_size = 9.0f;
        ImVec2 test_size =
            font ? font->CalcTextSizeA(font_size, FLT_MAX, 0.0f, "A")
                 : ImVec2(0, font_size);
        float field_height = test_size.y;
        float spacing = 1.0f;
        if (side == 1) {
          base_y -= fields_before * (field_height + spacing);
        } else {
          base_y += fields_before * (field_height + spacing);
        }
      }

      float centerX = (boxPos.x + boxBR.x) * 0.5f;
      float textX = centerX - (text_size.x * 0.5f);
      textX = std::floor(textX + 0.5f);
      base_y = std::floor(base_y + 0.5f);

      return ImVec2(textX, base_y);
    };

    for (const std::string &field_name : field_order_2) {
      if (field_name == "distance" && storage::visuals::distance) {
        char distanceStr[32];
        if (distance > 0.0f && distance < 10000.0f && std::isfinite(distance)) {
          snprintf(distanceStr, sizeof(distanceStr), "[%.1fM]", distance);
        } else {
          snprintf(distanceStr, sizeof(distanceStr), "[0.0M]");
        }
        ImFont *font = Visualize.visitor;
        float font_size = 9.0f;
        float char_spacing = 1.0f;

        float total_width = 0.0f;
        float max_height = 0.0f;
        for (size_t i = 0; i < strlen(distanceStr); i++) {
          char c = distanceStr[i];
          char char_str[2] = {c, '\0'};
          ImVec2 char_size =
              font ? font->CalcTextSizeA(font_size, FLT_MAX, 0.0f, char_str)
                   : ImGui::CalcTextSize(char_str);
          total_width +=
              char_size.x + (i < strlen(distanceStr) - 1 ? char_spacing : 0.0f);
          max_height = std::max(max_height, char_size.y);
        }
        ImVec2 textSize(total_width, max_height);

        ImVec2 textPos = calculateFieldPosition2(
            "distance", textSize, globals::TypeTextDistance, boxPos, boxBR);
        if (storage::visuals::tool) {
          textPos.y -= 2.0f;
        }
        ImU32 textColor = ImGui::ColorConvertFloat4ToU32(
            ImVec4(storage::distance_color[0], storage::distance_color[1],
                   storage::distance_color[2], storage::distance_color[3]));
        ImDrawList *draw_bg = ImGui::GetBackgroundDrawList();
        if (draw_bg) {
          ImU32 outlineColor = IM_COL32(0, 0, 0, 255);
          Visualize.DrawTextWithSpacingAndOutline(
              draw_bg, font, font_size, textPos, textColor, outlineColor,
              std::string(distanceStr), char_spacing);
        }
      } else if (field_name == "name" && storage::visuals::name) {
        std::string toDisplay = (storage::visuals::name_display_mode == 0)
                                    ? player.name
                                    : player.displayname;
        ImFont *font = Visualize.visitor;
        float font_size = 9.0f;
        float char_spacing = 1.0f;

        float total_width = 0.0f;
        float max_height = 0.0f;
        for (size_t i = 0; i < toDisplay.length(); i++) {
          char c = toDisplay[i];
          char char_str[2] = {c, '\0'};
          ImVec2 char_size =
              font ? font->CalcTextSizeA(font_size, FLT_MAX, 0.0f, char_str)
                   : ImGui::CalcTextSize(char_str);
          total_width +=
              char_size.x + (i < toDisplay.length() - 1 ? char_spacing : 0.0f);
          max_height = std::max(max_height, char_size.y);
        }
        ImVec2 textSize(total_width, max_height);

        ImVec2 textPos = calculateFieldPosition2(
            "name", textSize, globals::TypeTextName, boxPos, boxBR);
        textPos.y -= 1.0f;

        ImU32 nameColor = ImGui::ColorConvertFloat4ToU32(
            ImVec4(storage::name_color[0], storage::name_color[1],
                   storage::name_color[2], storage::name_color[3]));
        ImU32 outlineColor = IM_COL32(0, 0, 0, 255);
        ImDrawList *draw_list = ImGui::GetBackgroundDrawList();
        if (draw_list) {
          Visualize.DrawTextWithSpacingAndOutline(
              draw_list, font, font_size, textPos, nameColor, outlineColor,
              toDisplay, char_spacing);
        }
      } else if (field_name == "tool" && storage::visuals::tool) {
        std::string toolName = "None";

        if (player.currentToolName.length() > 0) {
          toolName = player.currentToolName;
        } else if (player.tool.address) {
          toolName = player.tool.GetName();
        }

        if (toolName != "None" && !toolName.empty()) {
          if (toolName.front() == '[' && toolName.back() == ']') {
            toolName = toolName.substr(1, toolName.length() - 2);
          }
          toolName = "[" + toolName + "]";

          ImFont *font = Visualize.visitor;
          float font_size = 9.0f;
          float char_spacing = 1.0f;

          float total_width = 0.0f;
          float max_height = 0.0f;
          for (size_t i = 0; i < toolName.length(); i++) {
            char c = toolName[i];
            char char_str[2] = {c, '\0'};
            ImVec2 char_size =
                font ? font->CalcTextSizeA(font_size, FLT_MAX, 0.0f, char_str)
                     : ImGui::CalcTextSize(char_str);
            total_width +=
                char_size.x + (i < toolName.length() - 1 ? char_spacing : 0.0f);
            max_height = std::max(max_height, char_size.y);
          }
          ImVec2 textSize(total_width, max_height);

          ImVec2 textPos = calculateFieldPosition2(
              "tool", textSize, globals::TypeTextTool, boxPos, boxBR);

          ImU32 toolColor = ImGui::ColorConvertFloat4ToU32(
              ImVec4(storage::tool_esp_color[0], storage::tool_esp_color[1],
                     storage::tool_esp_color[2], storage::tool_esp_color[3]));

          ImDrawList *draw_list = ImGui::GetBackgroundDrawList();
          if (draw_list) {
            ImU32 outlineColor = IM_COL32(0, 0, 0, 255);
            Visualize.DrawTextWithSpacingAndOutline(
                draw_list, font, font_size, textPos, toolColor, outlineColor,
                toolName, char_spacing);
          }
        }
      }
    }

    if (storage::visuals::weapon_icon_esp && Visualize.weapon_icon_font) {
      std::string toolName = "";

      if (player.currentToolName.length() > 0) {
        toolName = player.currentToolName;
      } else if (player.tool.address) {
        toolName = player.tool.GetName();
      }

      if (!toolName.empty() && toolName != "None") {
        const char *weaponIcon = GetWeaponIcon(toolName);

        if (weaponIcon && strlen(weaponIcon) > 0) {
          float iconFontSize = 12.0f;
          ImVec2 iconTextSize = Visualize.weapon_icon_font->CalcTextSizeA(
              iconFontSize, FLT_MAX, 0.0f, weaponIcon);

          // Center the icon below the box
          float centerX = (boxPos.x + boxBR.x) * 0.5f;
          float iconX = centerX - (iconTextSize.x * 0.5f);

          float iconY = boxBR.y + 2.0f;
          if (storage::visuals::tool) {
            iconY += 12.0f;
          }
          if (storage::visuals::distance && globals::TypeTextDistance == 0) {
            iconY += 10.0f;
          }

          ImVec2 iconPos(iconX, iconY);

          ImU32 iconColor = ImGui::ColorConvertFloat4ToU32(
              ImVec4(storage::visuals::weapon_icon_color[0],
                     storage::visuals::weapon_icon_color[1],
                     storage::visuals::weapon_icon_color[2],
                     storage::visuals::weapon_icon_color[3]));

          ImDrawList *draw_list = ImGui::GetBackgroundDrawList();
          if (draw_list) {
            const ImVec2 offsets[4] = {{-1, -1}, {-1, 1}, {1, 1}, {1, -1}};
            for (const auto &off : offsets) {
              ImVec2 pos = {iconPos.x + off.x, iconPos.y + off.y};
              draw_list->AddText(Visualize.weapon_icon_font, iconFontSize, pos,
                                 IM_COL32(0, 0, 0, 255), weaponIcon);
            }
            draw_list->AddText(Visualize.weapon_icon_font, iconFontSize,
                               iconPos, iconColor, weaponIcon);
          }
        }
      }
    }
  }
}

void DrawTracerESP(const Engine::PlayerIns &player, const ImVec2 &topLeft,
                   const ImVec2 &bottomRight, ImDrawList *draw,
                   const Engine::Vector2 &dimensions,
                   const Engine::Matrix4x4 &viewMatrix) {
  if (!draw)
    return;

  ImVec2 origin;
  ImVec2 screen_size = ImGui::GetIO().DisplaySize;

  if (storage::tracer_type == 0) {
    origin = ImVec2(screen_size.x * 0.5f, screen_size.y);
  } else if (storage::tracer_type == 1) {
    origin = ImVec2(screen_size.x * 0.5f, 0.0f);
  } else {
    origin = ImVec2(screen_size.x * 0.5f, screen_size.y);

    HWND rw = FindWindowA(nullptr, "Roblox");
    POINT pt;
    if (rw && GetCursorPos(&pt) && ScreenToClient(rw, &pt)) {
      origin = ImVec2((float)pt.x, (float)pt.y);
    }
  }

  ImU32 col = ImGui::ColorConvertFloat4ToU32(
      ImVec4(storage::tracers_color[0], storage::tracers_color[1],
             storage::tracers_color[2], storage::tracers_color[3]));
  col = (col & 0x00FFFFFF) | 0xFF000000;

  ImVec2 target = ImVec2(std::round((topLeft.x + bottomRight.x) * 0.5f), std::round(topLeft.y));
  origin.x = std::round(origin.x);
  origin.y = std::round(origin.y);

  ImU32 shadow = IM_COL32(0, 0, 0, 255);
  if (storage::tracer_type == 0 || storage::tracer_type == 1 ||
      storage::tracer_type == 2) {
    draw->AddLine(origin, target, shadow, 3.0f);
    draw->AddLine(origin, target, col, 1.0f);
  } else if (storage::tracer_type == 3) {
    float dx = fabsf(target.x - origin.x);
    float dy = fabsf(target.y - origin.y);
    float dip = 120.0f + 0.4f * dx + 0.25f * dy;
    ImVec2 ctrl(std::round((origin.x + target.x) * 0.5f),
                std::round((origin.y + target.y) * 0.5f + dip));

    const int segments = 32;
    std::vector<ImVec2> points;
    points.push_back(origin);
    for (int i = 1; i <= segments; ++i) {
      float t = (float)i / (float)segments;
      float it = 1.0f - t;
      points.push_back(ImVec2(std::round(it * it * origin.x + 2 * it * t * ctrl.x + t * t * target.x),
                             std::round(it * it * origin.y + 2 * it * t * ctrl.y + t * t * target.y)));
    }

    // Shadow pass (3px)
    for (size_t i = 0; i < points.size() - 1; ++i)
      draw->AddLine(points[i], points[i+1], shadow, 3.0f);
    // Color pass (1px)
    for (size_t i = 0; i < points.size() - 1; ++i)
      draw->AddLine(points[i], points[i+1], col, 1.0f);
  }
}


void render_player_chams(Engine::PlayerIns &player, ImDrawList *draw,
                         Engine::Vector2 dimensions,
                         Engine::Matrix4x4 viewMatrix) {
  std::vector<Engine::Instance> parts;
  if (player.character.address) {
    parts = player.character.GetChildren();
  }

  if (parts.empty())
    return;



  if (!storage::visuals::chams) return;

  static const std::vector<Engine::Vector3> local_corners = {
      {-1, -1, -1}, {1, -1, -1}, {1, 1, -1}, {-1, 1, -1},
      {-1, -1, 1},  {1, -1, 1},  {1, 1, 1},  {-1, 1, 1}};

  auto project_part = [&](Engine::Instance &part) -> std::vector<ImVec2> {
    std::vector<ImVec2> projected;
    if (!part.address)
      return projected;

    // Cache primitive and part data
    uintptr_t prim = mem::read<uintptr_t>(part.address + Offsets::BasePart::Primitive);
    if (!prim) return projected;

    Engine::Vector3 size = part.GetSize();
    Engine::Vector3 pos = mem::read<Engine::Vector3>(prim + Offsets::BasePart::Position);
    Engine::Matrix3x3 rot = mem::read<Engine::Matrix3x3>(prim + Offsets::BasePart::Rotation);

    for (const auto &lc : local_corners) {
      Engine::Vector3 world = pos + rot * Engine::Vector3{lc.x * size.x * 0.5f,
                                                          lc.y * size.y * 0.5f,
                                                          lc.z * size.z * 0.5f};
      auto screen = Engine::WorldToScreen(world, dimensions, viewMatrix);
      if (screen.x >= 0.f && screen.y >= 0.f)
        projected.push_back(ImVec2(screen.x, screen.y)); 
    }

    if (projected.size() < 3)
      return {};

    // Convex Hull (Monotone Chain)
    std::sort(projected.begin(), projected.end(),
              [](const ImVec2 &a, const ImVec2 &b) {
                return a.x < b.x || (a.x == b.x && a.y < b.y);
              });

    std::vector<ImVec2> hull;
    auto cross = [](const ImVec2 &O, const ImVec2 &A, const ImVec2 &B) {
      return (A.x - O.x) * (B.y - O.y) - (A.y - O.y) * (B.x - O.x);
    };

    for (auto &p : projected) {
      while (hull.size() >= 2 &&
             cross(hull[hull.size() - 2], hull[hull.size() - 1], p) <= 0)
        hull.pop_back();
      hull.push_back(p);
    }

    size_t t = hull.size() + 1;
    for (int i = (int)projected.size() - 1; i >= 0; --i) {
      auto &p = projected[i];
      while (hull.size() >= t &&
             cross(hull[hull.size() - 2], hull[hull.size() - 1], p) <= 0)
        hull.pop_back();
      hull.push_back(p);
    }

    hull.pop_back();
    return hull;
  };

  Clipper2Lib::Paths64 all_parts;
  for (auto &part : parts) {
    auto hull = project_part(part);
    if (hull.size() < 3)
      continue;

    Clipper2Lib::Path64 path;
    for (auto &pt : hull)
      path.push_back({static_cast<int64_t>(pt.x * 1000.0),
                      static_cast<int64_t>(pt.y * 1000.0)});
    all_parts.push_back(path);
  }

  if (all_parts.empty())
    return;

  auto unified_solution =
      Clipper2Lib::Union(all_parts, Clipper2Lib::FillRule::NonZero);

  std::vector<std::vector<ImVec2>> all_polys;
  for (auto &sp : unified_solution) {
    // Restabilization: Filter out garbage geometry (Increase threshold to 500)
    if (Clipper2Lib::Area(sp) < 500.0) 
      continue;

    std::vector<ImVec2> poly;
    for (auto &pt : sp) {
        ImVec2 p(pt.x / 1000.0f, pt.y / 1000.0f);
        
        // Restabilization: Increase pruning to 2px to prevent vertex overlapping jitter
        if (poly.empty()) {
            poly.push_back(p);
        } else {
            float dx = poly.back().x - p.x;
            float dy = poly.back().y - p.y;
            if (dx * dx + dy * dy >= 4.0f) { // 2px distance squared
                poly.push_back(p);
            }
        }
    }

    if (poly.size() >= 3)
      all_polys.push_back(poly);
  }

  ImU32 fill_color = ImGui::ColorConvertFloat4ToU32(
      {storage::chams_color[0], storage::chams_color[1],
       storage::chams_color[2], storage::chams_color[3]});

  ImDrawListFlags originalFlags = draw->Flags;
  draw->Flags |= ImDrawListFlags_AntiAliasedLines | ImDrawListFlags_AntiAliasedFill;

  for (auto &poly : all_polys) {
    draw->AddConcavePolyFilled(poly.data(), (int)poly.size(), fill_color);
  }

  // Draw high-contrast outlines for Chams
  ImU32 outlineShadow = IM_COL32(0, 0, 0, 255);
  for (auto &poly : all_polys) {
    draw->AddPolyline(poly.data(), (int)poly.size(), outlineShadow, true, 3.0f); // 3px strict shadow
    draw->AddPolyline(poly.data(), (int)poly.size(), IM_COL32_WHITE, true, 1.0f);  // 1px crisp outline
  }

  draw->Flags = originalFlags;
}
