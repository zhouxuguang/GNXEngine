//
//  AABB.cpp
//  GNXEngine
//
//  Created by zhouxuguang on 2021/5/30.
//

#include "AABB.h"
#include <algorithm>
#include <limits>

NS_MATHUTIL_BEGIN

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
		// Leftmost point.
        min.x = std::min(positions[i].x, min.x);

		// Lowest point.
        min.y = std::min(positions[i].y, min.y);

		// Farthest point.
        min.z = std::min(positions[i].z, min.z);

		// Rightmost point.
        max.x = std::max(positions[i].x, max.x);

		// Highest point.
        max.y = std::max(positions[i].y, max.y);

		// Nearest point.
        max.z = std::max(positions[i].z, max.z);
	}

	return AxisAlignedBox<T>(min, max);
}

template class AxisAlignedBox<float>;
template class AxisAlignedBox<double>;

NS_MATHUTIL_END
