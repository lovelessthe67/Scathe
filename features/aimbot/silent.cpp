#include <Windows.h>
#include <cmath>
#include <future>
#include <immintrin.h>
#include <mmsystem.h>
#include <random>
#include <thread>
#include <vector>
#pragma comment(lib, "winmm.lib")
#include "engine/Players/offsets.hpp"
#include "engine/mem_module/memory.hpp"
#include "core/storage/Globals.hpp"
#include "core/storage/include.h"
#include "silent.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Global cache moved to storage::g_cached_viewmatrix

static bool isWithinFOV_Stabilized(Engine::PlayerIns &player,
                                   Engine::Vector2 &screen_pos, float fov) {
  POINT cursor_point;
  if (!GetCursorPos(&cursor_point))
    return false;
  HWND rblx = FindWindowA(0, "roblox");
  if (rblx)
    ScreenToClient(rblx, &cursor_point);
  Engine::Vector2 cursor = {(float)cursor_point.x, (float)cursor_point.y};
  return (screen_pos - cursor).getMagnitude() <= fov;
}

Engine::Instance getFreeAimClosestPart(Engine::PlayerIns &player,
                                       const POINT &cursor_point) {
  Engine::Vector2 dimensions = storage::visualengine.GetDimensions();
  Engine::Matrix4x4 view_matrix = storage::visualengine.GetViewMatrix();
  Engine::Vector2 cursor = {static_cast<float>(cursor_point.x),
                            static_cast<float>(cursor_point.y)};
  float min_distance = FLT_MAX;
  Engine::Instance closest_part;

  if (!player.head.address)
    return closest_part;

  Engine::Vector3 part_position = player.head.GetPartPos();

  Engine::Vector2 part_screen_position =
      Engine::WorldToScreen(part_position, dimensions, view_matrix);

  float distance = (part_screen_position - cursor).getMagnitude();

  if (distance < min_distance) {
    min_distance = distance;
    closest_part = player.head;
  }

  return closest_part;
}
Engine::Instance getClosestPoint(Engine::PlayerIns &player,
                                 const POINT &cursor_point) {
  Engine::Vector2 dimensions = storage::visualengine.GetDimensions();
  Engine::Matrix4x4 view_matrix = storage::visualengine.GetViewMatrix();
  Engine::Vector2 cursor = {static_cast<float>(cursor_point.x),
                            static_cast<float>(cursor_point.y)};
  std::vector<Engine::Instance> parts = {
      player.head,          player.rootPart,     player.upperTorso,
      player.lowerTorso,    player.leftUpperLeg, player.leftFoot,
      player.rightFoot,     player.leftUpperArm, player.leftHand,
      player.rightUpperArm, player.rightHand,
  };
  float min_distance = FLT_MAX;
  Engine::Instance closest_point;

  for (const auto &part : parts) {
    if (!part.address)
      continue;

    Engine::Vector3 part_position = part.GetPartPos();

    Engine::Vector2 part_screen_position =
        Engine::WorldToScreen(part_position, dimensions, view_matrix);

    float distance = (part_screen_position - cursor).getMagnitude();

    if (distance < min_distance) {
      min_distance = distance;
      closest_point = part;
    }
  }

  return closest_point;
}

