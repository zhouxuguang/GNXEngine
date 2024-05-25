
#ifndef __MATRIX3X3_H_INCLUDED__JF456994566664
#define __MATRIX3X3_H_INCLUDED__JF456994566664

#include "Math3DCommon.h"
#include "Vector3.h"

NS_MATHUTIL_BEGIN

//3X3 matrix

template <typename T>
class MATH3D_API Matrix3x3
{
public:
	Matrix3x3(void);

	explicit Matrix3x3(const T arr[3][3]);

	explicit Matrix3x3(const T* pfArr);

	Matrix3x3(const Matrix3x3& rkMatrix);

	Matrix3x3(T a1,T a2,T a3,
		T b1,T b2,T b3,
		T c1,T c2,T c3);
    
    Matrix3x3(const Vector3<T>& vec1, const Vector3<T>& vec2, const Vector3<T>& vec3);

	~Matrix3x3(void);

	void MakeIdentity();

	//重载操作符

	Matrix3x3 operator +(T fValue) const;

	Matrix3x3 operator -(T fValue) const;

	Matrix3x3 operator *(T fValue) const;

	Matrix3x3 operator /(T fValue) const;

	Vector3<T> operator *(const Vector3<T>& vec);

	Matrix3x3 operator -() const;

	inline T* operator [](size_t nIndex)
	{
		return m_Values[nIndex];
	}

	inline const T* operator [](size_t nIndex) const
	{
		return m_Values[nIndex];
	}

	//重载两个矩阵之间的操作符
	Matrix3x3 operator +(const Matrix3x3& rhs) const;

	Matrix3x3 operator -(const Matrix3x3& rhs) const;

	Matrix3x3 operator *(const Matrix3x3& rhs) const;

	//初始化
	void Init(T a1,T a2,T a3,
		T b1,T b2,T b3,
		T c1,T c2,T c3);

	//计算行列式
	T Determinant() const;

	bool Inverse (Matrix3x3& rkInverse, T fTolerance = 1e-06) const;

	Matrix3x3 Inverse (T fTolerance = 1e-06) const;

	//矩阵转置
	Matrix3x3 Transpose() const;

private:
	union		//矩阵元素
	{
		T m_adfValues[9];
		T m_Values[3][3];
	};
public:
    static const Matrix3x3 IDENTITY;
    static const Matrix3x3 ZERO;
	
};

typedef Matrix3x3<float> Matrix3x3f;
typedef Matrix3x3<double> Matrix3x3d;

NS_MATHUTIL_END

#endif
