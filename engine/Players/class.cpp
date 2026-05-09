#include <algorithm>
#include <iostream>
#include <string>
#include <windows.h>

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

#include "engine/mem_module/memory.hpp"
#include "core/storage/Globals.hpp"
#include "offsets.hpp"
#include <regex>

void Engine::TeleportTo(const Engine::Vector3 pos) {
  auto localPlayer = storage::players.GetLocalPlayer();
  if (!localPlayer.address) return;

  auto character = localPlayer.GetModelInstance();
  if (!character.address) return;

  auto hrp = character.FindFirstChild("HumanoidRootPart");
  if (hrp.address) {
    Engine::Vector3 targetPos = pos;
    targetPos.y += 5.0f;
    hrp.SetPartPos(targetPos);
    hrp.SetPartVelocity({0.0f, 0.0f, 0.0f});
  }
}

std::vector<uintptr_t> Engine::GetAllChildren(uintptr_t Parent) {
  std::vector<uintptr_t> result;

  uintptr_t ChildrenPtr = mem::read<uintptr_t>(Parent + Offsets::Instance::ChildrenStart);
  if (!ChildrenPtr) return result;

  uintptr_t start = mem::read<uintptr_t>(ChildrenPtr);
  uintptr_t end = mem::read<uintptr_t>(ChildrenPtr + 8);

  if (!start || !end || end < start || (end - start) > 0x10000) return result;

  for (uintptr_t i = start; i < end; i += 16) {
    uintptr_t child = mem::read<uintptr_t>(i);
    if (child)
      result.push_back(child);
  }

  return result;
}

Engine::Instance Engine::Instance::ReturnDataModel() {
  auto VisualEngine = mem::read<Engine::Instance>(
      mem::GetProcessBase() + Offsets::VisualEngine::Pointer);

  storage::visualengine = VisualEngine;

  auto FakeDataModel = mem::read<Engine::Instance>(
      VisualEngine.address + Offsets::VisualEngine::ToDataModel1);

  auto fakedm = mem::read<std::uint64_t>(mem::GetProcessBase() +
                                         Offsets::FakeDataModel::Pointer);

  auto DataModel = mem::read<Engine::Instance>(
      fakedm + Offsets::FakeDataModel::RealDataModel);

  return DataModel;
}

Engine::Vector3 Engine::Instance::GetSize() const {
  return mem::read<Engine::Vector3>(this->GetPart() + Offsets::BasePart::Size);
}

struct stringed {
  char data[200];
};

std::string read_string(std::uint64_t address) {
  auto character = mem::read<stringed>(address);
  return character.data;
}

std::string mem::fetchstring(std::uint64_t address) {
  int length = mem::read<int>(address + 0x18);

  if (length >= 16u) {
    std::uintptr_t padding = mem::read<std::uintptr_t>(address);
    return read_string(padding);
  }
  std::string name = read_string(address);
  return name;
}

std::string Engine::Instance::GetDisplayName() const {
  const std::uint64_t name_pointer =
      mem::read<std::uint64_t>(this->address + Offsets::Player::DisplayName);

  std::string ptr_result;

  if (name_pointer) {
    ptr_result = mem::fetchstring(name_pointer);

    if (ptr_result == ("")) {
      ptr_result =
          mem::fetchstring(this->address + Offsets::Player::DisplayName);
      return ptr_result;
    }
  }

  return ("Unable to read displayname");
}

std::string Engine::Instance::GetName() const {
  return mem::fetchstring(
      mem::read<uintptr_t>(this->address + Offsets::Instance::Name));
}

std::string Engine::Instance::GetClass() const {
  std::uint64_t classAddress = mem::read<std::uint64_t>(
      this->address + Offsets::Instance::ClassDescriptor);
  if (!classAddress) return "";
  
  std::uint64_t sizeAddress = classAddress + Offsets::Instance::ChildrenEnd;
  std::uint64_t classNameSize = mem::read<std::uint64_t>(sizeAddress);
  return Engine::Instance::ReadString(classNameSize);
}

