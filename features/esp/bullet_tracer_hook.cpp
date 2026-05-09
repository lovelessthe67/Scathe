#include <Windows.h>
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

#include "bullet_tracers.h"
#include "core/storage/Globals.hpp"
#include "core/storage/include.h"
#include "features/aimbot/silent.h"
#include <algorithm>
#include <cctype>

extern bool g_menuVisible;

namespace BulletTracers {

// Get the gun position (right hand + forward offset)
Engine::Vector3 GetRightHandPosition() {
    if (!storage::localplayer.address)
        return Engine::Vector3(0, 0, 0);
    
    auto character = storage::localplayer.character;
    if (!character.address)
        return storage::localplayer.rootPart.GetPartPos();
    
    // Get camera direction for forward offset
    Engine::Matrix3x3 cameraRot = storage::camera.GetCameraRotation();
    Engine::Vector3 forward = Engine::Vector3(-cameraRot.data[2], -cameraRot.data[5], -cameraRot.data[8]);
    
    // Normalize forward vector
    float mag = sqrtf(forward.x * forward.x + forward.y * forward.y + forward.z * forward.z);
    if (mag > 0.0f) {
        forward.x /= mag;
        forward.y /= mag;
        forward.z /= mag;
    }
    
    // Try to get right hand part
    auto rightHand = character.FindFirstChild("RightHand");
    if (rightHand.address) {
        Engine::Vector3 handPos = rightHand.GetPartPos();
        // Offset 5 units forward
        return Engine::Vector3(
            handPos.x + forward.x * 5.0f,
            handPos.y,
            handPos.z + forward.z * 5.0f
        );
    }
    
    auto rightArm = character.FindFirstChild("Right Arm");
    if (rightArm.address) {
        Engine::Vector3 armPos = rightArm.GetPartPos();
        // Offset 5 units forward
        return Engine::Vector3(
            armPos.x + forward.x * 5.0f,
            armPos.y,
            armPos.z + forward.z * 5.0f
        );
    }
    
    // Fallback to root part with offset
    Engine::Vector3 rootPos = storage::localplayer.rootPart.GetPartPos();
    return Engine::Vector3(
        rootPos.x + forward.x * 5.0f + 1.5f,
        rootPos.y + 1.5f,
        rootPos.z + forward.z * 5.0f
    );
}

// Convert screen position to world ray direction
Engine::Vector3 ScreenToWorldDirection(int screenX, int screenY) {
    Engine::Vector2 dimensions = storage::visualengine.GetDimensions();
    Engine::Matrix4x4 viewMatrix = storage::visualengine.GetViewMatrix();
    Engine::Vector3 cameraPos = storage::camera.GetCameraPos();
    
    // Normalize screen coordinates to -1 to 1
    float ndcX = (2.0f * screenX) / dimensions.x - 1.0f;
    float ndcY = 1.0f - (2.0f * screenY) / dimensions.y;
    
    // Get camera rotation
    Engine::Matrix3x3 cameraRot = storage::camera.GetCameraRotation();
    
    // Get camera basis vectors
    Engine::Vector3 right = Engine::Vector3(cameraRot.data[0], cameraRot.data[3], cameraRot.data[6]);
    Engine::Vector3 up = Engine::Vector3(cameraRot.data[1], cameraRot.data[4], cameraRot.data[7]);
    Engine::Vector3 forward = Engine::Vector3(-cameraRot.data[2], -cameraRot.data[5], -cameraRot.data[8]);
    
    // Calculate ray direction based on mouse position
    float fov = 70.0f; // Approximate FOV
    float aspectRatio = dimensions.x / dimensions.y;
    float tanHalfFov = tanf((fov * 0.5f) * (3.14159f / 180.0f));
    
    Engine::Vector3 direction = Engine::Vector3(
        forward.x + right.x * ndcX * tanHalfFov * aspectRatio + up.x * ndcY * tanHalfFov,
        forward.y + right.y * ndcX * tanHalfFov * aspectRatio + up.y * ndcY * tanHalfFov,
        forward.z + right.z * ndcX * tanHalfFov * aspectRatio + up.z * ndcY * tanHalfFov
    );
    
    // Normalize
    float mag = sqrtf(direction.x * direction.x + direction.y * direction.y + direction.z * direction.z);
    if (mag > 0.0f) {
        direction.x /= mag;
        direction.y /= mag;
        direction.z /= mag;
    }
    
    return direction;
}

// Get where the mouse is pointing in 3D space
Engine::Vector3 GetMouseTargetPosition() {
    POINT cursorPos;
    GetCursorPos(&cursorPos);
    ScreenToClient(FindWindowA(nullptr, "Roblox"), &cursorPos);
    
    Engine::Vector3 cameraPos = storage::camera.GetCameraPos();
    Engine::Vector3 direction = ScreenToWorldDirection(cursorPos.x, cursorPos.y);
    
    float maxDistance = 1000.0f;
    float closestDistance = maxDistance;
    Engine::Vector3 hitPosition = Engine::Vector3(
        cameraPos.x + direction.x * maxDistance,
        cameraPos.y + direction.y * maxDistance,
        cameraPos.z + direction.z * maxDistance
    );
    
    // Check for player hits along the ray
    for (auto& player : storage::player_cache) {
        if (!player.address || player.address == storage::localplayer.address)
            continue;
        
        if (!player.rootPart.address)
            continue;
        
        // Get player head position (or root if no head)
        Engine::Vector3 targetPos = player.head.address ? player.head.GetPartPos() : player.rootPart.GetPartPos();
        
        // Vector from camera to player
        Engine::Vector3 toTarget = Engine::Vector3(
            targetPos.x - cameraPos.x,
            targetPos.y - cameraPos.y,
            targetPos.z - cameraPos.z
        );
        
        float distToTarget = sqrtf(toTarget.x * toTarget.x + toTarget.y * toTarget.y + toTarget.z * toTarget.z);
        
        // Project target onto ray
        float dotProduct = (toTarget.x * direction.x + toTarget.y * direction.y + toTarget.z * direction.z);
        
        if (dotProduct > 0 && dotProduct < closestDistance) {
            // Find closest point on ray to target
            Engine::Vector3 closestPoint = Engine::Vector3(
                cameraPos.x + direction.x * dotProduct,
                cameraPos.y + direction.y * dotProduct,
                cameraPos.z + direction.z * dotProduct
            );
            
            // Distance from target to ray
            float dx = targetPos.x - closestPoint.x;
            float dy = targetPos.y - closestPoint.y;
            float dz = targetPos.z - closestPoint.z;
            float distToRay = sqrtf(dx * dx + dy * dy + dz * dz);
            
            // If close enough to ray, consider it a hit
            if (distToRay < 3.0f) { // 3 unit radius for hit detection
                closestDistance = dotProduct;
                hitPosition = targetPos;
            }
        }
    }
    
    return hitPosition;
}

bool IsValidToolForTracers() {
    // Check if player has a tool equipped
    if (!storage::localplayer.address)
        return false;
    
    auto character = storage::localplayer.character;
    if (!character.address)
        return false;
    
    // Find equipped tool
    auto tool = character.FindFirstChildOfClass("Tool");
    if (!tool.address)
        return false; // No tool equipped
    
    // Get tool name
    std::string toolName = tool.GetName();
    
    // Block wallet and phone (case-insensitive)
    std::string lowerToolName = toolName;
    std::transform(lowerToolName.begin(), lowerToolName.end(), lowerToolName.begin(), ::tolower);
    
    if (lowerToolName == "wallet" || lowerToolName == "phone") {
        return false; // Wallet and Phone are blocked
    }
    
    // Only allow tools with brackets []
    if (toolName.find('[') == std::string::npos || toolName.find(']') == std::string::npos) {
        return false; // Not a valid bracketed tool
    }
    
    // Check if we're in Da Hood (gameid 1008451066 or placeid 2788229376)
    bool isDaHood = (storage::gameid == 1008451066 || storage::placeid == 2788229376);
    
    if (isDaHood) {
        // Da Hood specific blacklist - block knife with brackets
        if (lowerToolName.find("[knife]") != std::string::npos) {
            return false; // [Knife] is blacklisted in Da Hood
        }
    }
    
    return true; // Tool is valid
}

std::string GetEquippedToolName() {
    if (!storage::localplayer.address)
        return "";
    
    auto character = storage::localplayer.character;
    if (!character.address)
        return "";
    
    auto tool = character.FindFirstChildOfClass("Tool");
    if (!tool.address)
        return "";
    
    return tool.GetName();
}

int GetEquippedToolAmmo() {
    if (!storage::localplayer.address)
        return 0;
    
    auto character = storage::localplayer.character;
    if (!character.address)
        return 0;
    
    auto tool = character.FindFirstChildOfClass("Tool");
    if (!tool.address)
        return 0;
    
    // Find Ammo child
    auto ammo = tool.FindFirstChild("Ammo");
    if (!ammo.address)
        return 0;
    
    // Read int value
    return ammo.getIntFromValue();
}

void CheckAndRegisterTracerOnClick() {
    if (!storage::visuals::bullet_tracers_enabled)
        return;
    
    // Don't fire tracers when menu is open
    if (g_menuVisible)
        return;
    
    // Check if player has a valid tool equipped
    if (!IsValidToolForTracers())
        return;
    
    // Check ammo - don't fire if ammo is 0
    int currentAmmo = GetEquippedToolAmmo();
    if (currentAmmo <= 0)
        return;
    
    // Get equipped tool name
    std::string toolName = GetEquippedToolName();
    
    // Check if left mouse button is pressed
    static bool wasPressed = false;
    static auto lastFireTime = std::chrono::steady_clock::now();
    bool isPressed = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
    
    auto currentTime = std::chrono::steady_clock::now();
    auto timeSinceLastFire = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastFireTime);
    
