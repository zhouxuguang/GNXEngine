
#include "Matrix4x4.h"
#include "Vector4.h"
#include "Vector3.h"

NS_MATHUTIL_BEGIN

template <typename T>
const size_t Matrix4x4<T>::MATRIX4_SIZE = sizeof(T) * 16;

template <typename T>
const Matrix4x4<T> Matrix4x4<T>::IDENTITY = Matrix4x4<T>(
                                 1.0, 0.0, 0.0, 0.0,
                                 0.0, 1.0, 0.0, 0.0,
                                 0.0, 0.0, 1.0, 0.0,
                                 0.0, 0.0, 0.0, 1.0);

template <typename T>
const Matrix4x4<T> Matrix4x4<T>::ZERO = Matrix4x4<T>(
                             0, 0, 0, 0,
                             0, 0, 0, 0,
                             0, 0, 0, 0,
                             0, 0, 0, 0 );

template <typename T>
Matrix4x4<T>::Matrix4x4(void)
{
    memcpy(m_adfValues, Matrix4x4::IDENTITY.m_adfValues, 16 * sizeof(T));
}

template <typename T>
Matrix4x4<T>::~Matrix4x4(void)
{
	memset(m_adfValues, 0, MATRIX4_SIZE);
}

template <typename T>
Matrix4x4<T>::Matrix4x4(T *pfMatValues)
{
	memcpy(m_adfValues, pfMatValues, MATRIX4_SIZE);
}

template <typename T>
Matrix4x4<T>::Matrix4x4(const Matrix4x4<T>& rtMat)
{
	for (int i = 0; i < 16; i ++)
	{
		m_adfValues[i] = rtMat.m_adfValues[i];
	}
}

template <typename T>
Matrix4x4<T>::Matrix4x4(T m00, T m01, T m02, T m03, T m10, T m11, T m12, T m13, T m20, T m21, T m22, T m23, T m30, T m31, T m32, T m33)
{
	m_pValue[0][0] = m00;
	m_pValue[0][1] = m01;
	m_pValue[0][2] = m02;
	m_pValue[0][3] = m03;
	m_pValue[1][0] = m10;
	m_pValue[1][1] = m11;
	m_pValue[1][2] = m12;
	m_pValue[1][3] = m13;
	m_pValue[2][0] = m20;
	m_pValue[2][1] = m21;
	m_pValue[2][2] = m22;
	m_pValue[2][3] = m23;
	m_pValue[3][0] = m30;
	m_pValue[3][1] = m31;
	m_pValue[3][2] = m32;
	m_pValue[3][3] = m33;
}

template <typename T>
Matrix4x4<T>& Matrix4x4<T>::operator =(const Matrix4x4<T> &rhs)
{
	for (int i = 0; i < 16; i ++)
	{
		m_adfValues[i] = rhs[i/4][i%4];
	}
	return *this;
}

template <typename T>
Matrix4x4<T> Matrix4x4<T>::operator +(T fValue)
{
	Matrix4x4<T> mtRes;
	int i = 0;
	for (i = 0; i < 4; i ++)
	{
		for (int j = 0; j < 4; j++)
		{
			mtRes[i][j] = m_pValue[i][j] + fValue;
		}
	}

	return mtRes;
}

template <typename T>
Matrix4x4<T> Matrix4x4<T>::operator -(T fValue)
{
	Matrix4x4 mtRes;
	int i = 0;
	for (i = 0; i < 4; i ++)
	{
		for (int j = 0; j < 4; j++)
		{
			mtRes[i][j] = m_pValue[i][j] - fValue;
		}
	}

	return mtRes;
}

template <typename T>
Matrix4x4<T> Matrix4x4<T>::operator *(T fValue)
{
	Matrix4x4<T> mtRes;
	int i = 0;
	for (i = 0; i < 4; i ++)
	{
		for (int j = 0; j < 4; j++)
		{
			mtRes[i][j] = m_pValue[i][j] * fValue;
		}
	}

	return mtRes;
}

