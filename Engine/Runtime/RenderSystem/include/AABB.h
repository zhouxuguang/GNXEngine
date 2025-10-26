//
//  AABB.h
//  GNXEngine
//
//  Created by zhouxuguang on 2021/5/30.
//

#ifndef GNXENGINE_RENDERSYSTEM_AABB_INCLUDE_NMSDV
#define GNXENGINE_RENDERSYSTEM_AABB_INCLUDE_NMSDV

#include "RSDefine.h"
#include "Runtime/MathUtil/include/Matrix4x4.h"
#include "Runtime/MathUtil/include/Vector3.h"

USING_NS_MATHUTIL

NS_RENDERSYSTEM_BEGIN

/**
 * @brief 轴对齐包围盒
 */
template <typename T>
class AxisAlignedBox 
{
public:
    /**
     * @brief 创建一个中心点位于原点的空的AABB
     */
    constexpr AxisAlignedBox() noexcept
    {
    }

    /**
     * @brief 通过最大值和最小值创建包围盒
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
     * @brief 从一系列点创建包围盒
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