    // Determine fire rate based on weapon type
    int fireDelay = 100; // Default for automatic weapons (SMG, LMG, AK47)
    bool isAutomatic = true;
    int pelletCount = 1;
    float spreadAmount = 0.0f;
    
    // Weapon-specific settings
    if (toolName.find("[Revolver]") != std::string::npos || toolName.find("[Rifle]") != std::string::npos) {
        fireDelay = 500; // 0.5 seconds between shots
        isAutomatic = false;
    } else if (toolName.find("[Double-Barrel SG]") != std::string::npos || toolName.find("[DoubleBarrelSG]") != std::string::npos) {
        fireDelay = 500;
        isAutomatic = false;
        pelletCount = 4;
        spreadAmount = 15.0f; // Spread in degrees
    } else if (toolName.find("[TacticalShotgun]") != std::string::npos) {
        fireDelay = 300;
        isAutomatic = false;
        pelletCount = 4;
        spreadAmount = 12.0f;
    }
    
    // Fire logic
    bool shouldFire = false;
    
    if (isPressed && !wasPressed) {
        // Initial press
        shouldFire = true;
        lastFireTime = currentTime;
    } else if (isPressed && isAutomatic && timeSinceLastFire.count() >= fireDelay) {
        // Holding - only fire if automatic
        shouldFire = true;
        lastFireTime = currentTime;
    }
    
