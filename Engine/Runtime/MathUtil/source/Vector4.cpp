
#include <memory.h>
#include "Vector4.h"

NS_MATHUTIL_BEGIN

template <typename T>
Vector4<T>::Vector4()
{
    x = 0.0;
    y = 0.0;
    z = 0.0;
    w = 0.0;
}

template <typename T>
Vector4<T>::Vector4(const T *pfMatrix)
{
	memcpy(&x, pfMatrix, sizeof(T) * 4);
}

template <typename T>
Vector4<T>::Vector4(T a1, T a2, T a3, T a4)
{
	x = a1;
	y = a2;
	z = a3;
	w = a4;
}

template <typename T>
Vector4<T>::Vector4(const Vector3<T>& rhs) : x(rhs.x), y(rhs.y), z(rhs.z), w(1.0)
{

}

template <typename T>
Vector4<T>::~Vector4(void)
{
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
	w = 1.0;
	return *this;
}

template <typename T>
T Vector4<T>::DotProduct(const Vector4 &a) const
{
    return a.x * x + a.y * y + a.z * z + a.w * w;
}

template class Vector4<float>;
template class Vector4<double>;


NS_MATHUTIL_END

