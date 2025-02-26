//
//  AABB.cpp
//  GNXEngine
//
//  Created by zhouxuguang on 2021/5/30.
//

#include "AABB.h"
#include <algorithm>
#include <limits>

NS_RENDERSYSTEM_BEGIN
    static const float MAX_FLOAT = std::numeric_limits<float>::max();
static const float MIN_FLOAT = std::numeric_limits<float>::min();

AABB::AABB()
{
    reset();
}

AABB::AABB(const Vector3f& min, const Vector3f& max)
{
    set(min, max);
}

Vector3f AABB::getCenter()
{
    Vector3f center;
    center.x = 0.5f * (mMin.x + mMax.x);
    center.y = 0.5f * (mMin.y + mMax.y);
    center.z = 0.5f * (mMin.z + mMax.z);

    return center;
}

void AABB::getCorners(Vector3f *dst) const
{
    assert(dst);
    
    // Near face, specified counter-clockwise looking towards the origin from the positive z-axis.
    // Left-top-front.
//    dst[0].set(mMin.x, mMax.y, mMax.z);
//    // Left-bottom-front.
//    dst[1].set(mMin.x, mMin.y, mMax.z);
//    // Right-bottom-front.
//    dst[2].set(mMin.x, mMin.y, mMax.z);
//    // Right-top-front.
//    dst[3].set(_max.x, _max.y, mMax.z);
//
//    // Far face, specified counter-clockwise looking towards the origin from the negative z-axis.
//    // Right-top-back.
//    dst[4].set(mMax.x, mMax.y, mMin.z);
//    // Right-bottom-back.
//    dst[5].set(mMax.x, mMin.y, mMin.z);
//    // Left-bottom-back.
//    dst[6].set(mMax.x, mMin.y, mMin.z);
//    // Left-top-back.
//    dst[7].set(mMin.x, mMax.y, mMin.z);
}

bool AABB::intersects(const AABB& aabb) const
{
    return ((mMin.x >= aabb.mMin.x && mMin.x <= aabb.mMax.x) || (aabb.mMin.x >= mMin.x && aabb.mMin.x <= mMax.x)) &&
           ((mMin.y >= aabb.mMin.y && mMin.y <= aabb.mMax.y) || (aabb.mMin.y >= mMin.y && aabb.mMin.y <= mMax.y)) &&
           ((mMin.z >= aabb.mMin.z && mMin.z <= aabb.mMax.z) || (aabb.mMin.z >= mMin.z && aabb.mMin.z <= mMax.z));
}

bool AABB::containPoint(const Vector3f& point) const
{
    if (point.x < mMin.x) return false;
    if (point.y < mMin.y) return false;
    if (point.z < mMin.z) return false;
    if (point.x > mMax.x) return false;
    if (point.y > mMax.y) return false;
    if (point.z > mMax.z) return false;
    return true;
}

void AABB::merge(const AABB& box)
{
    // Calculate the new minimum point.
    mMin.x = std::min(mMin.x, box.mMin.x);
    mMin.y = std::min(mMin.y, box.mMin.y);
    mMin.z = std::min(mMin.z, box.mMin.z);

    // Calculate the new maximum point.
    mMax.x = std::max(mMax.x, box.mMax.x);
    mMax.y = std::max(mMax.y, box.mMax.y);
    mMax.z = std::max(mMax.z, box.mMax.z);
}

void AABB::set(const Vector3f& min, const Vector3f& max)
{
    this->mMin = min;
    this->mMax = max;
}

void AABB::reset()
{
    mMin.x = MAX_FLOAT;
    mMin.y = MAX_FLOAT;
    mMin.z = MAX_FLOAT;
    
    mMax.x = MIN_FLOAT;
    mMax.y = MIN_FLOAT;
    mMax.z = MIN_FLOAT;
}

bool AABB::isEmpty() const
{
    return mMin.x > mMax.x || mMin.y > mMax.y || mMin.z > mMax.z;
}

void AABB::updateMinMax(const Vector3f* point, size_t num)
{
    for (size_t i = 0; i < num; i++)
    {
        // Leftmost point.
        mMin.x = std::min(point[i].x, mMin.x);

        // Lowest point.
        mMin.y = std::min(point[i].y, mMin.y);

        // Farthest point.
        mMin.z = std::min(point[i].z, mMin.z);

        // Rightmost point.
        mMax.x = std::max(point[i].x, mMax.x);

        // Highest point.
        mMax.y = std::max(point[i].y, mMax.y);

        // Nearest point.
        mMax.z = std::max(point[i].z, mMax.z);
    }
}

void AABB::transform(const Matrix4x4f& mat)
{
    Vector3f corners[8];
     // Near face, specified counter-clockwise
    // Left-top-front.
//    corners[0].set(_min.x, _max.y, _max.z);
//    // Left-bottom-front.
//    corners[1].set(_min.x, _min.y, _max.z);
//    // Right-bottom-front.
//    corners[2].set(_max.x, _min.y, _max.z);
//    // Right-top-front.
//    corners[3].set(_max.x, _max.y, _max.z);
//
//    // Far face, specified clockwise
//    // Right-top-back.
//    corners[4].set(_max.x, _max.y, _min.z);
//    // Right-bottom-back.
//    corners[5].set(_max.x, _min.y, _min.z);
//    // Left-bottom-back.
//    corners[6].set(_min.x, _min.y, _min.z);
//    // Left-top-back.
//    corners[7].set(_min.x, _max.y, _min.z);

    // Transform the corners, recalculate the min and max points along the way.
//    for (int i = 0; i < 8; i++)
//        mat.transformPoint(&corners[i]);
    
    reset();
    
    updateMinMax(corners, 8);
}

template <typename T>
AxisAlignedBox<T> AxisAlignedBox<T>::FromPositions(const std::vector<Vector3<T>>& positions)
{
	if (positions.size() == 0) 
    {
		return AxisAlignedBox();
	}

	Vector3<T> min = positions[0];
    Vector3<T> max = positions[0];

	for (size_t i = 1; i < positions.size(); i++) 
    {
		/*min = glm::min(min, positions[i]);
		max = glm::max(max, positions[i]);*/
	}

	return AxisAlignedBox<T>(min, max);
}

template class AxisAlignedBox<float>;
template class AxisAlignedBox<double>;

NS_RENDERSYSTEM_END
