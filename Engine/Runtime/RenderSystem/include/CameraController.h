//
//  CameraController.h
//  GNXEngine
//
//  Base class for all camera controllers.
//

#ifndef CameraController_INCLUDE_H
#define CameraController_INCLUDE_H

#include "RSDefine.h"
#include "Camera.h"
#include "Runtime/GNXEngine/include/Events/Event.h"

NS_RENDERSYSTEM_BEGIN

// Camera controller type enum
enum class CameraControllerType
{
    Editor,  // Orbit camera: drag rotate, middle-drag pan, scroll zoom (default)
    FPS      // First-person: WASD movement, mouse look
};

// Abstract base class for camera controllers
class RENDERSYSTEM_API CameraController
{
public:
    CameraController(CameraPtr camera) : mCamera(camera) {}
    virtual ~CameraController() = default;

    // Call each frame with deltaTime in seconds
    virtual void Update(float deltaTime) = 0;

    // Feed events from the application event system
    virtual void OnEvent(GNXEngine::Event& e) = 0;

    // Re-derive internal state from the camera's current position/target
    // Called automatically on the first frame after camera setup
    virtual void SyncFromCamera() = 0;

    virtual bool IsSynced() const = 0;

    // Get the camera pointer
    CameraPtr GetCamera() const { return mCamera; }

protected:
    CameraPtr mCamera;
};

NS_RENDERSYSTEM_END

#endif /* CameraController_INCLUDE_H */