std::vector<Engine::Instance> Engine::Instance::GetChildren() const {
  std::vector<Engine::Instance> children;

  uintptr_t ChildrenPtr = mem::read<uintptr_t>(this->address + Offsets::Instance::ChildrenStart);
  if (!ChildrenPtr)
    return children;

  uintptr_t start = mem::read<uintptr_t>(ChildrenPtr);
  uintptr_t end = mem::read<uintptr_t>(ChildrenPtr + 8);

  if (!start || !end || end < start || (end - start) > 0x10000)
    return children;

  for (auto i = start; i < end; i += 16) {
    uintptr_t child = mem::read<uintptr_t>(i);
    if (child)
        children.emplace_back(Instance{ child });
  }
  return children;
}

Engine::Instance Engine::Instance::GetDataModel() {
  return static_cast<Engine::Instance>(
      mem::read<std::uint64_t>(Engine::Instance::GetDataModelPointer().address +
                               Offsets::FakeDataModel::RealDataModel));
}

Engine::Instance Engine::Instance::GetVisualEngine() {
  return static_cast<Engine::Instance>(mem::read<std::uint64_t>(
      Engine::Instance::GetRenderView() + Offsets::RenderView::VisualEngine));
}

Engine::Instance Engine::Instance::FindFirstChild(std::string Name) {
  for (const auto &child : this->GetChildren()) {
    if (child.GetName() == Name)
      return child;
  }
  return Engine::Instance();
}

std::uint32_t Engine::Instance::GetColor3() const {
  uint32_t rawValue =
      mem::read<uint32_t>(this->address + Offsets::BasePart::Color3);
  uint32_t maskedValue = rawValue & 0x00FFFFFF;
  return maskedValue;
}

uintptr_t Engine::Instance::FindFirstChildGay(uintptr_t Parent,
                                              const std::string &ChildName) {
  uintptr_t ChildrenPtr =
      mem::read<uintptr_t>(Parent + Offsets::Instance::ChildrenStart);
  if (!ChildrenPtr)
    return 0;

  uintptr_t Start = mem::read<uintptr_t>(ChildrenPtr);
  uintptr_t End = mem::read<uintptr_t>(ChildrenPtr + sizeof(uintptr_t));

  for (uintptr_t i = Start; i < End; i += (sizeof(uintptr_t) * 2)) {
    uintptr_t Child = mem::read<uintptr_t>(i);
    if (!Child)
      continue;

    std::string Name = ReadString(Child + Offsets::Instance::Name);
    if (Name == ChildName)
      return Child;
  }

  return 0;
}

Engine::Instance Engine::Instance::GetParent() {
  return mem::read<Engine::Instance>(this->address + Offsets::Instance::Parent);
}

std::uint64_t Engine::Instance::GetRenderView() {
  return static_cast<uint64_t>(
      mem::read<uintptr_t>(Engine::TaskScheduler{}.GetJobByName("RenderJob") +
                           Offsets::TaskScheduler::RenderJobToRenderView));
}

Engine::Instance Engine::Instance::GetDataModelPointer() {
  return static_cast<Engine::Instance>(
      mem::read<uintptr_t>(Engine::Instance::GetRenderView() + 0x128));
}

Engine::Instance Engine::Instance::GetTeam() {
  return mem::read<Engine::Instance>(this->address + Offsets::Player::Team);
}

Engine::Instance Engine::Instance::GetService(std::string Name) {
  if (Name == "Workspace") {
    return storage::datamodel.FindFirstChildOfClass("Workspace");
  } else if (Name == "Players") {
    return storage::datamodel.FindFirstChildOfClass("Players");
  }
  return Engine::Instance();
}

Engine::Instance Engine::Instance::ReadService(std::string arg) {
  Engine::Instance returned{};
  for (auto children : this->GetChildren()) {
    if (children.GetClass() == arg)
      returned = children;
  }
  return returned;
}

Engine::Vector3 Engine::Instance::GetCameraPos() {
  return mem::read<Engine::Vector3>(this->address + Offsets::Camera::Position);
}

std::vector<Engine::Instance> Engine::Instance::GetPlayerList() {
  std::vector<Engine::Instance> result;

  auto Players = Engine::Instance{}.GetService("Players");
  if (!Players.address) {
    return result;
  }

  auto PlayerList = Players.GetChildren();
  for (auto &player : PlayerList) {
    if (!player.address)
      continue;

    result.push_back(player);
  }

  return result;
}

