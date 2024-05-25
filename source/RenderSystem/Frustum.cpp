//
//  Frustum.cpp
//  GNXEngine
//
//  Created by zhouxuguang on 2021/6/13.
//

#include "Frustum.h"
#include "Camera.h"
#include "AABB.h"
#include "OBB.h"

NS_RENDERSYSTEM_BEGIN

Frustum::Frustum()
{
    //
}

Frustum::~Frustum()
{
    //
}

/**
 * init frustum from camera.
 */
bool Frustum::initFrustum(const Camera& camera)
{
    mInitialized = true;
    createPlane(camera);
    return true;
}

/**
 * is aabb out of frustum.
 */
bool Frustum::isOutOfFrustum(const AABB& aabb) const
{
    if (mInitialized)
    {
        Vector3f point;

        int plane = 6;
        for (int i = 0; i < plane; i++)
        {
            const Vector3f& normal = mPlane[i].getNormal();
            point.x = normal.x < 0 ? aabb.mMax.x : aabb.mMin.x;
            point.y = normal.y < 0 ? aabb.mMax.y : aabb.mMin.y;
            point.z = normal.z < 0 ? aabb.mMax.z : aabb.mMin.z;
            
            if (mPlane[i].getSide(point) == PointSide::FRONT_PLANE)
                return true;
        }
    }
    return false;
}

/**
 * is obb out of frustum
 */
bool Frustum::isOutOfFrustum(const OBB& obb) const
{
    if (mInitialized)
    {
        Vector3f point;
        int plane = 6;
        Vector3f obbExtentX = obb.m_xAxis * obb.m_extents.x;
        Vector3f obbExtentY = obb.m_yAxis * obb.m_extents.y;
        Vector3f obbExtentZ = obb.m_zAxis * obb.m_extents.z;
        
        for (int i = 0; i < plane; i++)
        {
            const Vector3f& normal = mPlane[i].getNormal();
            point = obb.m_center;
            point = normal.DotProduct(obb.m_xAxis) > 0 ? point - obbExtentX : point + obbExtentX;
            point = normal.DotProduct(obb.m_yAxis) > 0 ? point - obbExtentY : point + obbExtentY;
            point = normal.DotProduct(obb.m_zAxis) > 0 ? point - obbExtentZ : point + obbExtentZ;

            if (mPlane[i].getSide(point) == PointSide::FRONT_PLANE)
                return true;
        }
    }
    return  false;
}

/**
 * create clip plane
 */
void Frustum::createPlane(const Camera& camera)
{
    const Matrix4x4f& mat = camera.GetViewMatrix();
    //ref http://www.lighthouse3d.com/tutorials/view-frustum-culling/clip-space-approach-extracting-the-planes/
    
    //Fast Extraction of Viewing Frustum Planes from the WorldView-Projection Matrix
    
    //extract frustum plane
//    mPlane[0].initPlane(-Vector3(mat[3] + mat[0], mat[7] + mat[4], mat[11] + mat[8]), (mat[15] + mat[12]));//left
//    mPlane[1].initPlane(-Vector3(mat[3] - mat[0], mat[7] - mat[4], mat[11] - mat[8]), (mat[15] - mat[12]));//right
//    mPlane[2].initPlane(-Vector3(mat[3] + mat[1], mat[7] + mat[5], mat[11] + mat[9]), (mat[15] + mat[13]));//bottom
//    mPlane[3].initPlane(-Vector3(mat[3] - mat[1], mat[7] - mat[5], mat[11] - mat[9]), (mat[15] - mat[13]));//top
    //Vector4 temp1 = mat[3] + mat[2];
    //mPlane[4].initPlane(-Vector3(mat[3] + mat[2]), (mat[15] + mat[14]));//near
    mPlane[5].initPlane(-Vector3f(mat[3] - mat[2], mat[7] - mat[6], mat[11] - mat[10]), (mat[15] - mat[14]));//far
}

NS_RENDERSYSTEM_END
