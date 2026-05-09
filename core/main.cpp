#include "ui/helpers/helpers.h"
#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"
#include <chrono>
#include <d3d11.h>
#include <dwmapi.h>
#include <tchar.h>
#include <thread>

#include "core/Core.hpp"
#include "features/esp/WeaponIcon.hpp"
#include "features/esp/esp.h"
#include "features/esp/tahoma.h"
#include "features/esp/visitor.h"
#include "resources/fonts/tahoma_custom.h"
#include <shellapi.h>
#include "../project/resource.h"
#define WM_TRAYICON (WM_USER + 1)
#include "features/notifications/NotificationManager.hpp"
NOTIFYICONDATAW nid = {0};

void CreateTrayIcon(HWND hwnd) {
  nid.cbSize = sizeof(NOTIFYICONDATAW);
  nid.hWnd = hwnd;
  nid.uID = 1;
  nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
  nid.uCallbackMessage = WM_TRAYICON;
  nid.hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON1));
  wcscpy_s(nid.szTip, L"Scathe");
  Shell_NotifyIconW(NIM_ADD, &nid);
}

void ShowTrayMenu(HWND hwnd) {
  HMENU hMenu = CreatePopupMenu();
  AppendMenuW(hMenu, MF_STRING, 1, L"Show Console");
  AppendMenuW(hMenu, MF_STRING, 2, L"Hide Console");
  AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
  AppendMenuW(hMenu, MF_STRING, 3, L"Exit");
  POINT pt;
  GetCursorPos(&pt);
  SetForegroundWindow(hwnd);
  int cmd = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_NONOTIFY, pt.x, pt.y, 0,
                           hwnd, NULL);
  DestroyMenu(hMenu);
  HWND hConsole = GetConsoleWindow();
  if (cmd == 1 && hConsole)
    ShowWindow(hConsole, SW_SHOW);
  else if (cmd == 2 && hConsole)
    ShowWindow(hConsole, SW_HIDE);
  else if (cmd == 3)
    exit(0);
}
#include "core/auth/auth.h"
#include "features/explorer/explorer.h"
#include "core/storage/Globals.hpp"
#include "core/storage/include.h"

#pragma comment(lib, "dwmapi.lib")
#pragma comment(linker, "/subsystem:windows /ENTRY:mainCRTStartup")

#include "utils/DiscordRPC.hpp"

static ID3D11Device *g_pd3dDevice = nullptr;
static ID3D11DeviceContext *g_pd3dDeviceContext = nullptr;
static IDXGISwapChain *g_pSwapChain = nullptr;
static bool g_SwapChainOccluded = false;
static UINT g_ResizeWidth = 0, g_ResizeHeight = 0;
static ID3D11RenderTargetView *g_mainRenderTargetView = nullptr;

bool shouldExit = false;
bool g_menuVisible = true;
bool g_engineInitialized = false;

bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

ID3D11Device *GetD3DDevice() { return g_pd3dDevice; }

void InitializeEngine() {
  std::thread engineThread([]() {
    Engine::Initializer();
    g_engineInitialized = true;
  });
  engineThread.detach();
}