static Engine::PlayerIns
getClosestPlayerFromCursor(const Engine::Matrix4x4 &use_matrix) {
  POINT cursor_point;
  HWND rblxWnd = FindWindowA(nullptr, "roblox");
  if (!rblxWnd) {
    return {};
  }

  if (!GetCursorPos(&cursor_point)) {
    return {};
  }

  if (!ScreenToClient(rblxWnd, &cursor_point)) {
    return {};
  }

  Engine::Vector2 cursor = {static_cast<float>(cursor_point.x),
                            static_cast<float>(cursor_point.y)};

  std::vector<Engine::PlayerIns> players_snapshot = storage::player_cache;
  if (players_snapshot.empty()) {
    return {};
  }

#undef max
  Engine::PlayerIns closestPlayer{};
  float shortestDistance = std::numeric_limits<float>::max();

  Engine::Vector2 dimensions = storage::visualengine.GetDimensions();

  int checked = 0;
  for (Engine::PlayerIns &player : players_snapshot) {
    if (player.address == 0 || player.address == storage::localplayer.address)
      continue;

    bool sameTeam =
        (player.team.address != 0 && storage::localplayer.team.address != 0 &&
         player.team.address == storage::localplayer.team.address);
    if (storage::silent_teamcheck && sameTeam)
      continue;

    if (storage::knockhecksilent &&
        (player.knockedOut.getBoolFromValue() || player.health <= 0.0f))
      continue;

    Engine::Instance part = player.upperTorso;
    if (storage::silent_closest_to_mouse) { // Closest to Mouse Checkbox
      part = getClosestPoint(player, cursor_point);
    } else if (storage::silent_hitbox == 0) { // Head
      part = player.head;
    }

    if (!part.address)
      continue;

    Engine::Vector3 partPosition = part.GetPartPos();
    Engine::Vector2 partScreen =
        Engine::WorldToScreen(partPosition, dimensions, use_matrix);

    if (partScreen.x == 0 && partScreen.y == 0) {
      continue;
    }

    if (storage::silent_use_fov &&
        (partScreen - cursor).getMagnitude() > storage::silent_fov)
      continue;

    if (storage::silent_wallcheck &&
        !storage::wallcheck.isVisible(
            storage::localplayer.rootPart.GetPartPos(), partPosition))
      continue;

    float distance_from_cursor = (partScreen - cursor).getMagnitude();
    if (distance_from_cursor < shortestDistance) {
      shortestDistance = distance_from_cursor;
      closestPlayer = player;
    }

    checked++;
  }

  return closestPlayer;
}

static Engine::PlayerIns getClosestPlayerFromCursor() {
  return getClosestPlayerFromCursor(storage::visualengine.GetViewMatrix());
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
  return (magnitude <= storage::silent_fov);
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

static void run(Engine::PlayerIns player, POINT cursor_point) {
  bool sameTeam =
      (player.team.address != 0 && storage::localplayer.team.address != 0 &&
       player.team.address == storage::localplayer.team.address);
  if (storage::silent_teamcheck && sameTeam)
    return;

  Engine::Instance visualengine = storage::visualengine;
  Engine::Vector2 dimensions = visualengine.GetDimensions();
  Engine::Matrix4x4 view_matrix = visualengine.GetViewMatrix();

  Engine::Instance camera_parent =
      storage::workspace.FindFirstChildOfClass("Camera");
  Engine::Instance camera = camera_parent.FindFirstChild("Part");

  auto pos = camera.GetPartPos();
  auto rot = camera.GetRotation();

  Engine::Vector3 target_pos;
  if (storage::silent_closest_to_mouse) {
    target_pos = getClosestPoint(player, cursor_point).GetPartPos();
  } else if (storage::silent_hitbox == 0) {
    target_pos = player.head.GetPartPos();
  } else if (storage::silent_hitbox == 1) {
    target_pos = player.upperTorso.GetPartPos();
  } else if (storage::silent_hitbox == 2) {
    std::vector<Engine::Instance> parts = {player.head, player.upperTorso,
                                           player.lowerTorso};
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, parts.size() - 1);
    target_pos = parts[dis(gen)].GetPartPos();
  } else {
    target_pos = player.head.GetPartPos();
  }

  Engine::Matrix3x3 target_matrix = Look_At_To_Matrix(pos, target_pos);
  target_matrix = {
      target_matrix.data[0],
      target_matrix.data[1],
      target_matrix.data[2],
      target_matrix.data[3],
      0.01f,
      target_matrix.data[5],
      target_matrix.data[6],
      target_matrix.data[7],
      target_matrix.data[8],
  };

  camera.SetRotation(target_matrix);

  std::this_thread::sleep_for(std::chrono::milliseconds(2));
}

std::uint64_t c_silent_help::cached_input_object = 0;

void c_silent_help::set_frame_pos_x(uint64_t position) {
  mem::write<uint64_t>(address + Offsets::Silent::FramePositionOffsetX,
                       position);
}

void c_silent_help::set_frame_pos_y(uint64_t position) {
  mem::write<uint64_t>(address + Offsets::Silent::FramePositionOffsetY,
                       position);
}

std::uint64_t get_current_input_object(std::uint64_t base_address) {
  std::uint64_t object_address = mem::read<std::uint64_t>(
      base_address + Offsets::MouseService::InputObject +
      sizeof(std::shared_ptr<void *>));

  return object_address;
}

#include <thread>

