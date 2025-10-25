//
//  Frustum.h
//  GNXEngine
//
//  Created by zhouxuguang on 2021/6/13.
//

#ifndef GNX_ENGINE_FRUSTUM_INCLUDE_H
#define GNX_ENGINE_FRUSTUM_INCLUDE_H

#include "Plane.h"
#include "Runtime/MathUtil/include/Matrix4x4.h"
#include "OBB.h"
#include "AABB.h"
#include "Sphere.h"

NS_RENDERSYSTEM_BEGIN

// Frustum平面定义
enum
{
	kPlaneFrustumLeft = 0,
	kPlaneFrustumRight,
	kPlaneFrustumBottom,
	kPlaneFrustumTop,
	kPlaneFrustumNear,
	kPlaneFrustumFar,
	kPlaneFrustumNum,
};

// 相机视锥体
template <typename T>
class Frustum
{
public:
    Frustum();
    
    ~Frustum();
    
    /**
     * 从矩阵创建视锥体
     */
    bool InitFrustum(const Matrix4x4<T>& comboMatrix);

    /**
     * 判断AABB和视锥体的关系，判断AABB是否在Frustum内
     */
    bool IsBoxInFrustum(const AxisAlignedBox<T>& aabb) const;

    /**
     * 判断OBB和视锥体的关系
     */
    bool IsOutOfFrustum(const OrientedBoundingBox<T>& obb) const;
    
    /**
     * 判断球和视锥体的关系
     */
    bool IsSphereInFrustum(const Sphere<T> sphere) const;
    
private:
    /**
     * 创建裁剪平面
     */
    void createPlane(const Matrix4x4<T>& comboMatrix);

    Vector4<T> mPlanes[kPlaneFrustumNum];             // 裁剪平面, left, right, bottom, top，near, far
    Vector4<T> mFrustumCorners[8];
};

typedef Frustum<float> Frustumf;
typedef Frustum<double> Frustumd;

NS_RENDERSYSTEM_END

#endif /* GNX_ENGINE_FRUSTUM_INCLUDE_H */
