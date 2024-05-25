//
//  Ray.h
//  GNXEngine
//
//  Created by zhouxuguang on 2021/6/1.
//

#ifndef GNXENGINE_RENDERSYSTEM_RAY_INCLUDE_MDEEVGFDGV
#define GNXENGINE_RENDERSYSTEM_RAY_INCLUDE_MDEEVGFDGV

#include "RSDefine.h"
#include "MathUtil/Matrix4x4.h"
#include "MathUtil/Vector3.h"
#include "AABB.h"
#include "OBB.h"
#include "Plane.h"

USING_NS_MATHUTIL

NS_RENDERSYSTEM_BEGIN

/**
 * @brief Ray is a line with one end. usually use it to check intersects with some object,such as Plane, OBB, AABB
 * @js NA
 **/
class Ray
{
public:
    /**
     * Constructor.
     *
     * @lua new
     */
    Ray();

    /**
     * Constructor.
     * @lua NA
     */
    Ray(const Ray& ray);
    
    /**
     * Constructs a new ray initialized to the specified values.
     *
     * @param origin The ray's origin.
     * @param direction The ray's direction.
     * @lua new
     */
    Ray(const Vector3f& origin, const Vector3f& direction);

    /**
     * Destructor.
     * @lua NA
     */
    ~Ray();
    
    Vector3f getOrigin() const;
    
    Vector3f getDirection() const;

    /**
     * Check whether this ray intersects with the specified AABB.
     */
    bool intersects(const AABB& aabb, float* distance = nullptr) const;
    
    /**
     * Check whether this ray intersects with the specified OBB.
     */
    bool intersects(const OBB& obb, float* distance = nullptr) const;

    float dist(const Plane& plane) const;
    
    Vector3f intersects(const Plane& plane) const;
    
    /**
     * Sets this ray to the specified values.
     *
     * @param origin The ray's origin.
     * @param direction The ray's direction.
     */
    void set(const Vector3f& origin, const Vector3f& direction);

    /**
     * Transforms this ray by the given transformation matrix.
     *
     * @param matrix The transformation matrix to transform by.
     */
    void transform(const Matrix4x4f& matrix);

    Vector3f mOrigin;        // The ray origin position.
    Vector3f mDirection;     // The ray direction vector.
};

NS_RENDERSYSTEM_END

#endif /* GNXENGINE_RENDERSYSTEM_RAY_INCLUDE_MDEEVGFDGV */
