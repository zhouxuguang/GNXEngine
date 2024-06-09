#include "Vector2.h"

NS_MATHUTIL_BEGIN


template <typename T>
Vector2<T>::Vector2()
{

}

template <typename T>
Vector2<T>::Vector2(const T fX, const T fY) : x( fX ), y( fY )
{

}

template <typename T>
Vector2<T>::Vector2(const T scaler) : x( scaler), y( scaler )
{

}

template <typename T>
Vector2<T>::Vector2(const T afCoordinate[2]) : x( afCoordinate[0] ), y( afCoordinate[1] )
{

}

//template <typename T>
//Vector2<T>::Vector2(const int afCoordinate[2])
//{
//	x = (Real)afCoordinate[0];
//	y = (Real)afCoordinate[1];
//}

template <typename T>
Vector2<T>::Vector2(T* const r) : x( r[0] ), y( r[1] )
{

}

template <typename T>
Vector2<T>::~Vector2(void)
{

}

template <typename T>
void Vector2<T>::Swap(Vector2& other)
{
	std::swap(x, other.x);
	std::swap(y, other.y);
}

template <typename T>
T Vector2<T>::Length() const
{
	return sqrt( x * x + y * y );
}

template <typename T>
T Vector2<T>::SquaredLength() const
{
	return x * x + y * y;
}

template <typename T>
T Vector2<T>::Distance(const Vector2& rhs) const
{
	return (*this - rhs).Length();
}

template <typename T>
T Vector2<T>::SquaredDistance(const Vector2& rhs) const
{
	return (*this - rhs).SquaredLength();
}

template <typename T>
T Vector2<T>::DotProduct(const Vector2& vec) const
{
	return x * vec.x + y * vec.y;
}

template <typename T>
T Vector2<T>::Normalise()
{
	Real fLength = sqrt( x * x + y * y);
	if ( fLength > Real(0.0f) )
	{
		Real fInvLength = 1.0f / fLength;
		x *= fInvLength;
		y *= fInvLength;
	}

	return fLength;
}

template <typename T>
T Vector2<T>::CrossProduct(const Vector2& rkVector) const
{
	return x * rkVector.y - y * rkVector.x;
}

template <typename T>
Vector2<T> Vector2<T>::Reflect(const Vector2& normal) const
{
	return Vector2( *this - ( 2 * this->DotProduct(normal) * normal ) );
}

template class Vector2<float>;
template class Vector2<double>;
template class Vector2<int>;

NS_MATHUTIL_END