Engine::Instance Engine::Instance::GetLocalPlayer() const {
  return mem::read<Engine::Instance>(this->address +
                                     Offsets::Player::LocalPlayer);
}

Engine::Instance Engine::Instance::GetModelInstance() const {
  return mem::read<Engine::Instance>(this->address +
                                     Offsets::Player::ModelInstance);
}

std::uintptr_t Engine::Instance::GetPart() const {
  return mem::read<std::uintptr_t>(this->address +
                                   Offsets::BasePart::Primitive);
}

Engine::Matrix3x3 Engine::Instance::GetCameraRotation() {
  return mem::read<Engine::Matrix3x3>(this->address +
                                      Offsets::Camera::Rotation);
}

Engine::Instance Engine::Instance::WriteCameraRot(Engine::Matrix3x3 pos) {
  mem::write<Engine::Matrix3x3>(this->address + Offsets::Camera::Rotation, pos);
}

Engine::Vector3 Engine::Instance::GetPartPos() const {
  const std::uintptr_t part = this->GetPart();
  return mem::read<Engine::Vector3>(part + Offsets::BasePart::Position);
}

Engine::Vector3 Engine::Instance::get_move_dir() {
  return mem::read<Engine::Vector3>(this->address +
                                    Offsets::BasePart::Position);
}

Engine::Vector3 Engine::Instance::GetPartPos2() const {
  return mem::read<Engine::Vector3>(this->address +
                                    Offsets::BasePart::Position);
}

Engine::Vector3 Engine::Instance::GetPartvelocity() const {
  const std::uintptr_t part = this->GetPart();
  return mem::read<Engine::Vector3>(part +
                                    Offsets::BasePart::AssemblyLinearVelocity);
}

void Engine::Instance::SetPartVelocity(const Engine::Vector3 &velocity) {
  const std::uintptr_t part = this->GetPart();
  if (!part)
    return;
  mem::write<Engine::Vector3>(part + Offsets::BasePart::AssemblyLinearVelocity,
                              velocity);
}

void Engine::Instance::SetAnchored(bool enable) {
  const std::uintptr_t part = this->GetPart();
  if (!part)
    return;
  BYTE val = mem::read<BYTE>(part + Offsets::BasePart::PrimitiveFlags);
  if (enable)
    val |= 0x02;
  else
    val &= ~0x02;
  mem::write<BYTE>(part + Offsets::BasePart::PrimitiveFlags, val);
}

Engine::Instance Engine::Instance::GetCamera() const {
  return mem::read<Engine::Instance>(this->address +
                                     Offsets::Workspace::CurrentCamera);
}

uintptr_t Engine::Instance::primitive() {
  if (!is_valid_address(address))
    return 0;
  return mem::read<uintptr_t>(this->address + Offsets::BasePart::Primitive);
}

void Engine::Instance::write_position(Engine::Vector3 arg) {
  mem::write<Engine::Vector3>(
      Engine::Instance::primitive() + Offsets::BasePart::Position, arg);
}

void Engine::Instance::write_velocity(Engine::Vector3 arg) {
  mem::write<Engine::Vector3>(Engine::Instance::primitive() +
                                  Offsets::BasePart::AssemblyLinearVelocity,
                              arg);
}

Engine::Matrix3x3 Engine::Instance::GetRotation() const {
  return mem::read<Engine::Matrix3x3>(this->GetPart() +
                                      Offsets::BasePart::Rotation);
}

void Engine::Instance::SetPartPos(const Engine::Vector3 &position) {
  if (!this->address) return;
  
  mem::write<Engine::Vector3>(this->address + Offsets::BasePart::Position, position);

  const std::uintptr_t part = this->GetPart();
  if (part) {
    mem::write<Engine::Vector3>(part + Offsets::BasePart::Position, position);
  }
}

void Engine::Instance::SetSize(const Engine::Vector3 &size) {
  const std::uintptr_t part = this->GetPart();
  mem::write<Engine::Vector3>(part + Offsets::BasePart::Size, size);
}

