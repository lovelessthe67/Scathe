#!/usr/bin/env python3
"""
Automatic Offset Updater for Scathe
Fetches latest offsets from https://offsets.ntgetwritewatch.workers.dev/offsets.hpp
and updates engine/Players/offsets.hpp with proper namespace organization
"""

import requests
import re
import sys
import os

# Change to script directory
script_dir = os.path.dirname(os.path.abspath(__file__))
os.chdir(script_dir)

OFFSET_URL = "https://offsets.ntgetwritewatch.workers.dev/offsets.hpp"
OFFSET_URL_2 = "https://imtheo.lol/Offsets/Offsets.hpp"
OFFSET_FILE = "engine/Players/offsets.hpp"

def find_offset_file():
    """Find the offset file in different possible locations"""
    possible_paths = [
        "engine/Players/offsets.hpp",
        "engine\\Players\\offsets.hpp",
        "src/engine/Players/offsets.hpp",
        "src\\engine\\Players\\offsets.hpp",
        "../engine/Players/offsets.hpp",
        "..\\engine\\Players\\offsets.hpp",
        "offsets.hpp",
    ]
    
    print("[*] Current directory:", os.getcwd())
    print("[*] Checking paths:")
    
    for path in possible_paths:
        exists = os.path.exists(path)
        print(f"    {path} -> {'Found' if exists else 'Not found'}")
        if exists:
            return path
    
    # Try to find it recursively
    print("[*] Searching recursively...")
    for root, dirs, files in os.walk('.'):
        if 'offsets.hpp' in files and 'Players' in root:
            found_path = os.path.join(root, 'offsets.hpp')
            print(f"    Found: {found_path}")
            return found_path
    
    return None

