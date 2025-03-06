//
//  Frustum.h
//  GNXEngine
//
//  Created by zhouxuguang on 2021/6/13.
//

#ifndef GNX_ENGINE_FRUSTUM_INCLUDE_H
#define GNX_ENGINE_FRUSTUM_INCLUDE_H

#include "Plane.h"
#include "mathutil/Matrix4x4.h"
#include "OBB.h"
#include "AABB.h"

NS_RENDERSYSTEM_BEGIN

// 相机视锥体
template <typename T>
class Frustum
{
public:
    Frustum();
    
    ~Frustum();
    
    /**
     * init frustum from camera.
     */
    bool initFrustum(const Matrix4x4<T>& comboMatrix);

    /**
     * is aabb out of frustum.
     */
    bool isOutOfFrustum(const AxisAlignedBox<T>& aabb) const;

    /**
     * is obb out of frustum
     */
    bool isOutOfFrustum(const OrientedBoundingBox<T>& obb) const;
    
private:
    /**
     * create clip plane
     */
    void createPlane(const Matrix4x4<T>& comboMatrix);

    Plane<T> mPlane[6];             // clip plane, left, right, top, bottom, near, far
    bool mInitialized = false;
};

typedef Frustum<float> Frustumf;
typedef Frustum<double> Frustumd;

NS_RENDERSYSTEM_END

#endif /* GNX_ENGINE_FRUSTUM_INCLUDE_H */