template <typename T>
Vector3<T> Matrix4x4<T>::operator * ( const Vector3<T> &v ) const
{
	Vector3<T> r;

	T fInvW = 1.0f / ( m_pValue[3][0] * v.x + m_pValue[3][1] * v.y + m_pValue[3][2] * v.z + m_pValue[3][3] );

	r.x = ( m_pValue[0][0] * v.x + m_pValue[0][1] * v.y + m_pValue[0][2] * v.z + m_pValue[0][3] ) * fInvW;
	r.y = ( m_pValue[1][0] * v.x + m_pValue[1][1] * v.y + m_pValue[1][2] * v.z + m_pValue[1][3] ) * fInvW;
	r.z = ( m_pValue[2][0] * v.x + m_pValue[2][1] * v.y + m_pValue[2][2] * v.z + m_pValue[2][3] ) * fInvW;

	return r;
}

template <typename T>
Vector4<T> Matrix4x4<T>::operator*(const Vector4<T>& v)
{
	return Vector4<T>(
		m_pValue[0][0] * v[0] + m_pValue[0][1] * v[1] + m_pValue[0][2] * v[2] + m_pValue[0][3] * v[3], 
		m_pValue[1][0] * v[0] + m_pValue[1][1] * v[1] + m_pValue[1][2] * v[2] + m_pValue[1][3] * v[3],
		m_pValue[2][0] * v[0] + m_pValue[2][1] * v[1] + m_pValue[2][2] * v[2] + m_pValue[2][3] * v[3],
		m_pValue[3][0] * v[0] + m_pValue[3][1] * v[1] + m_pValue[3][2] * v[2] + m_pValue[3][3] * v[3]
		);
}

template <typename T>
Matrix4x4<T> Matrix4x4<T>::operator*(const Matrix4x4<T>& other)
{
	T adfResult[16] = {0};

	for (int i = 0; i < 4; i ++)
	{
		for (int j = 0; j < 4; j ++)
		{
			for (int k = 0; k < 4; k ++)
			{
				adfResult[i*4+j] += m_pValue[i][k]*other[k][j];
			}
		}
	}

	return Matrix4x4<T>(adfResult);
}

template <typename T>
Matrix4x4<T> Matrix4x4<T>::operator*(const Matrix4x4<T>& other) const
{
    T adfResult[16] = {0};

    for (int i = 0; i < 4; i ++)
    {
        for (int j = 0; j < 4; j ++)
        {
            for (int k = 0; k < 4; k ++)
            {
                adfResult[i*4+j] += m_pValue[i][k]*other[k][j];
            }
        }
    }

    return Matrix4x4<T>(adfResult);
}

template <typename T>
Matrix4x4<T>& Matrix4x4<T>::operator *=(const Matrix4x4<T>& other)
{
	Real adfResult[16] = {0};

	for (int i = 0; i < 4; i ++)
	{
		for (int j = 0; j < 4; j ++)
		{
			for (int k = 0; k < 4; k ++)
			{
				adfResult[i*4+j] += m_pValue[i][k]*other[k][j];
			}
		}
	}

	memcpy((void*)m_adfValues, adfResult, MATRIX4_SIZE);

	return *this;
}

template <typename T>
Matrix4x4<T> Matrix4x4<T>::operator + (const Matrix4x4<T>& other) const
{
    Matrix4x4<T> mtRes;
    for (int i = 0; i < 4; i ++)
    {
        for (int j = 0; j < 4; j++)
        {
            mtRes[i][j] = m_pValue[i][j] + other.m_pValue[i][j];
        }
    }

    return mtRes;
}

template <typename T>
Matrix4x4<T> Matrix4x4<T>::operator /(T fValue)
{
	Matrix4x4<T> mtRes;
	for (int i = 0; i < 4; i ++)
	{
		for (int j = 0; j < 4; j++)
		{
			mtRes[i][j] = m_pValue[i][j] / fValue;
		}
	}

	return mtRes;
}

template <typename T>
void Matrix4x4<T>::MakeIdentity()
{
    memcpy(m_adfValues, &Matrix4x4::IDENTITY, MATRIX4_SIZE);
}