# Mapping of offset names to their proper namespaces
NAMESPACE_MAPPING = {
    'AnimationTrack': ['Animation', 'Animator', 'IsPlaying', 'Looped', 'Speed'],
    'BasePart': ['AssemblyAngularVelocity', 'AssemblyLinearVelocity', 'Color3', 'Material', 
                 'MaterialType', 'Position', 'Primitive', 'PrimitiveFlags', 'PrimitiveOwner', 
                 'Rotation', 'Shape', 'Size', 'Transparency', 'ValidatePrimitive', 'Velocity',
                 'PartSize', 'CFrame'],
    'ByteCode': ['Pointer', 'Size'],
    'Camera': ['CameraSubject', 'CameraType', 'FieldOfView', 'Position', 'Rotation', 'Camera',
               'CameraPos', 'CameraRotation', 'CameraMaxZoomDistance', 'CameraMinZoomDistance',
               'CameraMode', 'FOV'],
    'ClickDetector': ['MaxActivationDistance', 'MouseIcon', 'ClickDetectorMaxActivationDistance'],
    'DataModel': ['CreatorId', 'GameId', 'GameLoaded', 'JobId', 'PlaceId', 'PlaceVersion',
                  'PrimitiveCount', 'ScriptContext', 'ServerIP', 'Workspace', 'DataModelPrimitiveCount'],
    'FFlags': ['DebugDisableTimeoutDisconnect', 'EnableLoadModule', 'PartyPlayerInactivityTimeoutInSeconds',
               'TaskSchedulerTargetFps', 'WebSocketServiceEnableClientCreation', 'FFlagList', 'FFlagToValueGetSet'],
    'FakeDataModel': ['Pointer', 'RealDataModel', 'FakeDataModelPointer', 'FakeDataModelToDataModel'],
    'GuiObject': ['BackgroundColor3', 'BorderColor3', 'Image', 'LayoutOrder', 'Position', 'RichText',
                  'Rotation', 'ScreenGui_Enabled', 'Size', 'Text', 'TextColor3', 'Visible', 'ViewportSize',
                  'FramePositionOffsetX', 'FramePositionOffsetY', 'FramePositionX', 'FramePositionY',
                  'FrameRotation', 'FrameSizeOffsetX', 'FrameSizeOffsetY', 'FrameSizeX', 'FrameSizeY',
                  'FrameVisible', 'InsetMaxX', 'InsetMaxY', 'InsetMinX', 'InsetMinY', 'ScreenGuiEnabled',
                  'TextLabelText', 'TextLabelVisible'],
    'Humanoid': ['AutoRotate', 'FloorMaterial', 'Health', 'HipHeight', 'HumanoidState', 'HumanoidStateID',
                 'Jump', 'JumpHeight', 'JumpPower', 'MaxHealth', 'MaxSlopeAngle', 'MoveDirection', 'RigType',
                 'Walkspeed', 'WalkspeedCheck', 'AutoJumpEnabled', 'DisplayName', 'HealthDisplayDistance',
                 'NameDisplayDistance', 'RootPartR15', 'RootPartR6', 'Sit', 'EvaluateStateMachine',
                 'HumanoidDisplayName', 'HumanoidStateId', 'WalkSpeed', 'WalkSpeedCheck'],
    'Instance': ['AttributeContainer', 'AttributeList', 'AttributeToNext', 'AttributeToValue',
                 'ChildrenEnd', 'ChildrenStart', 'ClassBase', 'ClassDescriptor', 'ClassName', 'Name',
                 'Parent', 'Children', 'ClassDescriptorToClassName', 'InstanceAttributePointer1',
                 'InstanceAttributePointer2', 'InstanceCapabilities', 'NameSize', 'OnDemandInstance'],
    'Lighting': ['Ambient', 'Brightness', 'ClockTime', 'ColorShift_Bottom', 'ColorShift_Top',
                 'ExposureCompensation', 'FogColor', 'FogEnd', 'FogStart', 'GeographicLatitude',
                 'OutdoorAmbient'],
    'LocalScript': ['ByteCode', 'LocalScriptByteCode', 'LocalScriptBytecodePointer', 'LocalScriptHash'],
    'MeshPart': ['MeshId', 'Texture', 'MeshPartColor3', 'MeshPartTexture'],
    'Misc': ['Adornee', 'AnimationId', 'RequireLock', 'StringLength', 'Value', 'Value1', 'ValueGetSetToValue'],
    'Model': ['PrimaryPart', 'Scale'],
    'ModuleScript': ['ByteCode', 'ModuleScriptByteCode', 'ModuleScriptBytecodePointer', 'ModuleScriptHash'],
    'MouseService': ['InputObject', 'MousePosition', 'SensitivityPointer', 'MouseSensitivity'],
    'Player': ['CameraMode', 'Country', 'DisplayName', 'Gender', 'LocalPlayer', 'MaxZoomDistance',
               'MinZoomDistance', 'ModelInstance', 'Mouse', 'Team', 'UserId', 'Ping', 'CharacterAppearanceId',
               'PlayerMouse'],
    'PlayerConfigurer': ['OverrideDuration', 'Pointer', 'PlayerConfigurerPointer'],
    'PlayerMouse': ['Icon', 'Workspace'],
    'PrimitiveFlags': ['Anchored', 'CanCollide', 'CanTouch', 'AnchoredMask', 'CanCollideMask', 'CanTouchMask'],
    'ProximityPrompt': ['ActionText', 'Enabled', 'GamepadKeyCode', 'HoldDuration', 'KeyCode',
                        'MaxActivationDistance', 'ObjectText', 'RequiresLineOfSight',
                        'ProximityPromptActionText', 'ProximityPromptEnabled', 'ProximityPromptGamepadKeyCode',
                        'ProximityPromptHoldDuraction', 'ProximityPromptMaxActivationDistance',
                        'ProximityPromptMaxObjectText'],
    'Silent': ['FramePositionOffsetX', 'FramePositionOffsetY'],
    'RenderView': ['DeviceD3D11', 'VisualEngine', 'DataModelToRenderView1', 'DataModelToRenderView2',
                   'DataModelToRenderView3'],
    'RunService': ['HeartbeatFPS', 'HeartbeatTask'],
    'Silent': ['FramePositionOffsetX', 'FramePositionOffsetY'],
    'Sky': ['MoonAngularSize', 'MoonTextureId', 'SkyboxBk', 'SkyboxDn', 'SkyboxFt', 'SkyboxLf',
            'SkyboxOrientation', 'SkyboxRt', 'SkyboxUp', 'StarCount', 'SunAngularSize', 'SunTextureId'],
    'SpecialMesh': ['MeshId', 'Scale'],
    'StatsItem': ['Value'],
    'TaskScheduler': ['FakeDataModelToDataModel', 'JobEnd', 'JobName', 'JobStart', 'MaxFPS', 'Pointer',
                      'RenderJobToFakeDataModel', 'RenderJobToRenderView', 'TaskSchedulerMaxFPS',
                      'TaskSchedulerPointer', 'JobsPointer', 'Job_Name', 'RenderJobToDataModel'],
    'Team': ['BrickColor', 'TeamColor'],
    'Textures': ['Decal_Texture', 'Texture_Texture', 'DecalTexture'],
    'VisualEngine': ['Dimensions', 'Pointer', 'ToDataModel1', 'ToDataModel2', 'ViewMatrix',
                     'VisualEnginePointer', 'VisualEngineToDataModel1', 'VisualEngineToDataModel2', 'viewmatrix'],
    'Workspace': ['CurrentCamera', 'DistributedGameTime', 'Gravity', 'GravityContainer',
                  'PrimitivesPointer1', 'PrimitivesPointer2', 'ReadOnlyGravity', 'WorkspaceToWorld'],
}

