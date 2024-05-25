//
//  Camera.h
//  GNXEngine
//
//  Created by zhouxuguang on 2021/5/29.
//

#ifndef GNXENGINE_RENDERSYSYTEM_CAMERA_INCLUDE_H
#define GNXENGINE_RENDERSYSYTEM_CAMERA_INCLUDE_H


#include "RSDefine.h"
#include "MathUtil/Matrix4x4.h"
#include "MathUtil/Vector3.h"
#include "RenderCore/RenderDefine.h"
#include "SceneObject.h"

USING_NS_MATHUTIL
USING_NS_RENDERCORE

NS_RENDERSYSTEM_BEGIN

class Camera : public SceneObject
{
public:
    Camera(RenderDeviceType renderType, const std::string& name);
    
    ~Camera();
    
    std::string GetName() const;
    
    //摆放相机
    void LookAt(const Vector3f& position, const Vector3f& target, const Vector3f& up);
    
    //设置投影矩阵的参数
    void SetLens(float fovY, float aspect, float zNear, float zFar);
    
    void SetPosition(float x, float y, float z);
    
    void SetPosition(const Vector3f& position);
    
    //获得相机的世界坐标位置
    Vector3f GetPosition() const;
    
    //获得视图矩阵
    Matrix4x4f GetViewMatrix() const;
    
    //获得投影矩阵
    Matrix4x4f GetProjectionMatrix() const;
    
    void SetNearClipDistance(float nearClipDistance);
    
    float GetNearZ() const;
    
    void SetFarClipDistance(float farClipDistance);
    
    float GetFarZ() const;
    
    float GetFOV() const;
    
    float GetAspect() const;
    
    Vector3f GetTarget() const;
    
private:
    bool mViewDirty = true;
    Matrix4x4f mProjection;
    Matrix4x4f mView;
    Matrix4x4f mAdjust;    //调整矩阵，将OpenGL的投影矩阵适配道各个后端图形API
    
    //相机相对于世界坐标的位置和朝向
    Vector3f mPosition;
    Vector3f mRight;
    Vector3f mUp;
    Vector3f mLook;
    
    //缓存视锥体信息
    float mNearZ = 0.0f;
    float mFarZ = 0.0f;
    float mAspect = 0.0f;
    float mFov = 0.0f;
    
    RenderDeviceType mRenderDeviceType;
    std::string mName;
    
    void UpdateViewMatrix();
};

typedef std::shared_ptr<Camera> CameraPtr;

NS_RENDERSYSTEM_END

#endif /* GNXENGINE_RENDERSYSYTEM_CAMERA_INCLUDE_H */
