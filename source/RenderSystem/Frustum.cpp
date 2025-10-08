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
bool Frustum<T>::InitFrustum(const Matrix4x4<T>& comboMatrix)
{
	createPlane(comboMatrix);
	return true;
}

enum MyEnum
{
	INTERSECT = 0, 
	INSIDE = 1,
	OUTSIDE = 2,
};

// Returns: INTERSECT : 0 
//          INSIDE : 1 
//          OUTSIDE : 2 
template<typename T>
int FrustumAABBIntersect(const Plane<T>* planes, const Vector3<T>& mins, const Vector3<T>& maxs)
{
	int ret = INSIDE;
	Vector3<T> vmin, vmax;

	for (int i = 0; i < 6; ++i) 
	{
		// X axis 
		if (planes[i].getNormal().x > 0)
		{
			vmin.x = mins.x;
			vmax.x = maxs.x;
		}
		else 
		{
			vmin.x = maxs.x;
			vmax.x = mins.x;
		}
		// Y axis 
		if (planes[i].getNormal().y > 0)
		{
			vmin.y = mins.y;
			vmax.y = maxs.y;
		}
		else 
		{
			vmin.y = maxs.y;
			vmax.y = mins.y;
		}
		// Z axis 
		if (planes[i].getNormal().z > 0)
		{
			vmin.z = mins.z;
			vmax.z = maxs.z;
		}
		else 
		{
			vmin.z = maxs.z;
			vmax.z = mins.z;
		}
		if (planes[i].getNormal().DotProduct(vmin) - planes[i].getDist() > 0)
		{
			return OUTSIDE;
		}
			
		if (planes[i].getNormal().DotProduct(vmax) - planes[i].getDist() >= 0)
		{
			ret = INTERSECT;
		}
	}
	return ret;
}

template<typename T>
static inline void GetFrustumCorners(const Matrix4x4<T> viewProj, Vector4<T>* points)
{
	const Vector4<T> corners[] = { Vector4<T>(-1, -1, -1, 1), Vector4<T>(1, -1, -1, 1), Vector4<T>(1, 1, -1, 1), Vector4<T>(-1, 1, -1, 1),
							 Vector4<T>(-1, -1, 1, 1),  Vector4<T>(1, -1, 1, 1),  Vector4<T>(1, 1, 1, 1),  Vector4<T>(-1, 1, 1, 1) };

	const Matrix4x4<T> invViewProj = viewProj.Inverse();

	for (int i = 0; i != 8; i++)
	{
		const Vector4<T> q = invViewProj * corners[i];
		points[i] = q / q.w;
	}
}

template<typename T>
static inline bool IsBoxInFrustumIMPL(const Vector4<T>* frustumPlanes, const Vector4<T>* frustumCorners, const AxisAlignedBox<T>& box)
{
	for (int i = 0; i < 6; i++) 
	{
		int r = 0;
		r += (Vector4<T>::DotProduct(frustumPlanes[i], Vector4<T>(box.minimum.x, box.minimum.y, box.minimum.z, 1.0)) < 0.0) ? 1 : 0;
		r += (Vector4<T>::DotProduct(frustumPlanes[i], Vector4<T>(box.maximum.x, box.minimum.y, box.minimum.z, 1.0)) < 0.0) ? 1 : 0;
		r += (Vector4<T>::DotProduct(frustumPlanes[i], Vector4<T>(box.minimum.x, box.maximum.y, box.minimum.z, 1.0)) < 0.0) ? 1 : 0;
		r += (Vector4<T>::DotProduct(frustumPlanes[i], Vector4<T>(box.maximum.x, box.maximum.y, box.minimum.z, 1.0)) < 0.0) ? 1 : 0;
		r += (Vector4<T>::DotProduct(frustumPlanes[i], Vector4<T>(box.minimum.x, box.minimum.y, box.maximum.z, 1.0)) < 0.0) ? 1 : 0;
		r += (Vector4<T>::DotProduct(frustumPlanes[i], Vector4<T>(box.maximum.x, box.minimum.y, box.maximum.z, 1.0)) < 0.0) ? 1 : 0;
		r += (Vector4<T>::DotProduct(frustumPlanes[i], Vector4<T>(box.minimum.x, box.maximum.y, box.maximum.z, 1.0)) < 0.0) ? 1 : 0;
		r += (Vector4<T>::DotProduct(frustumPlanes[i], Vector4<T>(box.maximum.x, box.maximum.y, box.maximum.z, 1.0)) < 0.0) ? 1 : 0;
		if (r == 8)
		{
			return false;
		}
	}

	// check frustum outside/inside box
	int r = 0;
	r = 0;
	for (int i = 0; i < 8; i++)
		r += ((frustumCorners[i].x > box.maximum.x) ? 1 : 0);
	if (r == 8)
		return false;
	r = 0;
	for (int i = 0; i < 8; i++)
		r += ((frustumCorners[i].x < box.minimum.x) ? 1 : 0);
	if (r == 8)
		return false;
	r = 0;
	for (int i = 0; i < 8; i++)
		r += ((frustumCorners[i].y > box.maximum.y) ? 1 : 0);
	if (r == 8)
		return false;
	r = 0;
	for (int i = 0; i < 8; i++)
		r += ((frustumCorners[i].y < box.minimum.y) ? 1 : 0);
	if (r == 8)
		return false;
	r = 0;
	for (int i = 0; i < 8; i++)
		r += ((frustumCorners[i].z > box.maximum.z) ? 1 : 0);
	if (r == 8)
		return false;
	r = 0;
	for (int i = 0; i < 8; i++)
		r += ((frustumCorners[i].z < box.minimum.z) ? 1 : 0);
	if (r == 8)
		return false;

	return true;
}

