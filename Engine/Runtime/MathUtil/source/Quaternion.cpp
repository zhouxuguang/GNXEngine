

#include <assert.h>
#include <math.h>

#include "Quaternion.h"
#include "MathUtil.h"
#include "Vector3.h"
#include "Matrix3x3.h"

NS_MATHUTIL_BEGIN

#define QUAT_EPSILON 0.0000001f

//template <typename T>
//const Quaternion<T> Quaternion<T>::IDENTITY(1, 0, 0, 0);

template <typename T>
Quaternion<T>::Quaternion()
{
    w = 1;
    x = 0;
    y = 0;
    z = 0;
}

template <typename T>
Quaternion<T>::Quaternion(T w, T x, T y, T z)
{
    this->w = w;
    this->x = x;
    this->y = y;
    this->z = z;
}

template <typename T>
Quaternion<T>::Quaternion(const Quaternion<T>& rhs)
{
    w = rhs.w;
    x = rhs.x;
    y = rhs.y;
    z = rhs.z;
}

template <typename T>
Quaternion<T>& Quaternion<T>::operator =(const Quaternion<T>& rhs)
{
    if (&rhs == this)
    {
        return *this;
    }
    w = rhs.w;
    x = rhs.x;
    y = rhs.y;
    z = rhs.z;

    return *this;
}

template <typename T>
Vector3<T> Quaternion<T>::operator* (const Vector3<T>& v) const
{
#if 0
    // nVidia SDK implementation
    Vector3<T> uv, uuv;
    Vector3<T> qvec(x, y, z);
    
    uv = Vector3<T>::CrossProduct(uv,v);
    uuv = Vector3<T>::CrossProduct(uuv, uv);
    uv *= (2.0 * w);
    uuv *= 2.0;
    
    return v + uv + uuv;
#else
    
//    tvec3<T, P> const QuatVector(q.x, q.y, q.z);
//
//    tvec3<T, P> const uv(glm::cross(QuatVector, v));
//
//    tvec3<T, P> const uuv(glm::cross(QuatVector, uv));
//
//    return v + ((uv * q.w) + uuv) * static_cast<T>(2);
    
    Vector3<T> qvec(x, y, z);
    Vector3<T> uv = Vector3<T>::CrossProduct(qvec, v);
    Vector3<T> uuv = Vector3<T>::CrossProduct(qvec, uv);

    return v + ((uv * w) + uuv) * 2.0;
#endif
    
}

template <typename T>
Quaternion<T>::~Quaternion()
{
    w = 1;
    x = 0;
    y = 0;
    z = 0;
}

template <typename T>
void Quaternion<T>::FromRotateMatrix(const Matrix3x3<T>& kRot)
{
    // Algorithm in Ken Shoemake's article in 1987 SIGGRAPH course notes
    // article "Quaternion Calculus and Fast Animation".

    T fTrace = kRot[0][0] + kRot[1][1] + kRot[2][2];
    T fRoot;

    if (fTrace > 0.0)
    {
        // |w| > 1/2, may as well choose w > 1/2
        fRoot = sqrt(fTrace + 1.0);  // 2w
        w = 0.5 * fRoot;
        fRoot = 0.5 / fRoot;  // 1/(4w)
        x = (kRot[2][1] - kRot[1][2]) * fRoot;
        y = (kRot[0][2] - kRot[2][0]) * fRoot;
        z = (kRot[1][0] - kRot[0][1]) * fRoot;
    }
    else
    {
        // |w| <= 1/2
        static size_t s_iNext[3] = { 1, 2, 0 };
        size_t i = 0;
        if ( kRot[1][1] > kRot[0][0] )
            i = 1;
        if ( kRot[2][2] > kRot[i][i] )
            i = 2;
        size_t j = s_iNext[i];
        size_t k = s_iNext[j];

        fRoot = sqrt(kRot[i][i] - kRot[j][j] - kRot[k][k] + 1.0);
        T* apkQuat[3] = { &x, &y, &z };
        *apkQuat[i] = 0.5 * fRoot;
        fRoot = 0.5 / fRoot;
        w = (kRot[k][j] - kRot[j][k]) * fRoot;
        *apkQuat[j] = (kRot[j][i] + kRot[i][j]) * fRoot;
        *apkQuat[k] = (kRot[k][i] + kRot[i][k]) * fRoot;
    }
}

template <typename T>
void Quaternion<T>::ToRotateMatrix(Matrix3x3<T>& mRot) const
{
    T a1 = 1.0f - 2 * y * y - 2 * z * z;
    T a2 = 2 * x * y + 2 * w * z;
    T a3 = 2 * x * z - 2 * w * y;

    T b1 = 2 * x * y - 2 * w * z;
    T b2 = 1.0f - 2 * x * x - 2 * z * z;
    T b3 = 2 * y * z + 2 * w * x;

    T c1 = 2 * x * z + 2 * w * y;
    T c2 = 2 * y * z - 2 * w * x;
    T c3 = 1.0f - 2 * x * x - 2 * y * y;

    mRot.Init(a1, a2, a3, b1, b2, b3, c1, c2, c3);
}

