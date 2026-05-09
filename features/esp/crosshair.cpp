#include "crosshair.h"
#include "../aimbot/silent.h"
#include "core/storage/include.h"
#include "esp.h"
#include <Windows.h>
#include <algorithm>
#include <cmath>

namespace crosshair {
void render() {
  if (!storage::visuals::crosshair_enabled)
    return;
  if (!GetForegroundWindow())
    return;

  static float fadeParam = 0.0f;
  static int lastStyle = storage::visuals::crosshair_styleIdx;
  static float currentSpeed = 0.0f;
  static float angle = 0.0f;
  static ImVec2 lastPos = ImVec2(0, 0);

  float time = ImGui::GetTime();
  float dt = ImGui::GetIO().DeltaTime;
  POINT pt;

  GetCursorPos(&pt);
  ScreenToClient(GetForegroundWindow(), &pt);
  ImVec2 mpos{(float)pt.x, (float)pt.y};

  bool hasSilentTarget = false;
  ImVec2 silentTargetPos = mpos;

  bool silentAimActive =
      storage::silent_aim && storage::silentaimkeybind.enabled;

  if (silentAimActive && g_silent_found_target && g_silent_data_ready &&
      g_silent_partpos.x > 0.0f && g_silent_partpos.y > 0.0f &&
      g_silent_partpos.x < 10000.0f && g_silent_partpos.y < 10000.0f) {
    silentTargetPos = ImVec2(g_silent_partpos.x, g_silent_partpos.y);
    hasSilentTarget = true;
  } else if (silentAimActive && silentaim_target.address != 0) {
    Engine::Instance headPart = silentaim_target.head;
    if (headPart.address != 0) {
      Engine::Vector3 headPos = headPart.GetPartPos();

      Engine::Instance visualengine = storage::visualengine;
      Engine::Vector2 dimensions = visualengine.GetDimensions();
      Engine::Matrix4x4 viewMatrix = visualengine.GetViewMatrix();
      Engine::Vector2 screenPos =
          Engine::WorldToScreen(headPos, dimensions, viewMatrix);

      if (screenPos.x != -1.0f && screenPos.y != -1.0f) {
        silentTargetPos = ImVec2(screenPos.x, screenPos.y);
        hasSilentTarget = true;
      }
    }
  }

  float lerpSpeed = 5.0f;
  ImVec2 targetPos = hasSilentTarget ? silentTargetPos : mpos;
  mpos.x = lastPos.x +
           (targetPos.x - lastPos.x) * std::clamp(lerpSpeed * dt, 0.0f, 1.0f);
  mpos.y = lastPos.y +
           (targetPos.y - lastPos.y) * std::clamp(lerpSpeed * dt, 0.0f, 1.0f);
  lastPos = mpos;

  if (storage::visuals::crosshair_styleIdx != lastStyle) {
    fadeParam = 0.0f;
    lastStyle = storage::visuals::crosshair_styleIdx;
  }

  float targetSpeed = storage::visuals::crosshair_baseSpeed * 0.5f;
  switch (storage::visuals::crosshair_styleIdx) {
  case 0:
    targetSpeed = storage::visuals::crosshair_baseSpeed * 0.5f;
    fadeParam = 0.0f;
    break;
  case 1:
    fadeParam = std::clamp(
        fadeParam + dt / storage::visuals::crosshair_fadeDuration, 0.0f, 1.0f);
    targetSpeed = storage::visuals::crosshair_baseSpeed *
                  (0.1f + 0.9f * (fadeParam * fadeParam));
    break;
  }

  const float accel = 5.0f;
  currentSpeed +=
      (targetSpeed - currentSpeed) * std::clamp(accel * dt, 0.0f, 1.0f);
  angle += currentSpeed * (3.1415926f / 180.0f) * dt;
  if (angle > 6.2831853f)
    angle -= 6.2831853f;

  float showGap = storage::visuals::crosshair_gap;
  if (storage::visuals::crosshair_gapTween) {
    float raw = fmodf(time * storage::visuals::crosshair_gapSpeed, 2.0f);
    float e = raw < 1.0f ? (1.0f - (1.0f - raw) * (1.0f - raw))
                         : 1.0f - ((raw - 1.0f) * (raw - 1.0f));
    showGap = storage::visuals::crosshair_gap * e;
  }

  ImDrawList *dl = ImGui::GetForegroundDrawList();
  
  // Enable anti-aliasing for smooth rotation if requested, 
  // but keep coordinates snapped for static precision.
  ImDrawListFlags originalFlags = dl->Flags;
  dl->Flags |= ImDrawListFlags_AntiAliasedLines;

  ImU32 colLine = IM_COL32((int)(storage::visuals::crosshair_color[0] * 255),
                           (int)(storage::visuals::crosshair_color[1] * 255),
                           (int)(storage::visuals::crosshair_color[2] * 255),
                           (int)(storage::visuals::crosshair_color[3] * 255));
  ImU32 colOut = IM_COL32(0, 0, 0, 255);
  
  // Force integer thickness for zero-blur
  float thick = std::floor(storage::visuals::crosshair_thickness + 0.5f);
  if (thick < 1.0f) thick = 1.0f;

  ImVec2 center = ImVec2(std::round(mpos.x), std::round(mpos.y));

  for (int i = 0; i < 4; ++i) {
    float a = angle + i * 3.1415926f * 0.5f;
    ImVec2 d{cosf(a), sinf(a)};
    
    // Snap points to pixel grid
    ImVec2 p0{std::round(center.x + d.x * showGap), std::round(center.y + d.y * showGap)};
    ImVec2 p1{std::round(center.x + d.x * (showGap + storage::visuals::crosshair_size)),
              std::round(center.y + d.y * (showGap + storage::visuals::crosshair_size))};

    // Strict high-contrast shadow (2px offset for outer glow)
    dl->AddLine(p0, p1, colOut, thick + 2.0f);
    dl->AddLine(p0, p1, colLine, thick);
  }

  const char *txt = "Scathe";
  const char *xyz = ".cc";

  ImFont *logoFont = Visualize.visitor;
  float fontSize = 13.0f;

  ImVec2 wordSize = logoFont->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, txt);
  ImVec2 xyzSize = logoFont->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, xyz);
  float totalW = wordSize.x + xyzSize.x;

  ImVec2 base{std::round(center.x - totalW * 0.5f),
              std::round(center.y + showGap + storage::visuals::crosshair_size + 4)};

  ImU32 colW = IM_COL32(255, 255, 255, 255);
  ImU32 colAccent = colLine;

  // Render logo with high-quality 8-direction outline
  Visualize.DrawTextWithSpacingAndOutline(dl, logoFont, fontSize, base, colW, colOut, txt, 0.0f);
  Visualize.DrawTextWithSpacingAndOutline(dl, logoFont, fontSize, ImVec2(base.x + wordSize.x, base.y), colAccent, colOut, xyz, 0.0f);

  dl->Flags = originalFlags;
}
} // namespace crosshair