Engine::Vector2 Engine::Instance::GetDimensions() {
  return mem::read<Engine::Vector2>(this->address +
                                    Offsets::VisualEngine::Dimensions);
}

Engine::Instance Engine::Instance::FindFirstChildOfClass(std::string Name) {
  for (const auto &child : this->GetChildren()) {
    if (child.GetClass() == Name)
      return child;
  }
  return Engine::Instance();
}

Engine::Matrix4x4 Engine::Instance::GetViewMatrix() {
  return mem::read<Engine::Matrix4x4>(this->address +
                                      Offsets::VisualEngine::ViewMatrix);
}

Engine::Matrix3x3 Engine::Instance::GetRotation() {
  return mem::read<Engine::Matrix3x3>(this->GetPart() +
                                      Offsets::BasePart::Rotation);
}

Engine::Matrix3x3 Engine::Instance::GetCframeLOl() const {
  return mem::read<Engine::Matrix3x3>(this->GetPart() + 0x128);
}

Engine::CFrame Engine::Instance::GetCframeLOlz() const {
  return mem::read<Engine::CFrame>(this->GetPart() + 0x128);
}

uint64_t Engine::Instance::GetGameID() {
  return mem::read<uint64_t>(this->address + Offsets::DataModel::GameId);
}

uint64_t Engine::Instance::GetPlaceID() {
  return mem::read<uint64_t>(this->address + Offsets::DataModel::PlaceId);
}

uint64_t Engine::Instance::GetUserID() {
  return mem::read<uint64_t>(this->address + Offsets::Player::UserId);
}

float Engine::Instance::GetHealth() {
  auto one = mem::read<std::uint64_t>(this->address + Offsets::Humanoid::Health);
  auto two = mem::read<std::uint64_t>(
      mem::read<std::uint64_t>(this->address + Offsets::Humanoid::Health));

  std::uint64_t normalizedHealth = one ^ two;
  float transformer;
  std::memcpy(&transformer, &normalizedHealth, sizeof(transformer));
  return transformer;
}

float Engine::Instance::GetWalkSpeed() {
  return mem::read<float>(this->address + Offsets::Humanoid::Walkspeed);
}

void Engine::Instance::WriteRot(Engine::Matrix3x3 arg) {
  mem::write<Engine::Matrix3x3>(this->address + Offsets::Camera::Rotation, arg);
}

float Engine::Instance::GetJumpPower() {
  return mem::read<float>(this->address + Offsets::Humanoid::JumpPower);
}

float Engine::Instance::GetHipHeight() {
  return mem::read<float>(this->address + Offsets::Humanoid::HipHeight);
}

void Engine::Instance::SetWalkSpeed(float value) {
  mem::write<float>(this->address + Offsets::Humanoid::Walkspeed, value);
  mem::write<float>(this->address + Offsets::Humanoid::WalkspeedCheck, value);
}

void Engine::Instance::SetJumpPower(float value) {
  mem::write<float>(this->address + Offsets::Humanoid::JumpPower, value);
}

Engine::Vector2 Engine::WorldToScreen(Engine::Vector3 world,
                                      Engine::Vector2 dimensions,
                                      Engine::Matrix4x4 viewmatrix) {
  Engine::Vector4 clipCoords = {
      world.x * viewmatrix.data[0] + world.y * viewmatrix.data[1] +
          world.z * viewmatrix.data[2] + viewmatrix.data[3],
      world.x * viewmatrix.data[4] + world.y * viewmatrix.data[5] +
          world.z * viewmatrix.data[6] + viewmatrix.data[7],
      world.x * viewmatrix.data[8] + world.y * viewmatrix.data[9] +
          world.z * viewmatrix.data[10] + viewmatrix.data[11],
      world.x * viewmatrix.data[12] + world.y * viewmatrix.data[13] +
          world.z * viewmatrix.data[14] + viewmatrix.data[15]};

  if (clipCoords.w <= 1e-6f) {
    return {-1.0f, -1.0f};
  }

  float inv_w = 1.0f / clipCoords.w;
  Engine::Vector3 ndc = {clipCoords.x * inv_w, clipCoords.y * inv_w,
                         clipCoords.z * inv_w};

  return {(dimensions.x / 2.0f) * (ndc.x + 1.0f),
          (dimensions.y / 2.0f) * (1.0f - ndc.y)};
}

