/* Roblox version: version-89d89cb2d6b649be (LIVE)
   Auto-updated from: https://imtheo.lol/Offsets/Offsets.hpp
   
   _____ _   _                         _ 
  |  ___| | | |                       | |
  | |__ | |_| |__   ___ _ __ ___  __ _| |
  |  __|| __| '_ \ / _ \ '__/ _ \/ _` | |
  | |___| |_| | | |  __/ | |  __/ (_| | |
  \____/ \__|_| |_|\___|_|  \___|\__,_|_|
       https://discord.gg/etherealrbx
            My External
  ------------------------------------------
     My discord for offsets and methods:
    https://discord.gg/GM8rK3uAcF
 */

#include <cstdint>
#include <string>

namespace Offsets {
    inline std::string ClientVersion = "unknown";

    namespace AnimationTrack {
        inline constexpr uintptr_t Animation = 0xd0;
        inline constexpr uintptr_t Animator = 0x118;
        inline constexpr uintptr_t IsPlaying = 0x518;
        inline constexpr uintptr_t Looped = 0xf5;
        inline constexpr uintptr_t Speed = 0xe4;
    }

    namespace BasePart {
        inline constexpr uintptr_t AssemblyAngularVelocity = 0xfc;
        inline constexpr uintptr_t AssemblyLinearVelocity = 0xf0;
        inline constexpr uintptr_t CFrame = 0xC0;
        inline constexpr uintptr_t Color3 = 0x194;
        inline constexpr uintptr_t Material = 0x0;
        inline constexpr uintptr_t MaterialType = 0x226;
        inline constexpr uintptr_t PartSize = 0x1B0;
        inline constexpr uintptr_t Position = 0xe4;
        inline constexpr uintptr_t Primitive = 0x148;
        inline constexpr uintptr_t PrimitiveFlags = 0x1ae;
        inline constexpr uintptr_t PrimitiveOwner = 0x1f0;
        inline constexpr uintptr_t Rotation = 0xc0;
        inline constexpr uintptr_t Shape = 0x1b1;
        inline constexpr uintptr_t Size = 0x1b0;
        inline constexpr uintptr_t Transparency = 0xf0;
        inline constexpr uintptr_t ValidatePrimitive = 0x6;
        inline constexpr uintptr_t Velocity = 0xF0;
    }

    namespace ByteCode {
        inline constexpr uintptr_t Pointer = 0x10;
        inline constexpr uintptr_t Size = 0x20;
    }

    namespace Camera {
        inline constexpr uintptr_t Camera = 0x458;
        inline constexpr uintptr_t CameraMaxZoomDistance = 0x2F0;
        inline constexpr uintptr_t CameraMinZoomDistance = 0x2F4;
        inline constexpr uintptr_t CameraMode = 0x2f8;
        inline constexpr uintptr_t CameraPos = 0x11C;
        inline constexpr uintptr_t CameraRotation = 0xF8;
        inline constexpr uintptr_t CameraSubject = 0xe8;
        inline constexpr uintptr_t CameraType = 0x158;
        inline constexpr uintptr_t FieldOfView = 0x160;
        inline constexpr uintptr_t FOV = 0x160;
        inline constexpr uintptr_t Position = 0x11c;
        inline constexpr uintptr_t Rotation = 0xf8;
    }

    namespace ClickDetector {
        inline constexpr uintptr_t ClickDetectorMaxActivationDistance = 0x100;
        inline constexpr uintptr_t MaxActivationDistance = 0x148;
        inline constexpr uintptr_t MouseIcon = 0xe0;
    }

    namespace DataModel {
        inline constexpr uintptr_t CreatorId = 0x188;
        inline constexpr uintptr_t DataModelPrimitiveCount = 0x448;
        inline constexpr uintptr_t GameId = 0x190;
        inline constexpr uintptr_t GameLoaded = 0x608;
        inline constexpr uintptr_t JobId = 0x138;
        inline constexpr uintptr_t PlaceId = 0x198;
        inline constexpr uintptr_t PlaceVersion = 0x1b4;
        inline constexpr uintptr_t PrimitiveCount = 0x448;
        inline constexpr uintptr_t ScriptContext = 0x3f0;
        inline constexpr uintptr_t ServerIP = 0x5f0;
        inline constexpr uintptr_t Workspace = 0x178;
    }

