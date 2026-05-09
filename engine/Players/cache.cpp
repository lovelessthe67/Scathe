#include "../../overlay/xorstr/xorstr.hpp"
#include "engine/mem_module/memory.hpp"
#include "core/storage/Globals.hpp"
#include "offsets.hpp"
#include <ShlObj.h>
#include <TlHelp32.h>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <windows.h>
#include <wininet.h>

void syncList22(std::vector<Engine::WorkSpaceInstance> &oldList,
                const std::vector<Engine::WorkSpaceInstance> &newList) {
  ::SetThreadPriority(::GetCurrentThread(), 0x80);
  if (newList.empty())
    return;
  std::unordered_map<uintptr_t, Engine::WorkSpaceInstance> newEntitiesMap;
  for (const auto &entity : newList)
    newEntitiesMap[entity.address] = entity;
  oldList.erase(
      std::remove_if(oldList.begin(), oldList.end(),
                     [&newEntitiesMap](const Engine::WorkSpaceInstance &item) {
                       return newEntitiesMap.find(item.address) ==
                              newEntitiesMap.end();
                     }),
      oldList.end());
  for (const auto &entity : newList) {
    auto it = std::find_if(oldList.begin(), oldList.end(),
                           [&entity](const Engine::WorkSpaceInstance &item) {
                             if (item.address == entity.address)
                               return item.address == entity.address;
                           });
    if (it != oldList.end())
      *it = entity;
    else
      oldList.push_back(entity);
  }
}
std::vector<Engine::Instance> FrontLinesGetPlayers() {
  std::vector<Engine::Instance> returnplayers;
  if (!globals::frontlineteamcheck) {

    for (Engine::Instance playermodel : storage::workspace.GetChildren()) {
      if (playermodel.GetName() != "soldier_model")
        continue;
      returnplayers.push_back(playermodel);
    }
  } else {
    for (Engine::Instance playermodel : storage::workspace.GetChildren()) {
      if (playermodel.GetName() != "soldier_model")
        continue;
      if (playermodel.FindFirstChild("friendly_marker").address)
        continue;
      returnplayers.push_back(playermodel);
    }
  }
  return returnplayers;
}

std::vector<Engine::Instance> getBlackhawkRescuePlayers() {
  auto folder = storage::workspace;

  std::vector<Engine::Instance> targetPlayers;

  for (Engine::Instance &player : folder.GetChildren()) {
    if (player.GetName() == "Male") {
      targetPlayers.push_back(player);
    }
  }

  return targetPlayers;
}

std::vector<Engine::Instance> getWildWestPlayers() {
  auto folder = storage::workspace.FindFirstChild("WORKSPACE_Entities")
                    .FindFirstChild("Players");

  std::vector<Engine::Instance> targetPlayers;

  for (Engine::Instance &player : folder.GetChildren()) {
    targetPlayers.push_back(player);
  }

  return targetPlayers;
}

Engine::Instance PhantomForcesGetTeam(Engine::Instance PlayerModel) {
  auto folder = PlayerModel.FindFirstChildOfClass("Folder");
  if (!folder.address) {
    return storage::datamodel.FindFirstChildOfClass("Teams").FindFirstChild(
        "Ghosts");
  }

  for (auto &child : folder.GetChildren()) {
    if (child.GetColor3() == 0x5A4B36) {
      return storage::datamodel.FindFirstChildOfClass("Teams").FindFirstChild(
          "Phantoms");
    }
  }

  return storage::datamodel.FindFirstChildOfClass("Teams").FindFirstChild(
      "Ghosts");
}

std::vector<Engine::Instance> getAftermathPlayers() {
  std::vector<Engine::Instance> targetPlayers;

  Engine::Instance players = storage::workspace.FindFirstChild("Characters");
  if (!players.address) players = storage::workspace.FindFirstChild("characters");
  if (!players.address) players = storage::workspace.FindFirstChild("Entities");
  if (!players.address) players = storage::workspace.FindFirstChild("Live");
  if (!players.address) players = storage::workspace.FindFirstChild("Players");
  if (!players.address) players = storage::workspace.FindFirstChild("CharModels");
  if (!players.address) players = storage::workspace.FindFirstChild("Zombies");

  if (!players.address) return targetPlayers;

  for (Engine::Instance &player : players.GetChildren()) {
    // Removed the 'first child skip' as it causes issues if only one player is present
    targetPlayers.push_back(player);
  }

  return targetPlayers;
}

std::vector<Engine::Instance> getDeadlinePlayers() {
  std::vector<Engine::Instance> targetPlayers;

  Engine::Instance players = storage::workspace.FindFirstChild("characters");

  for (Engine::Instance &player : players.GetChildren()) {
    targetPlayers.push_back(player);
  }

  return targetPlayers;
}

std::vector<Engine::Instance> getRiotfallPlayers() {
  std::vector<Engine::Instance> targetPlayers;

  Engine::Instance players = storage::workspace.FindFirstChild("Characters");

  for (Engine::Instance &player : players.GetChildren()) {
    targetPlayers.push_back(player);
  }

  return targetPlayers;
}

std::vector<Engine::Instance> getTridentSurvivalPlayers() {
  std::vector<Engine::Instance> targetPlayers;

  std::vector<Engine::Instance> items = storage::workspace.GetChildren();

  for (Engine::Instance item : items) {
    if (item.GetClass() != "Model") {
      continue;
    }

    if (item.FindFirstChild("HumanoidRootPart").address != 0) {
      targetPlayers.push_back(item);
    }
  }

  return targetPlayers;
}
std::vector<Engine::Instance> getLoneSurvivalPlayers() {
  std::vector<Engine::Instance> targetPlayers;

  std::vector<Engine::Instance> items =
      storage::workspace.FindFirstChild("Players").GetChildren();

  for (Engine::Instance item : items) {
    targetPlayers.push_back(item);
  }

  return targetPlayers;
}