int main(int, char **) {
  AllocConsole();
  FILE *f;
  freopen_s(&f, "CONOUT$", "w", stdout);
  freopen_s(&f, "CONIN$", "r", stdin);

  HINSTANCE hInstance = GetModuleHandle(nullptr);
  HWND hConsole = GetConsoleWindow();
  if (hConsole)
    ShowWindow(hConsole, SW_SHOW);

  std::cout << "[*] Initializing KeyAuth...\n";
  
  // ===== KEYSYSTEM OFF FOR NOW =====
  // Temporarily bypassing authentication - everyone gets access
  AuthManager::bypassAuth();
  std::cout << "[+] Auth bypassed - access granted.\n";
  if (hConsole)
      ShowWindow(hConsole, SW_HIDE);
  
  /* // Original auth code - commented out for now
  if (AuthManager::initialize()) {
      std::cout << "[+] KeyAuth Initialized.\n";
      AuthManager::loginFlow();
      if (AuthManager::isAuthenticated()) {
          std::cout << "[+] Authenticated. Starting cheat...\n";
          if (hConsole)
              ShowWindow(hConsole, SW_HIDE);
      }
  } else {
      std::cout << "[-] Failed to initialize KeyAuth. Error: " << AuthManager::getStatus() << "\n";
      std::this_thread::sleep_for(std::chrono::seconds(3));
      exit(0);
  }
  */

  ImGui_ImplWin32_EnableDpiAwareness();
  float main_scale = ImGui_ImplWin32_GetDpiScaleForMonitor(
      ::MonitorFromPoint(POINT{0, 0}, MONITOR_DEFAULTTOPRIMARY));

  int screenWidth = GetSystemMetrics(SM_CXSCREEN);
  int screenHeight = GetSystemMetrics(SM_CYSCREEN);

  WNDCLASSEXW wc = {sizeof(wc),
                    CS_CLASSDC,
                    WndProc,
                    0L,
                    0L,
                    GetModuleHandle(nullptr),
                    nullptr,
                    nullptr,
                    nullptr,
                    nullptr,
                    L"ScatheOverlay",
                    nullptr};
  ::RegisterClassExW(&wc);

  DWORD exStyle = WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TOOLWINDOW;
  if (!g_menuVisible)
    exStyle |= WS_EX_TRANSPARENT;

  HWND hwnd = ::CreateWindowExW(exStyle, wc.lpszClassName, L"Scathe",
                                WS_POPUP, 0, 0, screenWidth, screenHeight,
                                nullptr, nullptr, wc.hInstance, nullptr);

  SetLayeredWindowAttributes(hwnd, 0, 255, LWA_ALPHA);

  const MARGINS margins = {-1};
  DwmExtendFrameIntoClientArea(hwnd, &margins);

  if (!CreateDeviceD3D(hwnd)) {
    CleanupDeviceD3D();
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
    return 1;
  }

  ::ShowWindow(hwnd, SW_SHOW);
  ::UpdateWindow(hwnd);

  CreateTrayIcon(hwnd);

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  (void)io;

  ImGui::StyleColorsDark();

  ImGuiStyle &style = ImGui::GetStyle();
  ImVec4 lightBlue = ImVec4(137.0f / 255.0f, 207.0f / 255.0f, 240.0f / 255.0f, 1.0f);
  style.Colors[ImGuiCol_CheckMark] = lightBlue;
  style.Colors[ImGuiCol_SliderGrab] = ImVec4(lightBlue.x, lightBlue.y, lightBlue.z, 0.7f);
  style.Colors[ImGuiCol_SliderGrabActive] = lightBlue;
  style.Colors[ImGuiCol_Header] = ImVec4(lightBlue.x, lightBlue.y, lightBlue.z, 0.3f);
  style.Colors[ImGuiCol_HeaderHovered] = ImVec4(lightBlue.x, lightBlue.y, lightBlue.z, 0.5f);
  style.Colors[ImGuiCol_HeaderActive] = lightBlue;
  style.Colors[ImGuiCol_Button] = ImVec4(lightBlue.x, lightBlue.y, lightBlue.z, 0.4f);
  style.Colors[ImGuiCol_ButtonHovered] = ImVec4(lightBlue.x, lightBlue.y, lightBlue.z, 0.6f);
  style.Colors[ImGuiCol_ButtonActive] = lightBlue;
  style.Colors[ImGuiCol_FrameBgActive] = ImVec4(lightBlue.x, lightBlue.y, lightBlue.z, 0.2f);
  style.Colors[ImGuiCol_SeparatorActive] = lightBlue;
  style.Colors[ImGuiCol_ResizeGripActive] = lightBlue;
  style.Colors[ImGuiCol_Tab] = ImVec4(lightBlue.x, lightBlue.y, lightBlue.z, 0.2f);
  style.Colors[ImGuiCol_TabHovered] = ImVec4(lightBlue.x, lightBlue.y, lightBlue.z, 0.5f);
  style.Colors[ImGuiCol_TabSelected] = ImVec4(lightBlue.x, lightBlue.y, lightBlue.z, 0.6f);
  style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(lightBlue.x, lightBlue.y, lightBlue.z, 0.35f);

  style.ScaleAllSizes(main_scale);
  style.WindowBorderSize = 0.0f;
  style.ChildBorderSize = 0.0f;
  style.PopupBorderSize = 0.0f;
  style.TabBorderSize = 0.0f;

  ImGui_ImplWin32_Init(hwnd);
  ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

  ImFontConfig vcrConfig;
  vcrConfig.OversampleH = 8;
  vcrConfig.OversampleV = 8;
  vcrConfig.PixelSnapH = true;
  vcrConfig.RasterizerMultiply = 1.5f;
  vcrConfig.FontDataOwnedByAtlas = false;

  const ImWchar *Ranges = io.Fonts->GetGlyphRangesDefault();

  Visualize.visitor = io.Fonts->AddFontFromMemoryTTF(
      (void *)TAHOMA_0_TTF, sizeof(TAHOMA_0_TTF), 13.0f, &vcrConfig, Ranges);

  if (!Visualize.visitor) {
    Visualize.visitor = io.Fonts->AddFontDefault();
  }

  ImFontConfig weaponIconConfig;
  weaponIconConfig.OversampleH = 6;
  weaponIconConfig.OversampleV = 6;
  weaponIconConfig.PixelSnapH = true;
  weaponIconConfig.RasterizerMultiply = 1.5f;
  weaponIconConfig.FontDataOwnedByAtlas = false;

  Visualize.weapon_icon_font = io.Fonts->AddFontFromMemoryTTF(
      (void *)cs_icon, sizeof(cs_icon), 24.0f, &weaponIconConfig);

  if (!Visualize.weapon_icon_font) {
    Visualize.weapon_icon_font = io.Fonts->AddFontDefault();
  }

  ImFontConfig vcrMenuConfig;
  vcrMenuConfig.OversampleH = 6;
  vcrMenuConfig.OversampleV = 6;
  vcrMenuConfig.PixelSnapH = true;
  vcrMenuConfig.RasterizerMultiply = 1.3f;
  vcrMenuConfig.FontDataOwnedByAtlas = false;
  Visualize.verdana_bold = io.Fonts->AddFontFromMemoryTTF(
      (void *)TAHOMA_0_TTF, sizeof(TAHOMA_0_TTF), 14.0f, &vcrMenuConfig, Ranges);

  if (!Visualize.verdana_bold) {
    Visualize.verdana_bold = io.Fonts->AddFontDefault();
  }

  io.Fonts->Build();

  ImVec4 clear_color = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
  helpers _helper;

  storage::init_menu_keybind();
  InitializeEngine();

  bool done = false;
  while (!done && !shouldExit) {
    MSG msg;
    while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
      ::TranslateMessage(&msg);
      ::DispatchMessage(&msg);
      if (msg.message == WM_QUIT)
        done = true;
    }
    if (done)
      break;

    if (g_SwapChainOccluded &&
        g_pSwapChain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED) {
      ::Sleep(10);
      continue;
    }
    g_SwapChainOccluded = false;

    if (g_ResizeWidth != 0 && g_ResizeHeight != 0) {
      CleanupRenderTarget();
      g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight,
                                  DXGI_FORMAT_UNKNOWN, 0);
      g_ResizeWidth = g_ResizeHeight = 0;
      CreateRenderTarget();
    }

    if (GetAsyncKeyState(storage::menu_key_bind.key) & 1) {
      g_menuVisible = !g_menuVisible;

      LONG_PTR exStyle = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
      if (g_menuVisible) {
        SetWindowLongPtr(hwnd, GWL_EXSTYLE, exStyle & ~WS_EX_TRANSPARENT);
        SetForegroundWindow(hwnd);
        SetFocus(hwnd);
      } else {
        SetWindowLongPtr(hwnd, GWL_EXSTYLE, exStyle | WS_EX_TRANSPARENT);
      }
      SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0,
                   SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_FRAMECHANGED);
    }

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    storage::update_keybinds();

    static bool last_streamproof = false;
    if (storage::streamproof != last_streamproof) {
      SetWindowDisplayAffinity(hwnd, storage::streamproof ? 0x00000011 : 0);
      last_streamproof = storage::streamproof;
    }

    if (g_engineInitialized) {
      hooks::esp();
      NotificationManager::getInstance().render();
    }

    if (g_menuVisible && AuthManager::isAuthenticated()) {
      ImGui::SetNextWindowSize(ImVec2(690, 500));
      ImGui::PushStyleColor(
          ImGuiCol_WindowBg,
          ImVec4(10.0f / 255.0f, 10.0f / 255.0f, 10.0f / 255.0f, 1.0f));
      ImGui::Begin("scathe merged", nullptr,
                   ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);
      ImGui::PopStyleColor();
      ImDrawList *draw_list = ImGui::GetWindowDrawList();
      ImVec2 window_pos = { std::round(ImGui::GetWindowPos().x), std::round(ImGui::GetWindowPos().y) };
      ImVec2 window_size = { std::round(ImGui::GetWindowSize().x), std::round(ImGui::GetWindowSize().y) };

      draw_list->AddLine(
          ImVec2(window_pos.x, window_pos.y + 25),
          ImVec2(window_pos.x + window_size.x, window_pos.y + 25),
          IM_COL32(55, 55, 55, 255), 1.0f);

      draw_list->AddLine(
          ImVec2(window_pos.x + 5, window_pos.y),
          ImVec2(window_pos.x + 5, window_pos.y + 25),
          IM_COL32(55, 55, 55, 255), 1.0f);

      ImVec2 pos = ImVec2(window_pos.x + 6, window_pos.y + 7);

      for (int x = -1; x <= 1; x++) {
        for (int y = -1; y <= 1; y++) {
          if (x == 0 && y == 0)
            continue;
          draw_list->AddText(ImVec2(pos.x + x, pos.y + y),
                             IM_COL32(0, 0, 0, 255), "##SCATHE");
        }
      }

      draw_list->AddText(ImVec2(pos.x, pos.y),
                         IM_COL32(137, 207, 240, 255), "##");
      
      float hashWidth = std::round(ImGui::CalcTextSize("##").x);
      draw_list->AddText(ImVec2(pos.x + hashWidth, pos.y), IM_COL32(255, 255, 255, 255),
                         "SCATHE");

      draw_list->AddRectFilled(ImVec2(window_pos.x + 1, window_pos.y + 26),
                               ImVec2(window_pos.x + window_size.x - 1,
                                      window_pos.y + window_size.y - 1),
                               IM_COL32(15, 15, 15, 255));
      draw_list->AddRectFilled(ImVec2(window_pos.x + 10, window_pos.y + 36),
                               ImVec2(window_pos.x + window_size.x - 10,
                                      window_pos.y + window_size.y - 10),
                               IM_COL32(17, 17, 17, 255));
      
      draw_list->AddRect(ImVec2(window_pos.x + 9, window_pos.y + 36),
                         ImVec2(window_pos.x + window_size.x - 9,
                                window_pos.y + window_size.y - 9),
                         IM_COL32(55, 55, 55, 255));
      draw_list->AddRect(ImVec2(window_pos.x + 8, window_pos.y + 35),
                         ImVec2(window_pos.x + window_size.x - 8,
                                window_pos.y + window_size.y - 8),
                         IM_COL32(0, 0, 0, 255));

      _helper.add_tab();
      _helper.render();

      draw_list->AddRect(
          ImVec2(window_pos.x, window_pos.y),
          ImVec2(window_pos.x + window_size.x, window_pos.y + window_size.y),
          IM_COL32(0, 0, 0, 255));
      draw_list->AddRect(ImVec2(window_pos.x + 1, window_pos.y + 1),
                         ImVec2(window_pos.x + window_size.x - 1,
                                window_pos.y + window_size.y - 1),
                         IM_COL32(55, 55, 55, 255));

      draw_list->AddLine(
          ImVec2(window_pos.x, window_pos.y),
          ImVec2(window_pos.x + window_size.x, window_pos.y),
          IM_COL32(137, 207, 240, 255), 1.0f);

      ImGui::End();
    }

    if (storage::show_keybind_list) {
      ImGui::SetNextWindowPos(ImVec2(10, 100), ImGuiCond_FirstUseEver);
      ImGui::SetNextWindowSize(ImVec2(200, 0), ImGuiCond_Always);
      
      ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(10.0f / 255.0f, 10.0f / 255.0f, 10.0f / 255.0f, 1.0f));
      ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(55.0f / 255.0f, 55.0f / 255.0f, 55.0f / 255.0f, 1.0f));
      ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
      ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
      
      int window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse;
      if (!g_menuVisible) {
        window_flags |= ImGuiWindowFlags_NoInputs;
      }
      
      if (ImGui::Begin("##KeybindList", nullptr, window_flags)) {
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        ImVec2 window_pos = { std::round(ImGui::GetWindowPos().x), std::round(ImGui::GetWindowPos().y) };
        ImVec2 window_size = { std::round(ImGui::GetWindowSize().x), std::round(ImGui::GetWindowSize().y) };
        
        draw_list->AddRect(
          ImVec2(window_pos.x + 2, window_pos.y + 2),
          ImVec2(window_pos.x + window_size.x - 2, window_pos.y + window_size.y - 2),
          IM_COL32(30, 30, 30, 255), 0.0f, 0, 1.0f
        );
        
        draw_list->AddLine(
          ImVec2(window_pos.x, window_pos.y + 25),
          ImVec2(window_pos.x + window_size.x, window_pos.y + 25),
          IM_COL32(55, 55, 55, 255), 1.0f
        );
        
        ImVec2 title_pos = ImVec2(window_pos.x + 6, window_pos.y + 7);
        for (int x = -1; x <= 1; x++) {
          for (int y = -1; y <= 1; y++) {
            if (x == 0 && y == 0) continue;
            draw_list->AddText(ImVec2(title_pos.x + x, title_pos.y + y), IM_COL32(0, 0, 0, 255), "Keybinds");
          }
        }
        draw_list->AddText(title_pos, IM_COL32(255, 255, 255, 255), "Keybinds");
        
        draw_list->AddRectFilled(
          ImVec2(window_pos.x + 1, window_pos.y + 26),
          ImVec2(window_pos.x + window_size.x - 1, window_pos.y + window_size.y - 1),
          IM_COL32(15, 15, 15, 255)
        );
        
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 30);
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 8);
        ImGui::BeginGroup();
        
        ImVec4 activeColor = ImVec4(137.0f / 255.0f, 207.0f / 255.0f, 240.0f / 255.0f, 1.0f);
        
        if (storage::aimbot && storage::aimbotkeybind.enabled) {
          ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "Aimbot");
          ImGui::SameLine(120);
          ImGui::TextColored(activeColor, "[%s]", storage::aimbotkeybind.get_key_name().c_str());
        }
        
        if (storage::silent_aim && storage::silentaimkeybind.enabled) {
          ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "Silent Aim");
          ImGui::SameLine(120);
          ImGui::TextColored(activeColor, "[%s]", storage::silentaimkeybind.get_key_name().c_str());
        }
        
        if (storage::triggerbot && storage::triggerbotkeybind.enabled) {
          ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "Triggerbot");
          ImGui::SameLine(120);
          ImGui::TextColored(activeColor, "[%s]", storage::triggerbotkeybind.get_key_name().c_str());
        }
        
        if (globals::rage::speed_enabled && globals::rage::walkspeed_bind.enabled) {
          ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "Walkspeed");
          ImGui::SameLine(120);
          ImGui::TextColored(activeColor, "[%s]", globals::rage::walkspeed_bind.get_key_name().c_str());
        }
        
        if (storage::spinbot_enabled && storage::spinbotkeybind.enabled) {
          ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "Spinbot");
          ImGui::SameLine(120);
          ImGui::TextColored(activeColor, "[%s]", storage::spinbotkeybind.get_key_name().c_str());
        }
        
        if (storage::noclip && storage::noclipkeybind.enabled) {
          ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "Noclip");
          ImGui::SameLine(120);
          ImGui::TextColored(activeColor, "[%s]", storage::noclipkeybind.get_key_name().c_str());
        }
        
        if (storage::fly_enabled && storage::flightkeybind.enabled) {
          ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "Fly");
          ImGui::SameLine(120);
          ImGui::TextColored(activeColor, "[%s]", storage::flightkeybind.get_key_name().c_str());
        }
        
        ImGui::EndGroup();
        ImGui::Dummy(ImVec2(0, 8));
        
        ImGui::End();
      }
      
      ImGui::PopStyleVar(2);
      ImGui::PopStyleColor(2);
    }

    ImGui::Render();
    const float clear_color_with_alpha[4] = {
        clear_color.x * clear_color.w, clear_color.y * clear_color.w,
        clear_color.z * clear_color.w, clear_color.w};
    g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView,
                                            nullptr);
    g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView,
                                               clear_color_with_alpha);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    HRESULT hr = g_pSwapChain->Present(storage::vsync ? 1 : 0, 0);
    g_SwapChainOccluded = (hr == DXGI_STATUS_OCCLUDED);

    Engine::DiscordRPC::Update();
  }

  Engine::DiscordRPC::Shutdown();

  ImGui_ImplDX11_Shutdown();
  ImGui_ImplWin32_Shutdown();
  ImGui::DestroyContext();

  CleanupDeviceD3D();
  ::DestroyWindow(hwnd);
  ::UnregisterClassW(wc.lpszClassName, wc.hInstance);

  return 0;
}

