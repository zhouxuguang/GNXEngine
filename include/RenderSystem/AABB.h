//
//  AABB.h
//  GNXEngine
//
//  Created by zhouxuguang on 2021/5/30.
//

#ifndef GNXENGINE_RENDERSYSTEM_AABB_INCLUDE_NMSDV
#define GNXENGINE_RENDERSYSTEM_AABB_INCLUDE_NMSDV

#include "RSDefine.h"
#include "MathUtil/Matrix4x4.h"
#include "MathUtil/Vector3.h"

USING_NS_MATHUTIL

NS_RENDERSYSTEM_BEGIN

/**
 * Axis Aligned Bounding Box (AABB), usually calculate some rough but fast collision detection.
 */
class AABB
{

public:
    /**
     * Constructor.
     * @lua new
     */
    AABB();
    
    /**
     * Constructor.
     * @lua new
     */
    AABB(const Vector3f& min, const Vector3f& max);
    
    /**
     * Gets the center point of the bounding box.
     */
    Vector3f getCenter();

    /* Near face, specified counter-clockwise looking towards the origin from the positive z-axis.
     * verts[0] : left top front
     * verts[1] : left bottom front
     * verts[2] : right bottom front
     * verts[3] : right top front
     *
     * Far face, specified counter-clockwise looking towards the origin from the negative z-axis.
     * verts[4] : right top back
     * verts[5] : right bottom back
     * verts[6] : left bottom back
     * verts[7] : left top back
     */
    void getCorners(Vector3f *dst) const;

    /**
     * Tests whether this bounding box intersects the specified bounding object.
     */
    bool intersects(const AABB& aabb) const;

    /**
     * check whether the point is in.
     */
    bool containPoint(const Vector3f& point) const;

    /**
     * Sets this bounding box to the smallest bounding box
     * that contains both this bounding object and the specified bounding box.
     */
    void merge(const AABB& box);

    /**
     * Sets this bounding box to the specified values.
     */
    void set(const Vector3f& min, const Vector3f& max);
    
    /**
     * Reset min and max value.If you invoke this method, isEmpty() shall return true.
     */
    void reset();
    
    /**
     * check the AABB object is empty(reset).
     */
    bool isEmpty() const;

    /**
     * update the _min and _max from the given point.
     */
    void updateMinMax(const Vector3f* point, ssize_t num);
    
    /**
     * Transforms the bounding box by the given transformation matrix.
     */
    void transform(const Matrix4x4f& mat);

public:
    Vector3f mMin;
    Vector3f mMax;
};

NS_RENDERSYSTEM_END

#endif /* GNXENGINE_RENDERSYSTEM_AABB_INCLUDE_NMSDV */