std::vector<Engine::Instance> bbGetPlayers() {
  std::vector<Engine::Instance> targetPlayers;
  auto charactersFolder = storage::workspace.FindFirstChild("Characters");
  
  if (charactersFolder.address == 0) {
    return targetPlayers;
  }

  for (auto &character : charactersFolder.GetChildren()) {
    if (character.GetClass() == "Model") {
      targetPlayers.push_back(character);
    }
  }

  return targetPlayers;
}
void DetectWeather() {
  if (storage::workspace.address == 0) {
    std::cout << "Workspace invalid\n";
    return;
  }

  Engine::Instance weatherClient =
      storage::workspace.FindFirstChild("WeatherClient");
  if (weatherClient.address == 0) {
    return;
  }

  std::cout << "WeatherClient found at " << weatherClient.GetName() << "\n";

  Engine::Instance activeModule =
      weatherClient.FindFirstChild("ActiveModuleName");
}
Engine::Instance IndexToBodyPart(Engine::Instance body, int index) {
  std::vector<Engine::Instance> childrenVector;
  for (auto &object : body.GetChildren()) {
    childrenVector.push_back(object);
  }
  if (index < childrenVector.size()) {
    return childrenVector[index];
  }
  return Engine::Instance();
}
namespace Engine {

int Instance::fetchPlayer(std::uint64_t address) const {
  return mem::read<int>(address);
}
std::atomic<bool> running{true};
std::mutex cachedPlayersMutex;
static std::unordered_map<uintptr_t, Engine::PlayerIns> g_entityMapCache;
static std::unordered_set<uintptr_t> g_knownAddresses;
static std::unordered_map<std::string, Engine::Instance> g_cachedChildren;
static std::vector<Engine::PlayerIns> g_tempPlayers;
static constexpr size_t MAX_PLAYERS = 200;
static constexpr size_t MAX_KNOWN_ADDRESSES = 500;
static constexpr size_t MAX_CACHED_CHILDREN = 100;
static std::atomic<int> g_cleanupCounter{0};
static constexpr int CLEANUP_INTERVAL = 1000;

// Split update system: full rescans less often, fast position updates frequently
static constexpr int FULL_RESCAN_INTERVAL = 40; // Full rescan every ~2 seconds (40 * 50ms)
static constexpr int FAST_UPDATE_MS = 50;       // Fast position updates every 50ms
static std::atomic<int> g_updateCounter{0};
void InitializeGlobalCaches() {
  g_entityMapCache.reserve(MAX_PLAYERS);
  g_knownAddresses.reserve(MAX_KNOWN_ADDRESSES);
  g_cachedChildren.reserve(MAX_CACHED_CHILDREN);
  g_tempPlayers.reserve(MAX_PLAYERS);
}
void PeriodicCleanup() {
  g_cleanupCounter++;
  if (g_cleanupCounter % CLEANUP_INTERVAL == 0) {
    if (g_knownAddresses.size() > MAX_KNOWN_ADDRESSES) {
      g_knownAddresses.clear();
      g_knownAddresses.reserve(MAX_KNOWN_ADDRESSES);
    }
    if (g_entityMapCache.size() > MAX_PLAYERS) {
      g_entityMapCache.clear();
      g_entityMapCache.reserve(MAX_PLAYERS);
    }
    g_cachedChildren.clear();
    g_cachedChildren.reserve(MAX_CACHED_CHILDREN);
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
}
void syncList(std::vector<Engine::PlayerIns> &oldList,
              const std::vector<Engine::PlayerIns> &newList) {
  ::SetThreadPriority(::GetCurrentThread(), THREAD_PRIORITY_NORMAL);

  if (newList.empty()) {
    oldList.clear();
    return;
  }
  g_entityMapCache.clear();
  for (const auto &entity : newList) {
    g_entityMapCache[entity.address] = entity;
  }
  oldList.erase(std::remove_if(oldList.begin(), oldList.end(),
                               [](const Engine::PlayerIns &item) {
                                 return g_entityMapCache.find(item.address) ==
                                        g_entityMapCache.end();
                               }),
                oldList.end());
  for (const auto &entity : newList) {
    auto it = std::find_if(oldList.begin(), oldList.end(),
                           [&entity](const Engine::PlayerIns &item) {
                             return item.address == entity.address;
                           });
    if (it != oldList.end())
      *it = entity;
    else
      oldList.push_back(entity);
  }
}

bool isOnScreen(const Engine::PlayerIns &player) {
  auto dimensions = storage::visualengine.GetDimensions();
  auto playerPosition = player.character.GetPartPos();
  auto screenPosition = Engine::WorldToScreen(
      playerPosition, dimensions, storage::visualengine.GetViewMatrix());

  return (screenPosition.x >= 0 && screenPosition.x <= dimensions.x) &&
         (screenPosition.y >= 0 && screenPosition.y <= dimensions.y);
}

// Fast update: only refresh positions and dynamic data for existing cached players
void FastUpdateCachedPlayers() {
  std::lock_guard<std::mutex> lock(cachedPlayersMutex);
  
  for (auto& player : storage::player_cache) {
    if (!player.address || !player.rootPart.address)
      continue;
    
    // Update positions only (fast memory reads)
    if (player.head.address) {
      player.headPos = player.head.GetPartPos();
    }
    if (player.rootPart.address) {
      player.rootpartpos = player.rootPart.GetPartPos();
    }
    
    // Update health (important for ESP)
    if (player.humanoid.address) {
      player.health = player.humanoid.GetHealth();
    }
    
    // Update knocked out status if applicable
    if (player.knockedOut.address) {
      // Already cached, just re-read value
    }
  }
  
  // Also update local player positions
  if (storage::localplayer.rootPart.address) {
    storage::localplayer.rootpartpos = storage::localplayer.rootPart.GetPartPos();
    if (storage::localplayer.head.address) {
      storage::localplayer.headPos = storage::localplayer.head.GetPartPos();
    }
  }
}
void Instance::updatePlayers() {
  InitializeGlobalCaches();

  auto &entityPool = storage::players;

  storage::camera = storage::workspace.FindFirstChild("Camera");
  storage::mouse_service = storage::game.FindFirstChild("MouseService").address;

  Sleep(3);
  auto mainUpdateLoop = [&]() {
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);

    while (running) {
      PeriodicCleanup();

      if (!storage::esp && !storage::aimbot && !storage::silent_aim) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        continue;
      }

      static int cache_tick = 0;
      if (cache_tick++ % 60 == 0) {
        storage::wallcheck.cacheWorkspace();
      }
      
      static auto last_bot_check = std::chrono::steady_clock::now();
      static std::vector<Engine::PlayerIns> cached_npcs;
      
      auto now = std::chrono::steady_clock::now();
      if (globals::botcheck && (globals::botmode == 1 || globals::botmode == 2)) {
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - last_bot_check).count() > 2000 || cached_npcs.empty()) {
          cached_npcs.clear();
          
          std::vector<Engine::Instance> modelsToCheck;
          
          Engine::Instance botsFolder = storage::workspace.FindFirstChild("Bots");
          if (botsFolder.address) {
            for (auto &bot : botsFolder.GetChildren()) {
              if (bot.GetClass() == "Model") {
                modelsToCheck.push_back(bot);
              }
            }
          }
          
          auto workspaceChildren = storage::workspace.GetChildren();
          for (auto &child : workspaceChildren) {
            if (child.GetClass() == "Model") {
              modelsToCheck.push_back(child);
            }
          }
          
          for (auto &child : modelsToCheck) {
            std::string modelName = child.GetName();
            if (modelName == storage::localplayer.name) continue;
            
            Engine::Instance humanoid = child.FindFirstChildOfClass("Humanoid");
            if (!humanoid.address) continue;
            
            Engine::Instance hrp = child.FindFirstChild("HumanoidRootPart");
            if (!hrp.address) continue;
            
            Engine::PlayerIns npc;
            npc.address = child.address;
            npc.name = modelName;
            npc.character = child;
            npc.humanoid = humanoid;
            npc.rootPart = hrp;
            npc.health = humanoid.GetHealth();
            npc.maxhealth = humanoid.GetMaxHealth();
            
            Engine::Instance head = child.FindFirstChild("Head");
            if (head.address) {
              npc.head = head;
              npc.headPos = head.GetPartPos();
              npc.headSize = head.GetSize();
            }
            
            npc.r15 = humanoid.GetRigType();
            
            for (auto &part : child.GetChildren()) {
              std::string partName = part.GetName();
              if (partName == "UpperTorso") {
                npc.upperTorso = part;
                npc.upperTorsoPos = part.GetPartPos();
                npc.upperTorsoSize = part.GetSize();
              }
              else if (partName == "LowerTorso") {
                npc.lowerTorso = part;
                npc.lowerTorsoPos = part.GetPartPos();
                npc.lowerTorsoSize = part.GetSize();
              }
              else if (partName == "Torso") {
                npc.upperTorso = part;
                npc.upperTorsoPos = part.GetPartPos();
                npc.upperTorsoSize = part.GetSize();
              }
              else if (partName == "LeftUpperArm") {
                npc.leftUpperArm = part;
                npc.leftUpperArmPos = part.GetPartPos();
                npc.leftUpperArmSize = part.GetSize();
              }
              else if (partName == "RightUpperArm") {
                npc.rightUpperArm = part;
                npc.rightUpperArmPos = part.GetPartPos();
                npc.rightUpperArmSize = part.GetSize();
              }
              else if (partName == "LeftLowerArm") {
                npc.leftLowerArm = part;
                npc.leftLowerArmPos = part.GetPartPos();
                npc.leftLowerArmSize = part.GetSize();
              }
              else if (partName == "RightLowerArm") {
                npc.rightLowerArm = part;
                npc.rightLowerArmPos = part.GetPartPos();
                npc.rightLowerArmSize = part.GetSize();
              }
              else if (partName == "LeftUpperLeg") {
                npc.leftUpperLeg = part;
                npc.leftUpperLegPos = part.GetPartPos();
                npc.leftUpperLegSize = part.GetSize();
              }
              else if (partName == "RightUpperLeg") {
                npc.rightUpperLeg = part;
                npc.rightUpperLegPos = part.GetPartPos();
                npc.rightUpperLegSize = part.GetSize();
              }
              else if (partName == "LeftLowerLeg") {
                npc.leftLowerLeg = part;
                npc.leftLowerLegPos = part.GetPartPos();
                npc.leftLowerLegSize = part.GetSize();
              }
              else if (partName == "RightLowerLeg") {
                npc.rightLowerLeg = part;
                npc.rightLowerLegPos = part.GetPartPos();
                npc.rightLowerLegSize = part.GetSize();
              }
              else if (partName == "LeftFoot") {
                npc.leftFoot = part;
                npc.leftFootPos = part.GetPartPos();
                npc.leftFootSize = part.GetSize();
              }
              else if (partName == "RightFoot") {
                npc.rightFoot = part;
                npc.rightFootPos = part.GetPartPos();
                npc.rightFootSize = part.GetSize();
              }
              else if (partName == "Left Arm") {
                npc.leftArm = part;
              }
              else if (partName == "Right Arm") {
                npc.rightArm = part;
              }
              else if (partName == "Left Leg") {
                npc.leftLeg = part;
              }
              else if (partName == "Right Leg") {
                npc.rightLeg = part;
              }
            }
            
            if (npc.rootPart.address && npc.head.address) {
              npc.rootpartpos = npc.rootPart.GetPartPos();
              npc.rootPartSize = npc.rootPart.GetSize();
              cached_npcs.push_back(std::move(npc));
            }
          }
          
          last_bot_check = now;
          std::cout << " " << cached_npcs.size() << " " << std::endl;
        }
      }
      
