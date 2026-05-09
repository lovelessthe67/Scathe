#pragma once
#include "utils/math/math.h"
#include <string>
#include <vector>

namespace Engine {
bool Initializer();
bool InitializeStorage();
bool hook_aimbot();
void TeleportTo(const Engine::Vector3 pos);
std::vector<uintptr_t> GetAllChildren(uintptr_t Parent);
class Instance {
public:
  static std::string ReadString(std::uint64_t addy);
  static void WriteString(std::uint64_t address, const std::string &value);
  Engine::Instance Adornee();
  Engine::Instance GetParent();
  std::string GetClass() const;
  uintptr_t primitive();
  std::vector<Engine::Instance> GetChildren() const;
  Engine::Instance FindFirstChildOfClass(std::string Name);
  Engine::Instance FindFirstChild(std::string Name);
  std::uint32_t GetColor3() const;
  uintptr_t FindFirstChildGay(uintptr_t Parent, const std::string &ChildName);
  Engine::Instance GetTeam();
  Engine::Instance GetService(std::string Name);
  Engine::Instance ReadService(std::string arg);
  Engine::Vector3 GetCameraPos();
  std::vector<Engine::Instance> GetPlayerList();
  std::uint64_t GetRenderView();
  Engine::Matrix4x4 GetViewMatrix();
  Engine::Matrix3x3 GetRotation();
  Engine::Matrix3x3 GetCframeLOl() const;
  Engine::CFrame GetCframeLOlz() const;
  Engine::Matrix3x3 GetCameraRotation();
  Engine::Instance GetVisualEngine();
  Engine::Instance ReturnDataModel();
  Engine::Vector3 GetSize() const;
  Engine::Instance teleporttoplayer(const std::string &targetPlayerName);
  Engine::Instance WriteCameraRot(Engine::Matrix3x3 pos);
  std::string GetName() const;
  std::string GetDisplayName() const;
  Engine::Instance GetDataModelPointer();
  Engine::Vector2 GetDimensions();
  Engine::Instance GetDataModel();
  Engine::Instance GetLocalPlayer() const;
  Engine::Instance GetModelInstance() const;
  std::uintptr_t GetPart() const;
  Engine::Vector3 GetPartPos() const;
  Engine::Vector3 GetPartPos2() const;
  Engine::Vector3 get_move_dir();
  Engine::Vector3 GetPartvelocity() const;
  void SetPartVelocity(const Engine::Vector3 &velocity);
  void SetAnchored(bool enable);
  Engine::Instance GetCamera() const;
  Engine::Matrix3x3 GetRotation() const;
  void SetPartPos(const Engine::Vector3 &position);
  void SetSize(const Engine::Vector3 &size);
  bool operator==(const Engine::Instance &other) const {
    return this->address == other.address;
  }
  uint64_t GetGameID();
  uint64_t GetPlaceID();
  uint64_t GetUserID();
  float GetHealth();
  float GetWalkSpeed();
  float GetJumpPower();
  float GetHipHeight();
  void SetWalkSpeed(float value);
  void WriteRot(Engine::Matrix3x3 pos);
  void SetJumpPower(float value);
  void SetIntValue(int value);
  int getIntFromValue() const;
  float GetMaxHealth() const;
  std::int32_t GetRigType();
  void SetUseJumpPower(bool usejumppower);
  void write_position(Engine::Vector3 pos);
  void write_velocity(Engine::Vector3 velocity);
  std::string get_specialmesh_id();
  void writedoublevalue(double val);
  void SpectatePart(uint64_t string);
  void SetBoolFromValue(bool value) const;
  bool getBoolFromValue() const;
  void SetBoolFromValue1(bool value) const;
  bool getBoolFromValue1() const;
  void SetCameraRotation(Engine::Matrix3x3 Rotation);
  void SetRotation(Engine::Matrix3x3 Rotation);
  void SetAnimationId(const std::string &AnimationID);
  static std::vector<uintptr_t> GetAllChildren(uintptr_t Parent);
  std::uint64_t GetInputObject(std::uint64_t base_address);
  void cached_input_objectzz();
  void SetFogColor(const float color);
  void CallCachedMouseService(std::uint64_t address);
  void FreeAim(Engine::Vector2 pos);
  void FogColorSet(Engine::Vector3 Color);
  void SetFogStart(float distance);
  void SetFogEnd(float distance);
  uint64_t SetFramePositionX(uint64_t position);
  uint64_t SetFramePositionY(uint64_t position);
  double DoubleValue();
  std::string TextLabelText();
  bool readBoolValue();
  bool GetCanCollide();
  bool SetCanCollide(bool enable);
  float GetTransparency() const;
  void SetTransparency(float transparency);
  void SetMeshId(const std::string &meshId);
  void SetTextureID(const std::string &textureId);
  void SetStringValue(const std::string &value);
  void SetStringValue1(const std::string &value);

  void SetMaterial(int16_t materialEnum);
  int16_t GetMaterial() const;

  uintptr_t address = 0;
  int fetchPlayer(std::uint64_t address) const;
  void updatePlayers();
  static std::uint64_t cached_input_object;
};
class TaskScheduler final {
public:
  std::uint64_t address = 0;
  uintptr_t GetTargetFPS();
  uintptr_t SetTargetFPS(double value);
  void PauseTask(uintptr_t jobAddress);
  uintptr_t GetScheduler();
  std::vector<Engine::Instance> GetJobs();
  std::string GetJobName(Engine::Instance instance);
  void ResumeTask(uintptr_t jobAddress);

