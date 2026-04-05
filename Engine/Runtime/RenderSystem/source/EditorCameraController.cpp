//
//  EditorCameraController.cpp
//  GNXEngine
//
//  Orbit editor camera: right-drag rotate, middle-drag pan, scroll zoom
//

#include "EditorCameraController.h"
#include "Runtime/MathUtil/include/MathUtil.h"
#include <cmath>

NS_RENDERSYSTEM_BEGIN

using namespace GNXEngine;

static constexpr float PITCH_LIMIT = 1.5533f; // ~89 degrees, slightly less than PI/2 to avoid gimbal lock

EditorCameraController::EditorCameraController(CameraPtr camera)
    : mCamera(camera)
{
    // Don't read camera state here — the camera may not be set up yet by the demo.
    // SyncFromCamera() will be called on the first Update() frame.
}

EditorCameraController::~EditorCameraController()
{
}

void EditorCameraController::SyncFromCamera()
{
    if (!mCamera) return;

    mathutil::Vector3f position = mCamera->GetPosition();
    mathutil::Vector3f target   = mCamera->GetTarget();

    mathutil::Vector3f toTarget = target - position;
    mDistance = toTarget.Length();
    if (mDistance < 1e-6f) mDistance = 10.0f;

    mFocusPoint = target;

    // Extract yaw/pitch from the vector FROM target TO camera (matches ApplyTransform convention)
    mathutil::Vector3f offset = position - target;
    mathutil::Vector3f normDir = offset.Normalize();
    mYaw   = std::atan2(normDir.x, normDir.z);
    mPitch = std::asin(std::clamp(normDir.y, -1.0f, 1.0f));

    mSynced = true;
}

void EditorCameraController::Update(float deltaTime)
{
    // No per-frame logic needed — all input is event-driven
}

void EditorCameraController::OnEvent(Event& e)
{
    EventDispatcher dispatcher(e);
    dispatcher.Dispatch<MouseMovedEvent>([this](MouseMovedEvent& ev) { return OnMouseMoved(ev); });
    dispatcher.Dispatch<MouseButtonPressedEvent>([this](MouseButtonPressedEvent& ev) { return OnMouseButtonPressed(ev); });
    dispatcher.Dispatch<MouseButtonReleasedEvent>([this](MouseButtonReleasedEvent& ev) { return OnMouseButtonReleased(ev); });
    dispatcher.Dispatch<MouseScrolledEvent>([this](MouseScrolledEvent& ev) { return OnMouseScrolled(ev); });
}

bool EditorCameraController::OnMouseButtonPressed(MouseButtonPressedEvent& e)
{
    if (e.GetMouseButton() == ButtonLeft)
    {
        mLeftDragging = true;
        return true;
    }
    if (e.GetMouseButton() == ButtonRight)
    {
        mRightDragging = true;
        return true;
    }
    if (e.GetMouseButton() == ButtonMiddle)
    {
        mMiddleDragging = true;
        return true;
    }
    return false;
}

bool EditorCameraController::OnMouseButtonReleased(MouseButtonReleasedEvent& e)
{
    if (e.GetMouseButton() == ButtonLeft)
    {
        mLeftDragging = false;
        return true;
    }
    if (e.GetMouseButton() == ButtonRight)
    {
        mRightDragging = false;
        return true;
    }
    if (e.GetMouseButton() == ButtonMiddle)
    {
        mMiddleDragging = false;
        return true;
    }
    return false;
}

bool EditorCameraController::OnMouseMoved(MouseMovedEvent& e)
{
    if (!mCamera) return false;

    float mouseX = e.GetX();
    float mouseY = e.GetY();

    float dx = mouseX - mLastMouseX;
    float dy = mouseY - mLastMouseY;
    mLastMouseX = mouseX;
    mLastMouseY = mouseY;

    if (mRightDragging || mLeftDragging)
    {
        // --- Orbit rotate ---
        mYaw   -= dx * mRotateSpeed;
        mPitch += dy * mRotateSpeed;
        mPitch  = std::clamp(mPitch, -PITCH_LIMIT, PITCH_LIMIT);

        ApplyTransform();
        return true;
    }

    if (mMiddleDragging)
    {
        // --- Pan ---
        // Build camera's local right and up axes (only from yaw, ignoring pitch for a flat feel)
        mathutil::Vector3f right(std::cos(mYaw), 0.0f, -std::sin(mYaw));
        mathutil::Vector3f up(0.0f, 1.0f, 0.0f);

        float panScale = mDistance * mPanSpeed;
        mFocusPoint -= right * dx * panScale;
        mFocusPoint += up    * dy * panScale;

        ApplyTransform();
        return true;
    }

    return false;
}

bool EditorCameraController::OnMouseScrolled(MouseScrolledEvent& e)
{
    if (!mCamera) return false;

    // --- Zoom (dolly) ---
    float offset = -e.GetYOffset() * mZoomSpeed;
    mDistance *= (1.0f + offset);
    mDistance  = std::clamp(mDistance, mMinDistance, mMaxDistance);

    ApplyTransform();
    return true;
}

void EditorCameraController::ApplyTransform()
{
    if (!mCamera) return;

    // Spherical -> Cartesian (camera position relative to focus)
    float cosPitch = std::cos(mPitch);
    mathutil::Vector3f offset(
        cosPitch * std::sin(mYaw)  * mDistance,
        std::sin(mPitch)           * mDistance,
        cosPitch * std::cos(mYaw)  * mDistance
    );

    mathutil::Vector3f position = mFocusPoint + offset;
    mathutil::Vector3f up(0.0f, 1.0f, 0.0f);

    mCamera->LookAt(position, mFocusPoint, up);
}

NS_RENDERSYSTEM_END
