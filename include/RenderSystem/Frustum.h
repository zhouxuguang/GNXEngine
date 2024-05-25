//
//  Frustum.h
//  GNXEngine
//
//  Created by zhouxuguang on 2021/6/13.
//

#ifndef GNX_ENGINE_FRUSTUM_INCLUDE_H
#define GNX_ENGINE_FRUSTUM_INCLUDE_H

#include "Plane.h"

NS_RENDERSYSTEM_BEGIN

class Camera;
class AABB;
class OBB;

class Frustum
{
public:
    Frustum();
    
    ~Frustum();
    
    /**
     * init frustum from camera.
     */
    bool initFrustum(const Camera& camera);

    /**
     * is aabb out of frustum.
     */
    bool isOutOfFrustum(const AABB& aabb) const;
    /**
     * is obb out of frustum
     */
    bool isOutOfFrustum(const OBB& obb) const;
    
private:
    /**
     * create clip plane
     */
    void createPlane(const Camera& camera);

    Plane mPlane[6];             // clip plane, left, right, top, bottom, near, far
    bool mInitialized = false;
};

NS_RENDERSYSTEM_END

#endif /* GNX_ENGINE_FRUSTUM_INCLUDE_H */
