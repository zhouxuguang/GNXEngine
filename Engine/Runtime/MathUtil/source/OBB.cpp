//
//  OBB.cpp
//  GNXEngine
//
//  Created by zhouxuguang on 2021/5/30.
//

#include "OBB.h"

NS_MATHUTIL_BEGIN

template<typename T>
AxisAlignedBox<T> OrientedBoundingBox<T>::ToAxisAligned() const noexcept
{
	const Matrix3x3<T> & halfAxes = this->mHalfAxes;

    Vector3<T> halfAxes1 = Vector3<T>(halfAxes[0][0], halfAxes[1][0], halfAxes[2][0]);
    Vector3<T> halfAxes2 = Vector3<T>(halfAxes[0][1], halfAxes[1][1], halfAxes[2][1]);
    Vector3<T> halfAxes3 = Vector3<T>(halfAxes[0][2], halfAxes[1][2], halfAxes[2][2]);

    Vector3<T> extent = halfAxes1.Abs() + halfAxes2.Abs() + halfAxes3.Abs();
	const Vector3<T>& center = this->mCenter;
    Vector3<T> ll = center - extent;
    Vector3<T> ur = center + extent;
	return AxisAlignedBox<T>(ll, ur);
}

template<typename T>
OrientedBoundingBox<T> OrientedBoundingBox<T>::FromAxisAligned(const AxisAlignedBox<T>& axisAligned) noexcept
{
	return OrientedBoundingBox(
		axisAligned.center,
		Matrix3x3<T>(
            axisAligned.length[0] * 0.5, 0.0, 0.0,
            0.0, axisAligned.length[1] * 0.5, 0.0,
            0.0, 0.0, axisAligned.length[2] * 0.5));
}

template class OrientedBoundingBox<float>;
template class OrientedBoundingBox<double>;

NS_MATHUTIL_END