template<typename T>
bool Frustum<T>::IsBoxInFrustum(const AxisAlignedBox<T>& aabb) const
{
#if 1
    return IsBoxInFrustumIMPL(mPlanes, mFrustumCorners, aabb);
#else
    int ret = FrustumAABBIntersect(mPlane, aabb.minimum, aabb.maximum);
    return ret == OUTSIDE;
#endif
}

template<typename T>
bool Frustum<T>::IsOutOfFrustum(const OrientedBoundingBox<T>& obb) const
{
	/*if (mInitialized)
	{
		Vector3<T> point;
		Vector3<T> obbExtentX = obb.mHalfAxes.col(0) * 2.0;
		Vector3<T> obbExtentY = obb.mHalfAxes.col(1) * 2.0;
		Vector3<T> obbExtentZ = obb.mHalfAxes.col(2) * 2.0;

		for (int i = 0; i < 6; i++)
		{
			const Vector3<T>& normal = mPlane[i].getNormal();
			point = obb.mCenter;
			point = normal.DotProduct(obbExtentX) > 0.0 ? point - obbExtentX : point + obbExtentX;
			point = normal.DotProduct(obbExtentY) > 0.0 ? point - obbExtentY : point + obbExtentY;
			point = normal.DotProduct(obbExtentZ) > 0.0 ? point - obbExtentZ : point + obbExtentZ;

			if (mPlane[i].getSide(point) == PointSide::FRONT_PLANE)
				return true;
		}
	}*/
	return  false;
}

template <typename T>
bool Frustum<T>::IsSphereInFrustum(const Sphere<T> sphere) const
{
    for (int i = 0; i < 6; ++ i)
    {
        Plane<T> plane(mPlanes[i]);
        T dist = plane.GetPointDistance(sphere.GetCenter());
        if (dist + sphere.GetRadius() < 0)
        {
            return false;
        }
    }
    return true;
}

template <typename T>
static void NormalizePlane(Vector4<T> &plane)
{
    T mag = 1.0 / sqrt(plane.x * plane.x + plane.y * plane.y + plane.z * plane.z);
    plane.x = plane.x / mag;
    plane.y = plane.y / mag;
    plane.z = plane.z / mag;
    plane.w = plane.w / mag;
}

/**
 * 创建边界平面
 */
template <typename T>
void Frustum<T>::createPlane(const Matrix4x4<T>& comboMatrix)
{
    // 参考这篇文章，本质上也是在ndc坐标下定义平面后，然后再逆变换到世界空间
    //Fast Extraction of Viewing Frustum Planes from the WorldView-Projection Matrix
    
    // 提取平面方程的系数
    mPlanes[0] = (comboMatrix[3] + comboMatrix[0]);   // left
    mPlanes[1] = (comboMatrix[3] - comboMatrix[0]);   // right
    mPlanes[2] = (comboMatrix[3] + comboMatrix[1]);   // bottom
    mPlanes[3] = (comboMatrix[3] - comboMatrix[1]);   // top
    mPlanes[4] = (comboMatrix[3] + comboMatrix[2]);   // near
    mPlanes[5] = (comboMatrix[3] - comboMatrix[2]);   // far
    
    for (int i = 0; i < 6; i ++)
    {
        NormalizePlane(mPlanes[i]);
    }

	GetFrustumCorners(comboMatrix, mFrustumCorners);
}

template class Frustum<float>;
template class Frustum<double>;

NS_RENDERSYSTEM_END