void cached_input_objectzz() {
  std::uint64_t cachedobj = 0;
  while (true) {
    if (g_mouseservice) {
      std::uint64_t inputobj =
          get_current_input_object(g_mouseservice.get()->address);

      if (inputobj != 0 && inputobj != 0xffffffffffffffff) {
        c_silent_help::cached_input_object = inputobj;
        cachedobj = inputobj;
      }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
}

void c_silent_help::initialize_mouse_service(std::uint64_t address) {
  cached_input_object = get_current_input_object(address);

  if (cached_input_object && cached_input_object != 0xFFFFFFFFFFFFFFFF) {
    const char *base_pointer =
        reinterpret_cast<const char *>(cached_input_object);

    _mm_prefetch(base_pointer + Offsets::MouseService::MousePosition,
                 _MM_HINT_T0);
    _mm_prefetch(base_pointer + Offsets::MouseService::MousePosition +
                     sizeof(Engine::Vector2),
                 _MM_HINT_T0);
  }
}

void c_silent_help::write_mouse_position(std::uint64_t address, float x,
                                         float y) {
  cached_input_object = get_current_input_object(address);
  if (cached_input_object != 0 && cached_input_object != 0xFFFFFFFFFFFFFFFF) {
    Engine::Vector2 new_position = {x, y};

    mem::write<Engine::Vector2>(cached_input_object +
                                    Offsets::MouseService::MousePosition,
                                new_position);
  }
}

static bool should_silent_aim_be_active() {
  if (!storage::silent_aim || !storage::silentaimkeybind.enabled) {
    return false;
  }
  bool isPF = (storage::gameid == 292439477 || storage::gameid == 113491250);
  if (isPF) {
    return (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
  }
  return true;
}

void rbx::silent::silent_aim_1() {
  Engine::PlayerIns target{};

  SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);

  HWND roblox_window = FindWindowA(0, "Roblox");

  for (;;) {
    std::this_thread::sleep_for(std::chrono::milliseconds(2));

    g_mouseservice = std::make_unique<Engine::Instance>(
        storage::datamodel.FindFirstChildOfClass("MouseService"));

    if (!g_mouseservice || !storage::datamodel.address ||
        !storage::visualengine.address) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      continue;
    }

    if (!should_silent_aim_be_active()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      g_silent_data_ready = false;
      if (g_silent_cached_target.address != 0) {
        g_silent_cached_target.address = 0;
      }
      target.address = 0;
      storage::silent_target_addr = 0;
      g_silent_found_target = false;
      g_silent_target_needs_reset = false;
      continue;
    }

    if (!storage::datamodel.address) {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      continue;
    }

    Engine::Instance players =
        storage::datamodel.FindFirstChildOfClass("Players");
    if (players.address == 0) {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      continue;
    }

    Engine::Instance local_player = Engine::Instance(mem::read<std::uint64_t>(
        players.address + Offsets::Player::LocalPlayer));

    if (local_player.address == 0) {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      continue;
    }

    static int aim_instance_check_counter = 0;
    if (aim_instance_check_counter++ % 10 == 0) {
      Engine::Instance player_gui = local_player.FindFirstChild("PlayerGui");
      if (player_gui.address != 0) {
        Engine::Instance main_screen_gui =
            player_gui.FindFirstChild("MainScreenGui");
        if (main_screen_gui.address == 0)
          main_screen_gui = player_gui.FindFirstChild("Main Screen");

        if (main_screen_gui.address != 0) {
          g_silent_aim_instance =
              Engine::Instance(main_screen_gui.FindFirstChild("Aim"));
        }
      }
    }

    bool isPF = (storage::gameid == 292439477 || storage::gameid == 113491250);
    bool forceNewTarget = storage::silent_closest_to_mouse && !isPF;
    if (forceNewTarget || !g_silent_found_target ||
        g_silent_cached_target.address == 0) {
      target = getClosestPlayerFromCursor();
      g_silent_cached_last_target = target;
      g_silent_found_target = target.head.address != 0;
      g_silent_cached_target = target;
      storage::silent_target_addr = target.address;
    } else {
      target = g_silent_cached_target;
      storage::silent_target_addr = target.address;
      bool sameTeam =
          (target.team.address != 0 && storage::localplayer.team.address != 0 &&
           target.team.address == storage::localplayer.team.address);
      if (storage::silent_teamcheck && sameTeam) {
        g_silent_found_target = false;
        g_silent_cached_target.address = 0;
        continue;
      }
      if (storage::knockhecksilent &&
          (target.knockedOut.getBoolFromValue() || target.health <= 0.0f)) {
        g_silent_found_target = false;
        g_silent_cached_target.address = 0;
        continue;
      }
      if (storage::silent_grabbedcheck && target.ifGrabbed.getBoolFromValue()) {
        g_silent_found_target = false;
        g_silent_cached_target.address = 0;
        continue;
      }
      if (storage::silent_wallcheck) {
        Engine::Vector3 target_check_pos;
        if (storage::silent_hitbox == 0)
          target_check_pos = target.head.GetPartPos();
        else if (storage::silent_hitbox == 1)
          target_check_pos = target.upperTorso.GetPartPos();
        else
          target_check_pos = target.rootPart.GetPartPos();

        if (!storage::wallcheck.isVisible(
                storage::localplayer.rootPart.GetPartPos(), target_check_pos)) {
          g_silent_found_target = false;
          g_silent_cached_target.address = 0;
          continue;
        }
      }
      if (storage::silent_use_fov) {
        Engine::Vector2 dimensions = storage::visualengine.GetDimensions();
        Engine::Matrix4x4 view_matrix = storage::visualengine.GetViewMatrix();
        Engine::Vector2 screen_pos =
            Engine::WorldToScreen(g_silent_cached_target.rootPart.GetPartPos(),
                                  dimensions, view_matrix);

        if (!isWithinFOV_Stabilized(g_silent_cached_target, screen_pos,
                                    storage::silent_fov)) {
          g_silent_found_target = false;
          g_silent_cached_target.address = 0;
          continue; // Reset target if out of FOV
        }
      }
    }

    if (g_silent_cached_target.address == 0)
      continue;

    Engine::Vector3 target_pos = g_silent_cached_target.head.GetPartPos();

    if (g_silent_found_target && g_silent_knock_check && g_silent_auto_switch) {
      Engine::Instance model_instance =
          Engine::Instance(target.address).GetModelInstance();
      if (model_instance.address != 0) {
        Engine::Instance body_effects =
            model_instance.FindFirstChild("BodyEffects");
        if (body_effects.address != 0) {
          Engine::Instance ko = body_effects.FindFirstChild("K.O");
          if (ko.address != 0) {
            bool ko_value = mem::read<bool>(ko.address + Offsets::Misc::Value);
            if (ko_value) {
              g_silent_found_target = false;
              continue;
            }
          }
        }
      }
    }

    if (g_silent_found_target && g_silent_cached_target.address != 0 &&
        storage::visualengine.address) {
      Engine::Instance target_part;
      if (storage::silent_closest_to_mouse) {
        POINT pt;
        GetCursorPos(&pt);
        HWND rblx = FindWindowA(0, "roblox");
        if (rblx)
          ScreenToClient(rblx, &pt);
        target_part = getClosestPoint(g_silent_cached_target, pt);
      } else if (storage::silent_hitbox == 0)
        target_part = g_silent_cached_target.head;
      else if (storage::silent_hitbox == 1)
        target_part = g_silent_cached_target.upperTorso;
      else {
        target_part = g_silent_cached_target.head;
      }

      if (target_part.address != 0) {
        Engine::Vector3 part_3d = target_part.GetPartPos();
        g_silent_partpos_3d = part_3d; // Store 3D position for bullet tracers
        Engine::Matrix4x4 view = storage::visualengine.GetViewMatrix();
        Engine::Vector2 dims = storage::visualengine.GetDimensions();

        g_silent_partpos = Engine::WorldToScreen(part_3d, dims, view);
        POINT cursor_point;
        GetCursorPos(&cursor_point);
        if (roblox_window)
          ScreenToClient(roblox_window, &cursor_point);

        g_silent_cached_position_x = static_cast<std::uint64_t>(cursor_point.x);
        g_silent_cached_position_y = static_cast<std::uint64_t>(
            dims.y - std::abs(dims.y - static_cast<float>(cursor_point.y)) -
            58);
        g_silent_data_ready = true;
      } else {
        g_silent_data_ready = false;
      }
    } else {
      g_silent_data_ready = false;
    }
  }
}

void rbx::silent::silent_aim_2() {
  c_silent_help mouse_service_instance{};
  bool mouse_service_initialized = false;

  for (;;) {
    if (!g_mouseservice) {
      mouse_service_initialized = false;
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      continue;
    }

    if (!should_silent_aim_be_active()) {
      mouse_service_initialized = false;
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      if (g_silent_cached_target.address != 0)
        g_silent_target_needs_reset = true;
      continue;
    }

    if (g_silent_cached_target.address != 0 && g_silent_data_ready &&
        g_mouseservice && g_mouseservice->address != 0) {
      if (g_silent_partpos.x < 0.0f || g_silent_partpos.y < 0.0f ||
          g_silent_partpos.x > 10000.0f || g_silent_partpos.y > 10000.0f) {
        continue;
      }

      try {
        if (!mouse_service_initialized) {
          mouse_service_instance.initialize_mouse_service(
              g_mouseservice->address);
          mouse_service_initialized = true;
        }

        if (g_silent_spoof_mouse && g_silent_aim_instance.address != 0) {
          c_silent_help aim_helper(g_silent_aim_instance.address);
          aim_helper.set_frame_pos_x(g_silent_cached_position_x);
          aim_helper.set_frame_pos_y(g_silent_cached_position_y);
        }

        mouse_service_instance.write_mouse_position(
            g_mouseservice->address, g_silent_partpos.x, g_silent_partpos.y);
      } catch (...) {
        mouse_service_initialized = false;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
      }
    }
  }
}

Engine::PlayerIns silentaim_target;
void hooks::silentaim() {
  if (storage::gameid != 113491250) {
    std::thread(rbx::silent::silent_aim_1).detach();
    std::thread(rbx::silent::silent_aim_2).detach();
    return;
  }

  Engine::PlayerIns savedPlayer{};
  bool has_target = false;

  while (true) {
    // Check if disabled first - sleep longer when not in use
    if (!storage::silent_aim || !storage::silentaimkeybind.enabled) {
      has_target = false;
      savedPlayer.address = 0;
      silentaim_target.address = 0;
      storage::silent_target_addr = 0;
      std::this_thread::sleep_for(std::chrono::milliseconds(50)); // Long sleep when disabled
      continue;
    }
    
    // Active: use 5ms for responsiveness
    std::this_thread::sleep_for(std::chrono::milliseconds(5));

    // V3 STABILIZER: Cache the Matrix before any modifications
    storage::g_cached_viewmatrix = storage::visualengine.GetViewMatrix();
    storage::g_has_cached_matrix = true;

    POINT cursor_point;
    GetCursorPos(&cursor_point);

    Engine::PlayerIns currentTarget;

    // TARGET LOCKING: Stay on the same player
    if (has_target && savedPlayer.address != 0) {
      bool still_valid = true;
      if (storage::knockhecksilent &&
          (savedPlayer.knockedOut.getBoolFromValue() ||
           savedPlayer.health <= 0.0f))
        still_valid = false;

      bool sameTeam =
          (savedPlayer.team.address != 0 &&
           storage::localplayer.team.address != 0 &&
           savedPlayer.team.address == storage::localplayer.team.address);
      if (storage::silent_teamcheck && sameTeam)
        still_valid = false;

      if (still_valid) {
        currentTarget = savedPlayer;
      } else {
        has_target = false;
        savedPlayer.address = 0;
        currentTarget =
            getClosestPlayerFromCursor(storage::g_cached_viewmatrix);
      }
    } else {
      currentTarget = getClosestPlayerFromCursor(storage::g_cached_viewmatrix);
    }

    if (currentTarget.address == 0) {
      has_target = false;
      silentaim_target.address = 0;
      storage::silent_target_addr = 0;
      continue;
    }

    silentaim_target = currentTarget;
    storage::silent_target_addr = currentTarget.address;
    savedPlayer = currentTarget;
    has_target = true;

    // FOV CHECK using CACHED matrix to stop flickering
    if (storage::silent_use_fov) {
      Engine::Instance part;
      if (storage::silent_closest_to_mouse)
        part = getClosestPoint(currentTarget, cursor_point);
      else if (storage::silent_hitbox == 0)
        part = currentTarget.head;
      else
        part = currentTarget.upperTorso;

      if (part.address != 0) {
        Engine::Vector2 screen_pos = Engine::WorldToScreen(
            part.GetPartPos(), storage::visualengine.GetDimensions(),
            storage::g_cached_viewmatrix);
        if (!isWithinFOV_Stabilized(currentTarget, screen_pos,
                                    storage::silent_fov)) {
          if (!has_target)
            continue; // Only skip if we haven't locked yet, or we can decide to
                      // unlock here
        }
      }
    }

    silentaim_target = currentTarget;
    savedPlayer = currentTarget;
    has_target = true;

    run(currentTarget, cursor_point);
  }
}