#include "Core.hpp"
#include "../../overlay/Overlay/overlay.hpp"
#include "engine/mem_module/memory.hpp"
#include "core/storage/Globals.hpp"
#include "core/storage/include.h"
#include <atomic>
#include <chrono>
#include <comdef.h>
#include <eh.h>
#include <iostream>
#include <mutex>
#include <thread>
#include <wbemidl.h>
#include <windows.h>

extern bool shouldExit;

namespace AutoRescan {
std::atomic<bool> active{true};
std::mutex updateMutex;
std::atomic<bool> needsRescan{false};
std::atomic<bool> forceRescan{false};
std::atomic<uint64_t> lastGameId{0};
std::atomic<uint64_t> lastPlaceId{0};
std::atomic<uintptr_t> lastWorkspace{0};
std::atomic<uintptr_t> lastPlayers{0};
std::atomic<uintptr_t> lastDataModel{0};
std::atomic<uintptr_t> lastLocalPlayer{0};
std::atomic<DWORD> lastProcessId{0};

constexpr int LOBBY_SCAN_MS = 800;
constexpr int LOADING_SCAN_MS = 300;
constexpr int INGAME_SCAN_MS = 600;
constexpr int TRANSITION_SCAN_MS = 200;
constexpr int ERROR_RECOVERY_MS = 1500;
constexpr int MEMORY_CHECK_MS = 3000;
constexpr int PROCESS_CHECK_MS = 2000;

std::atomic<int> consecutiveSuccesses{0};
std::atomic<int> consecutiveFailures{0};
std::atomic<bool> inTransition{false};
std::atomic<bool> memoryHealthy{true};
std::atomic<std::chrono::steady_clock::time_point> lastStateChange;
std::atomic<std::chrono::steady_clock::time_point> lastMemoryCheck;
}

struct GameState {
  uint64_t gameId = 0;
  uint64_t placeId = 0;
  uintptr_t workspace = 0;
  uintptr_t players = 0;
  uintptr_t datamodel = 0;
  uintptr_t localplayer = 0;
  DWORD processId = 0;

  enum class StateType {
    INVALID,
    LOBBY_EMPTY,
    LOBBY_SEARCHING,
    LOADING,
    IN_GAME,
    DISCONNECTING
  };

  StateType GetStateType() const {
    if (processId == 0 || datamodel == 0)
      return StateType::INVALID;

    if (gameId == 0 && placeId == 0)
      return StateType::LOBBY_EMPTY;

    if (gameId == 0 || placeId <= 1000)
      return StateType::LOBBY_SEARCHING;

    if (workspace == 0 || players == 0)
      return StateType::LOADING;

    if (gameId != 0 && placeId > 1000 && (workspace == 0 || localplayer == 0)) {
      return StateType::DISCONNECTING;
    }

    return StateType::IN_GAME;
  }

  bool IsValid() const { return GetStateType() != StateType::INVALID; }

  bool HasChanged(const GameState &other) const {
    return (gameId != other.gameId || placeId != other.placeId ||
            workspace != other.workspace || players != other.players ||
            datamodel != other.datamodel || localplayer != other.localplayer ||
            processId != other.processId);
  }

  bool HasCriticalChange(const GameState &other) const {
    return (gameId != other.gameId || placeId != other.placeId ||
            processId != other.processId || datamodel != other.datamodel);
  }

  bool RequiresImmediateRescan(const GameState &other) const {
    return (processId != other.processId ||
            (datamodel == 0 && other.datamodel != 0) ||
            (datamodel != 0 && other.datamodel == 0) ||
            (gameId == 0 && other.gameId != 0) ||
            (gameId != 0 && other.gameId == 0));
  }
};

static Engine::Instance g_staticInstance;
static std::mutex g_instanceMutex;

struct SmartCache {
  GameState cachedState;
  std::chrono::steady_clock::time_point lastUpdate;
  std::chrono::steady_clock::time_point lastFullScan;
  std::mutex cacheMutex;
  bool isValid = false;
  int cacheHits = 0;
  int totalQueries = 0;