void Engine::Instance::SetIntValue(int value) {
  mem::write<int>(this->address + Offsets::Misc::Value, value);
}

int Engine::Instance::getIntFromValue() const {
  return mem::read<int>(this->address + Offsets::Misc::Value);
}

float Engine::Instance::GetMaxHealth() const {
  auto one =
      mem::read<std::uint64_t>(this->address + Offsets::Humanoid::MaxHealth);
  auto two = mem::read<std::uint64_t>(
      mem::read<std::uint64_t>(this->address + Offsets::Humanoid::MaxHealth));

  std::uint64_t normalizedHealth = one ^ two;
  float transformer;
  std::memcpy(&transformer, &normalizedHealth, sizeof(transformer));
  return transformer;
}

std::string Engine::Instance::ReadString(std::uint64_t addy) {
  int length = mem::read<int>(addy + Offsets::Misc::StringLength);
  if (length <= 0 || length > 200)
    return {};

  std::uint64_t strAddr =
      (length >= 16) ? mem::read<std::uint64_t>(addy) : addy;

  char buffer[201]{};
  for (int i = 0; i < length && i < 200; ++i) {
    char ch = mem::read<char>(strAddr + i);
    buffer[i] = ch;
    if (ch == '\0')
      break;
  }

  return std::string(buffer);
}

void Engine::Instance::WriteString(std::uint64_t address,
                                   const std::string &value) {
  if (value.size() >= 16) {
      // Allocate memory for the new string
      LPVOID new_mem = mem::allocate(value.size() + 1);
      if (new_mem) {
          uint64_t remote_address = reinterpret_cast<uint64_t>(new_mem);
          
          // Write the string data to the allocated memory
          for (size_t i = 0; i < value.size(); i++) {
            mem::write<char>(remote_address + i, value[i]);
          }
          mem::write<char>(remote_address + value.size(), '\0');

          // Update the string object (Pointer, Size, Capacity)
          mem::write<uint64_t>(address, remote_address); // Pointer
          mem::write<uint64_t>(address + 0x10, value.size()); // Size
          mem::write<uint64_t>(address + 0x18, value.size() + 1); // Capacity (at least size+1)
      }
  } else {
      // Small String Optimization (SSO)
      // Write data directly to buffer at offset 0
      for (size_t i = 0; i < value.size(); i++) {
        mem::write<char>(address + i, value[i]);
      }
      mem::write<char>(address + value.size(), '\0');
      
      // Update fields
      mem::write<uint64_t>(address + 0x10, value.size()); // Size
      mem::write<uint64_t>(address + 0x18, 15); // Capacity for SSO is usually 15
  }
}

std::int32_t Engine::Instance::GetRigType() {
  std::uint8_t rigType =
      mem::read<std::uint8_t>(this->address + Offsets::Humanoid::RigType);
  return static_cast<std::int32_t>(rigType);
}

void Engine::Instance::SetUseJumpPower(bool usejumppower) {
  auto humanoid_instance =
      storage::players.GetLocalPlayer().GetModelInstance().FindFirstChild(
          "Humanoid");
  if (humanoid_instance.address) {
    mem::write<bool>(humanoid_instance.address + 0x1B8, bool(usejumppower));

    if (storage::players.GetLocalPlayer()
            .GetModelInstance()
            .FindFirstChildOfClass("Tool")
            .GetName() == "[Knife]") {
      mem::write<float>(
          humanoid_instance.address + Offsets::Humanoid::JumpPower, float(60));
    } else {
      mem::write<float>(
          humanoid_instance.address + Offsets::Humanoid::JumpPower, float(50));
    }
  }
}

inline std::optional<std::string> extractAssetId(const std::string &input) {
  std::regex idRegex(
      R"((?:id=|rbxassetid://|library/|catalog/|asset\?id=)?(\d{5,}))");
  std::smatch match;

  if (std::regex_search(input, match, idRegex)) {
    return match[1].str();
  }

  return std::nullopt;
}