bool CreateDeviceD3D(HWND hWnd) {
  DXGI_SWAP_CHAIN_DESC sd;
  ZeroMemory(&sd, sizeof(sd));
  sd.BufferCount = 2;
  sd.BufferDesc.Width = 0;
  sd.BufferDesc.Height = 0;
  sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  sd.BufferDesc.RefreshRate.Numerator = 60;
  sd.BufferDesc.RefreshRate.Denominator = 1;
  sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
  sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  sd.OutputWindow = hWnd;
  sd.SampleDesc.Count = 1;
  sd.SampleDesc.Quality = 0;
  sd.Windowed = TRUE;
  sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

  UINT createDeviceFlags = 0;
  D3D_FEATURE_LEVEL featureLevel;
  const D3D_FEATURE_LEVEL featureLevelArray[2] = {
      D3D_FEATURE_LEVEL_11_0,
      D3D_FEATURE_LEVEL_10_0,
  };
  HRESULT res = D3D11CreateDeviceAndSwapChain(
      nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags,
      featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain,
      &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
  if (res == DXGI_ERROR_UNSUPPORTED)
    res = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags,
        featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain,
        &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
  if (res != S_OK)
    return false;

  CreateRenderTarget();
  return true;
}

void CleanupDeviceD3D() {
  CleanupRenderTarget();
  if (g_pSwapChain) {
    g_pSwapChain->Release();
    g_pSwapChain = nullptr;
  }
  if (g_pd3dDeviceContext) {
    g_pd3dDeviceContext->Release();
    g_pd3dDeviceContext = nullptr;
  }
  if (g_pd3dDevice) {
    g_pd3dDevice->Release();
    g_pd3dDevice = nullptr;
  }
}

void CreateRenderTarget() {
  ID3D11Texture2D *pBackBuffer;
  g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
  g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr,
                                       &g_mainRenderTargetView);
  pBackBuffer->Release();
}

void CleanupRenderTarget() {
  if (g_mainRenderTargetView) {
    g_mainRenderTargetView->Release();
    g_mainRenderTargetView = nullptr;
  }
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd,
                                                             UINT msg,
                                                             WPARAM wParam,
                                                             LPARAM lParam);

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
    return true;

  switch (msg) {
  case WM_SIZE:
    if (wParam == SIZE_MINIMIZED)
      return 0;
    g_ResizeWidth = (UINT)LOWORD(lParam);
    g_ResizeHeight = (UINT)HIWORD(lParam);
    return 0;
  case WM_SYSCOMMAND:
    if ((wParam & 0xfff0) == SC_KEYMENU)
      return 0;
    break;
  case WM_DESTROY:
    Shell_NotifyIconW(NIM_DELETE, &nid);
    ::PostQuitMessage(0);
    return 0;
  case WM_TRAYICON:
    if (lParam == WM_RBUTTONUP)
      ShowTrayMenu(hWnd);
    return 0;
  case WM_NCHITTEST: {
    if (g_menuVisible)
      return HTCLIENT;
    return HTTRANSPARENT;
  }
  case WM_MOUSEACTIVATE:
    return MA_ACTIVATE;
  }
  return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}