template <typename T>
void Quaternion<T>::FromAngleAxis(const T fAngle, const Vector3<T>& vecAxis)
{
    T fAng = fAngle * DEGTORAD;
    
    Vector3<T> axis = vecAxis;
    if (fabs(vecAxis.Length() - 1.0) >= QUAT_EPSILON)   //不是单位向量，需要做单位化
    {
        axis.Normalize();
    }

    T thetaOver2 = fAng * 0.5;
    T sinThetaOver2 = sin(thetaOver2);

    w = cos(thetaOver2);
    x = axis.x * sinThetaOver2;
    y = axis.y * sinThetaOver2;
    z = axis.z * sinThetaOver2;
}

template <typename T>
void Quaternion<T>::ToAngleAxis(T& fAngle, Vector3<T>& vecAxis) const
{
    T fSqrLength = x * x + y * y + z * z;
    if (fSqrLength > 0.0)
    {
        fAngle = 2.0 * acos(w);
        T fInvLength = 1.0 / sqrt(fSqrLength);
        vecAxis.x = x * fInvLength;
        vecAxis.y = y * fInvLength;
        vecAxis.z = z * fInvLength;
    }
    else
    {
        // angle is 0 (mod 2*pi), so any axis will do
        fAngle = 0.0;
        vecAxis.x = 1.0;
        vecAxis.y = 0.0;
        vecAxis.z = 0.0;
    }
}

template <typename T>
void Quaternion<T>::FromEulerAngles(T yaw, T pitch, T roll)
{
    // Abbreviations for the various angular functions
    // 下面这个是正确的
    T cy = cos(yaw * 0.5);
    T sy = sin(yaw * 0.5);
    T cp = cos(pitch * 0.5);
    T sp = sin(pitch * 0.5);
    T cr = cos(roll * 0.5);
    T sr = sin(roll * 0.5);
 
    w = cy * cp * cr + sy * sp * sr;
    x = cy * cp * sr + sy * sp * cr;
    y = cy * sp * cr - sy * cp * sr;
    z = sy * cp * cr - cy * sp * sr;
}

template <typename T>
void Quaternion<T>::FromToRotation(const Vector3<T>& from, const Vector3<T>& to)
{
    //不标准化，认为外面已经标准化
    Vector3<T> p0 = (from);
    Vector3<T> p1 = (to);
 
    if (p0 == -p1)
    {
        Vector3<T> mostOrthogonal = Vector3<T>(1, 0, 0);
 
        if (abs(p0.y) < abs(p0.x))
        {
            mostOrthogonal = Vector3<T>(0, 1, 0);
        }
 
        if (abs(p0.z) < abs(p0.y) && abs(p0.z) < abs(p0.x))
        {
            mostOrthogonal = Vector3<T>(0, 0, 1);
        }
 
        Vector3<T> axis = Vector3<T>::CrossProduct(p0, mostOrthogonal).Normalize();
        x = axis.x;
        y = axis.y;
        z = axis.z;
        w = 0;
    }
 
    Vector3<T> half = (p0 + p1).Normalize();
    Vector3<T> axis = Vector3<T>::CrossProduct(p0, half);
 
    x = axis.x;
    y = axis.y;
    z = axis.z;
    w = p0.DotProduct(half);
}

template <typename T>
T Quaternion<T>::Norm() const
{
    return w * w + x * x + y * y + z * z;
}

template <typename T>
Quaternion<T> Quaternion<T>::Normalized() const
{
    T lenSq = w * w + x * x + y * y + z * z;
    if (lenSq < QUAT_EPSILON)
    {
        return Quaternion<T>();
    }
    T i_len = 1.0 / sqrt(lenSq);

    return Quaternion<T>(
        w * i_len,
        x * i_len,
        y * i_len,
        z * i_len
    );
}

template <typename T>
void Quaternion<T>::Normalize()
{
    T mag = sqrt(w * w + x * x + y * y + z * z);

    if (mag > 0.0)
    {
        T oneOverMag = 1.0 / mag;
        w *= oneOverMag;
        x *= oneOverMag;
        y *= oneOverMag;
        z *= oneOverMag;

    }
    else
    {
        assert(false);
    }
}

template <typename T>
Quaternion<T> Quaternion<T>::operator-() const
{
    return Quaternion(-w, -x, -y, -z);
}

template <typename T>
Quaternion<T> Quaternion<T>::operator +(const Quaternion &other) const
{
    return Quaternion(
            w + other.w,
                      x + other.x,
                      y + other.y,
                      z + other.z
        );
}

template <typename T>
Quaternion<T> Quaternion<T>::operator *(const Quaternion<T> &a) const
{
    Quaternion<T> result;

    result.w = w * a.w - x * a.x - y * a.y - z * a.z;
    result.x = w * a.x + x * a.w + z * a.y - y * a.z;
    result.y = w * a.y + y * a.w + x * a.z - z * a.x;
    result.z = w * a.z + z * a.w + y * a.x - x * a.y;

    return result;
}