  std::chrono::milliseconds
  getCacheValidityDuration(GameState::StateType stateType) const {
    switch (stateType) {
    case GameState::StateType::LOBBY_EMPTY:
      return std::chrono::milliseconds(1500);
    case GameState::StateType::LOBBY_SEARCHING:
      return std::chrono::milliseconds(800);
    case GameState::StateType::LOADING:
      return std::chrono::milliseconds(300);
    case GameState::StateType::IN_GAME:
      return std::chrono::milliseconds(1200);
    case GameState::StateType::DISCONNECTING:
      return std::chrono::milliseconds(200);
    default:
      return std::chrono::milliseconds(400);
    }
  }

  float getCacheHitRate() const {
    return totalQueries > 0 ? (float)cacheHits / totalQueries : 0.0f;
  }
} g_smartCache;

bool QuickMemoryCheck() {
  auto now = std::chrono::steady_clock::now();
  auto lastCheck = AutoRescan::lastMemoryCheck.load();
  auto timeSinceCheck =
      std::chrono::duration_cast<std::chrono::milliseconds>(now - lastCheck);

  if (timeSinceCheck < std::chrono::milliseconds(AutoRescan::MEMORY_CHECK_MS)) {
    return AutoRescan::memoryHealthy.load();
  }

  if (!mem::handleProcess || mem::handleProcess == INVALID_HANDLE_VALUE) {
    AutoRescan::memoryHealthy = false;
    return false;
  }

  DWORD testValue;
  SIZE_T bytesRead;
  bool healthy =
      (ReadProcessMemory(mem::handleProcess, (LPCVOID)mem::base, &testValue,
                         sizeof(DWORD), &bytesRead) != FALSE);

  AutoRescan::memoryHealthy = healthy;
  AutoRescan::lastMemoryCheck = now;

  return healthy;
}

GameState CaptureCurrentState() {
  auto now = std::chrono::steady_clock::now();

  {
    std::lock_guard<std::mutex> lock(g_smartCache.cacheMutex);
    g_smartCache.totalQueries++;

    if (g_smartCache.isValid) {
      auto cacheValidity = g_smartCache.getCacheValidityDuration(
          g_smartCache.cachedState.GetStateType());
      auto timeSinceUpdate =
          std::chrono::duration_cast<std::chrono::milliseconds>(
              now - g_smartCache.lastUpdate);

      if (timeSinceUpdate < cacheValidity && AutoRescan::memoryHealthy.load()) {
        g_smartCache.cacheHits++;
        return g_smartCache.cachedState;
      }
    }
  }

  GameState state;

  try {
    if (!QuickMemoryCheck()) {
      std::lock_guard<std::mutex> lock(g_smartCache.cacheMutex);
      g_smartCache.isValid = false;
      return state;
    }

    state.processId = mem::PID;

    std::lock_guard<std::mutex> instanceLock(g_instanceMutex);

    bool fullScan = false;
    {
      std::lock_guard<std::mutex> lock(g_smartCache.cacheMutex);
      auto timeSinceFullScan =
          std::chrono::duration_cast<std::chrono::milliseconds>(
              now - g_smartCache.lastFullScan);
      fullScan = (timeSinceFullScan > std::chrono::milliseconds(5000)) ||
                 !g_smartCache.isValid;
    }

    storage::fakedm = g_staticInstance.GetDataModelPointer();
    storage::datamodel = g_staticInstance.ReturnDataModel();
    state.datamodel = storage::datamodel.address;

    if (storage::datamodel.address) {
      storage::game = g_staticInstance.GetDataModel();
      if (storage::game.address) {
        state.gameId = storage::game.GetGameID();
        state.placeId = storage::game.GetPlaceID();
      }
    }

    if (fullScan || state.gameId != 0 || state.placeId != 0) {
      storage::workspace = g_staticInstance.GetService("Workspace");
      storage::players = storage::datamodel.FindFirstChildOfClass("Players");

      state.workspace = storage::workspace.address;
      state.players = storage::players.address;

      static bool playersLogged = false;
      static bool workspaceLogged = false;

      if (storage::players.address && !playersLogged) {
        auto logTime = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(logTime);
        std::tm tm = *std::localtime(&time_t);
        char timeStr[9];
        std::strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &tm);
        printf("[INFO] [%s] Players: 0x%llx\n", timeStr,
               (unsigned long long)storage::players.address);
        playersLogged = true;
      }
      if (storage::workspace.address && !workspaceLogged) {
        auto logTime = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(logTime);
        std::tm tm = *std::localtime(&time_t);
        char timeStr[9];
        std::strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &tm);
        printf("[INFO] [%s] Workspace: 0x%llx\n", timeStr,
               (unsigned long long)storage::workspace.address);
        workspaceLogged = true;
      }

      if (storage::players.address) {
        auto localPlayer = storage::players.GetLocalPlayer();
        state.localplayer = localPlayer.address;
      }

      std::lock_guard<std::mutex> lock(g_smartCache.cacheMutex);
      g_smartCache.lastFullScan = now;
    } else {
      std::lock_guard<std::mutex> lock(g_smartCache.cacheMutex);
      if (g_smartCache.isValid) {
        state.workspace = g_smartCache.cachedState.workspace;
        state.players = g_smartCache.cachedState.players;
        state.localplayer = g_smartCache.cachedState.localplayer;
      }
    }

    {
      std::lock_guard<std::mutex> lock(g_smartCache.cacheMutex);
      g_smartCache.cachedState = state;
      g_smartCache.lastUpdate = now;
      g_smartCache.isValid = true;
    }

  } catch (...) {
    std::lock_guard<std::mutex> lock(g_smartCache.cacheMutex);
    g_smartCache.isValid = false;
    AutoRescan::consecutiveFailures++;
  }

  return state;
}

