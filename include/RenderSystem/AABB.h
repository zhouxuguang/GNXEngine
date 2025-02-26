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
    void updateMinMax(const Vector3f* point, size_t num);
    
    /**
     * Transforms the bounding box by the given transformation matrix.
     */
    void transform(const Matrix4x4f& mat);

public:
    Vector3f mMin;
    Vector3f mMax;
};

/**
 * @brief An Axis-Aligned Bounding Box (AABB), where the axes of the box are
 * aligned with the axes of the coordinate system.
 */
template <typename T>
class AxisAlignedBox 
{
public:
    /**
     * @brief Creates an empty AABB with a length, width, and height of zero,
     * with the center located at (0, 0, 0).
     */
    constexpr AxisAlignedBox() noexcept
    {
    }

    /**
     * @brief Creates a new AABB using the range of coordinates the box covers.
     *
     */
    constexpr AxisAlignedBox(const Vector3<T> minimum_, const Vector3<T> maximum_) noexcept
        : minimum(minimum_),
        maximum(maximum_),
        length(maximum - minimum),
        center((minimum + maximum) * 0.5)
    {
    }

    // 最小点
    Vector3<T> minimum;
    // 最大点
    Vector3<T> maximum;
	// 各个方向的长度
	Vector3<T> length;
	// 中心点坐标
    Vector3<T> center;

    /**
     * @brief Creates a tight-fitting, axis-aligned bounding box that contains all
     * of the input positions.
     *
     * @param positions The positions.
     * @returns An axis-aligned bounding box derived from the input positions.
     */
    static AxisAlignedBox FromPositions(const std::vector<Vector3<T>>& positions);
};

typedef AxisAlignedBox<float> AxisAlignedBoxf;
typedef AxisAlignedBox<double> AxisAlignedBoxd;

NS_RENDERSYSTEM_END

#endif /* GNXENGINE_RENDERSYSTEM_AABB_INCLUDE_NMSDV */