    if (shouldFire) {
        // Get right hand position as bullet origin
        Engine::Vector3 handPos = GetRightHandPosition();
        
        Engine::Vector3 baseTargetPos;
        
        // Check if silent aim is active and has a target with valid 3D position
        if (storage::silent_aim && g_silent_found_target && 
            g_silent_partpos_3d.x != 0.0f && g_silent_partpos_3d.y != 0.0f && g_silent_partpos_3d.z != 0.0f) {
            // Use the exact 3D position that silent aim is targeting
            baseTargetPos = g_silent_partpos_3d;
        } else {
            // Use mouse position
            baseTargetPos = GetMouseTargetPosition();
        }
        
        // Register tracers based on pellet count
        if (pelletCount == 1) {
            // Single bullet
            RegisterBulletTracer(handPos, baseTargetPos);
        } else {
            // Multiple pellets with spread
            Engine::Vector3 direction = Engine::Vector3(
                baseTargetPos.x - handPos.x,
                baseTargetPos.y - handPos.y,
                baseTargetPos.z - handPos.z
            );
            
            // Normalize direction
            float mag = sqrtf(direction.x * direction.x + direction.y * direction.y + direction.z * direction.z);
            if (mag > 0.0f) {
                direction.x /= mag;
                direction.y /= mag;
                direction.z /= mag;
            }
            
            // Get camera rotation for spread calculation
            Engine::Matrix3x3 cameraRot = storage::camera.GetCameraRotation();
            Engine::Vector3 right = Engine::Vector3(cameraRot.data[0], cameraRot.data[3], cameraRot.data[6]);
            Engine::Vector3 up = Engine::Vector3(cameraRot.data[1], cameraRot.data[4], cameraRot.data[7]);
            
            // Fire multiple pellets with random spread
            for (int i = 0; i < pelletCount; i++) {
                // Random spread in degrees
                float spreadX = ((rand() % 1000) / 1000.0f - 0.5f) * spreadAmount;
                float spreadY = ((rand() % 1000) / 1000.0f - 0.5f) * spreadAmount;
                
                // Convert to radians
                float radX = spreadX * (3.14159f / 180.0f);
                float radY = spreadY * (3.14159f / 180.0f);
                
                // Apply spread to direction
                Engine::Vector3 spreadDir = Engine::Vector3(
                    direction.x + right.x * radX + up.x * radY,
                    direction.y + right.y * radX + up.y * radY,
                    direction.z + right.z * radX + up.z * radY
                );
                
                // Normalize spread direction
                float spreadMag = sqrtf(spreadDir.x * spreadDir.x + spreadDir.y * spreadDir.y + spreadDir.z * spreadDir.z);
                if (spreadMag > 0.0f) {
                    spreadDir.x /= spreadMag;
                    spreadDir.y /= spreadMag;
                    spreadDir.z /= spreadMag;
                }
                
                // Calculate target position with spread
                float distance = mag;
                Engine::Vector3 spreadTarget = Engine::Vector3(
                    handPos.x + spreadDir.x * distance,
                    handPos.y + spreadDir.y * distance,
                    handPos.z + spreadDir.z * distance
                );
                
                RegisterBulletTracer(handPos, spreadTarget);
            }
        }
    }
    
    wasPressed = isPressed;
}

} // namespace BulletTracers
