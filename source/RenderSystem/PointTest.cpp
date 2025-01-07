#include "PointTest.h"

NS_RENDERSYSTEM_BEGIN

bool PointTest::PointInSphere(const Vector3f& point, const Sphere& sphere)
{
	return (point - sphere.mCenter).LengthSq() < sphere.mRadius * sphere.mRadius;
}

Vector3f PointTest::ClosestPoint(const Sphere& sphere, const Vector3f& point)
{
	// Find a normalized vector from the center of the sphere to the test point 
	Vector3f sphereToPoint = point - sphere.mCenter;
	sphereToPoint.Normalize();

	// 使用半径缩放向量
	sphereToPoint = sphereToPoint * sphere.mRadius;

	//加上球心偏移
	return sphereToPoint + sphere.mCenter;
}

bool PointTest::PointInAABB(const Vector3f& point, const AABB& aabb)
{
	const Vector3f& min = aabb.mMin;
	const Vector3f& max = aabb.mMax;

	if (point.x < min.x || point.y < min.y || point.z < min.z) 
	{
		return false;
	}
	if (point.x > max.x || point.y > max.y || point.z > max.z) 
	{
		return false;
	}

	return true;
}

Vector3f PointTest::ClosestPoint(const AABB& aabb, const Vector3f& point)
{
	Vector3f result = point;
	const Vector3f& min = aabb.mMin;
	const Vector3f& max = aabb.mMax;

	result.x = (result.x < min.x) ? min.x : result.x;
	result.y = (result.y < min.x) ? min.y : result.y;
	result.z = (result.z < min.x) ? min.z : result.z;

	result.x = (result.x > max.x) ? max.x : result.x;
	result.y = (result.y > max.x) ? max.y : result.y;
	result.z = (result.z > max.x) ? max.z : result.z;

	return result;
}

bool PointTest::PointInOBB(const Vector3f& point, const OBB& obb)
{
	Vector3f dir = point - obb.mCenter;

	/*for (int i = 0; i < 3; ++i) 
	{
		const float* orientation = &obb.orientation.asArray[i * 3];
		vec3 axis(orientation[0], orientation[1], orientation[2]);

		float distance = Dot(dir, axis);

		if (distance > obb.size.asArray[i]) 
		{
			return false;
		}
		if (distance < -obb.size.asArray[i]) 
		{
			return false;
		}
	}*/

	return true;
}

NS_RENDERSYSTEM_END