bool PerformSmartRescan(GameState::StateType expectedState) {
  try {
    static auto lastProcessCheck = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    auto timeSinceProcessCheck =
        std::chrono::duration_cast<std::chrono::milliseconds>(now -
                                                               lastProcessCheck);

    if (timeSinceProcessCheck >
        std::chrono::milliseconds(AutoRescan::PROCESS_CHECK_MS)) {
      DWORD currentPID = mem::FindProcess(TEXT("RobloxPlayerBeta.exe"));
      if (currentPID != mem::PID || currentPID == 0) {
        if (currentPID == 0) {
          return false;
        }

        mem::PID = currentPID;
        if (mem::handleProcess) {
          CloseHandle(mem::handleProcess);
        }
        mem::handleProcess = OpenProcess(PROCESS_VM_READ | PROCESS_VM_WRITE |
                                             PROCESS_VM_OPERATION,
                                         FALSE, mem::PID);
        if (!mem::handleProcess) {
          return false;
        }
        mem::base = mem::GetProcessBase();
        if (!mem::base) {
          return false;
        }

        std::lock_guard<std::mutex> lock(g_smartCache.cacheMutex);
        g_smartCache.isValid = false;
        AutoRescan::memoryHealthy = false;
        storage::player_cache.clear();
        storage::localplayer = Engine::PlayerIns{};
      }
      lastProcessCheck = now;
    }

    if (!QuickMemoryCheck()) {
      return false;
    }

    std::lock_guard<std::mutex> instanceLock(g_instanceMutex);

    storage::fakedm = g_staticInstance.GetDataModelPointer();
    storage::datamodel = g_staticInstance.ReturnDataModel();

    if (!storage::datamodel.address) {
      return false;
    }

    storage::game = g_staticInstance.GetDataModel();

    if (storage::datamodel.address) {
      storage::gameid = storage::datamodel.GetGameID();
      storage::placeid = storage::datamodel.GetPlaceID();
    }

    bool needWorkspaceAndPlayers =
        (expectedState == GameState::StateType::IN_GAME ||
         expectedState == GameState::StateType::LOADING ||
         storage::gameid != 0 || storage::placeid != 0);

    if (needWorkspaceAndPlayers) {
      storage::workspace = g_staticInstance.GetService("Workspace");
      storage::players = storage::datamodel.FindFirstChildOfClass("Players");
    }

    storage::wallcheck.cacheWorkspace();

    AutoRescan::consecutiveSuccesses++;
    AutoRescan::consecutiveFailures = 0;
    return true;

  } catch (...) {
    AutoRescan::consecutiveFailures++;
    AutoRescan::consecutiveSuccesses = 0;
    return false;
  }
}

