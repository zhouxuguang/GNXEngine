//
//  ArcballManipulate.cpp
//  rendersystem
//
//  Created by zhouxuguang on 2024/4/20.
//

#include <math.h>
#include "ArcballManipulate.h"
#include "InputSystem.h"

NS_RENDERSYSTEM_BEGIN

static float rotationSpeed = 2.0;
static float translationSpeed = 3.0;
static float mouseScrollSensitivity = 0.1;
static float mousePanSensitivity = 0.008;

static float minDistance = 0.2;
static float maxDistance = 1000;

ArcballManipulate::ArcballManipulate(CameraPtr cameraPtr)
{
    this->cameraPtr = cameraPtr;
}

ArcballManipulate::~ArcballManipulate()
{
    //
}

void ArcballManipulate::Update()
{
    InputSystem* input = InputSystem::GetInstance();
    
    float scrollSensitivity = mouseScrollSensitivity;
    
    mathutil::Vector3f cameraPos = cameraPtr->GetPosition();
    mathutil::Vector3f cameraTarget = cameraPtr->GetTarget();

    // 处理鼠标缩放
    float distance = (cameraTarget - cameraPos).Length();
    distance -= (input->mouseScroll.x + input->mouseScroll.y) * scrollSensitivity;
    distance = std::min(maxDistance, std::max(minDistance, distance));
    input->mouseScroll = MousePoint();
    
    //处理鼠标拖拽
    if (input->leftMouseDown)
    {
        float sensitivity = mousePanSensitivity;
        rotation.x += input->mouseDelta.y * sensitivity;
        rotation.y -= input->mouseDelta.x * sensitivity;
        rotation.x = std::max(float(-M_PI) / 3.0f, std::min((float)rotation.x, (float)M_PI / 3.0f));
        
//        Quaternionf deltaRotate;
//        deltaRotate.FromEulerAngles(0, -input->mouseDelta.x * sensitivity, input->mouseDelta.y * sensitivity);
//        
//        rotation *= deltaRotate;
        
        input->mouseDelta = MousePoint();
    }
    
    //计算出旋转矩阵
    Matrix4x4f rotateX = Matrix4x4f::CreateRotationX(rotation.x * RADTODEG);
    Matrix4x4f rotateY = Matrix4x4f::CreateRotationY(rotation.y * RADTODEG);
    Matrix4x4f rotateMatrix = rotateX * rotateY;
    
    // 距离向量
    Vector3f posVec = (cameraPos - cameraTarget).Normalize();
    Vector3f distanceVector = Vector3f(0, 0, distance);

    // 旋转后的距离向量
    Vector3f rotatedVector = rotateMatrix * distanceVector;
    //rotatedVector *= distance;
    
    //计算新的相机位置
    cameraPos = cameraTarget + Vector3f(rotatedVector.x, rotatedVector.y, rotatedVector.z);
    
    cameraPtr->SetPosition(cameraPos);
}

NS_RENDERSYSTEM_END
