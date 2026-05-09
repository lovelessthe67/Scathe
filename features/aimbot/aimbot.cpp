#include "core/Core.hpp"
#include "../esp/esp.h"
#include "utils/math/math.h"
#include "core/storage/Globals.hpp"
#include "core/storage/include.h"
#include <Windows.h>
#include <algorithm>
#include <cmath>
#include <future>
#include <immintrin.h>
#include <mmsystem.h>
#include <random>
#include <thread>
#include <vector>

#pragma comment(lib, "winmm.lib")

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

#define M_PI 3.14159265358979323846

static Engine::Vector3 Recalculate_Velocity(Engine::PlayerIns player) {
  Engine::Vector3 old_Position = player.rootPart.GetPartPos();
  return (player.rootPart.GetPartPos() - old_Position) / 0.115;
}

Engine::Instance FindPartByName(Engine::Instance &character,
                                const std::string &partName) {
  return character.FindFirstChild(partName);
}

static float sigmoid(float x) { return 1 / (1 + std::exp(-x)); }

static Engine::Vector3 Random_Vector3(const float x, const float y) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<float> dis_x(-x, x);
  std::uniform_real_distribution<float> dis_y(-y, y);
  return {dis_x(gen), dis_y(gen), dis_x(gen)};
}

static bool isWithinFOV(const Engine::Vector3 &hit_position_3D) {
  POINT cursor_point;
  GetCursorPos(&cursor_point);
  ScreenToClient(FindWindowA(0, ("roblox")), &cursor_point);

  auto cursor_pos_x = cursor_point.x;
  auto cursor_pos_y = cursor_point.y;

  Engine::Instance visualengine = storage::visualengine;
  Engine::Vector2 screen_dimensions = visualengine.GetDimensions();
  Engine::Vector2 hit_position_2D = Engine::WorldToScreen(
      hit_position_3D, screen_dimensions, visualengine.GetViewMatrix());

  float magnitude =
      (hit_position_2D - Engine::Vector2{static_cast<float>(cursor_pos_x),
                                         static_cast<float>(cursor_pos_y)})
          .getMagnitude();
  return (magnitude <= storage::aimbot_fov);
}

static float Ease(float t) {
  switch (storage::aimbot_easing_style) {
  case 0:
    return t;
  case 1:
    return 1 - std::cos((t * M_PI) / 2);
  case 2:
    return t * t;
  case 3:
    return t * t * t;
  case 4:
    return t * t * t * t;
  case 5:
    return t * t * t * t * t;
  case 6:
    return t == 0 ? 0 : std::pow(2, 10 * (t - 1));
  case 7:
    return 1 - std::sqrt(1 - std::pow(t, 2));
  case 8:
    return t * t * (2.70158f * t - 1.70158f);
  case 9:
    if (t < 1 / 2.75f)
      return 7.5625f * t * t;
    else if (t < 2 / 2.75f)
      return 7.5625f * (t -= 1.5f / 2.75f) * t + 0.75f;
    else if (t < 2.5f / 2.75f)
      return 7.5625f * (t -= 2.25f / 2.75f) * t + 0.9375f;
    else
      return 7.5625f * (t -= 2.625f / 2.75f) * t + 0.984375f;
  case 10:
    return t == 0 ? 0
           : t == 1
               ? 1
               : -std::pow(2, 10 * (t - 1)) * std::sin((t - 1.1f) * 5 * M_PI);
  default:
    return t;
  }
}

static float SmoothEase(float t, float strength = 1.0f) {
  t = Ease(t);
  return std::pow(t, strength);
}

static Engine::Matrix3x3 Slerp_Matrix3(const Engine::Matrix3x3 &a,
                                       const Engine::Matrix3x3 &b, float t) {
  t = SmoothEase(t, 1.2f);
  if (t == 1)
    return b;

  Engine::Matrix3x3 result{};
  for (int i = 0; i < 9; ++i) {
    result.data[i] = a.data[i] + (b.data[i] - a.data[i]) * t;
  }
  return result;
}