void SmartChangeDetectionLoop() {
  SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);

  GameState previousState;
  GameState::StateType previousStateType = GameState::StateType::INVALID;
  auto lastRescanTime = std::chrono::steady_clock::now();

  auto getSmartDelay = [&](GameState::StateType currentType, bool hasChanges,
                            bool isTransition) -> int {
    int failures = AutoRescan::consecutiveFailures.load();
    if (failures > 3)
      return AutoRescan::ERROR_RECOVERY_MS;
    if (failures > 0)
      return 1000 + (failures * 200);

    int successes = AutoRescan::consecutiveSuccesses.load();
    float successBonus = min(0.3f, successes * 0.05f);

    if (isTransition || hasChanges) {
      return (int)(AutoRescan::TRANSITION_SCAN_MS * (1.0f - successBonus));
    }

    int baseDelay;
    switch (currentType) {
    case GameState::StateType::LOBBY_EMPTY:
      baseDelay = AutoRescan::LOBBY_SCAN_MS;
      break;
    case GameState::StateType::LOBBY_SEARCHING:
      baseDelay = AutoRescan::LOBBY_SCAN_MS - 200;
      break;
    case GameState::StateType::LOADING:
      baseDelay = AutoRescan::LOADING_SCAN_MS;
      break;
    case GameState::StateType::IN_GAME:
      baseDelay = AutoRescan::INGAME_SCAN_MS;
      break;
    case GameState::StateType::DISCONNECTING:
      baseDelay = AutoRescan::LOADING_SCAN_MS;
      break;
    default:
      baseDelay = 600;
    }

    return (int)(baseDelay * (1.0f - successBonus));
  };

  while (AutoRescan::active) {
    try {
      GameState currentState = CaptureCurrentState();
      GameState::StateType currentStateType = currentState.GetStateType();

      bool hasAnyChange = currentState.HasChanged(previousState);
      bool hasCriticalChange = currentState.HasCriticalChange(previousState);
      bool requiresImmediate =
          currentState.RequiresImmediateRescan(previousState);
      bool stateTypeChanged = (currentStateType != previousStateType);
      bool forceRescan = AutoRescan::needsRescan.exchange(false) ||
                         AutoRescan::forceRescan.exchange(false);

      bool isTransition = stateTypeChanged || requiresImmediate;
      if (isTransition) {
        AutoRescan::inTransition = true;
        AutoRescan::lastStateChange = std::chrono::steady_clock::now();
        uint64_t lastGameId = AutoRescan::lastGameId.load();
        uint64_t currentGameId = currentState.gameId;
        if (lastGameId != 0 && currentGameId != 0 &&
            lastGameId != currentGameId) {
          storage::player_cache.clear();
          storage::localplayer = Engine::PlayerIns{};
          AutoRescan::lastGameId = currentGameId;
        } else if (currentGameId != 0) {
          AutoRescan::lastGameId = currentGameId;
        }
      } else {
        auto now = std::chrono::steady_clock::now();
        auto timeSinceChange =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                now - AutoRescan::lastStateChange.load());
        if (timeSinceChange > std::chrono::milliseconds(2000)) {
          AutoRescan::inTransition = false;
        }
      }

      if (hasAnyChange || stateTypeChanged || forceRescan ||
          requiresImmediate) {
        auto now = std::chrono::steady_clock::now();
        auto timeSinceLastRescan =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                now - lastRescanTime);

        int requiredCooldown = 100;

        if (requiresImmediate || forceRescan) {
          requiredCooldown = 50;
        } else if (hasCriticalChange) {
          requiredCooldown = 200;
        } else if (stateTypeChanged) {
          requiredCooldown = 300;
        } else {
          requiredCooldown = 500;
        }

        if (timeSinceLastRescan.count() >= requiredCooldown || forceRescan ||
            requiresImmediate) {
          std::lock_guard<std::mutex> lock(AutoRescan::updateMutex);

          bool rescanSuccess = false;
          int maxRetries = requiresImmediate ? 3 : 2;

          for (int retry = 0; retry < maxRetries && !rescanSuccess; retry++) {
            if (retry > 0) {
              std::this_thread::sleep_for(
                  std::chrono::milliseconds(200 * retry));
            }
            rescanSuccess = PerformSmartRescan(currentStateType);
          }

          if (rescanSuccess) {
            std::lock_guard<std::mutex> cacheLock(g_smartCache.cacheMutex);
            g_smartCache.isValid = false;
            previousState = CaptureCurrentState();
            previousStateType = previousState.GetStateType();
            lastRescanTime = now;
          }
        }
      } else {
        if (currentState.IsValid()) {
          previousState = currentState;
          previousStateType = currentStateType;
        }
      }

    } catch (...) {
      AutoRescan::consecutiveFailures++;
    }

    GameState::StateType currentType = previousState.GetStateType();
    bool hasChanges = (AutoRescan::consecutiveFailures.load() == 0);
    bool isTransition = AutoRescan::inTransition.load();
    int delayMs = getSmartDelay(currentType, hasChanges, isTransition);

    std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
  }
}