std::string Engine::Instance::get_specialmesh_id() {
  const auto str = Engine::Instance::ReadString(this->address +
                                                Offsets::SpecialMesh::MeshId);

  if (!str.empty()) {
    if (auto id = extractAssetId(str))
      return *id;
  }

  return "0";
}

void Engine::Instance::writedoublevalue(double val) {
  mem::write<double>(this->address + Offsets::Misc::Value, val);
}

double Engine::Instance::DoubleValue() {
  return mem::read<double>(this->address + Offsets::Misc::Value);
}

std::string Engine::Instance::TextLabelText() {
  return ReadString(this->address + 0xc10);
}

void Engine::Instance::SpectatePart(uint64_t string) {
  mem::write<std::uint64_t>(storage::game.FindFirstChild("Workspace")
                                     .FindFirstChild("Camera")
                                     .address +
                                 Offsets::Camera::CameraSubject,
                             string);
}

void Engine::Instance::SetBoolFromValue(bool value) const {
  mem::write<bool>(this->address + Offsets::Misc::Value, value);
}

bool Engine::Instance::getBoolFromValue() const {
  return mem::read<std::uint8_t>(this->address + Offsets::Misc::Value);
}

void Engine::Instance::SetBoolFromValue1(bool value) const {
  mem::write<bool>(this->address + Offsets::Misc::Value1, value);
}

bool Engine::Instance::getBoolFromValue1() const {
  return mem::read<std::uint8_t>(this->address + Offsets::Misc::Value1);
}

void Engine::Instance::SetCameraRotation(Engine::Matrix3x3 Rotation) {
  mem::write<Engine::Matrix3x3>(this->address + Offsets::Camera::Rotation,
                                Rotation);
}

void Engine::Instance::SetRotation(Engine::Matrix3x3 Rotation) {
  mem::write<Engine::Matrix3x3>(
      Engine::Instance::primitive() + Offsets::BasePart::Rotation, Rotation);
}

void Engine::Instance::SetAnimationId(const std::string &AnimationID) {
  WriteString(this->address + Offsets::Misc::AnimationId, AnimationID);
}

std::uint64_t Engine::Instance::GetInputObject(std::uint64_t base_address) {
  return mem::read<std::uint64_t>(base_address + 0x118);
}

std::uint64_t Engine::Instance::cached_input_object = 0;