static Engine::Vector2 SmoothMouseDelta(const Engine::Vector2 &current,
                                        const Engine::Vector2 &target,
                                        float smoothFactor) {
  return current + (target - current) * smoothFactor;
}

static Engine::Matrix3x3 Lerp_Matrix3(const Engine::Matrix3x3 &a,
                                      const Engine::Matrix3x3 &b, float t) {
  t = Ease(t);
  if (t == 1)
    return b;

  Engine::Matrix3x3 result{};
  for (int i = 0; i < 9; ++i) {
    result.data[i] = a.data[i] + (b.data[i] - a.data[i]) * t;
  }
  return result;
}

static Engine::Vector3 Cross_Product(const Engine::Vector3 &vec1,
                                     const Engine::Vector3 &vec2) {
  return {vec1.y * vec2.z - vec1.z * vec2.y, vec1.z * vec2.x - vec1.x * vec2.z,
          vec1.x * vec2.y - vec1.y * vec2.x};
}

static Engine::Matrix3x3
Look_At_To_Matrix(const Engine::Vector3 &cameraPosition,
                  const Engine::Vector3 &targetPosition) {
  Engine::Vector3 forward = (targetPosition - cameraPosition).normalize();
  Engine::Vector3 right = Cross_Product({0, 1, 0}, forward).normalize();
  Engine::Vector3 up = Cross_Product(forward, right);

  Engine::Matrix3x3 lookAtMatrix{};
  lookAtMatrix.data[0] = -right.x;
  lookAtMatrix.data[1] = up.x;
  lookAtMatrix.data[2] = -forward.x;
  lookAtMatrix.data[3] = right.y;
  lookAtMatrix.data[4] = up.y;
  lookAtMatrix.data[5] = -forward.y;
  lookAtMatrix.data[6] = -right.z;
  lookAtMatrix.data[7] = up.z;
  lookAtMatrix.data[8] = -forward.z;

  return lookAtMatrix;
}

void InitializePlayerParts(Engine::PlayerIns &player) {
  std::vector<std::string> partNames = {
      "Head",       "HumanoidRootPart", "UpperTorso",
      "LowerTorso", "LeftUpperLeg",     "LeftUpperArm",
      "LeftHand",   "RightUpperArm",    "RightHand"};

  std::vector<Engine::Instance> parts;

  for (const std::string &partName : partNames) {
    parts.push_back(FindPartByName(player.character, partName));
  }

  player.head = parts[0];
  player.rootPart = parts[1];
  player.upperTorso = parts[2];
  player.lowerTorso = parts[3];
  player.leftUpperLeg = parts[4];
  player.leftUpperArm = parts[5];
  player.leftHand = parts[6];
  player.rightUpperArm = parts[7];
  player.rightHand = parts[8];
}

Engine::Instance getClosestPart(Engine::PlayerIns &player,
                                const POINT &cursor_point) {
  std::vector<Engine::Instance> parts = {
      player.head,          player.rootPart,     player.upperTorso,
      player.lowerTorso,    player.leftUpperLeg, player.leftFoot,
      player.rightFoot,     player.leftUpperArm, player.leftHand,
      player.rightUpperArm, player.rightHand,
  };

  Engine::Vector2 dimensions = storage::visualengine.GetDimensions();
  Engine::Matrix4x4 view_matrix = storage::visualengine.GetViewMatrix();
  Engine::Vector2 cursor = {static_cast<float>(cursor_point.x),
                            static_cast<float>(cursor_point.y)};
  float min_distance = FLT_MAX;
  Engine::Instance closest_part;

  for (size_t i = 0; i < parts.size(); ++i) {
    if (!parts[i].address)
      continue;

    Engine::Vector3 part_position = parts[i].GetPartPos();
    Engine::Vector2 part_screen_position =
        Engine::WorldToScreen(part_position, dimensions, view_matrix);
    float distance = (part_screen_position - cursor).getMagnitude();

    if (distance < min_distance) {
      min_distance = distance;
      closest_part = parts[i];
    }
  }

  return closest_part;
}