      g_tempPlayers.clear();

      if (!storage::players.address) {
        storage::player_cache.clear();
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        continue;
      }

      if (storage::gameid == 1168263273 || storage::placeid == 3233893879) {
        auto bbPlayers = bbGetPlayers();
        
        Engine::Instance teamsFolder = storage::game.FindFirstChild("Teams");
        Engine::Instance localPlayerInst = storage::players.GetLocalPlayer();
        Engine::Instance localTeam;
        
        if (globals::badbuisnessteamcheck && teamsFolder.address && localPlayerInst.address) {
          std::string localPlayerName = localPlayerInst.GetName();
          
          for (auto &teamFolder : teamsFolder.GetChildren()) {
            if (teamFolder.GetClass() != "Folder")
              continue;
            
            for (auto &teamMember : teamFolder.GetChildren()) {
              if (teamMember.GetName() == localPlayerName) {
                localTeam = teamFolder;
                break;
              }
            }
            
            if (localTeam.address)
              break;
          }
        }

        for (Engine::Instance PlayerModel : bbPlayers) {
          if (PlayerModel.GetClass() != "Model")
            continue;

          Engine::PlayerIns plr;
          plr.address = PlayerModel.address;
          plr.name = PlayerModel.GetName();
          plr.character = PlayerModel;
          plr.r15 = 1;

          Engine::Instance body = PlayerModel.FindFirstChild("Body");
          if (!body.address)
            continue;

          for (const auto &part : body.GetChildren()) {
            std::string partClass = part.GetClass();
            if (partClass != "Part" && partClass != "MeshPart")
              continue;

            std::string partName = part.GetName();

            if (partName == "Head") {
              plr.head = part;
              plr.headPos = part.GetPartPos();
              plr.headSize = part.GetSize();
            } else if (partName == "Abdomen") {
              plr.rootPart = part;
              plr.rootpartpos = part.GetPartPos();
              plr.rootPartSize = part.GetSize();
              plr.lowerTorso = part;
              plr.lowerTorsoPos = part.GetPartPos();
              plr.lowerTorsoSize = part.GetSize();
            } else if (partName == "Chest") {
              plr.upperTorso = part;
              plr.upperTorsoPos = part.GetPartPos();
              plr.upperTorsoSize = part.GetSize();
            } else if (partName == "Neck") {
              plr.neck = part;
            } else if (partName == "Hips") {
              plr.rootJoint = part;
            } else if (partName == "LeftArm") {
              plr.leftUpperArm = part;
              plr.leftUpperArmPos = part.GetPartPos();
              plr.leftUpperArmSize = part.GetSize();
            } else if (partName == "LeftForearm") {
              plr.leftLowerArm = part;
              plr.leftLowerArmPos = part.GetPartPos();
              plr.leftLowerArmSize = part.GetSize();
            } else if (partName == "LeftHand") {
              plr.leftHand = part;
              plr.leftHandPos = part.GetPartPos();
              plr.leftHandSize = part.GetSize();
            } else if (partName == "RightArm") {
              plr.rightUpperArm = part;
              plr.rightUpperArmPos = part.GetPartPos();
              plr.rightUpperArmSize = part.GetSize();
            } else if (partName == "RightForearm") {
              plr.rightLowerArm = part;
              plr.rightLowerArmPos = part.GetPartPos();
              plr.rightLowerArmSize = part.GetSize();
            } else if (partName == "RightHand") {
              plr.rightHand = part;
              plr.rightHandPos = part.GetPartPos();
              plr.rightHandSize = part.GetSize();
            } else if (partName == "LeftLeg") {
              plr.leftUpperLeg = part;
              plr.leftUpperLegPos = part.GetPartPos();
              plr.leftUpperLegSize = part.GetSize();
            } else if (partName == "LeftForeleg") {
              plr.leftLowerLeg = part;
              plr.leftLowerLegPos = part.GetPartPos();
              plr.leftLowerLegSize = part.GetSize();
            } else if (partName == "LeftFoot") {
              plr.leftFoot = part;
              plr.leftFootPos = part.GetPartPos();
              plr.leftFootSize = part.GetSize();
            } else if (partName == "RightLeg") {
              plr.rightUpperLeg = part;
              plr.rightUpperLegPos = part.GetPartPos();
              plr.rightUpperLegSize = part.GetSize();
            } else if (partName == "RightForeleg") {
              plr.rightLowerLeg = part;
              plr.rightLowerLegPos = part.GetPartPos();
              plr.rightLowerLegSize = part.GetSize();
            } else if (partName == "RightFoot") {
              plr.rightFoot = part;
              plr.rightFootPos = part.GetPartPos();
              plr.rightFootSize = part.GetSize();
            }
          }

          if (!plr.head.address || !plr.rootPart.address)
            continue;

          Engine::Instance healthObj = PlayerModel.FindFirstChild("Health");
          if (healthObj.address) {
            plr.health = healthObj.DoubleValue();
            Engine::Instance maxHealthObj = healthObj.FindFirstChild("MaxHealth");
            if (maxHealthObj.address) {
              plr.maxhealth = maxHealthObj.DoubleValue();
            }
          }

          Engine::Instance clothes = PlayerModel.FindFirstChild("Clothes");
          if (clothes.address == 0) {
            storage::localplayer = plr;
            continue;
          }
          
          if (globals::badbuisnessteamcheck && localTeam.address && teamsFolder.address) {
            bool isTeammate = false;
            
            for (auto &teamFolder : teamsFolder.GetChildren()) {
              if (teamFolder.GetClass() != "Folder")
                continue;
              
              for (auto &teamMember : teamFolder.GetChildren()) {
                if (teamMember.GetName() == plr.name) {
                  if (teamFolder.address == localTeam.address) {
                    isTeammate = true;
                  }
                  break;
                }
              }
              
              if (isTeammate)
                break;
            }
            
            if (isTeammate)
              continue;
          }
          
          g_tempPlayers.push_back(std::move(plr));
          g_knownAddresses.insert(plr.address);
        }

        std::lock_guard<std::mutex> lock(cachedPlayersMutex);
        syncList(storage::player_cache, g_tempPlayers);
      } else if (storage::gameid == 8767500166 || storage::placeid == 8767500166) {
        Engine::Instance npcsFolder = storage::workspace.FindFirstChild("NPCs");
        if (npcsFolder.address) {
          std::vector<Engine::Instance> npcFolders;
          
          Engine::Instance hostile = npcsFolder.FindFirstChild("Hostile");
          Engine::Instance other = npcsFolder.FindFirstChild("Other");
          Engine::Instance custom = npcsFolder.FindFirstChild("Custom");
          
          if (hostile.address) npcFolders.push_back(hostile);
          if (other.address) npcFolders.push_back(other);
          if (custom.address) npcFolders.push_back(custom);
          
          for (auto &folder : npcFolders) {
            for (auto &npc : folder.GetChildren()) {
              if (npc.GetClass() != "Model")
                continue;
              
              Engine::PlayerIns plr;
              plr.address = npc.address;
              plr.name = npc.GetName();
              plr.character = npc;
              plr.r15 = 0;
              
              Engine::Instance humanoid = npc.FindFirstChildOfClass("Humanoid");
              if (humanoid.address) {
                plr.humanoid = humanoid;
                plr.health = humanoid.GetHealth();
                plr.maxhealth = humanoid.GetMaxHealth();
              }
              
              for (auto &part : npc.GetChildren()) {
                std::string partName = part.GetName();
                
                if (partName == "Head") {
                  plr.head = part;
                } else if (partName == "Torso") {
                  plr.upperTorso = part;
                  plr.rootPart = part;
                } else if (partName == "Left Arm") {
                  plr.leftArm = part;
                } else if (partName == "Right Arm") {
                  plr.rightArm = part;
                } else if (partName == "Left Leg") {
                  plr.leftLeg = part;
                } else if (partName == "Right Leg") {
                  plr.rightLeg = part;
                } else if (partName == "HumanoidRootPart") {
                  plr.rootPart = part;
                }
              }
              
              if (!plr.rootPart.address || !plr.head.address)
                continue;
              
              g_tempPlayers.push_back(std::move(plr));
              g_knownAddresses.insert(plr.address);
            }
          }
        }
        
        std::lock_guard<std::mutex> lock(cachedPlayersMutex);
        syncList(storage::player_cache, g_tempPlayers);
      } else if (storage::gameid == 5286749994 || storage::gameid == 15327728308 || storage::placeid == 15327728308 || storage::placeid == 112237800564065) {
        auto players = getAftermathPlayers();

        for (auto &player : players) {
          Engine::PlayerIns plr{};
          plr.address = player.address;
          plr.name = player.GetName();

          for (auto &part : player.GetChildren()) {
            std::string name = part.GetName();

            if (name == "Head") {
              plr.head = part;
            }
            if (name == "Torso" || name == "UpperTorso")
              plr.upperTorso = part;
            if (name == "LowerTorso")
              plr.lowerTorso = part;
            if (name == "LeftLowerArm")
              plr.leftLowerArm = part;
            if (name == "LeftUpperArm")
              plr.leftUpperArm = part;
            if (name == "LeftHand")
              plr.leftHand = part;
            if (name == "LeftLowerLeg")
              plr.leftLowerLeg = part;
            if (name == "LeftUpperLeg")
              plr.leftUpperLeg = part;
            if (name == "LeftFoot")
              plr.leftFoot = part;
            if (name == "RightLowerArm")
              plr.rightLowerArm = part;
            if (name == "RightUpperArm")
              plr.rightUpperArm = part;
            if (name == "RightHand")
              plr.rightHand = part;
            if (name == "RightLowerLeg")
              plr.rightLowerLeg = part;
            if (name == "RightUpperLeg")
              plr.rightUpperLeg = part;
            if (name == "RightFoot")
              plr.rightFoot = part;
            if (name == "Root" || name == "HumanoidRootPart")
              plr.rootPart = part;
          }

          g_tempPlayers.push_back(std::move(plr));
          g_knownAddresses.insert(plr.address);
        }

        std::lock_guard<std::mutex> lock(cachedPlayersMutex);
        syncList(storage::player_cache, g_tempPlayers);
      } else if (storage::gameid == 2132866904) {
        for (Engine::Instance PlayerModel : FrontLinesGetPlayers()) {
          Engine::PlayerIns plr;
          plr.address = PlayerModel.address;
          plr.name = PlayerModel.GetName();

          g_cachedChildren.clear();
          for (auto &child : PlayerModel.GetChildren()) {
            g_cachedChildren[child.GetName()] = child;
          }
          plr.head = g_cachedChildren["TPVBodyVanillaHead"];
          plr.rootPart = g_cachedChildren["TPVBodyVanillaTorsoFront"];
          plr.leftUpperArm = g_cachedChildren["TPVBodyVanillaArmL"];
          plr.rightUpperArm = g_cachedChildren["TPVBodyVanillaArmR"];
          plr.leftHand = g_cachedChildren["TPVBodyVanillaGloveL"];
          plr.rightHand = g_cachedChildren["TPVBodyVanillaGloveR"];
          plr.leftUpperLeg = g_cachedChildren["TPVBodyVanillaLegL"];
          plr.rightUpperLeg = g_cachedChildren["TPVBodyVanillaLegR"];
          plr.leftLowerLeg = g_cachedChildren["TPVBodyVanillaKneecapL"];
          plr.rightLowerLeg = g_cachedChildren["TPVBodyVanillaKneecapR"];
          plr.leftFoot = g_cachedChildren["TPVBodyVanillaShoesL"];
          plr.rightFoot = g_cachedChildren["TPVBodyVanillaShoesR"];
          plr.lowerTorso = g_cachedChildren["TPVBodyVanillaTorsoBack"];

          plr.team = storage::players.GetLocalPlayer().GetTeam();

          Engine::Instance healthObj = PlayerModel.FindFirstChild("Health");
          if (healthObj.address) {
            plr.health = healthObj.DoubleValue();
            Engine::Instance maxHealthObj =
                healthObj.FindFirstChild("MaxHealth");
            if (maxHealthObj.address) {
              plr.maxhealth = maxHealthObj.DoubleValue();
            }
          }

          Engine::Instance backpack = PlayerModel.FindFirstChild("Backpack");
          if (backpack.address) {
            plr.tool = backpack.FindFirstChild("Primary");
            plr.currentToolName =
                plr.tool.address ? plr.tool.GetName() : "[NONE]";
          }

          g_tempPlayers.push_back(std::move(plr));
          g_knownAddresses.insert(plr.address);
        }
      } else if (storage::gameid == 1054526971) {
        auto players = getBlackhawkRescuePlayers();

        for (auto &player : players) {
          Engine::PlayerIns plr{};
          plr.address = player.address;
          plr.name = player.GetName();

          for (auto &part : player.GetChildren()) {
            std::string name = part.GetName();

            if (name == "Head") {
              plr.head = part;
            }
            if (name == "Torso")
              plr.upperTorso = part;
            if (name == "LowerTorso")
              plr.lowerTorso = part;
            if (name == "LeftLowerArm")
              plr.leftLowerArm = part;
            if (name == "LeftUpperArm")
              plr.leftUpperArm = part;
            if (name == "LeftHand")
              plr.leftHand = part;
            if (name == "LeftLowerLeg")
              plr.leftLowerLeg = part;
            if (name == "LeftUpperLeg")
              plr.leftUpperLeg = part;
            if (name == "LeftFoot")
              plr.leftFoot = part;
            if (name == "RightLowerArm")
              plr.rightLowerArm = part;
            if (name == "RightUpperArm")
              plr.rightUpperArm = part;
            if (name == "RightHand")
              plr.rightHand = part;
            if (name == "RightLowerLeg")
              plr.rightLowerLeg = part;
            if (name == "RightUpperLeg")
              plr.rightUpperLeg = part;
            if (name == "RightFoot")
              plr.rightFoot = part;
            if (name == "Root")
              plr.rootPart = part;
          }

          g_tempPlayers.push_back(std::move(plr));
          g_knownAddresses.insert(plr.address);
        }

        std::lock_guard<std::mutex> lock(cachedPlayersMutex);
        syncList(storage::player_cache, g_tempPlayers);
      } else if (storage::gameid == 807930589) {
        auto players = getWildWestPlayers();

        for (auto &player : players) {
          Engine::PlayerIns plr{};
          plr.address = player.address;

          Engine::Instance localPlayerCheck =
              Engine::Instance(mem::read<uintptr_t>(
                  storage::players.address + Offsets::Player::LocalPlayer));
          if (localPlayerCheck.address) {
            storage::localplayer.name = localPlayerCheck.GetName();
          }

          std::string name = player.GetName();
          if (name == storage::localplayer.name)
            continue;

          plr.name = name;

          for (auto &part : player.GetChildren()) {
            std::string name = part.GetName();

            if (name == "Head") {
              plr.head = part;
            }
            if (name == "Torso")
              plr.upperTorso = part;
            if (name == "LowerTorso")
              plr.lowerTorso = part;
            if (name == "LeftLowerArm")
              plr.leftLowerArm = part;
            if (name == "LeftUpperArm")
              plr.leftUpperArm = part;
            if (name == "LeftHand")
              plr.leftHand = part;
            if (name == "LeftLowerLeg")
              plr.leftLowerLeg = part;
            if (name == "LeftUpperLeg")
              plr.leftUpperLeg = part;
            if (name == "LeftFoot")
              plr.leftFoot = part;
            if (name == "RightLowerArm")
              plr.rightLowerArm = part;
            if (name == "RightUpperArm")
              plr.rightUpperArm = part;
            if (name == "RightHand")
              plr.rightHand = part;
            if (name == "RightLowerLeg")
              plr.rightLowerLeg = part;
            if (name == "RightUpperLeg")
              plr.rightUpperLeg = part;
            if (name == "RightFoot")
              plr.rightFoot = part;
            if (name == "HumanoidRootPart")
              plr.rootPart = part;
          }

          g_tempPlayers.push_back(std::move(plr));
          g_knownAddresses.insert(plr.address);
        }

        std::lock_guard<std::mutex> lock(cachedPlayersMutex);
        syncList(storage::player_cache, g_tempPlayers);
      } else if (storage::gameid == 3021395192) {
        auto players = getRiotfallPlayers();

        for (auto &player : players) {
          Engine::PlayerIns plr{};
          plr.address = player.address;

          Engine::Instance localPlayerCheck =
              Engine::Instance(mem::read<uintptr_t>(
                  storage::players.address + Offsets::Player::LocalPlayer));
          if (localPlayerCheck.address) {
            storage::localplayer.name = localPlayerCheck.GetName();
          }

          std::string name = player.GetName();
          if (name == storage::localplayer.name)
            continue;

          plr.name = name;

          for (auto &part : player.GetChildren()) {
            std::string name = part.GetName();

            if (name == "Top") {
              plr.head = part;
            }
            if (name == "Center")
              plr.rootPart = part;
            // if (name == "Center") plr.upperTorso = part;
            if (name == "Bottom")
              plr.rightLeg = part;
            if (name == "Bottom")
              plr.leftLeg = part;
          }

          g_tempPlayers.push_back(std::move(plr));
          g_knownAddresses.insert(plr.address);
        }

        std::lock_guard<std::mutex> lock(cachedPlayersMutex);
        syncList(storage::player_cache, g_tempPlayers);
      } else if (storage::gameid == 4712109542) {
        auto players = getLoneSurvivalPlayers();

        for (auto &player : players) {
          Engine::PlayerIns plr{};
          plr.address = player.address;

          Engine::Instance localPlayerCheck =
              Engine::Instance(mem::read<uintptr_t>(
                  storage::players.address + Offsets::Player::LocalPlayer));
          if (localPlayerCheck.address) {
            storage::localplayer.name = localPlayerCheck.GetName();
          }

          std::string name = player.GetName();
          if (name == storage::localplayer.name)
            continue;

          plr.name = name;

          for (auto &part : player.GetChildren()) {
            std::string name = part.GetName();

            if (name == "Head") {
              plr.head = part;
            }
            if (name == "Torso")
              plr.upperTorso = part;
            if (name == "LowerTorso")
              plr.lowerTorso = part;
            if (name == "LeftLowerArm")
              plr.leftLowerArm = part;
            if (name == "LeftUpperArm")
              plr.leftUpperArm = part;
            if (name == "LeftHand")
              plr.leftHand = part;
            if (name == "LeftLowerLeg")
              plr.leftLowerLeg = part;
            if (name == "LeftUpperLeg")
              plr.leftUpperLeg = part;
            if (name == "LeftFoot")
              plr.leftFoot = part;
            if (name == "RightLowerArm")
              plr.rightLowerArm = part;
            if (name == "RightUpperArm")
              plr.rightUpperArm = part;
            if (name == "RightHand")
              plr.rightHand = part;
            if (name == "RightLowerLeg")
              plr.rightLowerLeg = part;
            if (name == "RightUpperLeg")
              plr.rightUpperLeg = part;
            if (name == "RightFoot")
              plr.rightFoot = part;
            if (name == "HumanoidRootPart")
              plr.rootPart = part;
          }

          g_tempPlayers.push_back(std::move(plr));
          g_knownAddresses.insert(plr.address);
        }

        std::lock_guard<std::mutex> lock(cachedPlayersMutex);
        syncList(storage::player_cache, g_tempPlayers);
      } else if (storage::gameid == 113491250 || storage::gameid == 292439477) {
        std::unordered_map<std::string, Engine::Instance> playerserviceTeams;

        if (storage::players.address) {
          auto playerserviceChildren = storage::players.GetChildren();
          for (auto &player : playerserviceChildren) {
            if (!player.address)
              continue;
            if (player.GetClass() != "Player")
              continue;

            std::string playerName = player.GetName();
            Engine::Instance team = mem::read<Engine::Instance>(
                player.address + Offsets::Player::Team);
            playerserviceTeams[playerName] = team;
          }
        }

        Engine::Instance TeamsFolder =
            storage::workspace.FindFirstChild("Players");

        if (TeamsFolder.address && TeamsFolder.GetChildren().size() == 2) {
          auto teams = TeamsFolder.GetChildren();

          Engine::Instance localPlayerTeam;
          auto localPlayer = storage::players.GetLocalPlayer();
          if (localPlayer.address) {
            localPlayerTeam = mem::read<Engine::Instance>(
                localPlayer.address + Offsets::Player::Team);
            storage::localplayer.address = localPlayer.address;
            storage::localplayer.team = localPlayerTeam;
          }

          for (auto &team : teams) {
            for (Engine::Instance PlayerModel : team.GetChildren()) {
              Engine::PlayerIns plr;
              plr.address = PlayerModel.address;

              for (auto part : PlayerModel.GetChildren()) {
                auto mesh = part.FindFirstChildOfClass("SpecialMesh");
                if (!mesh.address)
                  continue;

                Engine::Instance billboard =
                    part.FindFirstChildOfClass("BillboardGui");

                if (billboard.address != 0) {
                  Engine::Instance textLabel =
                      billboard.FindFirstChildOfClass("TextLabel");
                  if (textLabel.address != 0) {
                    plr.name = mem::fetchstring(textLabel.address + 0xe40);
                  }

                  // Force team mapping from Folder Name
                  Engine::Instance teamsService =
                      storage::datamodel.FindFirstChildOfClass("Teams");
                  if (teamsService.address) {
                    plr.team = teamsService.FindFirstChild(team.GetName());
                  }

                  auto it = playerserviceTeams.find(plr.name);
                  if (it != playerserviceTeams.end() && !plr.team.address) {
                    plr.team = it->second;
                  }

                  plr.head = part;
                  plr.headSize = {1.f, 1.f, 1.f};
                  continue;
                }

                if (part.FindFirstChildOfClass("SpotLight").address != 0) {
                  plr.upperTorso = part;
                  plr.upperTorsoSize = {2.f, 2.f, 1.f};
                  continue;
                }

                if (!plr.pfLimbs1.address)
                  plr.pfLimbs1 = part;
                else if (!plr.pfLimbs2.address)
                  plr.pfLimbs2 = part;
                else if (!plr.pfLimbs3.address)
                  plr.pfLimbs3 = part;
                else if (!plr.pfLimbs4.address)
                  plr.pfLimbs4 = part;
                else if (!plr.pfLimbs5.address)
                  plr.pfLimbs5 = part;
              }

              if (localPlayerTeam.address && plr.team.address) {
                if (plr.team.address == localPlayerTeam.address) {
                  continue;
                }
              }

              g_tempPlayers.push_back(std::move(plr));
            }
          }
        }
        std::lock_guard<std::mutex> lock(cachedPlayersMutex);
        syncList(storage::player_cache, g_tempPlayers);
      } else if (storage::gameid == 5995470825) {
        try {
          if (!storage::players.address)
            return;
          auto localPlayerInst = storage::players.GetLocalPlayer();
          auto children = storage::players.GetChildren();
          for (Engine::Instance &player : children) {
            if (!player.address)
              continue;
            if (player.GetClass() != "Player")
              continue;
            std::string playerName = player.GetName();
            if (!storage::localplayercheck &&
                !storage::visuals::allow_local_player &&
                player.address == localPlayerInst.address)
              continue;
            Engine::Instance character;
            bool foundCharacter = false;
            if (storage::workspace.address) {
              auto workspaceChildren = storage::workspace.GetChildren();
              for (auto &child : workspaceChildren) {
                if (!child.address)
                  continue;
                if (child.GetName() == playerName &&
                    child.GetClass() == "Model") {
                  auto humanoid = child.FindFirstChild("Humanoid");
                  auto rootPart = child.FindFirstChild("HumanoidRootPart");
                  if (humanoid.address && rootPart.address) {
                    character = child;
                    foundCharacter = true;
                    break;
                  }
                }
              }
            }
            if (!foundCharacter)
              continue;
            Engine::PlayerIns entity;
            entity.name = playerName;
            entity.address = player.address;
            entity.character = character;
            entity.displayname = player.GetDisplayName();
            entity.team = player.GetTeam();
            entity.rootPart = character.FindFirstChild("HumanoidRootPart");
            entity.humanoid = character.FindFirstChild("Humanoid");
            entity.head = character.FindFirstChild("Head");
            if (!entity.rootPart.address || !entity.humanoid.address)
              continue;
            entity.health = entity.humanoid.GetHealth();
            entity.maxhealth = entity.humanoid.GetMaxHealth();
            entity.r15 = entity.humanoid.GetRigType();
            entity.rootpartpos = entity.rootPart.GetPartPos();
            if (entity.head.address)
              entity.headPos = entity.head.GetPartPos();
            g_cachedChildren.clear();
            for (auto &child : character.GetChildren())
              if (child.address)
                g_cachedChildren[child.GetName()] = child;
            if (g_cachedChildren["LeftUpperLeg"].address) {
              entity.r15 = 1;
              entity.upperTorso = g_cachedChildren["UpperTorso"];
              entity.lowerTorso = g_cachedChildren["LowerTorso"];
              entity.leftHand = g_cachedChildren["LeftHand"];
              entity.rightHand = g_cachedChildren["RightHand"];
              entity.leftLowerArm = g_cachedChildren["LeftLowerArm"];
              entity.rightLowerArm = g_cachedChildren["RightLowerArm"];
              entity.leftUpperArm = g_cachedChildren["LeftUpperArm"];
              entity.rightUpperArm = g_cachedChildren["RightUpperArm"];
              entity.leftFoot = g_cachedChildren["LeftFoot"];
              entity.leftLowerLeg = g_cachedChildren["LeftLowerLeg"];
              entity.leftUpperLeg = g_cachedChildren["LeftUpperLeg"];
              entity.rightLowerLeg = g_cachedChildren["RightLowerLeg"];
              entity.rightFoot = g_cachedChildren["RightFoot"];
              entity.rightUpperLeg = g_cachedChildren["RightUpperLeg"];
            } else {
              entity.r15 = 0;
              entity.upperTorso = g_cachedChildren["Torso"];
              entity.leftUpperArm = g_cachedChildren["Left Arm"];
              entity.rightUpperArm = g_cachedChildren["Right Arm"];
              entity.leftUpperLeg = g_cachedChildren["Left Leg"];
              entity.rightUpperLeg = g_cachedChildren["Right Leg"];
            }
            auto tool = character.FindFirstChildOfClass("Tool");
            entity.currentTool = tool;
            entity.currentToolName = tool.address ? tool.GetName() : "[NONE]";
            entity.bodyEffects = character.FindFirstChild("BodyEffects");
            if (entity.bodyEffects.address) {
              entity.knockedOut = entity.bodyEffects.FindFirstChild("K.O");
              entity.ifGrabbed = entity.bodyEffects.FindFirstChild("Grabbed");
            }
            g_tempPlayers.push_back(std::move(entity));
          }
        } catch (...) {
        }
        std::lock_guard<std::mutex> lock(cachedPlayersMutex);
        syncList(storage::player_cache, g_tempPlayers);
      } else if (storage::gameid == 4283416256) {
        auto players = getDeadlinePlayers();

        for (auto &player : players) {
          Engine::PlayerIns plr{};
          plr.address = player.address;
          plr.name = player.GetName();

          g_cachedChildren.clear();
          auto charChildren = player.GetChildren();
          for (auto &child : charChildren) {
            if (child.address) {
              g_cachedChildren[child.GetName()] = child;
            }
          }

          for (auto &part : player.GetChildren()) {
            std::string name = part.GetName();

            if (name == "Head") {
              plr.head = part;
              // plr.name =
              // part.FindFirstChild("Nametag").FindFirstChild("tag");
            }
            if (name == "Torso")
              plr.upperTorso = part;
            if (name == "LowerTorso")
              plr.lowerTorso = part;
            if (name == "LeftLowerArm")
              plr.leftLowerArm = part;
            if (name == "LeftUpperArm")
              plr.leftUpperArm = part;
            if (name == "LeftHand")
              plr.leftHand = part;
            if (name == "LeftLowerLeg")
              plr.leftLowerLeg = part;
            if (name == "LeftUpperLeg")
              plr.leftUpperLeg = part;
            if (name == "LeftFoot")
              plr.leftFoot = part;
            if (name == "RightLowerArm")
              plr.rightLowerArm = part;
            if (name == "RightUpperArm")
              plr.rightUpperArm = part;
            if (name == "RightHand")
              plr.rightHand = part;
            if (name == "RightLowerLeg")
              plr.rightLowerLeg = part;
            if (name == "RightUpperLeg")
              plr.rightUpperLeg = part;
            if (name == "RightFoot")
              plr.rightFoot = part;
            if (name == "HumanoidRootPart")
              plr.rootPart = part;
          }

          g_tempPlayers.push_back(std::move(plr));
          g_knownAddresses.insert(plr.address);
        }

        std::lock_guard<std::mutex> lock(cachedPlayersMutex);
        syncList(storage::player_cache, g_tempPlayers);
      } else if (storage::gameid == 4620241901) {
        auto players = getTridentSurvivalPlayers();

        for (auto &player : players) {
          Engine::PlayerIns plr{};
          plr.address = player.address;
          plr.name = player.GetName();

          for (auto &part : player.GetChildren()) {
            std::string name = part.GetName();

            if (name == "Head") {
              plr.head = part;
              plr.name = mem::fetchstring(
                  part.FindFirstChild("Nametag").FindFirstChild("tag").address +
                  0xa98);
            }
            if (name == "Torso")
              plr.upperTorso = part;
            if (name == "LowerTorso")
              plr.lowerTorso = part;
            if (name == "LeftLowerArm")
              plr.leftLowerArm = part;
            if (name == "LeftUpperArm")
              plr.leftUpperArm = part;
            if (name == "LeftHand")
              plr.leftHand = part;
            if (name == "LeftLowerLeg")
              plr.leftLowerLeg = part;
            if (name == "LeftUpperLeg")
              plr.leftUpperLeg = part;
            if (name == "LeftFoot")
              plr.leftFoot = part;
            if (name == "RightLowerArm")
              plr.rightLowerArm = part;
            if (name == "RightUpperArm")
              plr.rightUpperArm = part;
            if (name == "RightHand")
              plr.rightHand = part;
            if (name == "RightLowerLeg")
              plr.rightLowerLeg = part;
            if (name == "RightUpperLeg")
              plr.rightUpperLeg = part;
            if (name == "RightFoot")
              plr.rightFoot = part;
            if (name == "HumanoidRootPart")
              plr.rootPart = part;
          }

          g_tempPlayers.push_back(std::move(plr));
          g_knownAddresses.insert(plr.address);
        }

        std::lock_guard<std::mutex> lock(cachedPlayersMutex);
        syncList(storage::player_cache, g_tempPlayers);
      } else {
        for (Engine::Instance &player : entityPool.GetChildren()) {
          if (!player.address)
            continue;
          if (player.GetClass() != "Player")
            continue;

          auto character = player.GetModelInstance();
          if (!character.address)
            continue;

          g_cachedChildren.clear();
          for (auto &child : character.GetChildren()) {
            g_cachedChildren[child.GetName()] = child;
          }

          Engine::PlayerIns entity;
          entity.name = player.GetName();
          entity.displayname = player.GetDisplayName();
          entity.address = player.address;
          entity.character = character;
          entity.head = g_cachedChildren["Head"];
          entity.rootPart = g_cachedChildren["HumanoidRootPart"];
          entity.humanoid = g_cachedChildren["Humanoid"];

          if (entity.humanoid.address) {
            entity.r15 = entity.humanoid.GetRigType();
            entity.health = entity.humanoid.GetHealth();
            entity.maxhealth = entity.humanoid.GetMaxHealth();
          }
          if (g_cachedChildren["LeftUpperLeg"].address) {
            entity.r15 = 1;
            entity.upperTorso = g_cachedChildren["UpperTorso"];
            entity.lowerTorso = g_cachedChildren["LowerTorso"];
            entity.leftHand = g_cachedChildren["LeftHand"];
            entity.rightHand = g_cachedChildren["RightHand"];
            entity.leftLowerArm = g_cachedChildren["LeftLowerArm"];
            entity.rightLowerArm = g_cachedChildren["RightLowerArm"];
            entity.leftUpperArm = g_cachedChildren["LeftUpperArm"];
            entity.rightUpperArm = g_cachedChildren["RightUpperArm"];
            entity.leftFoot = g_cachedChildren["LeftFoot"];
            entity.leftLowerLeg = g_cachedChildren["LeftLowerLeg"];
            entity.leftUpperLeg = g_cachedChildren["LeftUpperLeg"];
            entity.rightLowerLeg = g_cachedChildren["RightLowerLeg"];
            entity.rightFoot = g_cachedChildren["RightFoot"];
            entity.rightUpperLeg = g_cachedChildren["RightUpperLeg"];
          } else {
            entity.r15 = 0;
            entity.upperTorso = g_cachedChildren["Torso"];
            entity.leftArm = g_cachedChildren["Left Arm"];
            entity.rightArm = g_cachedChildren["Right Arm"];
            entity.leftLeg = g_cachedChildren["Left Leg"];
            entity.rightLeg = g_cachedChildren["Right Leg"];

            // Keep synonyms for compatibility
            entity.leftUpperArm = entity.leftArm;
            entity.rightUpperArm = entity.rightArm;
            entity.leftUpperLeg = entity.leftLeg;
            entity.rightUpperLeg = entity.rightLeg;
          }
          auto tool = character.FindFirstChildOfClass("Tool");
          entity.currentTool = tool;
          entity.currentToolName = tool.address ? tool.GetName() : "[NONE]";

          entity.bodyEffects = character.FindFirstChild("BodyEffects");
          if (entity.bodyEffects.address) {
            entity.knockedOut = entity.bodyEffects.FindFirstChild("K.O");
            entity.ifGrabbed = entity.bodyEffects.FindFirstChild("Grabbed");
          }
          entity.team = player.GetTeam();
          if (entity.head.address) {
            entity.headPos = entity.head.GetPartPos();
            entity.headSize = entity.head.GetSize();
          }
          if (entity.rootPart.address) {
            entity.rootpartpos = entity.rootPart.GetPartPos();
            entity.rootPartSize = entity.rootPart.GetSize();
          }
          if (player.address == storage::players.GetLocalPlayer().address) {
            storage::localplayer = entity;
            // Only skip adding to cache if localplayercheck is false AND Allow
            // Local Player is disabled
            if (!storage::localplayercheck &&
                !storage::visuals::allow_local_player) {
              continue;
            }
          }

          g_tempPlayers.push_back(std::move(entity));
        }
      }
      // Split update system: full rescan less often, fast updates in between
      g_updateCounter++;
      bool doFullRescan = (g_updateCounter % FULL_RESCAN_INTERVAL == 0) || storage::player_cache.empty();
      