template <typename T>
bool Matrix4x4<T>::IsIdentity() const
{
	return m_pValue[0][0]==1.0 && m_pValue[0][1]==0.0 && m_pValue[0][2]==0.0 &&  m_pValue[0][3]==0.0 &&
		m_pValue[1][0]==0.0 && m_pValue[1][1]==1.0 && m_pValue[1][2]==0.0 &&  m_pValue[1][3]==0.0 &&
		m_pValue[2][0]==0.0 && m_pValue[2][1]==0.0 && m_pValue[2][2]==1.0 &&  m_pValue[2][3]==0.0 &&
		m_pValue[3][0]==0.0 && m_pValue[3][1]==0.0 && m_pValue[3][2]==0.0 &&  m_pValue[3][3]==1.0;
}

template <typename T>
inline static T
MINOR(const Matrix4x4<T>& m, const size_t r0, const size_t r1, const size_t r2,
	  const size_t c0, const size_t c1, const size_t c2)
{
	return m[r0][c0] * (m[r1][c1] * m[r2][c2] - m[r2][c1] * m[r1][c2]) -
		m[r0][c1] * (m[r1][c0] * m[r2][c2] - m[r2][c0] * m[r1][c2]) +
		m[r0][c2] * (m[r1][c0] * m[r2][c1] - m[r2][c0] * m[r1][c1]);
}

template <typename T>
T Matrix4x4<T>::Determinant() const
{
	return m_pValue[0][0] * MINOR(*this, 1, 2, 3, 1, 2, 3) -
			m_pValue[0][1] * MINOR(*this, 1, 2, 3, 0, 2, 3) +
			m_pValue[0][2] * MINOR(*this, 1, 2, 3, 0, 1, 3) -
			m_pValue[0][3] * MINOR(*this, 1, 2, 3, 0, 1, 2);
}

template <typename T>
Matrix4x4<T> Matrix4x4<T>::Inverse() const
{
	T m00 = m_pValue[0][0], m01 = m_pValue[0][1], m02 = m_pValue[0][2], m03 = m_pValue[0][3];
	T m10 = m_pValue[1][0], m11 = m_pValue[1][1], m12 = m_pValue[1][2], m13 = m_pValue[1][3];
	T m20 = m_pValue[2][0], m21 = m_pValue[2][1], m22 = m_pValue[2][2], m23 = m_pValue[2][3];
	T m30 = m_pValue[3][0], m31 = m_pValue[3][1], m32 = m_pValue[3][2], m33 = m_pValue[3][3];

	T v0 = m20 * m31 - m21 * m30;
	T v1 = m20 * m32 - m22 * m30;
	T v2 = m20 * m33 - m23 * m30;
	T v3 = m21 * m32 - m22 * m31;
	T v4 = m21 * m33 - m23 * m31;
	T v5 = m22 * m33 - m23 * m32;

	T t00 = + (v5 * m11 - v4 * m12 + v3 * m13);
	T t10 = - (v5 * m10 - v2 * m12 + v1 * m13);
	T t20 = + (v4 * m10 - v2 * m11 + v0 * m13);
	T t30 = - (v3 * m10 - v1 * m11 + v0 * m12);

	T invDet = 1 / (t00 * m00 + t10 * m01 + t20 * m02 + t30 * m03);

	T d00 = t00 * invDet;
	T d10 = t10 * invDet;
	T d20 = t20 * invDet;
	T d30 = t30 * invDet;

	T d01 = - (v5 * m01 - v4 * m02 + v3 * m03) * invDet;
	T d11 = + (v5 * m00 - v2 * m02 + v1 * m03) * invDet;
	T d21 = - (v4 * m00 - v2 * m01 + v0 * m03) * invDet;
	T d31 = + (v3 * m00 - v1 * m01 + v0 * m02) * invDet;

	v0 = m10 * m31 - m11 * m30;
	v1 = m10 * m32 - m12 * m30;
	v2 = m10 * m33 - m13 * m30;
	v3 = m11 * m32 - m12 * m31;
	v4 = m11 * m33 - m13 * m31;
	v5 = m12 * m33 - m13 * m32;

	T d02 = + (v5 * m01 - v4 * m02 + v3 * m03) * invDet;
	T d12 = - (v5 * m00 - v2 * m02 + v1 * m03) * invDet;
	T d22 = + (v4 * m00 - v2 * m01 + v0 * m03) * invDet;
	T d32 = - (v3 * m00 - v1 * m01 + v0 * m02) * invDet;

	v0 = m21 * m10 - m20 * m11;
	v1 = m22 * m10 - m20 * m12;
	v2 = m23 * m10 - m20 * m13;
	v3 = m22 * m11 - m21 * m12;
	v4 = m23 * m11 - m21 * m13;
	v5 = m23 * m12 - m22 * m13;

	T d03 = - (v5 * m01 - v4 * m02 + v3 * m03) * invDet;
	T d13 = + (v5 * m00 - v2 * m02 + v1 * m03) * invDet;
	T d23 = - (v4 * m00 - v2 * m01 + v0 * m03) * invDet;
	T d33 = + (v3 * m00 - v1 * m01 + v0 * m02) * invDet;

	return Matrix4x4<T>(
		d00, d01, d02, d03,
		d10, d11, d12, d13,
		d20, d21, d22, d23,
		d30, d31, d32, d33);
}

