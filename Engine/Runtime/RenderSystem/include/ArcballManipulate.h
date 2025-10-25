//
//  ArcballManipulate.h
//  rendersystem
//
//  Created by zhouxuguang on 2024/4/20.
//

#ifndef ArcballManipulate_hpp
#define ArcballManipulate_hpp

#include "RSDefine.h"
#include "Camera.h"
#include "MathUtil/Quaternion.h"

NS_RENDERSYSTEM_BEGIN

class ArcballManipulate
{
public:
    ArcballManipulate(CameraPtr cameraPtr);
    ~ArcballManipulate();
    
    void Update();
private:
    CameraPtr cameraPtr = nullptr;
    Vector3f rotation = Vector3f(0, 0, 0);
    
    //Quaternionf rotation;
};

NS_RENDERSYSTEM_END

#endif /* ArcballManipulate_hpp */
