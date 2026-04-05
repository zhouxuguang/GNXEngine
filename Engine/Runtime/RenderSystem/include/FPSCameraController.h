//
//  FPSCameraController.h
//  GNXEngine
//
//  FPS Camera Controller for first-person camera roaming
//  WASD movement + mouse look with pointer lock
//

#ifndef FPSCameraController_INCLUDE_H
#define FPSCameraController_INCLUDE_H

#include "CameraController.h"
#include "Runtime/GNXEngine/include/Events/KeyEvent.h"
#include "Runtime/GNXEngine/include/Events/MouseEvent.h"
#include <unordered_map>

NS_RENDERSYSTEM_BEGIN

// First-person camera controller
// Supports WASD movement, mouse look, Shift for sprint, Space for up, Ctrl for down
class RENDERSYSTEM_API FPSCameraController : public CameraController
{
public:
    FPSCameraController(CameraPtr camera);

    ~FPSCameraController() override;

    // Call each frame with deltaTime in seconds
    void Update(float deltaTime) override;

    // Feed events from the application event system
    void OnEvent(GNXEngine::Event& e) override;

    // Sync internal state from camera (base class interface)
    void SyncFromCamera() override;

    bool IsSynced() const override { return mSynced; }

    // Getters
    float GetMoveSpeed() const { return mMoveSpeed; }
    float GetSprintMultiplier() const { return mSprintMultiplier; }
    float GetMouseSensitivity() const { return mMouseSensitivity; }

    // Setters
    void SetMoveSpeed(float speed) { mMoveSpeed = speed; }
    void SetSprintMultiplier(float multiplier) { mSprintMultiplier = multiplier; }
    void SetMouseSensitivity(float sensitivity) { mMouseSensitivity = sensitivity; }

private:
    bool OnKeyPressed(GNXEngine::KeyPressedEvent& e);
    bool OnKeyReleased(GNXEngine::KeyReleasedEvent& e);
    bool OnMouseMoved(GNXEngine::MouseMovedEvent& e);
    bool OnMouseButtonPressed(GNXEngine::MouseButtonPressedEvent& e);
    bool OnMouseButtonReleased(GNXEngine::MouseButtonReleasedEvent& e);
    bool OnMouseScrolled(GNXEngine::MouseScrolledEvent& e);

    // Rebuild direction vectors from yaw/pitch angles
    void UpdateDirectionVectors();

    // Euler angles (radians)
    float mYaw = 0.0f;    // rotation around Y axis
    float mPitch = 0.0f;  // rotation around X axis, clamped to [-PI/2, PI/2]

    // Key state tracking
    std::unordered_map<GNXEngine::KeyCode, bool> mKeyPressed;

    // Mouse state
    float mLastMouseX = 0.0f;
    float mLastMouseY = 0.0f;
    bool mFirstMouse = true;
    bool mPointerLocked = false;

    // Parameters
    float mMoveSpeed = 5.0f;
    float mSprintMultiplier = 3.0f;
    float mMouseSensitivity = 0.002f;

    // FOV control via mouse scroll
    float mFov = 0.0f;
    float mMinFov = 10.0f;
    float mMaxFov = 120.0f;

    bool mSynced = false;
};

NS_RENDERSYSTEM_END

#endif /* FPSCameraController_INCLUDE_H */