  bool RemoveTaskByName(const std::string &tarGetName);
  void UpdateJobPriority(uintptr_t jobAddress, unsigned int newPriority);
  uintptr_t GetJobByName(const std::string &tarGetName);

private:
};

class PlayerIns final {
public:
  bool operator==(const PlayerIns &other) const {
    return address == other.address;
  }

  Engine::Instance pfLimbs1;
  Engine::Instance pfLimbs2;
  Engine::Instance pfLimbs3;
  Engine::Instance pfLimbs4;
  Engine::Instance pfLimbs5;

  bool isSwimming;
  bool isJumping;
  bool isValid;
  bool isRunning;
  bool isWalking;
  std::uint64_t address;
  std::vector<Engine::Instance> children;
  std::string name;
  std::string classname;
  std::string displayname;
  Engine::Instance team;
  Engine::Instance character;
  Engine::Vector3 rootpartpos;
  Engine::Vector3 headpos;
  Vector3 headPos;
  Vector3 rootPartPos;
  Vector3 upperTorsoPos;
  Vector3 lowerTorsoPos;
  Vector3 rightUpperArmPos;
  Vector3 leftUpperArmPos;
  Vector3 rightLowerArmPos;
  Vector3 leftLowerArmPos;
  Vector3 rightHandPos;
  Vector3 leftHandPos;
  Vector3 rightUpperLegPos;
  Vector3 leftUpperLegPos;
  Vector3 rightLowerLegPos;
  Vector3 leftLowerLegPos;
  Vector3 rightFootPos;
  Vector3 leftFootPos;
  Vector3 headSize;
  Vector3 rootPartSize;
  Vector3 upperTorsoSize;
  Vector3 lowerTorsoSize;
  Vector3 rightUpperArmSize;
  Vector3 leftUpperArmSize;
  Vector3 rightLowerArmSize;
  Vector3 leftLowerArmSize;
  Vector3 rightHandSize;
  Vector3 leftHandSize;
  Vector3 rightUpperLegSize;
  Vector3 leftUpperLegSize;
  Vector3 rightLowerLegSize;
  Vector3 leftLowerLegSize;
  Vector3 rightFootSize;
  Vector3 leftFootSize;
  Matrix3x3 Rotation;
  Vector3 Size;
  int r15;
  int shield;
  Engine::Instance bodyEffects;
  Engine::Instance knockedOut;
  float health;
  float maxhealth;
  Engine::Instance ifGrabbed;
  Engine::Instance reloadcheckaimbot;
  Engine::Instance mousePosition;
  Engine::Instance mousePos;
  Engine::Instance currentTool;
  Engine::Instance aim;
  Engine::Instance flame_obj;
  Engine::Instance tool;
  std::string currentToolName;
  Engine::Instance hc_aim;
  Engine::Instance Hood_Game;
  Engine::Instance weapon_aim;
  Engine::Instance Crosshair2;
  Engine::Instance jail_aim;
  Engine::Instance armor_obj;
  bool hasGunEquipped = false;
  Engine::Instance humanoid;
  Engine::Instance head;
  Engine::Instance rootJoint;
  Engine::Instance neck;
  Engine::Instance rootPart;
  Engine::Instance pf_localplr;
  Engine::Instance upperTorso;
  Engine::Instance lowerTorso;
  Engine::Instance leftUpperLeg;
  Engine::Instance leftFoot;
  Engine::Instance rightUpperLeg;
  Engine::Instance rightFoot;
  Engine::Instance leftUpperArm;
  Engine::Instance leftHand;
  Engine::Instance rightUpperArm;
  Engine::Instance leftLowerLeg;
  Engine::Instance leftLowerArm;
  Engine::Instance rightLowerArm;
  Engine::Instance rightLowerLeg;
  Engine::Instance rightHand;
  Engine::Instance leftLeg;  // r6
  Engine::Instance rightLeg; // r6
  Engine::Instance leftArm;  // r6
  Engine::Instance rightArm; // r6
  Engine::Instance Vehicles;
};
class WorkSpaceInstance final {
public:
  bool operator==(const PlayerIns &other) const {
    return address == other.address;
  }
  std::uint64_t address;
  std::vector<Engine::Instance> children;
  std::string name;
  std::string displayname;
  Engine::Instance mousePosition;
  Engine::Instance mousePos;
  Engine::Instance aim;
  Engine::Instance hc_aim;
  Engine::Instance Hood_Game;
  Engine::Instance weapon_aim;
  Engine::Instance Crosshair2;
  Engine::Instance jail_aim;

  Engine::Instance Vehicles;
};
class WorkSpace final {
public:
  bool operator==(const PlayerIns &other) const {
    return address == other.address;
  }
  Engine::Instance lighting;
  Engine::Instance workspace;
  Engine::Instance datamodel;
  Engine::Instance players;
  std::uint64_t address;
  std::vector<Engine::Instance> children;
  std::string name;
  std::string displayname;
};
Engine::Vector2 WorldToScreen(Engine::Vector3 World, Engine::Vector2 Dimensions,
                              Engine::Matrix4x4 ViewMatrix);
Engine::Vector2 worldtoscreen2(Vector3 world);
} // namespace Engine
