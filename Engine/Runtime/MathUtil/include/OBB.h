//
//  OBB.h
//  GNXEngine
//
//  Created by zhouxuguang on 2021/5/30.
//

#ifndef GNXENGINE_MATHUTIL_OBB_INCLUDE_MNFD
#define GNXENGINE_MATHUTIL_OBB_INCLUDE_MNFD

#include "Math3DCommon.h"
#include "Matrix4x4.h"
#include "Vector3.h"
#include "AABB.h"

NS_MATHUTIL_BEGIN

template <typename T>
class MATH3D_API OrientedBoundingBox
{
public:
    /**
     * 构造OBB对象
     */
    OrientedBoundingBox(
        const Vector3<T>& center,
        const Matrix3x3<T>& halfAxes) noexcept
        : mCenter(center),
        mHalfAxes(halfAxes),
        // TODO: what should we do if halfAxes is singular?
        mInverseHalfAxes(halfAxes.Inverse()),
        mLengths(
            2.0 * mHalfAxes.col(0).Length(),
            2.0 * mHalfAxes.col(1).Length(),
            2.0 * mHalfAxes.col(2).Length())
    {
    }

    /**
     * @brief 将OBB转换为AABB
     */
    AxisAlignedBox<T> ToAxisAligned() const noexcept;

    /**
     * @brief 从AABB创建OBB
     */
    static OrientedBoundingBox FromAxisAligned(const AxisAlignedBox<T>& axisAligned) noexcept;

public:
    Vector3<T> mCenter;                 // 中心点坐标
    Matrix3x3<T> mHalfAxes;             // 每个轴向
    Matrix3x3<T> mInverseHalfAxes;      // 朝向矩阵的逆矩阵
    Vector3<T> mLengths;                // 每个朝向的长度
};

typedef OrientedBoundingBox<float> OrientedBoundingBoxf;
typedef OrientedBoundingBox<double> OrientedBoundingBoxd;

NS_MATHUTIL_END

#endif /* GNXENGINE_MATHUTIL_OBB_INCLUDE_MNFD */