void GameTransitionMonitor() {
  GameState::StateType lastKnownState = GameState::StateType::INVALID;
  auto stateChangeTime = std::chrono::steady_clock::now();

  while (AutoRescan::active) {
    try {
      GameState currentState = CaptureCurrentState();
      GameState::StateType currentStateType = currentState.GetStateType();

      if (currentStateType != lastKnownState) {
        stateChangeTime = std::chrono::steady_clock::now();
        lastKnownState = currentStateType;

        if (currentStateType == GameState::StateType::LOADING ||
            currentStateType == GameState::StateType::LOBBY_EMPTY ||
            currentStateType == GameState::StateType::DISCONNECTING) {
          AutoRescan::forceRescan = true;
        }
      }

      auto now = std::chrono::steady_clock::now();
      auto timeInState = std::chrono::duration_cast<std::chrono::milliseconds>(
          now - stateChangeTime);

      switch (currentStateType) {
      case GameState::StateType::LOADING:
        if (timeInState.count() > 12000) {
          AutoRescan::forceRescan = true;
          stateChangeTime = now;
        }
        break;

      case GameState::StateType::LOBBY_EMPTY:
        if (timeInState.count() > 5000) {
          AutoRescan::needsRescan = true;
          stateChangeTime = now;
        }
        break;

      case GameState::StateType::DISCONNECTING:
        if (timeInState.count() > 8000) {
          AutoRescan::forceRescan = true;
          stateChangeTime = now;
        }
        break;
      }

    } catch (...) {
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(800));
  }
}

void ForceRescan() {
  AutoRescan::forceRescan = true;
  std::lock_guard<std::mutex> lock(g_smartCache.cacheMutex);
  g_smartCache.isValid = false;
  AutoRescan::consecutiveFailures = 0;
}

void RequestRescan() { AutoRescan::needsRescan = true; }

float GetCacheEfficiency() {
  std::lock_guard<std::mutex> lock(g_smartCache.cacheMutex);
  return g_smartCache.getCacheHitRate();
}