def extract_version(content):
    """Extract Roblox version from offset content"""
    match = re.search(r'version-([a-f0-9]+)', content)
    if match:
        return f"version-{match.group(1)}"
    return "unknown"

def parse_offsets(content):
    """Parse all offsets from the web content"""
    offset_pattern = r'inline constexpr uintptr_t (\w+) = (0x[0-9A-Fa-f]+);'
    offsets = {}
    for name, value in re.findall(offset_pattern, content):
        offsets[name] = value
    return offsets

def fetch_and_parse_offsets():
    """Fetch and parse offsets from both websites"""
    all_offsets = {}
    
    # Try first website
    try:
        print("[*] Fetching offsets from ntgetwritewatch.workers.dev...")
        response = requests.get(OFFSET_URL, timeout=10)
        response.raise_for_status()
        content1 = response.text
        offsets1 = parse_offsets(content1)
        print(f"[+] Found {len(offsets1)} offsets from ntgetwritewatch")
        all_offsets.update(offsets1)
    except Exception as e:
        print(f"[-] Error fetching from ntgetwritewatch: {e}")
    
    # Try second website (priority source)
    try:
        print("[*] Fetching offsets from imtheo.lol...")
        response = requests.get(OFFSET_URL_2, timeout=10)
        response.raise_for_status()
        content2 = response.text
        offsets2 = parse_offsets(content2)
        print(f"[+] Found {len(offsets2)} offsets from imtheo.lol (priority)")
        # Merge with priority to imtheo.lol, but skip 0x0 values
        for name, value in offsets2.items():
            # Only overwrite if the value is not 0x0 (invalid offset)
            if value.lower() != '0x0':
                all_offsets[name] = value
            elif name not in all_offsets:
                # If we don't have this offset yet, add it even if it's 0x0
                all_offsets[name] = value
        print(f"[*] Skipped 0x0 offsets from imtheo.lol to preserve ntgetwritewatch values")
    except Exception as e:
        print(f"[-] Error fetching from imtheo.lol: {e}")
    
    if not all_offsets:
        return None, None
    
    print(f"[+] Total merged offsets: {len(all_offsets)}")
    
    # Get version from imtheo.lol or fallback
    version = "unknown"
    try:
        response = requests.get(OFFSET_URL_2, timeout=10)
        version = extract_version(response.text)
    except:
        try:
            response = requests.get(OFFSET_URL, timeout=10)
            version = extract_version(response.text)
        except:
            pass
    
    return all_offsets, version
    
    # Try first website
    try:
        print("[*] Fetching offsets from ntgetwritewatch.workers.dev...")
        response = requests.get(OFFSET_URL, timeout=10)
        response.raise_for_status()
        content1 = response.text
        offsets1 = parse_offsets(content1)
        print(f"[+] Found {len(offsets1)} offsets from ntgetwritewatch")
        all_offsets.update(offsets1)
    except Exception as e:
        print(f"[-] Error fetching from ntgetwritewatch: {e}")
    
    # Try second website (priority source)
    try:
        print("[*] Fetching offsets from imtheo.lol...")
        response = requests.get(OFFSET_URL_2, timeout=10)
        response.raise_for_status()
        content2 = response.text
        offsets2 = parse_offsets(content2)
        print(f"[+] Found {len(offsets2)} offsets from imtheo.lol (priority)")
        # Merge with priority to imtheo.lol
        all_offsets.update(offsets2)
    except Exception as e:
        print(f"[-] Error fetching from imtheo.lol: {e}")
    
    if not all_offsets:
        return None, None
    
    print(f"[+] Total merged offsets: {len(all_offsets)}")
    
    # Get version from imtheo.lol or fallback
    version = "unknown"
    try:
        response = requests.get(OFFSET_URL_2, timeout=10)
        version = extract_version(response.text)
    except:
        try:
            response = requests.get(OFFSET_URL, timeout=10)
            version = extract_version(response.text)
        except:
            pass
    
    return all_offsets, version