static Engine::PlayerIns getClosestPlayerFromCursor() {
  POINT cursor_point;
  GetCursorPos(&cursor_point);
  ScreenToClient(FindWindowA(nullptr, "roblox"), &cursor_point);

  Engine::Vector2 cursor = {static_cast<float>(cursor_point.x),
                            static_cast<float>(cursor_point.y)};

  std::vector<Engine::PlayerIns> candidates;
  {
    std::lock_guard<std::mutex> lg(storage::cached_players_mutex);
    candidates = storage::player_cache;
  }
  {
    std::lock_guard<std::mutex> lg(storage::custom_players_mutex);
    candidates.insert(candidates.end(), storage::custom_player_cache.begin(),
                      storage::custom_player_cache.end());
  }
  Engine::PlayerIns closestPlayer{};
  float shortestDistance = FLT_MAX;

  Engine::PlayerIns localPlayer = storage::localplayer;

  Engine::Vector2 dimensions = storage::visualengine.GetDimensions();
  Engine::Matrix4x4 viewMatrix = storage::visualengine.GetViewMatrix();

  for (Engine::PlayerIns &player : candidates) {
    if (player.address == localPlayer.address)
      continue;

    bool sameTeam =
        (player.team.address != 0 && localPlayer.team.address != 0 &&
         player.team.address == localPlayer.team.address);
    if (storage::team_prediction && sameTeam)
      continue;

    if (storage::ko_check && IsPlayerDeadOrKO(player))
      continue;

    if (globals::skipFriendlyESP && globals::IsPlayerFriendly(player.name)) {
      continue;
    }

    Engine::Instance part = player.rootPart;
    Engine::Vector3 partWorldPos = part.GetPartPos();

    if (storage::max_dist) {
      Engine::Vector3 localPos = localPlayer.rootPart.GetPartPos();
      float worldDist = (partWorldPos - localPos).magnitude();
      if (worldDist > storage::max_aimbot_distance)
        continue;
    }
    Engine::Vector2 screenPos =
        Engine::WorldToScreen(partWorldPos, dimensions, viewMatrix);

    float cursorDist = (screenPos - cursor).getMagnitude();
    if (cursorDist < shortestDistance) {
      shortestDistance = cursorDist;
      closestPlayer = player;
    }
  }

  return closestPlayer;
}

Engine::Vector3 normalize(const Engine::Vector3 &vec) {
  float length = sqrt(vec.x * vec.x + vec.y * vec.y + vec.z * vec.z);
  return (length != 0)
             ? Engine::Vector3{vec.x / length, vec.y / length, vec.z / length}
             : vec;
}

Engine::Vector3 crossProduct(const Engine::Vector3 &vec1,
                             const Engine::Vector3 &vec2) {
  return {vec1.y * vec2.z - vec1.z * vec2.y, vec1.z * vec2.x - vec1.x * vec2.z,
          vec1.x * vec2.y - vec1.y * vec2.x};
}

Engine::Matrix3x3 LookAtToMatrix(const Engine::Vector3 &cameraPosition,
                                 const Engine::Vector3 &targetPosition) {
  Engine::Vector3 forward =
      normalize(Engine::Vector3{(targetPosition.x - cameraPosition.x),
                                (targetPosition.y - cameraPosition.y),
                                (targetPosition.z - cameraPosition.z)});
  Engine::Vector3 right = normalize(crossProduct({0, 1, 0}, forward));
  Engine::Vector3 up = crossProduct(forward, right);

  Engine::Matrix3x3 lookAtMatrix{};
  lookAtMatrix.data[0] = -right.x;
  lookAtMatrix.data[1] = up.x;
  lookAtMatrix.data[2] = -forward.x;
  lookAtMatrix.data[3] = right.y;
  lookAtMatrix.data[4] = up.y;
  lookAtMatrix.data[5] = -forward.y;
  lookAtMatrix.data[6] = -right.z;
  lookAtMatrix.data[7] = up.z;
  lookAtMatrix.data[8] = -forward.z;

  return lookAtMatrix;
}

