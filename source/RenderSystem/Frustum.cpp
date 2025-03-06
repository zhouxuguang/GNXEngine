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
    
    //extract frustum plane
    //mPlane[0].initPlane(-Vector3(mat[3] + mat[0], mat[7] + mat[4], mat[11] + mat[8]), (mat[15] + mat[12]));//left
    //mPlane[1].initPlane(-Vector3(mat[3] - mat[0], mat[7] - mat[4], mat[11] - mat[8]), (mat[15] - mat[12]));//right
    //mPlane[2].initPlane(-Vector3(mat[3] + mat[1], mat[7] + mat[5], mat[11] + mat[9]), (mat[15] + mat[13]));//bottom
    //mPlane[3].initPlane(-Vector3(mat[3] - mat[1], mat[7] - mat[5], mat[11] - mat[9]), (mat[15] - mat[13]));//top
    //mPlane[4].initPlane(-Vector3(mat[3] + mat[2]), (mat[15] + mat[14]));//near
    //mPlane[5].initPlane(-Vector3f(mat[3] - mat[2], mat[7] - mat[6], mat[11] - mat[10]), (mat[15] - mat[14]));//far
}

template class Frustum<float>;
template class Frustum<double>;

NS_RENDERSYSTEM_END
