//
//  EditorCameraController.h
//  GNXEngine
//
//  Orbit editor camera: right-drag rotate, middle-drag pan, scroll zoom
//

#ifndef EditorCameraController_INCLUDE_H
#define EditorCameraController_INCLUDE_H

#include "RSDefine.h"
#include "Camera.h"
#include "Runtime/GNXEngine/include/Events/Event.h"
#include "Runtime/GNXEngine/include/Events/KeyEvent.h"
#include "Runtime/GNXEngine/include/Events/MouseEvent.h"

NS_RENDERSYSTEM_BEGIN

// Editor-style orbit camera controller
//   Right-click drag  -> orbit rotate (yaw/pitch around focus point)
//   Middle-click drag -> pan (translate focus + camera)
//   Scroll wheel      -> zoom (change distance to focus)
class RENDERSYSTEM_API EditorCameraController
{
public:
    EditorCameraController(CameraPtr camera);
    ~EditorCameraController();

    // Call each frame with deltaTime in seconds
    void Update(float deltaTime);

    // Feed events from the application event system
    void OnEvent(GNXEngine::Event& e);

    // --- Tuning parameters ---
    float GetRotateSpeed() const    { return mRotateSpeed; }
    float GetPanSpeed() const       { return mPanSpeed; }
    float GetZoomSpeed() const      { return mZoomSpeed; }
    float GetMinDistance() const     { return mMinDistance; }
    float GetMaxDistance() const     { return mMaxDistance; }

    void SetRotateSpeed(float v)     { mRotateSpeed = v; }
    void SetPanSpeed(float v)        { mPanSpeed = v; }
    void SetZoomSpeed(float v)       { mZoomSpeed = v; }
    void SetMinDistance(float v)     { mMinDistance = v; }
    void SetMaxDistance(float v)     { mMaxDistance = v; }

    // Focus point accessors
    mathutil::Vector3f GetFocusPoint() const { return mFocusPoint; }
    void SetFocusPoint(const mathutil::Vector3f& focus);

private:
    bool OnMouseMoved(GNXEngine::MouseMovedEvent& e);
    bool OnMouseButtonPressed(GNXEngine::MouseButtonPressedEvent& e);
    bool OnMouseButtonReleased(GNXEngine::MouseButtonReleasedEvent& e);
    bool OnMouseScrolled(GNXEngine::MouseScrolledEvent& e);

    // Rebuild camera LookAt from focus + yaw/pitch/distance
    void ApplyTransform();

    CameraPtr mCamera;

    // Orbit state
    mathutil::Vector3f mFocusPoint;   // the point the camera orbits around
    float mDistance = 10.0f;          // distance from focus to camera
    float mYaw   = 0.0f;             // radians, rotation around world Y
    float mPitch = 0.0f;             // radians, clamped to [-PI/2, PI/2]

    // Mouse tracking
    float mLastMouseX = 0.0f;
    float mLastMouseY = 0.0f;
    bool  mRightDragging  = false;
    bool  mMiddleDragging = false;

    // Speeds
    float mRotateSpeed = 0.005f;
    float mPanSpeed    = 0.01f;
    float mZoomSpeed   = 1.0f;

    // Distance limits
    float mMinDistance = 0.1f;
    float mMaxDistance = 1000000.0f;
};

NS_RENDERSYSTEM_END

#endif /* EditorCameraController_INCLUDE_H */
