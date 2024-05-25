

#include <assert.h>
#include <math.h>

#include "Quaternion.h"
#include "MathUtil.h"
#include "Vector3.h"
#include "Matrix3x3.h"

NS_MATHUTIL_BEGIN

#define QUAT_EPSILON 0.000001f

template <typename T>
const Quaternion<T> Quaternion<T>::IDENTITY(1, 0, 0, 0);

template <typename T>
Quaternion<T>::Quaternion()
{
	m_dfW = 1;
	m_dfX = 0;
	m_dfY = 0;
	m_dfZ = 0;
}

template <typename T>
Quaternion<T>::Quaternion(T w, T x, T y, T z)
{
	m_dfW = w;
	m_dfX = x;
	m_dfY = y;
	m_dfZ = z;
}

template <typename T>
Quaternion<T>::Quaternion(const Quaternion<T>& rhs)
{
	m_dfW = rhs.m_dfW;
	m_dfX = rhs.m_dfX;
	m_dfY = rhs.m_dfY;
	m_dfZ = rhs.m_dfZ;
}

template <typename T>
Quaternion<T>& Quaternion<T>::operator =(const Quaternion<T>& rhs)
{
	if (&rhs == this)
	{
		return *this;
	}
	m_dfW = rhs.m_dfW;
	m_dfX = rhs.m_dfX;
	m_dfY = rhs.m_dfY;
	m_dfZ = rhs.m_dfZ;

	return *this;
}

template <typename T>
Vector3<T> Quaternion<T>::operator* (const Vector3<T>& v) const
{
#if 0
    // nVidia SDK implementation
    Vector3<T> uv, uuv;
    Vector3<T> qvec(m_dfX, m_dfY, m_dfZ);
    
    uv = Vector3<T>::CrossProduct(uv,v);
    uuv = Vector3<T>::CrossProduct(uuv, uv);
    uv *= (2.0 * m_dfW);
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
    
    Vector3<T> qvec(m_dfX, m_dfY, m_dfZ);
    Vector3<T> uv = Vector3<T>::CrossProduct(qvec, v);
    Vector3<T> uuv = Vector3<T>::CrossProduct(qvec, uv);

    return v + ((uv * m_dfW) + uuv) * 2.0;
#endif
    
}

template <typename T>
Quaternion<T>::~Quaternion()
{
    m_dfW = 1;
    m_dfX = 0;
    m_dfY = 0;
    m_dfZ = 0;
}

template <typename T>
void Quaternion<T>::FromRotateMatrix(const Matrix3x3<T>& kRot)
{
	// Algorithm in Ken Shoemake's article in 1987 SIGGRAPH course notes
	// article "Quaternion Calculus and Fast Animation".

	T fTrace = kRot[0][0]+kRot[1][1]+kRot[2][2];
	T fRoot;

	if ( fTrace > 0.0 )
	{
		// |w| > 1/2, may as well choose w > 1/2
		fRoot = sqrt(fTrace + 1.0f);  // 2w
		m_dfW = 0.5f*fRoot;
		fRoot = 0.5f/fRoot;  // 1/(4w)
		m_dfX = (kRot[2][1]-kRot[1][2])*fRoot;
		m_dfY = (kRot[0][2]-kRot[2][0])*fRoot;
		m_dfZ = (kRot[1][0]-kRot[0][1])*fRoot;
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

		fRoot = sqrt(kRot[i][i]-kRot[j][j]-kRot[k][k] + 1.0f);
		T* apkQuat[3] = { &m_dfX, &m_dfY, &m_dfZ };
		*apkQuat[i] = 0.5f*fRoot;
		fRoot = 0.5f/fRoot;
		m_dfW = (kRot[k][j]-kRot[j][k])*fRoot;
		*apkQuat[j] = (kRot[j][i]+kRot[i][j])*fRoot;
		*apkQuat[k] = (kRot[k][i]+kRot[i][k])*fRoot;
	}
}

template <typename T>
void Quaternion<T>::ToRotateMatrix(Matrix3x3<T>& mRot) const
{
	T a1 = 1.0f - 2*m_dfY*m_dfY - 2*m_dfZ*m_dfZ;
	T a2 = 2*m_dfX*m_dfY + 2*m_dfW*m_dfZ;
	T a3 = 2*m_dfX*m_dfZ - 2*m_dfW*m_dfY;

	T b1 = 2*m_dfX*m_dfY - 2*m_dfW*m_dfZ;
	T b2 = 1.0f - 2*m_dfX*m_dfX - 2*m_dfZ*m_dfZ;
	T b3 = 2*m_dfY*m_dfZ + 2*m_dfW*m_dfX;

	T c1 = 2*m_dfX*m_dfZ + 2*m_dfW*m_dfY;
	T c2 = 2*m_dfY*m_dfZ - 2*m_dfW*m_dfX;
	T c3 = 1.0f - 2*m_dfX*m_dfX - 2*m_dfY*m_dfY;

	mRot.Init(a1,a2,a3,b1,b2,b3,c1,c2,c3);
}