void performCameraAimbot(const Engine::Vector3 &targetPos,
                         const Engine::Vector3 &cameraPos) {
  auto camera = storage::workspace.FindFirstChildOfClass("Camera");
  Engine::Matrix3x3 currentRotation = camera.GetCameraRotation();
  Engine::Matrix3x3 targetMatrix = LookAtToMatrix(cameraPos, targetPos);

  if (globals::smoothenable) {
    float smoothX = std::clamp(storage::smoothness_x, 1.0f, 100.0f);
    float smoothY = std::clamp(storage::smoothness_y, 1.0f, 100.0f);

    if (smoothX <= 1.1f && smoothY <= 1.1f) {
      camera.WriteRot(targetMatrix);
    } else {
      Engine::Matrix3x3 result{};
      for (int i = 0; i < 9; ++i) {
        float factor = (i % 3 == 0) ? (1.0f / smoothX) : (1.0f / smoothY);
        result.data[i] =
            currentRotation.data[i] +
            (targetMatrix.data[i] - currentRotation.data[i]) * factor;
      }
      camera.WriteRot(result);
    }
  } else {
    camera.WriteRot(targetMatrix);
  }

  if (globals::shake) {
    Engine::Matrix3x3 shaken = targetMatrix;
    for (int i = 0; i < 9; ++i) {
      if (i % 3 == 0) {
        shaken.data[i] +=
            (float(rand() % int(globals::shakeX * 2 + 1) - globals::shakeX) *
             0.001f);
      } else {
        shaken.data[i] +=
            (float(rand() % int(globals::shakeY * 2 + 1) - globals::shakeY) *
             0.001f);
      }
    }
    camera.WriteRot(shaken);
  }
}

Engine::Vector2 Bezier2D(const Engine::Vector2 &start,
                         const Engine::Vector2 &control,
                         const Engine::Vector2 &end, float t) {
  float u = 1 - t;
  return {u * u * start.x + 2 * u * t * control.x + t * t * end.x,
          u * u * start.y + 2 * u * t * control.y + t * t * end.y};
}

static bool foundTarget = false;

void performMouseAimbot(const Engine::Vector2 &screenCoords,
                        HWND robloxWindow) {
  POINT cursorPos;
  GetCursorPos(&cursorPos);
  ScreenToClient(robloxWindow, &cursorPos);

  float deltaX = screenCoords.x - cursorPos.x;
  float deltaY = screenCoords.y - cursorPos.y;

  float aimSensitivity = std::clamp(globals::aimbotSensitivity, 0.1f, 2.0f);

  if (globals::beizer) {
    Engine::Vector2 currentPos = {static_cast<float>(cursorPos.x),
                                  static_cast<float>(cursorPos.y)};
    Engine::Vector2 midPoint = {(currentPos.x + screenCoords.x) / 2.f,
                                (currentPos.y + screenCoords.y) / 2.f - 30.f};
    static float t = 0.f;
    t += 0.1f;
    if (t > 1.f)
      t = 1.f;
    Engine::Vector2 bezierPos = Bezier2D(currentPos, midPoint, screenCoords, t);
    deltaX = bezierPos.x - currentPos.x;
    deltaY = bezierPos.y - currentPos.y;
  } else if (globals::smoothenable) {
    float smooth = std::clamp(storage::smoothness, 1.0f, 100.0f);
    if (smooth > 1.1f) {
      deltaX /= smooth;
      deltaY /= smooth;
    }
  }

  if (globals::shake) {
    deltaX += float(rand() % (int)(globals::shakeX * 2 + 1)) - globals::shakeX;
    deltaY += float(rand() % (int)(globals::shakeY * 2 + 1)) - globals::shakeY;
  }

  deltaX *= aimSensitivity;
  deltaY *= aimSensitivity;

  if (globals::flickbot) {
    INPUT input = {};
    input.type = INPUT_MOUSE;
    input.mi.dx = static_cast<LONG>(deltaX);
    input.mi.dy = static_cast<LONG>(deltaY);
    input.mi.dwFlags = MOUSEEVENTF_MOVE;
    SendInput(1, &input, sizeof(INPUT));
    return;
  }

  if (std::isfinite(deltaX) && std::isfinite(deltaY) &&
      (std::abs(deltaX) >= 0.3f || std::abs(deltaY) >= 0.3f)) {

    INPUT input = {};
    input.type = INPUT_MOUSE;
    input.mi.dx = static_cast<LONG>(deltaX);
    input.mi.dy = static_cast<LONG>(deltaY);
    input.mi.dwFlags = MOUSEEVENTF_MOVE;

    SendInput(1, &input, sizeof(INPUT));
  }
}

