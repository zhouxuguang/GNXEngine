//
//  TestTransform.cpp
//  testNX
//
//  Created by zhouxuguang on 2022/10/3.
//

#include "TestTransform.hpp"
#include "RenderSystem/Transform.h"

using namespace RenderSystem;

void TestTransform()
{
    Transform transform;
    transform.position = Vector3f(100, 50, 25);
    transform.scale = Vector3f(1, 2, 3);
    transform.rotation.FromAngleAxis(45, Vector3f(10, 25, 48));
    
    Vector3f point = Vector3f(25, 10, 100);
    Vector3f pointOut = transform.TransformPoint(point);
    
    Transform invTransform = transform.Inverse();
    Vector3f pointSrc = invTransform.TransformPoint(pointOut);
    
    printf("");
}