template <typename T>
Matrix4x4<T> Matrix4x4<T>::Transpose() const
{
	return Matrix4x4<T>(m_pValue[0][0], m_pValue[1][0], m_pValue[2][0], m_pValue[3][0],
		m_pValue[0][1], m_pValue[1][1], m_pValue[2][1], m_pValue[3][1],
		m_pValue[0][2], m_pValue[1][2], m_pValue[2][2], m_pValue[3][2],
		m_pValue[0][3], m_pValue[1][3], m_pValue[2][3], m_pValue[3][3]);
}

template <typename T>
Matrix3x3<T> Matrix4x4<T>::GetMatrix3() const
{
    Matrix3x3<T> matResult;
    for (int i = 0; i < 3; ++ i)
    {
        memcpy(matResult[i], m_pValue[i], sizeof(T)*3);
    }
    
    return matResult;
}

template <typename T>
Matrix4x4<T> Matrix4x4<T>::CreatePerspective(T fieldOfView, T aspectRatio, T zNearPlane, T zFarPlane)
{
    T dbFov = fieldOfView * DEGTORAD;
    T dbTan = tan(dbFov*0.5);
    return CreateFrustum(-zNearPlane*dbTan*aspectRatio,
                     zNearPlane*dbTan*aspectRatio,
                     -zNearPlane*dbTan,
                     zNearPlane*dbTan,
                     zNearPlane,zFarPlane);
}

template <typename T>
Matrix4x4<T> Matrix4x4<T>::CreateFrustum(T left, T right, T bottom, T top, T zNearPlane, T zFarPlane)
{
    Matrix4x4<T> dst;
    memset(&dst, 0, MATRIX4_SIZE);
    
    dst.m_adfValues[0] = 2*zNearPlane/(right-left);
    dst.m_adfValues[2] = (right+left)/(right-left);
    
    dst.m_adfValues[5] = 2*zNearPlane/(top-bottom);
    dst.m_adfValues[6] = (top+bottom)/(top-bottom);
    
    dst.m_adfValues[10] = -(zFarPlane+zNearPlane)/(zFarPlane-zNearPlane);
    dst.m_adfValues[11] = -2*zNearPlane*zFarPlane/(zFarPlane-zNearPlane);
    
    dst.m_adfValues[14] = -1;
    
    return dst;
}

template <typename T>
Matrix4x4<T> Matrix4x4<T>::CreateOrthographic(T left, T right, T bottom, T top, T zNearPlane, T zFarPlane)
{
    Matrix4x4<T> dst;
    T *matProj = &dst[0][0];
    memset(matProj, 0, MATRIX4_SIZE);
    
    matProj[0] = 2/(right-left);
    matProj[3] = -(right+left)/(right-left);
    
    matProj[5] = 2/(top-bottom);
    matProj[7] = -(top+bottom)/(top-bottom);
    
    matProj[10] = -2/(zFarPlane-zNearPlane);
    matProj[11] = -(zFarPlane+zNearPlane)/(zFarPlane-zNearPlane);
    
    matProj[15] = 1;
    
    return dst;
}

