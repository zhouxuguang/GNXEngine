#include "Matrix3x3.h"
#include "Vector3.h"

NS_MATHUTIL_BEGIN

template <typename T>
const Matrix3x3<T> Matrix3x3<T>::IDENTITY = Matrix3x3<T>(
                                                1.0, 0.0, 0.0,
                                                0.0, 1.0, 0.0,
                                                0.0, 0.0, 1.0);

template <typename T>
const Matrix3x3<T> Matrix3x3<T>::ZERO = Matrix3x3<T>(
                                            0, 0, 0,
                                            0, 0, 0,
                                            0, 0, 0 );

template <typename T>
Matrix3x3<T>::Matrix3x3(void)
{
	memset(m_adfValues,0,sizeof(T) * 9);
}

template <typename T>
Matrix3x3<T>::Matrix3x3(T a1,T a2,T a3,
					 T b1,T b2,T b3,
					 T c1,T c2,T c3)
{
	m_adfValues[0] = a1;
	m_adfValues[1] = a2;
	m_adfValues[2] = a3;
	m_adfValues[3] = b1;
	m_adfValues[4] = b2;
	m_adfValues[5] = b3;
	m_adfValues[6] = c1;
	m_adfValues[7] = c2;
	m_adfValues[8] = c3;
}

template <typename T>
Matrix3x3<T>::Matrix3x3(const Vector3<T>& vec1, const Vector3<T>& vec2, const Vector3<T>& vec3)
{
    memcpy(m_adfValues, &vec1, sizeof(T)*3);
    memcpy(m_adfValues+3, &vec2, sizeof(T)*3);
    memcpy(m_adfValues+6, &vec3, sizeof(T)*3);
}

template <typename T>
Matrix3x3<T>::Matrix3x3(const T arr[3][3])
{
	memcpy(m_adfValues,arr,sizeof(T)*9);
}

template <typename T>
Matrix3x3<T>::Matrix3x3(const Matrix3x3<T>& rkMatrix)
{
	memcpy(m_adfValues,rkMatrix.m_adfValues,sizeof(T)*9);
}

template <typename T>
Matrix3x3<T>::Matrix3x3(const T* pfArr)
{
	memcpy(m_adfValues,pfArr,sizeof(T)*9);
}

template <typename T>
Matrix3x3<T>::~Matrix3x3(void)
{
}

template <typename T>
Matrix3x3<T> Matrix3x3<T>::operator +(T fValue) const
{
	Matrix3x3<T> rtMat;
	rtMat[0][0] = m_Values[0][0] + fValue;
	rtMat[0][1] = m_Values[0][1] + fValue;
	rtMat[0][2] = m_Values[0][2] + fValue;

	rtMat[1][0] = m_Values[1][0] + fValue;
	rtMat[1][1] = m_Values[1][1] + fValue;
	rtMat[1][2] = m_Values[1][2] + fValue;

	rtMat[2][0] = m_Values[2][0] + fValue;
	rtMat[2][1] = m_Values[2][1] + fValue;
	rtMat[2][2] = m_Values[2][2] + fValue;

	return rtMat;
}

template <typename T>
Matrix3x3<T> Matrix3x3<T>::operator+(const Matrix3x3<T>& rhs) const
{
	Matrix3x3<T> rtMat;
	rtMat[0][0] = m_Values[0][0] + rhs[0][0];
	rtMat[0][1] = m_Values[0][1] + rhs[0][1];
	rtMat[0][2] = m_Values[0][2] + rhs[0][2];

	rtMat[1][0] = m_Values[1][0] + rhs[1][0];
	rtMat[1][1] = m_Values[1][1] + rhs[1][1];
	rtMat[1][2] = m_Values[1][2] + rhs[1][2];

	rtMat[2][0] = m_Values[2][0] + rhs[2][0];
	rtMat[2][1] = m_Values[2][1] + rhs[2][1];
	rtMat[2][2] = m_Values[2][2] + rhs[2][2];

	return rtMat;
}