void AutoRescanHandler() {
  GameState initialState = CaptureCurrentState();
  AutoRescan::lastGameId = initialState.gameId;
  AutoRescan::lastPlaceId = initialState.placeId;
  AutoRescan::lastStateChange = std::chrono::steady_clock::now();
  AutoRescan::lastMemoryCheck = std::chrono::steady_clock::now();

  std::thread(SmartChangeDetectionLoop).detach();
  std::thread(GameTransitionMonitor).detach();

  while (AutoRescan::active) {
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
  }
}

bool Engine::Initializer() {
  DWORD tempPID = mem::FindProcess(TEXT("RobloxPlayerBeta.exe"));

  if (tempPID) {
    Sleep(2000);
  } else {
    while (!tempPID) {
      Sleep(1000);
      tempPID = mem::FindProcess(TEXT("RobloxPlayerBeta.exe"));
    }
    Sleep(2000);
  }

  mem::PID = mem::FindProcess(TEXT("RobloxPlayerBeta.exe"));
  if (!mem::PID) {
    return false;
  }
  mem::handleProcess =
      OpenProcess(PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION,
                  FALSE, mem::PID);
  if (!mem::handleProcess) {
    return false;
  }
  mem::base = mem::GetProcessBase();
  if (!mem::base) {
    return false;
  }
  if (!PerformSmartRescan(GameState::StateType::LOBBY_EMPTY)) {
    return false;
  }
  std::thread([]() {
    const DWORD CHECK_INTERVAL = 5000;
    HANDLE processHandle =
        OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, mem::PID);

    while (true) {
      bool robloxRunning = false;
      if (processHandle) {
        DWORD exitCode;
        if (GetExitCodeProcess(processHandle, &exitCode)) {
          robloxRunning = (exitCode == STILL_ACTIVE);
        }
      } else {
        DWORD currentPID = mem::FindProcess(TEXT("RobloxPlayerBeta.exe"));
        robloxRunning = (currentPID != 0);
      }

      if (!robloxRunning) {
        if (processHandle) {
          CloseHandle(processHandle);
        }
        shouldExit = true;
        break;
      }
      Sleep(CHECK_INTERVAL);
    }

    if (processHandle) {
      CloseHandle(processHandle);
    }
  }).detach();

  static bool gameIdLogged = false;
  static bool placeIdLogged = false;

  if (!gameIdLogged && storage::gameid != 0) {
    auto logTime = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(logTime);
    std::tm tm = *std::localtime(&time_t);
    char timeStr[9];
    std::strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &tm);
    printf("[INFO] [%s] GameID: %llu\n", timeStr, storage::gameid);
    gameIdLogged = true;
  }

  if (!placeIdLogged && storage::placeid != 0) {
    auto logTime = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(logTime);
    std::tm tm = *std::localtime(&time_t);
    char timeStr[9];
    std::strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &tm);
    printf("[INFO] [%s] PlaceID: %llu\n", timeStr, storage::placeid);
    placeIdLogged = true;
  }
  storage::init_menu_keybind();

  try {
      std::thread(AutoRescanHandler).detach();
      std::thread(&Engine::Instance::updatePlayers, &g_staticInstance).detach();
      std::thread(hooks::hook_aimbot).detach();
      std::thread(hooks::silentaim).detach();
      std::thread(hooks::hvh).detach();
      std::thread(hitsound_thread).detach();
      std::thread(arsenal_skinchanger).detach();
  } catch (const std::exception& e) {
      std::cout << "[!] Exception during thread initialization: " << e.what() << "\n";
  } catch (...) {
      std::cout << "[!] Unknown exception during thread initialization.\n";
  }
  return true;
}

bool Engine::InitializeStorage() {
  return PerformSmartRescan(GameState::StateType::LOBBY_EMPTY);
}

void StopAutoRescan() {
  AutoRescan::active = false;
  std::lock_guard<std::mutex> lock(g_smartCache.cacheMutex);
  g_smartCache.isValid = false;
}