template <typename T>
Matrix4x4<T> Matrix4x4<T>::CreateScale(const Vector3<T>& scale)
{
    Matrix4x4<T> dst;
    memcpy(&dst, &Matrix4x4::IDENTITY, MATRIX4_SIZE);
    
    dst.m_adfValues[0] = scale.x;
    dst.m_adfValues[5] = scale.y;
    dst.m_adfValues[10] = scale.z;
    
    return dst;
}

template <typename T>
Matrix4x4<T> Matrix4x4<T>::CreateScale(T xScale, T yScale, T zScale)
{
    Matrix4x4<T> dst;
    memcpy(&dst, &Matrix4x4<T>::IDENTITY, MATRIX4_SIZE);
    
    dst.m_adfValues[0] = xScale;
    dst.m_adfValues[5] = yScale;
    dst.m_adfValues[10] = zScale;
    
    return dst;
}

template <typename T>
Matrix4x4<T> Matrix4x4<T>::CreateRotation(const Vector3<T>& axis, T angle)
{
	return Matrix4x4<T>::CreateRotation(axis.x, axis.y, axis.z, angle);
}

template <typename T>
Matrix4x4<T> Matrix4x4<T>::CreateRotationX(T angle)
{
    Matrix4x4<T> dst;
	T* values = dst.m_adfValues;
    memset(values, 0, MATRIX4_SIZE);

	values[0] = 1;
	values[15] = 1;

	T theta = angle * DEGTORAD;
	T sinTheta = sin(theta);
	T cosTheta = cos(theta);
	values[5] = cosTheta;
	values[6] = -sinTheta;
	values[9] = sinTheta;
	values[10] = cosTheta;
    
    return dst;
}

template <typename T>
Matrix4x4<T> Matrix4x4<T>::CreateRotationY(T angle)
{
    Matrix4x4<T> dst;
	T* values = dst.m_adfValues;
	memset(values, 0, MATRIX4_SIZE);

	values[5] = 1;
	values[15] = 1;

	T theta = angle * DEGTORAD;
	T sinTheta = sin(theta);
	T cosTheta = cos(theta);
	values[0] = cosTheta;
	values[2] = sinTheta;
	values[8] = -sinTheta;
	values[10] = cosTheta;
    
    return dst;
}

template <typename T>
Matrix4x4<T> Matrix4x4<T>::CreateRotationZ(T angle)
{
    Matrix4x4<T> dst;
	T* values = dst.m_adfValues;
	memset(values, 0, MATRIX4_SIZE);

	values[10] = 1;
	values[15] = 1;

	T theta = angle * DEGTORAD;
	T sinTheta = sin(theta);
	T cosTheta = cos(theta);
	values[0] = cosTheta;
	values[2] = -sinTheta;
	values[4] = sinTheta;
	values[5] = cosTheta;
    
    return dst;
}

template <typename T>
Matrix4x4<T> Matrix4x4<T>::CreateRotation(T x, T y, T z, T angle)
{
    Matrix4x4<T> dst;
	memset(dst.m_adfValues, 0, MATRIX4_SIZE);
    dst.m_adfValues[15] = 1;
    
    //旋转轴单位化
    double dbLen = sqrt(x*x+y*y+z*z);
    x /= dbLen;
    y /= dbLen;
    z /= dbLen;
    
    angle = angle*DEGTORAD;
    
    double db1costheta = 1.0-cos(angle);
    double dbSinTheta = sin(angle);
    double dbCosTheta = cos(angle);
    
    //逆时针为正
    dst.m_adfValues[0] = x*x*db1costheta + dbCosTheta;
    dst.m_adfValues[1] = x*y*db1costheta - z*dbSinTheta;
    dst.m_adfValues[2] = x*z*db1costheta + y*dbSinTheta;
    
    dst.m_adfValues[4] = x*y*db1costheta + z*dbSinTheta;
    dst.m_adfValues[5] = y*y*db1costheta + dbCosTheta;
    dst.m_adfValues[6] = y*z*db1costheta - x*dbSinTheta;
    
    dst.m_adfValues[8] = x*z*db1costheta - y*dbSinTheta;
    dst.m_adfValues[9] = y*z*db1costheta + x*dbSinTheta;
    dst.m_adfValues[10] = z*z*db1costheta + dbCosTheta;
    
    return dst;
}