template <typename T>
Matrix3x3<T> Matrix3x3<T>::operator -(T fValue) const
{
	Matrix3x3<T> rtMat;
	rtMat[0][0] = m_Values[0][0] - fValue;
	rtMat[0][1] = m_Values[0][1] - fValue;
	rtMat[0][2] = m_Values[0][2] - fValue;

	rtMat[1][0] = m_Values[1][0] - fValue;
	rtMat[1][1] = m_Values[1][1] - fValue;
	rtMat[1][2] = m_Values[1][2] - fValue;

	rtMat[2][0] = m_Values[2][0] - fValue;
	rtMat[2][1] = m_Values[2][1] - fValue;
	rtMat[2][2] = m_Values[2][2] - fValue;

	return rtMat;
}

template <typename T>
Matrix3x3<T> Matrix3x3<T>::operator *(T fValue) const
{
	Matrix3x3<T> rtMat;
	rtMat[0][0] = m_Values[0][0] * fValue;
	rtMat[0][1] = m_Values[0][1] * fValue;
	rtMat[0][2] = m_Values[0][2] * fValue;

	rtMat[1][0] = m_Values[1][0] * fValue;
	rtMat[1][1] = m_Values[1][1] * fValue;
	rtMat[1][2] = m_Values[1][2] * fValue;

	rtMat[2][0] = m_Values[2][0] * fValue;
	rtMat[2][1] = m_Values[2][1] * fValue;
	rtMat[2][2] = m_Values[2][2] * fValue;

	return rtMat;
}

template <typename T>
Matrix3x3<T> Matrix3x3<T>::operator /(T fValue) const
{
	Matrix3x3 rtMat;
	rtMat[0][0] = m_Values[0][0] / fValue;
	rtMat[0][1] = m_Values[0][1] / fValue;
	rtMat[0][2] = m_Values[0][2] / fValue;

	rtMat[1][0] = m_Values[1][0] / fValue;
	rtMat[1][1] = m_Values[1][1] / fValue;
	rtMat[1][2] = m_Values[1][2] / fValue;

	rtMat[2][0] = m_Values[2][0] / fValue;
	rtMat[2][1] = m_Values[2][1] / fValue;
	rtMat[2][2] = m_Values[2][2] / fValue;

	return rtMat;
}

template <typename T>
Vector3<T> Matrix3x3<T>::operator*(const Vector3<T> &v)
{
    return Vector3<T>(
                   m_Values[0][0] * v[0] + m_Values[0][1] * v[1] + m_Values[0][2] * v[2],
                   m_Values[1][0] * v[0] + m_Values[1][1] * v[1] + m_Values[1][2] * v[2],
                   m_Values[2][0] * v[0] + m_Values[2][1] * v[1] + m_Values[2][2] * v[2]
                   );
}

template <typename T>
Matrix3x3<T> Matrix3x3<T>::operator *(const Matrix3x3& rhs) const
{
	const T *pfOther = rhs.m_adfValues;
	T adfResult[9];
	memset(adfResult,0,sizeof(T)*9);

	for (int i = 0; i < 3; i ++)
	{
		for (int j = 0; j < 3; j ++)
		{
			for (int k = 0; k < 3; k ++)
			{
				adfResult[i*3+j] += m_adfValues[i*3+k]*pfOther[k*3+j];
			}
		}
	}

	return Matrix3x3(adfResult);
}

template <typename T>
void Matrix3x3<T>::Init(T a1,T a2,T a3,
					 T b1,T b2,T b3,
					 T c1,T c2,T c3)
{
	m_adfValues[0] = a1;
	m_adfValues[1] = a2;
	m_adfValues[2] = a3;
	m_adfValues[3] = b1;
	m_adfValues[4] = b2;
	m_adfValues[5] = b3;
	m_adfValues[6] = c1;
	m_adfValues[7] = c2;
	m_adfValues[8] = c3;
}

template <typename T>
Matrix3x3<T> Matrix3x3<T>::operator-() const
{
	Matrix3x3 rtMat;
	rtMat[0][0] = -m_Values[0][0];
	rtMat[0][1] = -m_Values[0][1];
	rtMat[0][2] = -m_Values[0][2];

	rtMat[1][0] = -m_Values[1][0];
	rtMat[1][1] = -m_Values[1][1];
	rtMat[1][2] = -m_Values[1][2];

	rtMat[2][0] = -m_Values[2][0];
	rtMat[2][1] = -m_Values[2][1];
	rtMat[2][2] = -m_Values[2][2];

	return rtMat;
}