template <typename T>
void Quaternion<T>::FromAngleAxis(const T fAngle, const Vector3<T>& vecAxis)
{
	T fAng = fAngle * DEGTORAD;
    
    Vector3<T> axis = vecAxis;
    if (fabs(vectorMag(vecAxis) - 1.0f) >= QUAT_EPSILON)   //不是单位向量，需要做单位化
    {
        axis.Normalize();
    }

	T	thetaOver2 = fAng * 0.5f;
	T	sinThetaOver2 = sin(thetaOver2);

	m_dfW = cos(thetaOver2);
	m_dfX = vecAxis.x * sinThetaOver2;
	m_dfY = vecAxis.y * sinThetaOver2;
	m_dfZ = vecAxis.z * sinThetaOver2;
}

template <typename T>
void Quaternion<T>::ToAngleAxis(T& fAngle, Vector3<T>& vecAxis) const
{
    T fSqrLength = m_dfX*m_dfX + m_dfY*m_dfY + m_dfZ*m_dfZ;
    if ( fSqrLength > 0.0 )
    {
        fAngle = 2.0*acos(m_dfW);
        T fInvLength = 1.0f / sqrt(fSqrLength);
        vecAxis.x = m_dfX*fInvLength;
        vecAxis.y = m_dfY*fInvLength;
        vecAxis.z = m_dfZ*fInvLength;
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
 
    m_dfW = cy * cp * cr + sy * sp * sr;
    m_dfX = cy * cp * sr + sy * sp * cr;
    m_dfY = cy * sp * cr - sy * cp * sr;
    m_dfZ = sy * cp * cr - cy * sp * sr;
}

template <typename T>
void Quaternion<T>::FromToRotation(const Vector3<T>& from, const Vector3<T>& to)
{
    //不标准化，认为外面已经标准化
    Vector3 p0 = (from);
    Vector3 p1 = (to);
 
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
        m_dfX = axis.x;
        m_dfY = axis.y;
        m_dfZ = axis.z;
        m_dfW = 0;
    }
 
    Vector3<T> half = (p0 + p1).Normalize();
    Vector3<T> axis = Vector3<T>::CrossProduct(p0, half);
 
    m_dfX = axis.x;
    m_dfY = axis.y;
    m_dfZ = axis.z;
    m_dfW = p0.DotProduct(half);
}

template <typename T>
T Quaternion<T>::Norm() const
{
	return m_dfX*m_dfX + m_dfY*m_dfY + m_dfZ*m_dfZ;
}

template <typename T>
Quaternion<T> Quaternion<T>::Normalized() const
{
    float lenSq = m_dfW*m_dfW + m_dfX*m_dfX + m_dfY*m_dfY + m_dfZ*m_dfZ;
    if (lenSq < QUAT_EPSILON)
    {
        return Quaternion<T>();
    }
    float i_len = 1.0f / sqrtf(lenSq);

    return Quaternion<T>(
        m_dfW * i_len,
        m_dfX * i_len,
        m_dfY * i_len,
        m_dfZ * i_len
    );
}

template <typename T>
void Quaternion<T>::Normalize()
{
    T mag = sqrt(m_dfW*m_dfW + m_dfX*m_dfX + m_dfY*m_dfY + m_dfZ*m_dfZ);

    if (mag > 0.0)
    {
        T    oneOverMag = 1.0 / mag;
        m_dfW *= oneOverMag;
        m_dfX *= oneOverMag;
        m_dfY *= oneOverMag;
        m_dfZ *= oneOverMag;

    }
    else
    {
        assert(false);
    }
}

template <typename T>
Quaternion<T> Quaternion<T>::operator-() const
{
    return Quaternion(
        -m_dfW, -m_dfX, -m_dfY, -m_dfZ
    );
}

template <typename T>
Quaternion<T> Quaternion<T>::operator +(const Quaternion &other) const
{
    return Quaternion(
            m_dfW + other.m_dfW,
                      m_dfX + other.m_dfX,
                      m_dfY + other.m_dfY,
                      m_dfZ + other.m_dfZ
        );
}

template <typename T>
Quaternion<T> Quaternion<T>::operator *(const Quaternion<T> &a) const
{
	Quaternion<T> result;

	result.m_dfW = m_dfW*a.m_dfW - m_dfX*a.m_dfX - m_dfY*a.m_dfY - m_dfZ*a.m_dfZ;
	result.m_dfX = m_dfW*a.m_dfX + m_dfX*a.m_dfW + m_dfZ*a.m_dfY - m_dfY*a.m_dfZ;
	result.m_dfY = m_dfW*a.m_dfY + m_dfY*a.m_dfW + m_dfX*a.m_dfZ - m_dfZ*a.m_dfX;
	result.m_dfZ = m_dfW*a.m_dfZ + m_dfZ*a.m_dfW + m_dfY*a.m_dfX - m_dfX*a.m_dfY;

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
            m_dfW * scale,
                      m_dfX * scale,
                      m_dfY * scale,
                      m_dfZ * scale
        );
}