Engine::Vector3 AddVector3(const Engine::Vector3 &first,
                           const Engine::Vector3 &sec) {
  return {first.x + sec.x, first.y + sec.y, first.z + sec.z};
}

Engine::Vector3 DivVector3(const Engine::Vector3 &first,
                           const Engine::Vector3 &sec) {
  return {first.x / sec.x, first.y / sec.y, first.z / sec.z};
}

void run(Engine::PlayerIns player, const Engine::Vector3 &resolvedVelocity) {
  Engine::Vector3 hit_position_3D;
  Engine::Instance hitbox;

  Engine::PlayerIns local = storage::localplayer;

  const HWND rblx = FindWindowA(nullptr, "roblox");

  POINT cursor;
  GetCursorPos(&cursor);
  ScreenToClient(FindWindowA(nullptr, "roblox"), &cursor);

  if (storage::closest_part) {
    hitbox = getClosestPart(player, cursor);
  } else {
    static Engine::Instance Engine::PlayerIns::*partMap[] = {
        &Engine::PlayerIns::head,         &Engine::PlayerIns::rootPart,
        &Engine::PlayerIns::upperTorso,   &Engine::PlayerIns::lowerTorso,
        &Engine::PlayerIns::leftHand,     &Engine::PlayerIns::rightHand,
        &Engine::PlayerIns::leftUpperArm, &Engine::PlayerIns::rightUpperArm,
        &Engine::PlayerIns::leftUpperLeg, &Engine::PlayerIns::rightUpperLeg,
        &Engine::PlayerIns::leftFoot,     &Engine::PlayerIns::rightFoot};
    hitbox = player.*partMap[std::clamp(storage::aimpart, 0, 11)];
  }

  hit_position_3D = hitbox.GetPartPos();
  if (!isWithinFOV(hit_position_3D))
    return;

  if (storage::wall_check &&
      !storage::wallcheck.isVisible(storage::localplayer.rootPart.GetPartPos(),
                                    hit_position_3D))
    return;

  auto visual = storage::visualengine;
  Engine::Matrix4x4 view = visual.GetViewMatrix();
  Engine::Vector2 dimensions = visual.GetDimensions();

  if (storage::gameid == 2132866904) {
    Engine::Vector3 headPos = player.head.GetPartPos();
    headPos.y -= 5.0f;
    hit_position_3D = headPos;
  }

  if (storage::camera_prediction) {
    Engine::Vector3 velocity =
        (storage::resolver ? resolvedVelocity : hitbox.GetPartvelocity());

    if (std::isfinite(velocity.x) && std::isfinite(velocity.y) &&
        std::isfinite(velocity.z) && std::abs(velocity.x) < 1000.0f &&
        std::abs(velocity.y) < 1000.0f && std::abs(velocity.z) < 1000.0f) {

      float predictionScale = 0.016f;
      hit_position_3D.x +=
          velocity.x * predictionScale * storage::camera_prediction_x;
      hit_position_3D.y +=
          velocity.y * predictionScale * storage::camera_prediction_y;
      hit_position_3D.z +=
          velocity.z * predictionScale * storage::camera_prediction_x;
    }
  }

  if (storage::shake)
    hit_position_3D += Random_Vector3(storage::shake_x, storage::shake_y);
  Engine::Vector2 screenPos =
      Engine::WorldToScreen(hit_position_3D, dimensions, view);
  if (screenPos.x == -1 || screenPos.y == -1)
    return;

  float smoothness = storage::smoothness_camera / 100.0f;
  smoothness = Ease(smoothness);

  Engine::Vector2 screenCoords =
      Engine::WorldToScreen(hit_position_3D, dimensions, view);
  if (screenCoords.x == -1.0f || screenCoords.y == -1.0f) {
    foundTarget = false;
    return;
  }

  if (storage::aimbot_type == 0) {
    Engine::Vector2 cursorVec = {static_cast<float>(cursor.x),
                                 static_cast<float>(cursor.y)};
    Engine::Vector2 diff = screenPos - cursorVec;

    float smooth =
        storage::mouse_smoothness < 1.0f ? 1.0f : storage::mouse_smoothness;
    Engine::Vector2 delta = {(diff.x / smooth) * storage::mouse_sensitivity,
                             (diff.y / smooth) * storage::mouse_sensitivity};

    if (std::abs(delta.x) >= 0.1f || std::abs(delta.y) >= 0.1f) {
      INPUT input = {};
      input.type = INPUT_MOUSE;
      input.mi.dx = static_cast<LONG>(delta.x);
      input.mi.dy = static_cast<LONG>(delta.y);
      input.mi.dwFlags = MOUSEEVENTF_MOVE;
      SendInput(1, &input, sizeof(INPUT));
    }
  }

  if (storage::aimbot_type == 1) {
    Engine::Instance camera =
        storage::workspace.FindFirstChildOfClass("Camera");
    Engine::Vector3 cameraPos = camera.GetCameraPos();

    performCameraAimbot(hit_position_3D, cameraPos);
  }
}

