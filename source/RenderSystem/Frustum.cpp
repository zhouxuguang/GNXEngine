//
//  Frustum.cpp
//  GNXEngine
//
//  Created by zhouxuguang on 2021/6/13.
//

#include "Frustum.h"
#include "Camera.h"

NS_RENDERSYSTEM_BEGIN

template <typename T>
Frustum<T>::Frustum()
{
    //
}

template <typename T>
Frustum<T>::~Frustum()
{
    //
}

template<typename T>
bool Frustum<T>::initFrustum(const Matrix4x4<T>& comboMatrix)
{
	mInitialized = true;
	createPlane(comboMatrix);
	return true;
}

template<typename T>
bool Frustum<T>::isOutOfFrustum(const AxisAlignedBox<T>& aabb) const
{
	if (mInitialized)
	{
		Vector3<T> point;
		for (int i = 0; i < 6; i++)
		{
			const Vector3<T>& normal = mPlane[i].getNormal();
			point.x = normal.x < 0.0 ? aabb.maximum.x : aabb.minimum.x;
			point.y = normal.y < 0.0 ? aabb.maximum.y : aabb.minimum.y;
			point.z = normal.z < 0.0 ? aabb.maximum.z : aabb.minimum.z;

			if (mPlane[i].getSide(point) == PointSide::FRONT_PLANE)
				return true;
		}
	}
	return false;
}

template<typename T>
bool Frustum<T>::isOutOfFrustum(const OrientedBoundingBox<T>& obb) const
{
	if (mInitialized)
	{
		Vector3<T> point;
		/*Vector3<T> obbExtentX = obb.m_xAxis * obb.m_extents.x;
		Vector3<T> obbExtentY = obb.m_yAxis * obb.m_extents.y;
		Vector3<T> obbExtentZ = obb.m_zAxis * obb.m_extents.z;

		for (int i = 0; i < 6; i++)
		{
			const Vector3<T>& normal = mPlane[i].getNormal();
			point = obb.mCenter;
			point = normal.DotProduct(obb.m_xAxis) > 0.0 ? point - obbExtentX : point + obbExtentX;
			point = normal.DotProduct(obb.m_yAxis) > 0.0 ? point - obbExtentY : point + obbExtentY;
			point = normal.DotProduct(obb.m_zAxis) > 0.0 ? point - obbExtentZ : point + obbExtentZ;

			if (mPlane[i].getSide(point) == PointSide::FRONT_PLANE)
				return true;
		}*/
	}
	return  false;
}

/**
 * create clip plane
 */
template <typename T>
void Frustum<T>::createPlane(const Matrix4x4<T>& comboMatrix)
{
    // 参考这篇文章
    //Fast Extraction of Viewing Frustum Planes from the WorldView-Projection Matrix
    
    // 提取平面方程的系数
    mPlane[0].initPlane(comboMatrix[3] + comboMatrix[0]);//left
    mPlane[1].initPlane(comboMatrix[3] - comboMatrix[0]);//right
    mPlane[2].initPlane(comboMatrix[3] + comboMatrix[1]);//bottom
    mPlane[3].initPlane(comboMatrix[3] - comboMatrix[1]);//top
    mPlane[4].initPlane(comboMatrix[3] + comboMatrix[2]);//near
    mPlane[5].initPlane(comboMatrix[3] - comboMatrix[2]);//far
}

template class Frustum<float>;
template class Frustum<double>;

NS_RENDERSYSTEM_END