    namespace FFlags {
        inline constexpr uintptr_t DebugDisableTimeoutDisconnect = 0x6826b58;
        inline constexpr uintptr_t EnableLoadModule = 0x68158d0;
        inline constexpr uintptr_t FFlagList = 0x7AFAD38;
        inline constexpr uintptr_t FFlagToValueGetSet = 0x30;
        inline constexpr uintptr_t PartyPlayerInactivityTimeoutInSeconds = 0x67e4a70;
        inline constexpr uintptr_t TaskSchedulerTargetFps = 0x753b0f8;
        inline constexpr uintptr_t WebSocketServiceEnableClientCreation = 0x6833c78;
    }

    namespace FakeDataModel {
        inline constexpr uintptr_t FakeDataModelPointer = 0x8006F88;
        inline constexpr uintptr_t FakeDataModelToDataModel = 0x1b0;
        inline constexpr uintptr_t Pointer = 0x8006f88;
        inline constexpr uintptr_t RealDataModel = 0x1c0;
    }

    namespace GuiObject {
        inline constexpr uintptr_t BackgroundColor3 = 0x558;
        inline constexpr uintptr_t BorderColor3 = 0x564;
        inline constexpr uintptr_t FramePositionOffsetX = 0x52C;
        inline constexpr uintptr_t FramePositionOffsetY = 0x534;
        inline constexpr uintptr_t FramePositionX = 0x528;
        inline constexpr uintptr_t FramePositionY = 0x530;
        inline constexpr uintptr_t FrameRotation = 0x188;
        inline constexpr uintptr_t FrameSizeOffsetX = 0x550;
        inline constexpr uintptr_t FrameSizeOffsetY = 0x554;
        inline constexpr uintptr_t FrameSizeX = 0x548;
        inline constexpr uintptr_t FrameSizeY = 0x54C;
        inline constexpr uintptr_t FrameVisible = 0x5C1;
        inline constexpr uintptr_t Image = 0xa28;
        inline constexpr uintptr_t InsetMaxX = 0x100;
        inline constexpr uintptr_t InsetMaxY = 0x104;
        inline constexpr uintptr_t InsetMinX = 0xF8;
        inline constexpr uintptr_t InsetMinY = 0xFC;
        inline constexpr uintptr_t LayoutOrder = 0x594;
        inline constexpr uintptr_t Position = 0x528;
        inline constexpr uintptr_t RichText = 0xae8;
        inline constexpr uintptr_t Rotation = 0x188;
        inline constexpr uintptr_t ScreenGuiEnabled = 0x51D;
        inline constexpr uintptr_t ScreenGui_Enabled = 0x4dc;
        inline constexpr uintptr_t Size = 0x548;
        inline constexpr uintptr_t Text = 0xe48;
        inline constexpr uintptr_t TextColor3 = 0xef8;
        inline constexpr uintptr_t TextLabelText = 0xAE8;
        inline constexpr uintptr_t TextLabelVisible = 0x5C1;
        inline constexpr uintptr_t ViewportSize = 0x2E8;
        inline constexpr uintptr_t Visible = 0x5c1;
    }

    namespace Humanoid {
        inline constexpr uintptr_t AutoJumpEnabled = 0x1DB;
        inline constexpr uintptr_t AutoRotate = 0x1d9;
        inline constexpr uintptr_t DisplayName = 0x130;
        inline constexpr uintptr_t EvaluateStateMachine = 0x1DD;
        inline constexpr uintptr_t FloorMaterial = 0x190;
        inline constexpr uintptr_t Health = 0x194;
        inline constexpr uintptr_t HealthDisplayDistance = 0x318;
        inline constexpr uintptr_t HipHeight = 0x1a0;
        inline constexpr uintptr_t HumanoidDisplayName = 0xD0;
        inline constexpr uintptr_t HumanoidState = 0x8d8;
        inline constexpr uintptr_t HumanoidStateId = 0x20;
        inline constexpr uintptr_t HumanoidStateID = 0x20;
        inline constexpr uintptr_t Jump = 0x1dd;
        inline constexpr uintptr_t JumpHeight = 0x1ac;
        inline constexpr uintptr_t JumpPower = 0x1b0;
        inline constexpr uintptr_t MaxHealth = 0x1b4;
        inline constexpr uintptr_t MaxSlopeAngle = 0x1b8;
        inline constexpr uintptr_t MoveDirection = 0x158;
        inline constexpr uintptr_t NameDisplayDistance = 0x324;
        inline constexpr uintptr_t RigType = 0x1c8;
        inline constexpr uintptr_t RootPartR15 = 0x620;
        inline constexpr uintptr_t RootPartR6 = 0x4C0;
        inline constexpr uintptr_t Sit = 0x1DC;
        inline constexpr uintptr_t Walkspeed = 0x1d4;
        inline constexpr uintptr_t WalkspeedCheck = 0x3c0;
        inline constexpr uintptr_t WalkSpeed = 0x1D4;
        inline constexpr uintptr_t WalkSpeedCheck = 0x3C0;
    }