def find_namespace(offset_name):
    """Find which namespace an offset belongs to"""
    for namespace, names in NAMESPACE_MAPPING.items():
        if offset_name in names:
            return namespace
    return None

def normalize_name(name):
    """Normalize offset name for comparison (remove underscores, lowercase)"""
    return name.replace('_', '').replace('-', '').lower()

def find_matching_offset(target_name, available_offsets):
    """Find a matching offset name from available offsets using fuzzy matching"""
    target_normalized = normalize_name(target_name)
    
    # First try exact match (case-sensitive)
    if target_name in available_offsets:
        return target_name, available_offsets[target_name]
    
    # Try case-insensitive exact match
    for name, value in available_offsets.items():
        if name.lower() == target_name.lower():
            return name, value
    
    # Try normalized match (removes underscores/hyphens)
    for name, value in available_offsets.items():
        if normalize_name(name) == target_normalized:
            return name, value
    
    return None, None

def update_existing_offsets(current_file, new_offsets):
    """Update existing offset file with new values while preserving structure"""
    
    print("[*] Reading current offset file...")
    with open(current_file, 'r') as f:
        content = f.read()
    
    # Extract current version
    current_version_match = re.search(r'ClientVersion = "([^"]+)"', content)
    current_version = current_version_match.group(1) if current_version_match else "unknown"
    
    print(f"[*] Current version: {current_version}")
    
    # Offsets to skip updating (known to cause issues)
    skip_offsets = {
        'Position', 'Rotation', 'Size',  # These get confused between namespaces
        'Pointer',  # Multiple namespaces use this
        'Workspace', 'Gravity',  # Critical offsets that shouldn't change
    }
    
    # Find all offset definitions in current file
    offset_pattern = r'(inline constexpr uintptr_t )(\w+)( = )(0x[0-9A-Fa-f]+)(;)'
    
    updated_count = 0
    unchanged_count = 0
    skipped_count = 0
    
    def replace_offset(match):
        nonlocal updated_count, unchanged_count, skipped_count
        
        prefix = match.group(1)
        name = match.group(2)
        equals = match.group(3)
        old_value = match.group(4)
        semicolon = match.group(5)
        
        # Skip certain offsets that cause issues
        if name in skip_offsets:
            skipped_count += 1
            print(f"    [~] {name}: {old_value} (skipped - critical offset)")
            return match.group(0)
        
        # Try to find matching offset in new offsets
        matched_name, new_value = find_matching_offset(name, new_offsets)
        
        if new_value and new_value != old_value:
            updated_count += 1
            print(f"    [+] {name}: {old_value} -> {new_value}")
            return f"{prefix}{name}{equals}{new_value}{semicolon}"
        else:
            unchanged_count += 1
            print(f"    [-] {name}: {old_value}")
            return match.group(0)
    
    print("\n[*] Checking for offset changes...")
    print("-" * 60)
    
    # Replace offsets
    new_content = re.sub(offset_pattern, replace_offset, content)
    
    print("-" * 60)
    
    # Update version
    new_version = extract_version('\n'.join([f'{k} = {v}' for k, v in new_offsets.items()]))
    if 'ClientVersion' in new_content:
        old_version_match = re.search(r'ClientVersion = "([^"]+)"', content)
        if old_version_match:
            old_ver = old_version_match.group(1)
            if old_ver != new_version:
                print(f"\n[*] Version update: {old_ver} -> {new_version}")
        new_content = re.sub(
            r'ClientVersion = "[^"]+"',
            f'ClientVersion = "{new_version}"',
            new_content
        )
    
    return new_content, updated_count, unchanged_count, skipped_count