template <typename T>
Quaternion<T> &Quaternion<T>::operator *=(const Quaternion<T> &a)
{
    *this = *this * a;

    return *this;
}

template <typename T>
Quaternion<T> Quaternion<T>::operator* (float scale) const
{
    return Quaternion(
            w * scale,
                      x * scale,
                      y * scale,
                      z * scale
        );
}

template <typename T>
bool Quaternion<T>::operator==(const Quaternion& right) const
{
    return (fabs(w - right.w) <= QUAT_EPSILON &&
            fabs(x - right.x) <= QUAT_EPSILON &&
            fabs(y - right.y) <= QUAT_EPSILON &&
            fabs(z - right.z) <= QUAT_EPSILON);
}

template <typename T>
T Quaternion<T>::DotProduct(const Quaternion<T> &b) const
{
    return w * b.w + x * b.x + y * b.y + z * b.z;
}

template <typename T>
Quaternion<T> Quaternion<T>::Inverse(const Quaternion<T>& rotation)
{
    Quaternion<T> result;
    T n = rotation.x * rotation.x + rotation.y * rotation.y +
        rotation.z * rotation.z + rotation.w * rotation.w;
    if (n == 1.0)
    {
        result.x = -rotation.x;
        result.y = -rotation.y;
        result.z = -rotation.z;
        
        return result;
    }
    
    n = 1.0 / n;
    result.x = -rotation.x * n;
    result.y = -rotation.y * n;
    result.z = -rotation.z * n;
    result.w = rotation.w * n;
    
    return result;
}

template <typename T>
Quaternion<T> Quaternion<T>::Mix(const Quaternion<T>& from, const Quaternion<T>& to, float t)
{
    return from * (1.0 - t) + to * t;
}

template <typename T>
Quaternion<T> Quaternion<T>::Slerp(const Quaternion<T> &p, const Quaternion<T> &q, T t)
{
    if (t <= 0.0) return p;
    if (t >= 1.0) return q;

    T cosOmega = p.DotProduct(q);

    T q1w = q.w;
    T q1x = q.x;
    T q1y = q.y;
    T q1z = q.z;
    if (cosOmega < 0.0) 
    {
        q1w = -q1w;
        q1x = -q1x;
        q1y = -q1y;
        q1z = -q1z;
        cosOmega = -cosOmega;
    }

    assert(cosOmega < 1.0);

    T k0, k1;
    if (cosOmega > 0.999999) 
    {
        k0 = 1.0 - t;
        k1 = t;

    } 
    else 
    {
        T sinOmega = sqrt(1.0 - cosOmega * cosOmega);
        T omega = atan2(sinOmega, cosOmega);
        T oneOverSinOmega = 1.0 / sinOmega;

        k0 = sin((1.0 - t) * omega) * oneOverSinOmega;
        k1 = sin(t * omega) * oneOverSinOmega;
    }

    Quaternion<T> result;
    result.x = k0 * p.x + k1 * q1x;
    result.y = k0 * p.y + k1 * q1y;
    result.z = k0 * p.z + k1 * q1z;
    result.w = k0 * p.w + k1 * q1w;

    return result;
}

template <typename T>
Quaternion<T> Quaternion<T>::Conjugate(const Quaternion<T> &q)
{
    Quaternion<T> result;

    result.w = q.w;

    result.x = -q.x;
    result.y = -q.y;
    result.z = -q.z;

    return result;
}

template <typename T>
Quaternion<T> Quaternion<T>::Pow(const Quaternion<T> &q, T fExponent)
{
    if (fabs(q.w) > 0.9999)
    {
        return q;
    }

    T alpha = acos(q.w);
    T newAlpha = alpha * fExponent;

    Quaternion result;
    result.w = cos(newAlpha);

    T mult = sin(newAlpha) / sin(alpha);
    result.x = q.x * mult;
    result.y = q.y * mult;
    result.z = q.z * mult;

    return result;
}

template <typename T>
Quaternion<T> Quaternion<T>::LookRotation(const Vector3<T>& direcion, const Vector3<T>& up)
{
    // Find orthonormal basis vectors
    Vector3<T> f = direcion;
    Vector3<T> u = up;
    Vector3<T> r = Vector3<T>::CrossProduct(u, f);
    u = Vector3<T>::CrossProduct(f, r);

    // From world forward to object forward
    Quaternion f2d;
    f2d.FromToRotation(Vector3<T>(0, 0, 1), f);

    // what direction is the new object up?
    Vector3<T> objectUp = f2d * Vector3<T>(0, 1, 0);
    // From object up to desired up
    Quaternion<T> u2u;
    u2u.FromToRotation(objectUp, u);

    // Rotate to forward direction first, then twist to correct up
    Quaternion<T> result = f2d * u2u;
    // Don't forget to normalize the result
    result.Normalize();
    
    return result;
}

template class Quaternion<float>;
template class Quaternion<double>;

NS_MATHUTIL_END
