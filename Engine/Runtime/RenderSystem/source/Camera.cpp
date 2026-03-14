//
//  Camera.cpp
//  GNXEngine
//
//  Created by zhouxuguang on 2021/5/29.
//

#include "Camera.h"
#include "BuildSetting.h"
#include <limits>

NS_RENDERSYSTEM_BEGIN

Camera::Camera(RenderDeviceType renderType, const std::string& name)
{
    mRenderDeviceType = renderType;
    mName = name;
    switch (mRenderDeviceType)
    {
        case METAL:
            mAdjust = mathutil::Matrix4x4f::IDENTITY;
            mAdjust[2][2] = 0.5;
            mAdjust[2][3] = 0.5;
            break;
            
        case VULKAN:
            mAdjust = mathutil::Matrix4x4f::IDENTITY;
            // https://matthewwellings.com/blog/the-new-vulkan-coordinate-system/
            // https://www.vincentparizet.com/blog/posts/vulkan_perspective_matrix/
			mAdjust[2][2] = 0.5;
			mAdjust[2][3] = 0.5;
            break;
            
        default:
            break;
    }
    
    // Reverse-Z 模式下，需要调整 mAdjust 矩阵
    // 传统 Z: 从 [-1,1] 映射到 [0,1]，使用 z' = 0.5*z + 0.5
    // Reverse-Z: 投影矩阵已处理，但仍需从 [-1,1] 映射到 [0,1]
    // 无限远 Reverse-Z 的投影矩阵输出 [-1, 0] 范围，需要映射到 [0, 1]
    // 当前 mAdjust 矩阵已经正确处理了这种映射
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

void Camera::SetLens(float fovY, uint32_t width, uint32_t height, float zNear, float zFar)
{
    mWidth = width;
    mHeight = height;
    
    float aspect = (float)width / (float)height;
    if (BuildSetting::mUseReverseZ)
    {
        // 使用无限远平面 + Reverse-Z
        // 优势：消除远平面裁剪问题，深度精度更均匀
        mProjection = Matrix4x4f::CreateInfiniteReverseZPerspective(fovY, aspect, zNear);
        mFarZ = std::numeric_limits<float>::max();  // 无限远
    }
    else
    {
        mProjection = Matrix4x4f::CreatePerspective(fovY, aspect, zNear, zFar);
        mFarZ = zFar;
    }
    mNearZ = zNear;
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

Vector3f Camera::GetViewDirection() const
{
    Vector3f viewDirection = mLook - mPosition;
    return viewDirection.Normalize();
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

Vector2i Camera::GetViewSize() const
{
    return Vector2i(mWidth, mHeight);
}

Rayf Camera::GenerateRay(float screenX, float screenY) const
{
    // https://antongerdelan.net/opengl/raycasting.html
    // 第一步，生成ndc空间的射线
    float x = (2.0f * screenX) / mWidth - 1.0f;
    float y = 1.0f - (2.0f * screenY) / mHeight;
    float z = 1.0f;
    Vector3f ray_nds = Vector3f(x, y, z);
    
    // 转换到齐次坐标
    Vector4f ray_clip = Vector4f(ray_nds.x, ray_nds.y, -1.0, 1.0);
    
    // Step 3: 4d Eye (Camera) Coordinates
    Vector4f ray_eye = mProjection.Inverse() * ray_clip;
    ray_eye = Vector4f(ray_eye.x, ray_eye.y, -1.0, 0.0);
    
    // 4d World Coordinates
    Vector4f rayWorld = mView.Inverse() * ray_eye;
    Vector3f ray_wor = Vector3f(rayWorld.x, rayWorld.y, rayWorld.z);
    // don't forget to normalise the vector at some point
    ray_wor = ray_wor.Normalize();
    
    return Ray(mPosition, ray_wor);
}

NS_RENDERSYSTEM_END
