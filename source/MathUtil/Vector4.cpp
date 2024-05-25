
#include <memory.h>
#include "Vector4.h"

NS_MATHUTIL_BEGIN

template <typename T>
Vector4<T>::Vector4()
{
    x = 0.0f;
    y = 0.0f;
    z = 0.0f;
    w = 0.0f;
}

template <typename T>
Vector4<T>::Vector4(const T *pfMatrix)
{
	memcpy(&x,pfMatrix,sizeof(T)*4);
}

template <typename T>
Vector4<T>::Vector4(T a1,T a2,T a3,T a4)
{
	x = a1;
	y = a2;
	z = a3;
	w = a4;
}

template <typename T>
Vector4<T>::Vector4(const Vector3<T>& rhs) : x(rhs.x), y(rhs.y), z(rhs.z), w(1.0f)
{

}

template <typename T>
Vector4<T>::~Vector4(void)
{
}

template <typename T>
Vector4<T>& Vector4<T>::operator +(T fValue)
{
	x += fValue;
	y += fValue;
	z += fValue;
	w += fValue;

	return *this;
}

template <typename T>
Vector4<T>& Vector4<T>::operator -(T fValue)
{
	x -= fValue;
	y -= fValue;
	z -= fValue;
	w -= fValue;

	return *this;
}

template <typename T>
Vector4<T>& Vector4<T>::operator *(T fValue)
{
	x *= fValue;
	y *= fValue;
	z *= fValue;
	w *= fValue;

	return *this;
}

template <typename T>
Vector4<T>& Vector4<T>::operator /(T fValue)
{
	x /= fValue;
	y /= fValue;
	z /= fValue;
	w /= fValue;

	return *this;
}

template <typename T>
Vector4<T>& Vector4<T>::operator=(const Vector4& rkVector)
{
	x = rkVector.x;
	y = rkVector.y;
	z = rkVector.z;
	w = rkVector.w;

	return *this;
}

template <typename T>
Vector4<T>& Vector4<T>::operator=(const Vector3<T>& rhs)
{
	x = rhs.x;
	y = rhs.y;
	z = rhs.z;
	w = 1.0f;
	return *this;
}

template class Vector4<float>;
template class Vector4<double>;


NS_MATHUTIL_END