template <typename T>
Matrix3x3<T> Matrix3x3<T>::operator-(const Matrix3x3<T>& rhs) const
{
	Matrix3x3 rtMat;
	rtMat[0][0] = m_Values[0][0] - rhs[0][0];
	rtMat[0][1] = m_Values[0][1] - rhs[0][1];
	rtMat[0][2] = m_Values[0][2] - rhs[0][2];

	rtMat[1][0] = m_Values[1][0] - rhs[1][0];
	rtMat[1][1] = m_Values[1][1] - rhs[1][1];
	rtMat[1][2] = m_Values[1][2] - rhs[1][2];

	rtMat[2][0] = m_Values[2][0] - rhs[2][0];
	rtMat[2][1] = m_Values[2][1] - rhs[2][1];
	rtMat[2][2] = m_Values[2][2] - rhs[2][2];

	return rtMat;
}

template <typename T>
T Matrix3x3<T>::Determinant() const
{
	return m_Values[0][0]*(m_Values[1][1]*m_Values[2][2] - m_Values[1][2]*m_Values[2][1]) +
		m_Values[0][1]*(m_Values[1][2]*m_Values[2][0] - m_Values[1][0]*m_Values[2][2]) + 
		m_Values[0][2]*(m_Values[1][0]*m_Values[2][1] - m_Values[1][1]*m_Values[2][0]);
}

template <typename T>
bool Matrix3x3<T>::Inverse(Matrix3x3& rkInverse, T fTolerance /*= 1e-06*/) const
{
	rkInverse[0][0] = m_Values[1][1]*m_Values[2][2] -
		m_Values[1][2]*m_Values[2][1];
	rkInverse[0][1] = m_Values[0][2]*m_Values[2][1] -
		m_Values[0][1]*m_Values[2][2];
	rkInverse[0][2] = m_Values[0][1]*m_Values[1][2] -
		m_Values[0][2]*m_Values[1][1];
	rkInverse[1][0] = m_Values[1][2]*m_Values[2][0] -
		m_Values[1][0]*m_Values[2][2];
	rkInverse[1][1] = m_Values[0][0]*m_Values[2][2] -
		m_Values[0][2]*m_Values[2][0];
	rkInverse[1][2] = m_Values[0][2]*m_Values[1][0] -
		m_Values[0][0]*m_Values[1][2];
	rkInverse[2][0] = m_Values[1][0]*m_Values[2][1] -
		m_Values[1][1]*m_Values[2][0];
	rkInverse[2][1] = m_Values[0][1]*m_Values[2][0] -
		m_Values[0][0]*m_Values[2][1];
	rkInverse[2][2] = m_Values[0][0]*m_Values[1][1] -
		m_Values[0][1]*m_Values[1][0];

	T fDet =
		m_Values[0][0]*rkInverse[0][0] +
		m_Values[0][1]*rkInverse[1][0]+
		m_Values[0][2]*rkInverse[2][0];

	if ( fabs(fDet) <= fTolerance )
		return false;

	T fInvDet = 1.0/fDet;
	for (size_t iRow = 0; iRow < 3; iRow++)
	{
		for (size_t iCol = 0; iCol < 3; iCol++)
			rkInverse[iRow][iCol] *= fInvDet;
	}

	return true;
}

template <typename T>
Matrix3x3<T> Matrix3x3<T>::Inverse(T fTolerance /*= 1e-06*/) const
{
	//”–±¿¿££ø
	Matrix3x3 kInverse;
	kInverse.MakeIdentity();
	Inverse(kInverse,fTolerance);
	return kInverse;
}

template <typename T>
void Matrix3x3<T>::MakeIdentity()
{
	memset(m_adfValues,0,sizeof(T)*9);
	m_adfValues[0] = 1;
	m_adfValues[4] = 1;
	m_adfValues[8] = 1;
}

template <typename T>
Matrix3x3<T> Matrix3x3<T>::Transpose() const
{
	return Matrix3x3(m_Values[0][0],m_Values[1][0],m_Values[2][0],
		m_Values[0][1],m_Values[1][1],m_Values[2][1],
		m_Values[0][2],m_Values[1][2],m_Values[2][2]);
}

template class Matrix3x3<float>;
template class Matrix3x3<double>;

NS_MATHUTIL_END
