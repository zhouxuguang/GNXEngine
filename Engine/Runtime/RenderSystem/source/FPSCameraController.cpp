//
//  FPSCameraController.cpp
//  GNXEngine
//
//  FPS Camera Controller for first-person camera roaming
//

#include "FPSCameraController.h"
#include "Runtime/MathUtil/include/MathUtil.h"
#include "Runtime/MathUtil/include/Matrix4x4.h"
#include <cmath>

NS_RENDERSYSTEM_BEGIN

using namespace GNXEngine;

static constexpr float PITCH_LIMIT = 1.57079632679f; // PI / 2

FPSCameraController::FPSCameraController(CameraPtr camera)
    : CameraController(camera)
{
    // Don't read camera state here — the camera may not be set up yet by the demo.
    // SyncFromCamera() will be called on the first Update() frame.
}

FPSCameraController::~FPSCameraController()
{
}

void FPSCameraController::SyncFromCamera()
{
    if (!mCamera) return;

    mathutil::Vector3f direction = mCamera->GetViewDirection();
    mYaw   = std::atan2(direction.x, direction.z);
    mPitch = std::asin(std::clamp(direction.y, -1.0f, 1.0f));
    mSynced = true;
}

void FPSCameraController::Update(float deltaTime)
{
    if (!mCamera) return;

    float speed = mMoveSpeed;

    // Sprint with Shift
    if (mKeyPressed[LeftShift] || mKeyPressed[RightShift])
    {
        speed *= mSprintMultiplier;
    }

    // Get forward and right vectors projected onto XZ plane
    // Note: in our coordinate system, looking along +Z means +X is to the LEFT,
    // so the right vector is the negation of the naive cross product.
    mathutil::Vector3f forward = mathutil::Vector3f(
        std::sin(mYaw), 0.0f, std::cos(mYaw)).Normalize();
    mathutil::Vector3f right = mathutil::Vector3f(
        -std::cos(mYaw), 0.0f, std::sin(mYaw)).Normalize();

    mathutil::Vector3f position = mCamera->GetPosition();

    // WASD movement
    if (mKeyPressed[W])
    {
        position += forward * speed * deltaTime;
    }
    if (mKeyPressed[S])
    {
        position -= forward * speed * deltaTime;
    }
    if (mKeyPressed[A])
    {
        position -= right * speed * deltaTime;
    }
    if (mKeyPressed[D])
    {
        position += right * speed * deltaTime;
    }

    // Vertical movement
    if (mKeyPressed[Space])
    {
        position.y += speed * deltaTime;
    }
    if (mKeyPressed[LeftControl] || mKeyPressed[RightControl])
    {
        position.y -= speed * deltaTime;
    }

    mCamera->SetPosition(position);
    UpdateDirectionVectors();
}

void FPSCameraController::OnEvent(Event& e)
{
    EventDispatcher dispatcher(e);
    dispatcher.Dispatch<KeyPressedEvent>([this](KeyPressedEvent& ev) { return OnKeyPressed(ev); });
    dispatcher.Dispatch<KeyReleasedEvent>([this](KeyReleasedEvent& ev) { return OnKeyReleased(ev); });
    dispatcher.Dispatch<MouseMovedEvent>([this](MouseMovedEvent& ev) { return OnMouseMoved(ev); });
    dispatcher.Dispatch<MouseButtonPressedEvent>([this](MouseButtonPressedEvent& ev) { return OnMouseButtonPressed(ev); });
    dispatcher.Dispatch<MouseButtonReleasedEvent>([this](MouseButtonReleasedEvent& ev) { return OnMouseButtonReleased(ev); });
    dispatcher.Dispatch<MouseScrolledEvent>([this](MouseScrolledEvent& ev) { return OnMouseScrolled(ev); });
}

bool FPSCameraController::OnKeyPressed(KeyPressedEvent& e)
{
    mKeyPressed[e.GetKeyCode()] = true;
    return false; // don't consume, let other handlers process
}

bool FPSCameraController::OnKeyReleased(KeyReleasedEvent& e)
{
    mKeyPressed[e.GetKeyCode()] = false;
    return false;
}

bool FPSCameraController::OnMouseMoved(MouseMovedEvent& e)
{
    if (!mPointerLocked || !mCamera) return false;

    float mouseX = e.GetX();
    float mouseY = e.GetY();

    if (mFirstMouse)
    {
        mLastMouseX = mouseX;
        mLastMouseY = mouseY;
        mFirstMouse = false;
    }

    float xOffset = mouseX - mLastMouseX;
    float yOffset = mLastMouseY - mouseY; // reversed: Y goes down

    mLastMouseX = mouseX;
    mLastMouseY = mouseY;

    mYaw += xOffset * mMouseSensitivity;
    mPitch += yOffset * mMouseSensitivity;

    // Clamp pitch to prevent gimbal lock
    mPitch = std::clamp(mPitch, -PITCH_LIMIT, PITCH_LIMIT);

    UpdateDirectionVectors();
    return true; // consume the event when pointer is locked
}

bool FPSCameraController::OnMouseButtonPressed(MouseButtonPressedEvent& e)
{
    // Left click to lock pointer
    if (e.GetMouseButton() == ButtonLeft && !mPointerLocked)
    {
        // Request pointer lock - will be handled by the platform window
        mPointerLocked = true;
        mFirstMouse = true;
        return true;
    }
    return false;
}

bool FPSCameraController::OnMouseButtonReleased(MouseButtonReleasedEvent& e)
{
    // Right click to unlock pointer
    if (e.GetMouseButton() == ButtonRight && mPointerLocked)
    {
        mPointerLocked = false;
        return true;
    }
    return false;
}

bool FPSCameraController::OnMouseScrolled(MouseScrolledEvent& e)
{
    if (!mCamera) return false;

    // Scroll to adjust FOV
    if (mFov <= 0.0f)
    {
        mFov = mCamera->GetFOV();
    }

    mFov -= e.GetYOffset() * 2.0f;
    mFov = std::clamp(mFov, mMinFov, mMaxFov);

    mathutil::Vector2i viewSize = mCamera->GetViewSize();
    if (viewSize.x > 0 && viewSize.y > 0)
    {
        mCamera->SetLens(mFov, viewSize.x, viewSize.y,
                         mCamera->GetNearZ(), mCamera->GetFarZ());
    }

    return true;
}

void FPSCameraController::UpdateDirectionVectors()
{
    if (!mCamera) return;

    // Spherical to Cartesian: direction from yaw and pitch
    mathutil::Vector3f forward(
        std::cos(mPitch) * std::sin(mYaw),
        std::sin(mPitch),
        std::cos(mPitch) * std::cos(mYaw)
    );
    forward.Normalize();

    mathutil::Vector3f position = mCamera->GetPosition();
    mathutil::Vector3f target = position + forward;
    mathutil::Vector3f up(0.0f, 1.0f, 0.0f);

    mCamera->LookAt(position, target, up);
}

NS_RENDERSYSTEM_END