      if (doFullRescan) {
        if (globals::botcheck) {
          if (globals::botmode == 0) {
            
          } else if (globals::botmode == 1) {
            g_tempPlayers.clear();
            for (auto &npc : cached_npcs) {
              g_tempPlayers.push_back(npc);
            }
          } else if (globals::botmode == 2) {
            for (auto &npc : cached_npcs) {
              g_tempPlayers.push_back(npc);
            }
          }
        }
        
        std::lock_guard<std::mutex> lock(cachedPlayersMutex);
        syncList(storage::player_cache, g_tempPlayers);
      } else {
        FastUpdateCachedPlayers();
      }

      std::this_thread::sleep_for(std::chrono::milliseconds(FAST_UPDATE_MS));
    }
  };
  std::thread updateThread(mainUpdateLoop);
  updateThread.detach();

  while (running) {
    Sleep(100);
  }
}
} // namespace Engine
void CleanupGlobalMemory() {
  if (Engine::g_entityMapCache.size() > Engine::MAX_PLAYERS) {
    Engine::g_entityMapCache.clear();
    Engine::g_entityMapCache.reserve(Engine::MAX_PLAYERS);
  }

  if (Engine::g_knownAddresses.size() > Engine::MAX_KNOWN_ADDRESSES) {
    Engine::g_knownAddresses.clear();
    Engine::g_knownAddresses.reserve(Engine::MAX_KNOWN_ADDRESSES);
  }

  Engine::g_cachedChildren.clear();
  Engine::g_cachedChildren.reserve(Engine::MAX_CACHED_CHILDREN);
}