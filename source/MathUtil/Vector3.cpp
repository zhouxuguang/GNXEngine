#include "Vector3.h"

NS_MATHUTIL_BEGIN


template <typename T>
const Vector3<T> Vector3<T>::ZERO( 0, 0, 0 );

template <typename T>
const Vector3<T> Vector3<T>::UNIT_SCALE(1, 1, 1);

template <typename T>
const Vector3<T> Vector3<T>::UNIT_X(1, 0, 0);

template <typename T>
const Vector3<T> Vector3<T>::UNIT_Y(0, 1, 0);

template <typename T>
const Vector3<T> Vector3<T>::UNIT_Z(0, 0, 1);

template <typename T>
const T Vector3<T>::Length() const
{
	return sqrt(x * x + y * y + z*z);
}

template <typename T>
T Vector3<T>::Length()
{
	return sqrt(x * x + y * y + z*z);
}

template <typename T>
T Vector3<T>::DotProduct(const Vector3<T> &a) const
{
	return x*a.x + y*a.y + z*a.z;
}

template <typename T>
Vector3<T> Vector3<T>::CrossProduct(const Vector3 &a, const Vector3 &b)
{
    return Vector3(
        a.y*b.z - a.z*b.y,
        a.z*b.x - a.x*b.z,
        a.x*b.y - a.y*b.x
        );
}

template <typename T>
const Vector3<T>& Vector3<T>::Normalize() const
{
	Real magSq = x*x + y*y + z*z;
	if (magSq > 0.0f) 
	{ 
		float oneOverMag = 1.0f / sqrt(magSq);
		x *= oneOverMag;
		y *= oneOverMag;
		z *= oneOverMag;
	}
    
    return *this;
}

template <typename T>
T Vector3<T>::LengthSq() const
{
	return x * x + y * y + z*z;
}

template <typename T>
Vector3<T> Vector3<T>::Reflection(const Vector3& vecLight,const Vector3& vecNormal)
{
	return vecNormal*(vecNormal.DotProduct(vecNormal))*2 - vecLight;
}

template <typename T>
Vector3<T> Vector3<T>::Refraction(const Vector3& vecLight,const Vector3& vecNormal, T eta)
{
	T k = 1.0 - eta*eta *(1.0-vecNormal.DotProduct(vecLight)*vecNormal.DotProduct(vecLight));
	if ( k < 0)
	{
		return Vector3::ZERO;
	}

	else
		return vecLight*eta - vecNormal * (eta*vecNormal.DotProduct(vecLight)+sqrt(k));
}

template <typename T>
Vector3<T> Vector3<T>::Lerp(const Vector3& s, const Vector3& e, float t)
{
    return Vector3(
        s.x + (e.x - s.x) * t,
        s.y + (e.y - s.y) * t,
        s.z + (e.z - s.z) * t
    );
}

template <typename T>
T Distance(const Vector3<T> &a, const Vector3<T> &b)
{
	T dx = a.x - b.x;
	T dy = a.y - b.y;
	T dz = a.z - b.z;
	return sqrt(dx*dx + dy*dy + dz*dz);
}

template <typename T>
T DistanceSquared(const Vector3<T> &a, const Vector3<T> &b)
{
	Real dx = a.x - b.x;
	Real dy = a.y - b.y;
	Real dz = a.z - b.z;
	return dx*dx + dy*dy + dz*dz;
}

template class Vector3<float>;
template class Vector3<double>;

NS_MATHUTIL_END
