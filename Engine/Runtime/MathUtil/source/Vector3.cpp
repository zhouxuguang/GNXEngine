#include "Vector3.h"

NS_MATHUTIL_BEGIN


//template <typename T>
//const Vector3<T> Vector3<T>::ZERO( 0, 0, 0 );
//
//template <typename T>
//const Vector3<T> Vector3<T>::UNIT_SCALE(1, 1, 1);
//
//template <typename T>
//const Vector3<T> Vector3<T>::UNIT_X(1, 0, 0);
//
//template <typename T>
//const Vector3<T> Vector3<T>::UNIT_Y(0, 1, 0);
//
//template <typename T>
//const Vector3<T> Vector3<T>::UNIT_Z(0, 0, 1);

template <typename T>
T Vector3<T>::Length() const
{
    return sqrt(x * x + y * y + z * z);
}

template <typename T>
T Vector3<T>::DotProduct(const Vector3<T> &a) const
{
    return x * a.x + y * a.y + z * a.z;
}

template <typename T>
Vector3<T> Vector3<T>::CrossProduct(const Vector3 &a, const Vector3 &b)
{
    return Vector3(
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
        );
}

template <typename T>
Vector3<T>& Vector3<T>::Normalize()
{
    T magSq = x * x + y * y + z * z;
    if (magSq > 0.0) 
    { 
        T oneOverMag = 1.0 / sqrt(magSq);
        x *= oneOverMag;
        y *= oneOverMag;
        z *= oneOverMag;
    }
    
    return *this;
}

template <typename T>
Vector3<T> Vector3<T>::Abs() const
{
    return Vector3(fabs(x), fabs(y), fabs(z));
}

template <typename T>
T Vector3<T>::LengthSq() const
{
    return x * x + y * y + z * z;
}

template <typename T>
Vector3<T> Vector3<T>::Reflection(const Vector3& incident, const Vector3& normal)
{
    return incident - normal * (normal.DotProduct(incident)) * 2.0;
}

template <typename T>
Vector3<T> Vector3<T>::Refraction(const Vector3& incident, const Vector3& normal, T eta)
{
    T k = 1.0 - eta * eta * (1.0 - normal.DotProduct(incident) * normal.DotProduct(incident));
    if (k < 0)
    {
        return Vector3<T>(0, 0, 0);
    }
    return incident * eta - normal * (eta * normal.DotProduct(incident) + sqrt(k));
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
T Vector3<T>::Distance(const Vector3<T>& other) const
{
    T dx = x - other.x;
    T dy = y - other.y;
    T dz = z - other.z;
    return sqrt(dx * dx + dy * dy + dz * dz);
}

template <typename T>
T Vector3<T>::DistanceSquared(const Vector3<T>& other) const
{
    T dx = x - other.x;
    T dy = y - other.y;
    T dz = z - other.z;
    return dx * dx + dy * dy + dz * dz;
}

template class Vector3<float>;
template class Vector3<double>;

NS_MATHUTIL_END