def generate_offset_file(version, organized_offsets, unmatched_offsets):
    """Generate the complete offsets.hpp file content"""
    
    output = f'''/* Roblox version: {version} (LIVE)
   Auto-updated from: https://offsets.ntgetwritewatch.workers.dev/offsets.hpp
   
   _____ _   _                         _ 
  |  ___| | | |                       | |
  | |__ | |_| |__   ___ _ __ ___  __ _| |
  |  __|| __| '_ \\ / _ \\ '__/ _ \\/ _` | |
  | |___| |_| | | |  __/ | |  __/ (_| | |
  \\____/ \\__|_| |_|\\___|_|  \\___|\__,_|_|
       https://discord.gg/etherealrbx
            My External
  ------------------------------------------
     My discord for offsets and methods:
    https://discord.gg/GM8rK3uAcF
 */

#include <cstdint>
#include <string>

namespace Offsets {{
    inline std::string ClientVersion = "{version}";

'''
    
    # Add organized namespaces in order
    namespace_order = [
        'AnimationTrack', 'BasePart', 'ByteCode', 'Camera', 'ClickDetector', 'DataModel',
        'FFlags', 'FakeDataModel', 'GuiObject', 'Humanoid', 'Instance', 'Lighting',
        'LocalScript', 'MeshPart', 'Misc', 'Model', 'ModuleScript', 'MouseService',
        'Player', 'PlayerConfigurer', 'PlayerMouse', 'PrimitiveFlags', 'ProximityPrompt',
        'Silent', 'RenderView', 'RunService', 'Sky', 'SpecialMesh', 'StatsItem',
        'TaskScheduler', 'Team', 'Textures', 'VisualEngine', 'Workspace'
    ]
    
    for namespace in namespace_order:
        if namespace in organized_offsets:
            output += f'    namespace {namespace} {{\n'
            for name, value in sorted(organized_offsets[namespace].items()):
                output += f'        inline constexpr uintptr_t {name} = {value};\n'
            output += '    }\n\n'
    
    # Add unmatched offsets if any
    if unmatched_offsets:
        output += '    // Unmatched offsets - please organize these manually\n'
        output += '    namespace Unmatched {\n'
        for name, value in sorted(unmatched_offsets.items()):
            output += f'        inline constexpr uintptr_t {name} = {value};\n'
        output += '    }\n\n'
    
    output += '}\n'
    
    return output

def main():
    print("=" * 60)
    print("Scathe Automatic Offset Updater")
    print("=" * 60)
    print()
    
    # Find offset file
    print("[*] Searching for offsets.hpp...")
    offset_file = find_offset_file()
    
    if not offset_file:
        print(f"[-] Error: offsets.hpp not found!")
        print(f"[-] Searched in:")
        print(f"    - engine/Players/offsets.hpp")
        print(f"    - src/engine/Players/offsets.hpp")
        print(f"    - Current directory")
        print(f"[-] Please run this script from the project root or src directory")
        sys.exit(1)
    
    print(f"[+] Found offsets file: {offset_file}")
    print()
    
    # Fetch and parse offsets from both sources
    offsets, version = fetch_and_parse_offsets()
    if not offsets:
        print("[-] Failed to fetch offsets from any source")
        sys.exit(1)
    
    print(f"[+] Found version: {version}")
    
    # Update existing file instead of replacing
    print("[*] Updating existing offsets...")
    print()
    
    new_content, updated_count, unchanged_count, skipped_count = update_existing_offsets(offset_file, offsets)
    
    print()
    print(f"[+] Updated {updated_count} offsets")
    print(f"[+] Kept {unchanged_count} offsets unchanged")
    print(f"[!] Skipped {skipped_count} critical offsets")
    
    if updated_count == 0:
        print("[!] No offsets were updated. Your offsets may already be up to date.")
        response = input("\n[?] Continue anyway? (y/n): ")
        if response.lower() != 'y':
            print("[*] Update cancelled")
            sys.exit(0)
    
    # Ask for confirmation
    print()
    response = input(f"[?] Update {offset_file} to {version}? (y/n): ")
    if response.lower() != 'y':
        print("[*] Update cancelled")
        sys.exit(0)
    
    # Backup old file
    backup_file = offset_file + ".backup"
    try:
        with open(offset_file, 'r') as f:
            old_content = f.read()
        with open(backup_file, 'w') as f:
            f.write(old_content)
        print(f"[+] Backed up old file to {backup_file}")
    except Exception as e:
        print(f"[!] Warning: Could not create backup: {e}")
    
    # Write new file
    try:
        with open(offset_file, 'w') as f:
            f.write(new_content)
        print(f"[+] Successfully updated {offset_file}")
        print()
        print("[+] Done! Offsets have been updated.")
        print("[*] Please rebuild your project for changes to take effect.")
    except Exception as e:
        print(f"[-] Error writing to file: {e}")
        sys.exit(1)

if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\n[*] Update cancelled by user")
        sys.exit(0)
    except Exception as e:
        print(f"\n[-] Unexpected error: {e}")
        import traceback
        traceback.print_exc()
    finally:
        print("\n" + "=" * 60)
        input("Press Enter to exit...")