Engine::PlayerIns aimbot_target;

void hooks::hook_aimbot() {
  Engine::PlayerIns saved_player{};
  Engine::Vector3 velocity{};
  Engine::PlayerIns current_player;
  bool isAimboting = false;

  const HWND rblx = FindWindowA(nullptr, "roblox");
  if (!rblx)
    return;

  while (true) {
    // Check if disabled first - sleep longer to save CPU when not in use
    if (!storage::aimbot || !storage::aimbotkeybind.enabled) {
      isAimboting = false;
      aimbot_target.address = 0;
      storage::aimbot_target_addr = 0;
      std::this_thread::sleep_for(std::chrono::milliseconds(50)); // Long sleep when disabled
      continue;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(8));

    if (storage::sticky && isAimboting && saved_player.address) {
      bool invalid = false;
      bool sameTeam =
          (saved_player.team.address != 0 &&
           storage::localplayer.team.address != 0 &&
           saved_player.team.address == storage::localplayer.team.address);
      if (storage::camlock_teamcheck && sameTeam)
        invalid = true;
      else if (storage::aimbot_knockcheck &&
               saved_player.knockedOut.getBoolFromValue())
        invalid = true;

      if (invalid) {
        isAimboting = false;
        saved_player.address = 0;
        current_player = getClosestPlayerFromCursor();
      } else {
        current_player = saved_player;
      }
    } else {
      current_player = getClosestPlayerFromCursor();
    }

    if (!current_player.address) {
      isAimboting = false;
      aimbot_target.address = 0;
      storage::aimbot_target_addr = 0;
      continue;
    }

    aimbot_target = current_player;
    storage::aimbot_target_addr = current_player.address;
    if (storage::closest_part)
      InitializePlayerParts(current_player);

    run(current_player, velocity);

    saved_player = current_player;
    isAimboting = true;
  }
}
