//
//  Frustum.cpp
//  GNXEngine
//
//  Created by zhouxuguang on 2021/6/13.
//

#include "Frustum.h"

NS_MATHUTIL_BEGIN

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
bool Frustum<T>::InitFrustum(const Matrix4x4<T>& comboMatrix, bool ndcZeroToOne)
{
	mNdcZeroToOne = ndcZeroToOne;
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

	for (int i = 0; i < kPlaneFrustumNum; ++i)
	{
		// X axis
		if (planes[i].GetNormal().x > 0)
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
		if (planes[i].GetNormal().y > 0)
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
		if (planes[i].GetNormal().z > 0)
		{
			vmin.z = mins.z;
			vmax.z = maxs.z;
		}
		else
		{
			vmin.z = maxs.z;
			vmax.z = mins.z;
		}
		if (planes[i].GetNormal().DotProduct(vmin) - planes[i].GetDist() > 0)
		{
			return OUTSIDE;
		}

		if (planes[i].GetNormal().DotProduct(vmax) - planes[i].GetDist() >= 0)
		{
			ret = INTERSECT;
		}
	}
	return ret;
}

template<typename T>
static inline void GetFrustumCorners(const Matrix4x4<T> viewProj, Vector4<T>* points, bool ndcZeroToOne)
{
	// NDC corner points depend on the depth convention:
	//   OpenGL: z ∈ [-1, 1]  → near z = -1, far z = 1
	//   Reverse-Z (Vulkan/Metal): z ∈ [0, 1]  → near z = 1, far z = 0
	//
	// For infinite far plane projections, z_ndc=0 (reverse-Z) or z_ndc=1 (OpenGL)
	// maps to z = infinity, causing w = 0 after inverse projection.
	// Use a small ε instead to get a "very far but finite" corner, avoiding w = 0.
	const T FAR_EPSILON = T(0.001);

	const Vector4<T> cornersGL[] = {
		Vector4<T>(-1, -1, -1, 1), Vector4<T>(1, -1, -1, 1),
		Vector4<T>(1, 1, -1, 1),   Vector4<T>(-1, 1, -1, 1),
		Vector4<T>(-1, -1, 1 - FAR_EPSILON, 1),  Vector4<T>(1, -1, 1 - FAR_EPSILON, 1),
		Vector4<T>(1, 1, 1 - FAR_EPSILON, 1),    Vector4<T>(-1, 1, 1 - FAR_EPSILON, 1)
	};
	const Vector4<T> cornersRZ[] = {
		Vector4<T>(-1, -1, 1, 1),  Vector4<T>(1, -1, 1, 1),
		Vector4<T>(1, 1, 1, 1),    Vector4<T>(-1, 1, 1, 1),
		Vector4<T>(-1, -1, FAR_EPSILON, 1),  Vector4<T>(1, -1, FAR_EPSILON, 1),
		Vector4<T>(1, 1, FAR_EPSILON, 1),    Vector4<T>(-1, 1, FAR_EPSILON, 1)
	};
	const Vector4<T>* corners = ndcZeroToOne ? cornersRZ : cornersGL;

	const Matrix4x4<T> invViewProj = viewProj.Inverse();

	for (int i = 0; i != 8; i++)
	{
		Vector4<T> q = invViewProj * corners[i];
		if (std::abs(q.w) < T(1e-7))
		{
			// Fallback: project direction to a large finite distance
			q = q * (T(1e6) / std::max(std::abs(q.x) + std::abs(q.y) + std::abs(q.z), T(1)));
			q.w = T(1);
		}
		else
		{
			q = q / q.w;
		}
		points[i] = q;
	}
}