void Engine::Instance::cached_input_objectzz() {
  while (true) {
    if (storage::mouse_service) {
      std::uint64_t inputobj = GetInputObject(storage::mouse_service);

      if (inputobj && inputobj != 0xFFFFFFFFFFFFFFFF) {
        cached_input_object = inputobj;
      }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
}

void Engine::Instance::FogColorSet(Engine::Vector3 Color) {
  mem::write<Engine::Vector3>(this->address + Offsets::Lighting::FogColor,
                               Color);
}

void Engine::Instance::SetFogStart(float distance) {
  mem::write<float>(this->address + Offsets::Lighting::FogStart, distance);
}

void Engine::Instance::SetFogEnd(float distance) {
  mem::write<float>(this->address + Offsets::Lighting::FogEnd, distance);
}

void Engine::Instance::CallCachedMouseService(std::uint64_t address) {
  Engine::Instance::cached_input_object = GetInputObject(address);

  if (cached_input_object && cached_input_object != 0xFFFFFFFFFFFFFFFF) {
    const char *base_ptr = reinterpret_cast<const char *>(cached_input_object);
    _mm_prefetch(base_ptr + Offsets::MouseService::MousePosition, _MM_HINT_T0);
    _mm_prefetch(base_ptr + Offsets::MouseService::MousePosition +
                     sizeof(Vector2),
                 _MM_HINT_T0);
  }
}

void Engine::Instance::FreeAim(Engine::Vector2 pos) {
  if (cached_input_object && cached_input_object != 0xFFFFFFFFFFFFFFFF) {
    mem::write<Vector2>(
        cached_input_object + Offsets::MouseService::MousePosition, pos);
  }
}

uint64_t Engine::Instance::SetFramePositionX(uint64_t position) {
  return mem::write<uint64_t>(address + Offsets::GuiObject::Position, position);
}

uint64_t Engine::Instance::SetFramePositionY(uint64_t position) {
  return mem::write<uint64_t>(address + Offsets::GuiObject::Position, position);
}

bool Engine::Instance::readBoolValue() {
  return mem::read<bool>(this->address + Offsets::Misc::Value);
}

bool Engine::Instance::GetCanCollide() {
  uintptr_t Primitive =
      mem::read<uintptr_t>(this->address + Offsets::BasePart::Primitive);
  if (!Primitive)
    return false;
  return (mem::read<BYTE>(Primitive + Offsets::PrimitiveFlags::CanCollide) &
          0x08) != 0;
}

bool Engine::Instance::SetCanCollide(bool enable) {
  uintptr_t Primitive =
      mem::read<uintptr_t>(this->address + Offsets::BasePart::Primitive);
  if (!Primitive)
    return false;

  BYTE val = mem::read<BYTE>(Primitive + Offsets::BasePart::PrimitiveFlags);

  if (enable)
    val |= 0x08;
  else
    val &= ~0x08;

  mem::write<BYTE>(Primitive + Offsets::BasePart::PrimitiveFlags, val);
  return enable;
}

float Engine::Instance::GetTransparency() const {
  if (!address)
    return 0.0f;
  return mem::read<float>(this->address + Offsets::BasePart::Transparency);
}

void Engine::Instance::SetTransparency(float transparency) {
  if (!address)
    return;
  mem::write<float>(this->address + Offsets::BasePart::Transparency,
                    transparency);
}

void Engine::Instance::SetMeshId(const std::string &meshId) {
  if (!address)
    return;

  std::string className = this->GetClass();

  if (className == "MeshPart") {
    WriteString(this->address + Offsets::MeshPart::MeshId, meshId);
    return;
  }

  auto specialMesh = this->FindFirstChildOfClass("SpecialMesh");
  if (specialMesh.address) {
    WriteString(specialMesh.address + Offsets::SpecialMesh::MeshId, meshId);
  }
}

void Engine::Instance::SetTextureID(const std::string &textureId) {
  if (!address)
    return;

  std::string className = this->GetClass();

  if (className == "MeshPart") {
    WriteString(this->address + Offsets::MeshPart::Texture, textureId);
    return;
  }

  WriteString(this->address + Offsets::Textures::Texture_Texture, textureId);
}

void Engine::Instance::SetStringValue(const std::string &value) {
  if (!address)
    return;
  WriteString(this->address + Offsets::Misc::Value, value);
}

void Engine::Instance::SetStringValue1(const std::string &value) {
  if (!address)
    return;
  WriteString(this->address + Offsets::Misc::Value1, value);
}

Engine::Instance Engine::Instance::Adornee() {
  return mem::read<Engine::Instance>(this->address + Offsets::Misc::Adornee);
}

Engine::Instance
Engine::Instance::teleporttoplayer(const std::string &targetPlayerName) {
  for (const auto &player : this->GetPlayerList()) {
    if (player.GetName() == targetPlayerName) {
      auto character = player.GetModelInstance();
      if (character.address) {
        auto hrp = character.FindFirstChild("HumanoidRootPart");
        if (hrp.address) {
          TeleportTo(hrp.GetPartPos());
          return character;
        }
      }
    }
  }
  return Engine::Instance();
}

void Engine::Instance::SetMaterial(int16_t materialEnum) {
  if (!address) return;
  const std::uintptr_t part = this->GetPart();
  if (!part) return;
  
  mem::write<int16_t>(part + Offsets::BasePart::MaterialType, materialEnum);
}

int16_t Engine::Instance::GetMaterial() const {
  if (!address) return 0;
  const std::uintptr_t part = this->GetPart();
  if (!part) return 0;

  return mem::read<int16_t>(part + Offsets::BasePart::MaterialType);
}


Engine::Vector2 Engine::worldtoscreen2(Vector3 world) {
  return WorldToScreen(world, storage::visualengine.GetDimensions(),
                       storage::g_cached_viewmatrix);
}

void Engine::Instance::SetFogColor(const float color) {
}