template <typename T>
bool Quaternion<T>::operator==(const Quaternion& right) const
{
    return (fabs(m_dfW - right.m_dfW) <= QUAT_EPSILON &&
            fabs(m_dfX - right.m_dfX) <= QUAT_EPSILON &&
            fabs(m_dfY - right.m_dfY) <= QUAT_EPSILON &&
            fabs(m_dfZ - right.m_dfZ) <= QUAT_EPSILON);
}

template <typename T>
T Quaternion<T>::DotProduct(const Quaternion<T> &b) const
{
	return m_dfW*b.m_dfW + m_dfX*b.m_dfX + m_dfY*b.m_dfY + m_dfZ*b.m_dfZ;
}

template <typename T>
Quaternion<T> Quaternion<T>::Inverse(const Quaternion<T>& rotation)
{
    Quaternion<T> result;
    T n = rotation.m_dfX * rotation.m_dfX + rotation.m_dfY * rotation.m_dfY +
        rotation.m_dfZ * rotation.m_dfZ + rotation.m_dfW * rotation.m_dfW;
    if (n == 1.0f)
    {
        result.m_dfX = -rotation.m_dfX;
        result.m_dfY = -rotation.m_dfY;
        result.m_dfZ = -rotation.m_dfZ;
        
        return result;
    }
    
    n = 1.0 / n;
    result.m_dfX = -rotation.m_dfX * n;
    result.m_dfY = -rotation.m_dfY * n;
    result.m_dfZ = -rotation.m_dfZ * n;
    result.m_dfW = rotation.m_dfW * n;
    
    return result;
}

template <typename T>
Quaternion<T> Quaternion<T>::Mix(const Quaternion<T>& from, const Quaternion<T>& to, float t)
{
    return from * (1.0f - t) + to * t;
}

template <typename T>
Quaternion<T> Quaternion<T>::Slerp(const Quaternion<T> &p, const Quaternion<T> &q, T t)
{
	if (t <= 0.0) return p;
	if (t >= 1.0) return q;

	double cosOmega = p.DotProduct(q);

	double q1w = q.m_dfW;
	double q1x = q.m_dfX;
	double q1y = q.m_dfY;
	double q1z = q.m_dfZ;
	if (cosOmega < 0.0f) 
	{
		q1w = -q1w;
		q1x = -q1x;
		q1y = -q1y;
		q1z = -q1z;
		cosOmega = -cosOmega;
	}

	assert(cosOmega < 1.1f);

	double k0, k1;
	if (cosOmega > 0.999999f) 
	{
		k0 = 1.0f-t;
		k1 = t;

	} 
	else 
	{
		double sinOmega = sqrt(1.0f - cosOmega*cosOmega);
		double omega = atan2(sinOmega, cosOmega);
		double oneOverSinOmega = 1.0f / sinOmega;

		k0 = sin((1.0f - t) * omega) * oneOverSinOmega;
		k1 = sin(t * omega) * oneOverSinOmega;
	}

	Quaternion<T> result;
	result.m_dfX = k0*p.m_dfX + k1*q1x;
	result.m_dfY = k0*p.m_dfY + k1*q1y;
	result.m_dfZ = k0*p.m_dfZ + k1*q1z;
	result.m_dfW = k0*p.m_dfW + k1*q1w;

	return result;
}

template <typename T>
Quaternion<T> Quaternion<T>::Conjugate(const Quaternion<T> &q)
{
	Quaternion<T> result;

	result.m_dfW = q.m_dfW;

	result.m_dfX = -q.m_dfX;
	result.m_dfY = -q.m_dfY;
	result.m_dfZ = -q.m_dfZ;

	return result;
}

template <typename T>
Quaternion<T> Quaternion<T>::Pow(const Quaternion<T> &q, T fExponent)
{
	if (fabs(q.m_dfW) > 0.9999)
	{
		return q;
	}

	double	alpha = acos(q.m_dfW);
	double	newAlpha = alpha * fExponent;

	Quaternion result;
	result.m_dfW = cos(newAlpha);

	double	mult = sin(newAlpha) / sin(alpha);
	result.m_dfX = q.m_dfX * mult;
	result.m_dfY = q.m_dfY * mult;
	result.m_dfZ = q.m_dfZ * mult;

	return result;
}

template <typename T>
Quaternion<T> Quaternion<T>::LookRotation(const Vector3<T>& direcion, const Vector3<T>& up)
{
    // Find orthonormal basis vectors
    Vector3<T> f = direcion.Normalize();
    Vector3<T> u = up.Normalize();
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
    // Donít forget to normalize the result
    result.Normalize();
    
    return result;
}

template class Quaternion<float>;
template class Quaternion<double>;

NS_MATHUTIL_END
