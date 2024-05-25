//
//  OBB.h
//  GNXEngine
//
//  Created by zhouxuguang on 2021/5/30.
//

#ifndef GNXENGINE_RENDERSYSTEM_OBB_INCLUDE_MNFD
#define GNXENGINE_RENDERSYSTEM_OBB_INCLUDE_MNFD

#include "RSDefine.h"
#include "MathUtil/Matrix4x4.h"
#include "MathUtil/Vector3.h"
#include "AABB.h"

USING_NS_MATHUTIL

NS_RENDERSYSTEM_BEGIN

/**
 * Oriented Bounding Box(OBB)
 * @brief the OBB is similar to the AABB but the bounding box has the same direction  it's collision detection more precise than AABB
 * @js NA
 */
class OBB
{
public:
    OBB();

    /*
     * Construct obb from oriented bounding box
     *
     * @lua NA
     */
    OBB(const AABB& aabb);
    
    /*
     * Construct obb from points
     *
     * @lua NA
     */
    OBB(const Vector3f* verts, int num);
    
    /*
     * Check point in
     */
    bool containPoint(const Vector3f& point) const;

    /*
     * Specify obb values
     */
    void set(const Vector3f& center, const Vector3f& _xAxis, const Vector3f& _yAxis, const Vector3f& _zAxis, const Vector3f& _extents);
    
    /*
     * Clear obb
     */
    void reset();

    /* face to the obb's -z direction
     * verts[0] : left top front
     * verts[1] : left bottom front
     * verts[2] : right bottom front
     * verts[3] : right top front
     *
     * face to the obb's z direction
     * verts[4] : right top back
     * verts[5] : right bottom back
     * verts[6] : left bottom back
     * verts[7] : left top back
     */
    void getCorners(Vector3f* verts) const;
    
    /*
     * Check intersect with other
     */
    bool intersects(const OBB& box) const;
    
    /**
     * Transforms the obb by the given transformation matrix.
     */
    void transform(const Matrix4x4f& mat);
    
protected:
    /*
    * compute extX, extY, extZ
    */
    void computeExtAxis()
    {
        m_extentX = m_xAxis * m_extents.x;
        m_extentY = m_yAxis * m_extents.y;
        m_extentZ = m_zAxis * m_extents.z;
    }
    
    /*
     * Project point to the target axis
     */
    float projectPoint(const Vector3f& point, const Vector3f& axis) const;
    
    /*
     * Calculate the min and max project value of through the box's corners
     */
    void getInterval(const OBB& box, const Vector3f& axis, float &min, float &max) const;
    
    /*
     * Get the edge of x y z axis direction
     */
    Vector3f getEdgeDirection(int index) const;
    
    /*
     * Get the face of x y z axis direction
     */
    Vector3f getFaceDirection(int index) const;

public:
    Vector3f m_center;   // obb center
    Vector3f m_xAxis;    // x axis of obb, unit vector
    Vector3f m_yAxis;    // y axis of obb, unit vector
    Vector3f m_zAxis;    // z axis of obb, unit vector
    Vector3f m_extentX;  // _xAxis * _extents.x
    Vector3f m_extentY;  // _yAxis * _extents.y
    Vector3f m_extentZ;  // _zAxis * _extents.z
    Vector3f m_extents;  // obb length along each axis
};

NS_RENDERSYSTEM_END

#endif /* GNXENGINE_RENDERSYSTEM_OBB_INCLUDE_MNFD */