    namespace Instance {
        inline constexpr uintptr_t AttributeContainer = 0x48;
        inline constexpr uintptr_t AttributeList = 0x18;
        inline constexpr uintptr_t AttributeToNext = 0x58;
        inline constexpr uintptr_t AttributeToValue = 0x18;
        inline constexpr uintptr_t ChildrenEnd = 0x8;
        inline constexpr uintptr_t ChildrenStart = 0x70;
        inline constexpr uintptr_t Children = 0x70;
        inline constexpr uintptr_t ClassBase = 0xc58;
        inline constexpr uintptr_t ClassDescriptor = 0x18;
        inline constexpr uintptr_t ClassDescriptorToClassName = 0x8;
        inline constexpr uintptr_t ClassName = 0x8;
        inline constexpr uintptr_t InstanceAttributePointer1 = 0x48;
        inline constexpr uintptr_t InstanceAttributePointer2 = 0x18;
        inline constexpr uintptr_t InstanceCapabilities = 0x578;
        inline constexpr uintptr_t Name = 0xb0;
        inline constexpr uintptr_t NameSize = 0x10;
        inline constexpr uintptr_t OnDemandInstance = 0x40;
        inline constexpr uintptr_t Parent = 0x68;
    }

    namespace Lighting {
        inline constexpr uintptr_t Ambient = 0xd8;
        inline constexpr uintptr_t Brightness = 0x120;
        inline constexpr uintptr_t ClockTime = 0x1b8;
        inline constexpr uintptr_t ColorShift_Bottom = 0xf0;
        inline constexpr uintptr_t ColorShift_Top = 0xe4;
        inline constexpr uintptr_t ExposureCompensation = 0x12c;
        inline constexpr uintptr_t FogColor = 0xfc;
        inline constexpr uintptr_t FogEnd = 0x134;
        inline constexpr uintptr_t FogStart = 0x138;
        inline constexpr uintptr_t GeographicLatitude = 0x190;
        inline constexpr uintptr_t OutdoorAmbient = 0x108;
    }

    namespace LocalScript {
        inline constexpr uintptr_t ByteCode = 0x150;
        inline constexpr uintptr_t LocalScriptByteCode = 0x1A8;
        inline constexpr uintptr_t LocalScriptBytecodePointer = 0x10;
        inline constexpr uintptr_t LocalScriptHash = 0x1B8;
    }

    namespace MeshPart {
        inline constexpr uintptr_t MeshId = 0x108;
        inline constexpr uintptr_t MeshPartColor3 = 0x194;
        inline constexpr uintptr_t MeshPartTexture = 0x318;
        inline constexpr uintptr_t Texture = 0x318;
    }

    namespace Misc {
        inline constexpr uintptr_t Adornee = 0x108;
        inline constexpr uintptr_t AnimationId = 0xd0;
        inline constexpr uintptr_t RequireLock = 0x0;
        inline constexpr uintptr_t StringLength = 0x10;
        inline constexpr uintptr_t Value = 0x1c8;
        inline constexpr uintptr_t Value1 = 0xe0;
        inline constexpr uintptr_t ValueGetSetToValue = 0xC0;
    }

    namespace Model {
        inline constexpr uintptr_t PrimaryPart = 0x270;
        inline constexpr uintptr_t Scale = 0x164;
    }

    namespace ModuleScript {
        inline constexpr uintptr_t ByteCode = 0x150;
        inline constexpr uintptr_t ModuleScriptByteCode = 0x150;
        inline constexpr uintptr_t ModuleScriptBytecodePointer = 0x10;
        inline constexpr uintptr_t ModuleScriptHash = 0x160;
    }

    namespace MouseService {
        inline constexpr uintptr_t InputObject = 0x100;
        inline constexpr uintptr_t MousePosition = 0xEC;
        inline constexpr uintptr_t MouseSensitivity = 0x80B3FA0;
        inline constexpr uintptr_t SensitivityPointer = 0x80b3fa0;
    }

