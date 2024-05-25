//
//  Camera.cpp
//  GNXEngine
//
//  Created by zhouxuguang on 2021/5/29.
//

#include "Camera.h"

NS_RENDERSYSTEM_BEGIN

Camera::Camera(RenderDeviceType renderType, const std::string& name)
{
    mRenderDeviceType = renderType;
    mName = name;
    switch (mRenderDeviceType)
    {
        case GLES:
            mAdjust = mathutil::Matrix4x4f::IDENTITY;
            break;
            
        case METAL:
            mAdjust = mathutil::Matrix4x4f::IDENTITY;
            mAdjust[2][2] = 0.5;
            mAdjust[2][3] = 0.5;
            break;
            
        case VULKAN:
            mAdjust = mathutil::Matrix4x4f::IDENTITY;
            break;
            
        default:
            break;
    }
}

Camera::~Camera()
{
}

std::string Camera::GetName() const
{
    return mName;
}

void Camera::LookAt(const Vector3f& position, const Vector3f& target, const Vector3f& up)
{
    mView = Matrix4x4f::CreateLookAt(position, target, up);
    mPosition = position;
    mLook = target;
    mUp = up;
    mViewDirty = false;
}

void Camera::SetLens(float fovY, float aspect, float zNear, float zFar)
{
    mProjection = Matrix4x4f::CreatePerspective(fovY, aspect, zNear, zFar);
    mNearZ = zNear;
    mFarZ = zFar;
    mAspect = aspect;
    mFov = fovY;
}

void Camera::UpdateViewMatrix()
{
    if (mViewDirty)
    {
        //更新视图矩阵
        LookAt(mPosition, mLook, mUp);
        mViewDirty = false;
    }
}

void Camera::SetPosition(float x, float y, float z)
{
    mPosition.x = x;
    mPosition.y = y;
    mPosition.z = z;
    mViewDirty = true;
    
    UpdateViewMatrix();
}

void Camera::SetPosition(const Vector3f& position)
{
    mPosition = position;
    mViewDirty = true;
    
    UpdateViewMatrix();
}

Vector3f Camera::GetPosition() const
{
    return mPosition;
}

Matrix4x4f Camera::GetViewMatrix() const
{
    return mView;
}

Matrix4x4f Camera::GetProjectionMatrix() const
{
    return mAdjust * mProjection;
}

void Camera::SetNearClipDistance(float nearClipDistance)
{
    mNearZ = nearClipDistance;
}

float Camera::GetNearZ() const
{
    return mNearZ;
}

void Camera::SetFarClipDistance(float farClipDistance)
{
    mFarZ = farClipDistance;
}

float Camera::GetFarZ() const
{
    return mFarZ;
}

float Camera::GetFOV() const
{
    return mFov;
}

float Camera::GetAspect() const
{
    return mAspect;
}

Vector3f Camera::GetTarget() const
{
    return mLook;
}

NS_RENDERSYSTEM_END