template<typename T>
static inline bool IsBoxInFrustumIMPL(const Vector4<T>* frustumPlanes, const Vector4<T>* frustumCorners, const AxisAlignedBox<T>& box)
{
	for (int i = 0; i < kPlaneFrustumNum; i++)
	{
		int r = 0;
		r += (frustumPlanes[i].DotProduct(Vector4<T>(box.minimum.x, box.minimum.y, box.minimum.z, 1.0)) < 0.0) ? 1 : 0;
		r += (frustumPlanes[i].DotProduct(Vector4<T>(box.maximum.x, box.minimum.y, box.minimum.z, 1.0)) < 0.0) ? 1 : 0;
		r += (frustumPlanes[i].DotProduct(Vector4<T>(box.minimum.x, box.maximum.y, box.minimum.z, 1.0)) < 0.0) ? 1 : 0;
		r += (frustumPlanes[i].DotProduct(Vector4<T>(box.maximum.x, box.maximum.y, box.minimum.z, 1.0)) < 0.0) ? 1 : 0;
		r += (frustumPlanes[i].DotProduct(Vector4<T>(box.minimum.x, box.minimum.y, box.maximum.z, 1.0)) < 0.0) ? 1 : 0;
		r += (frustumPlanes[i].DotProduct(Vector4<T>(box.maximum.x, box.minimum.y, box.maximum.z, 1.0)) < 0.0) ? 1 : 0;
		r += (frustumPlanes[i].DotProduct(Vector4<T>(box.minimum.x, box.maximum.y, box.maximum.z, 1.0)) < 0.0) ? 1 : 0;
		r += (frustumPlanes[i].DotProduct(Vector4<T>(box.maximum.x, box.maximum.y, box.maximum.z, 1.0)) < 0.0) ? 1 : 0;
		if (r == 8)
		{
			return false;
		}
	}

	// check frustum outside/inside box
	int r = 0;
	r = 0;
	for (int i = 0; i < 8; i++)
		r += ((frustumCorners[i].x > box.maximum.x) ?1 : 0);
	if (r == 8)
		return false;
	r = 0;
	for (int i = 0; i < 8; i++)
		r += ((frustumCorners[i].x < box.minimum.x) ?1 : 0);
	if (r == 8)
		return false;
	r = 0;
	for (int i = 0; i < 8; i++)
		r += ((frustumCorners[i].y > box.maximum.y) ?1 : 0);
	if (r == 8)
		return false;
	r = 0;
	for (int i = 0; i < 8; i++)
		r += ((frustumCorners[i].y < box.minimum.y) ?1 : 0);
	if (r == 8)
		return false;
	r = 0;
	for (int i = 0; i < 8; i++)
		r += ((frustumCorners[i].z > box.maximum.z) ?1 : 0);
	if (r == 8)
		return false;
	r = 0;
	for (int i = 0; i < 8; i++)
		r += ((frustumCorners[i].z < box.minimum.z) ?1 : 0);
	if (r == 8)
		return false;

	return true;
}

template<typename T>
bool Frustum<T>::IsBoxInFrustum(const AxisAlignedBox<T>& aabb) const
{
    // p-vertex test: for each frustum plane, test the AABB corner that is
    // MOST aligned with the plane normal (the "positive vertex").
    // Since frustum plane normals point inward (toward the frustum interior),
    // the p-vertex maximizes the plane dot product. If even the p-vertex is
    // on the negative side of the plane, ALL 8 corners must be outside,
    // and the box can be safely culled.
    for (int i = 0; i < kPlaneFrustumNum; i++)
    {
        const Vector4<T>& p = mPlanes[i];

        // Select the AABB corner farthest in the plane-normal direction.
        // This is the corner most likely to be inside the frustum.
        T px = (p.x > 0) ? aabb.maximum.x : aabb.minimum.x;
        T py = (p.y > 0) ? aabb.maximum.y : aabb.minimum.y;
        T pz = (p.z > 0) ? aabb.maximum.z : aabb.minimum.z;

        if (p.x * px + p.y * py + p.z * pz + p.w < 0.0)
            return false;
    }
    return true;
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
		}*/
	return  false;
}

template <typename T>
bool Frustum<T>::IsSphereInFrustum(const Sphere<T> sphere) const
{
    for (int i = 0; i < kPlaneFrustumNum; ++ i)
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
    //Fast Extraction of Viewing Frustum Planes from WorldView-Projection Matrix

    // 提取平面方程的系数
    // Left/right/bottom/top 对任何深度约定都一样
    mPlanes[kPlaneFrustumLeft]    = (comboMatrix[3] + comboMatrix[0]);   // left
    mPlanes[kPlaneFrustumRight]   = (comboMatrix[3] - comboMatrix[0]);   // right
    mPlanes[kPlaneFrustumBottom]  = (comboMatrix[3] + comboMatrix[1]);   // bottom
    mPlanes[kPlaneFrustumTop]     = (comboMatrix[3] - comboMatrix[1]);   // top

    if (mNdcZeroToOne)
    {
        // NDC z ∈ [0, 1] (Vulkan/Metal)
        // 两个z平面为 VP[2] 和 VP[3]-VP[2]
        // 无论是否Reverse-Z，平面集相同，仅near/far含义互换
        // Reverse-Z: near = VP[3]-VP[2] (z_ndc=1), far = VP[2] (z_ndc=0)
        // Standard-Z: near = VP[2] (z_ndc=0), far = VP[3]-VP[2] (z_ndc=1)
        // 对剔除来说只需6个平面都正确，哪个叫near哪个叫far不影响结果
        mPlanes[kPlaneFrustumNear] = (comboMatrix[3] - comboMatrix[2]);   // one z-plane
        mPlanes[kPlaneFrustumFar]  = comboMatrix[2];                       // other z-plane
    }
    else
    {
        // OpenGL: NDC z ∈ [-1, 1]
        mPlanes[kPlaneFrustumNear] = (comboMatrix[3] + comboMatrix[2]);   // near
        mPlanes[kPlaneFrustumFar]  = (comboMatrix[3] - comboMatrix[2]);   // far
    }

    for (int i = 0; i < kPlaneFrustumNum; i ++)
    {
        NormalizePlane(mPlanes[i]);
    }

	GetFrustumCorners(comboMatrix, mFrustumCorners, mNdcZeroToOne);
}

template class Frustum<float>;
template class Frustum<double>;

NS_MATHUTIL_END