template <typename T>
Matrix4x4<T> Matrix4x4<T>::CreateTranslate(const Vector3<T> &dist)
{
    return CreateTranslate(dist.x, dist.y, dist.z);
}

template <typename T>
Matrix4x4<T> Matrix4x4<T>::CreateTranslate(T x, T y, T z)
{
    Matrix4x4<T> dst;
    memset(&dst, 0, MATRIX4_SIZE);
    
    dst.m_adfValues[0] = 1;
    dst.m_adfValues[5] = 1;
    dst.m_adfValues[10] = 1;
    dst.m_adfValues[15] = 1;
    
    dst.m_adfValues[3] = x;
    dst.m_adfValues[7] = y;
    dst.m_adfValues[11] = z;
    
    return dst;
}

template <typename T>
Matrix4x4<T> Matrix4x4<T>::CreateLookAt(T eyePositionX, T eyePositionY, T eyePositionZ,
							 T targetCenterX, T targetCenterY, T targetCenterZ,
							 T upX, T upY, T upZ)
{
    Matrix4x4<T> dst;
    
	//首先确定观察平面法向量N
	double dNx = -(targetCenterX - eyePositionX);
	double dNy = -(targetCenterY - eyePositionY);
	double dNz = -(targetCenterZ - eyePositionZ);

	//确定观察向上的向量UP
	double dVx = upX;
	double dVy = upY;
	double dVz = upZ;

	//将向量N归一化
	double dbLength = sqrt(dNx*dNx + dNy*dNy + dNz*dNz);
	dNx = dNx/dbLength;
	dNy = dNy/dbLength;
	dNz = dNz/dbLength;

	//计算U向量
	dbLength = sqrt(dVx*dVx + dVy*dVy + dVz*dVz);
	double dUx = (dVy*dNz - dVz*dNy)/dbLength;
	double dUy = (dVz*dNx - dVx*dNz)/dbLength;
	double dUz = (dVx*dNy - dVy*dNx)/dbLength;

	//计算最终归一化的V向量
	dVx = (dNy*dUz - dNz*dUy);
	dVy = (dNz*dUx - dNx*dUz);
	dVz = (dNx*dUy - dNy*dUx);

	T *dbViewMatrix = dst.m_adfValues;
	memset(dbViewMatrix,0,sizeof(T)*16);
	dbViewMatrix[0] = dUx;
	dbViewMatrix[1] = dUy;
	dbViewMatrix[2] = dUz;
	dbViewMatrix[4] = dVx;
	dbViewMatrix[5] = dVy;
	dbViewMatrix[6] = dVz;
	dbViewMatrix[8] = dNx;
	dbViewMatrix[9] = dNy;
	dbViewMatrix[10] = dNz;
	dbViewMatrix[15] = 1.0f;

	dbViewMatrix[3] = -eyePositionX*dUx - eyePositionY*dUy - eyePositionZ*dUz;
	dbViewMatrix[7] = -eyePositionX*dVx - eyePositionY*dVy - eyePositionZ*dVz;
	dbViewMatrix[11] = -eyePositionX*dNx - eyePositionY*dNy - eyePositionZ*dNz;
    
    return dst;
}

template <typename T>
Matrix4x4<T> Matrix4x4<T>::CreateLookAt(const Vector3<T>& eyePosition, const Vector3<T>& targetPosition, const Vector3<T>& up)
{
	return Matrix4x4::CreateLookAt(eyePosition.x,eyePosition.y,eyePosition.z,
				  targetPosition.x,targetPosition.y,targetPosition.z,up.x,up.y,up.z);
}

template class Matrix4x4<float>;
template class Matrix4x4<double>;


NS_MATHUTIL_END