    namespace Player {
        inline constexpr uintptr_t CameraMode = 0x2f8;
        inline constexpr uintptr_t CharacterAppearanceId = 0x298;
        inline constexpr uintptr_t Country = 0x110;
        inline constexpr uintptr_t DisplayName = 0x130;
        inline constexpr uintptr_t Gender = 0xe68;
        inline constexpr uintptr_t LocalPlayer = 0x130;
        inline constexpr uintptr_t MaxZoomDistance = 0x2f0;
        inline constexpr uintptr_t MinZoomDistance = 0x2f4;
        inline constexpr uintptr_t ModelInstance = 0x360;
        inline constexpr uintptr_t Mouse = 0xcd8;
        inline constexpr uintptr_t Ping = 0xCC;
        inline constexpr uintptr_t PlayerMouse = 0xCD8;
        inline constexpr uintptr_t Team = 0x270;
        inline constexpr uintptr_t UserId = 0x298;
    }

    namespace PlayerConfigurer {
        inline constexpr uintptr_t OverrideDuration = 0x5894805;
        inline constexpr uintptr_t PlayerConfigurerPointer = 0x7FE4D98;
        inline constexpr uintptr_t Pointer = 0x7fe4d98;
    }

    namespace PlayerMouse {
        inline constexpr uintptr_t Icon = 0xe0;
        inline constexpr uintptr_t Workspace = 0x168;
    }

    namespace PrimitiveFlags {
        inline constexpr uintptr_t Anchored = 0x2;
        inline constexpr uintptr_t AnchoredMask = 0x2;
        inline constexpr uintptr_t CanCollide = 0x8;
        inline constexpr uintptr_t CanCollideMask = 0x8;
        inline constexpr uintptr_t CanTouch = 0x10;
        inline constexpr uintptr_t CanTouchMask = 0x10;
    }

    namespace ProximityPrompt {
        inline constexpr uintptr_t ActionText = 0xd0;
        inline constexpr uintptr_t Enabled = 0x156;
        inline constexpr uintptr_t GamepadKeyCode = 0x13c;
        inline constexpr uintptr_t HoldDuration = 0x140;
        inline constexpr uintptr_t KeyCode = 0x144;
        inline constexpr uintptr_t MaxActivationDistance = 0x148;
        inline constexpr uintptr_t ObjectText = 0xf0;
        inline constexpr uintptr_t ProximityPromptActionText = 0xD0;
        inline constexpr uintptr_t ProximityPromptEnabled = 0x156;
        inline constexpr uintptr_t ProximityPromptGamepadKeyCode = 0x13C;
        inline constexpr uintptr_t ProximityPromptHoldDuraction = 0x140;
        inline constexpr uintptr_t ProximityPromptMaxActivationDistance = 0x148;
        inline constexpr uintptr_t ProximityPromptMaxObjectText = 0xF0;
        inline constexpr uintptr_t RequiresLineOfSight = 0x157;
    }

    namespace RenderView {
        inline constexpr uintptr_t DataModelToRenderView1 = 0x1D0;
        inline constexpr uintptr_t DataModelToRenderView2 = 0x8;
        inline constexpr uintptr_t DataModelToRenderView3 = 0x28;
        inline constexpr uintptr_t DeviceD3D11 = 0x8;
        inline constexpr uintptr_t VisualEngine = 0x10;
    }

    namespace RunService {
        inline constexpr uintptr_t HeartbeatFPS = 0xb8;
        inline constexpr uintptr_t HeartbeatTask = 0xe8;
    }

    namespace Silent {
        inline constexpr uintptr_t FramePositionOffsetX = 0x52C;
        inline constexpr uintptr_t FramePositionOffsetY = 0x534;
    }

    namespace Sky {
        inline constexpr uintptr_t MoonAngularSize = 0x25c;
        inline constexpr uintptr_t MoonTextureId = 0xe0;
        inline constexpr uintptr_t SkyboxBk = 0x110;
        inline constexpr uintptr_t SkyboxDn = 0x140;
        inline constexpr uintptr_t SkyboxFt = 0x170;
        inline constexpr uintptr_t SkyboxLf = 0x1a0;
        inline constexpr uintptr_t SkyboxOrientation = 0x250;
        inline constexpr uintptr_t SkyboxRt = 0x1d0;
        inline constexpr uintptr_t SkyboxUp = 0x200;
        inline constexpr uintptr_t StarCount = 0x260;
        inline constexpr uintptr_t SunAngularSize = 0x254;
        inline constexpr uintptr_t SunTextureId = 0x230;
    }

    namespace SpecialMesh {
        inline constexpr uintptr_t MeshId = 0x108;
        inline constexpr uintptr_t Scale = 0x164;
    }

    namespace StatsItem {
        inline constexpr uintptr_t Value = 0x1c8;
    }

    namespace TaskScheduler {
        inline constexpr uintptr_t FakeDataModelToDataModel = 0x1b0;
        inline constexpr uintptr_t JobEnd = 0x1d8;
        inline constexpr uintptr_t JobName = 0x18;
        inline constexpr uintptr_t JobStart = 0x1d0;
        inline constexpr uintptr_t Job_Name = 0x18;
        inline constexpr uintptr_t JobsPointer = 0x8121BA0;
        inline constexpr uintptr_t MaxFPS = 0x1b0;
        inline constexpr uintptr_t Pointer = 0x81219c8;
        inline constexpr uintptr_t RenderJobToDataModel = 0x1B0;
        inline constexpr uintptr_t RenderJobToFakeDataModel = 0x38;
        inline constexpr uintptr_t RenderJobToRenderView = 0x218;
        inline constexpr uintptr_t TaskSchedulerMaxFPS = 0x1B0;
        inline constexpr uintptr_t TaskSchedulerPointer = 0x81219C8;
    }

    namespace Team {
        inline constexpr uintptr_t BrickColor = 0xd0;
        inline constexpr uintptr_t TeamColor = 0xD0;
    }

    namespace Textures {
        inline constexpr uintptr_t DecalTexture = 0x198;
        inline constexpr uintptr_t Decal_Texture = 0x198;
        inline constexpr uintptr_t Texture_Texture = 0x198;
    }

    namespace VisualEngine {
        inline constexpr uintptr_t Dimensions = 0x720;
        inline constexpr uintptr_t Pointer = 0x7ae30d0;
        inline constexpr uintptr_t ToDataModel1 = 0x700;
        inline constexpr uintptr_t ToDataModel2 = 0x1c0;
        inline constexpr uintptr_t ViewMatrix = 0x4b0;
        inline constexpr uintptr_t VisualEnginePointer = 0x7AE30D0;
        inline constexpr uintptr_t VisualEngineToDataModel1 = 0x700;
        inline constexpr uintptr_t VisualEngineToDataModel2 = 0x1C0;
        inline constexpr uintptr_t viewmatrix = 0x4B0;
    }

    namespace Workspace {
        inline constexpr uintptr_t CurrentCamera = 0x458;
        inline constexpr uintptr_t DistributedGameTime = 0x478;
        inline constexpr uintptr_t Gravity = 0x1d0;
        inline constexpr uintptr_t GravityContainer = 0x3d0;
        inline constexpr uintptr_t PrimitivesPointer1 = 0x3d0;
        inline constexpr uintptr_t PrimitivesPointer2 = 0x240;
        inline constexpr uintptr_t ReadOnlyGravity = 0x9b0;
        inline constexpr uintptr_t WorkspaceToWorld = 0x3D0;
    }

    // Unmatched offsets - please organize these manually
    namespace Unmatched {
        inline constexpr uintptr_t BanningEnabled = 0x14C;
        inline constexpr uintptr_t BeamBrightness = 0x190;
        inline constexpr uintptr_t BeamColor = 0x120;
        inline constexpr uintptr_t BeamLightEmission = 0x19C;
        inline constexpr uintptr_t BeamLightInfuence = 0x1A0;
        inline constexpr uintptr_t DataModelDeleterPointer = 0x8006F90;
        inline constexpr uintptr_t Deleter = 0x10;
        inline constexpr uintptr_t DeleterBack = 0x18;
        inline constexpr uintptr_t ForceNewAFKDuration = 0x1F8;
        inline constexpr uintptr_t PrimitiveValidateValue = 0x6;
        inline constexpr uintptr_t RequireBypass = 0x910;
        inline constexpr uintptr_t RunContext = 0x148;
        inline constexpr uintptr_t Sandboxed = 0xC5;
        inline constexpr uintptr_t SoundId = 0xE0;
        inline constexpr uintptr_t TagList = 0x0;
        inline constexpr uintptr_t Tool_Grip_Position = 0x48C;
    }

}
