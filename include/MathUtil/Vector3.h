/*******************************************************************************
@ 版权所有(C) 周旭光 2014
@ 文件名称	: vector3.h
@ 当前版本	: 1.0.0.1
@ 作    者	: guang (zhouxuguang236@126.com)
@ 设计日期	: 2014年12月1日
@ 内容摘要	: 
@ 修改记录	: 
@ 日    期		版    本		修改人		修改摘要

********************************************************************************/
#ifndef __VECTOR3_H_CD2C4FC7_29F5_439D_9403_106DAA2DC4CA__
#define __VECTOR3_H_CD2C4FC7_29F5_439D_9403_106DAA2DC4CA__

#include "Math3DCommon.h"

NS_MATHUTIL_BEGIN

template <typename T>
class MATH3D_API Vector3 
{
public:

	mutable T x,y,z;

	Vector3()
	{
		x = 0;
		y = 0;
		z = 0;
	}

	Vector3(const Vector3 &a) : x(a.x), y(a.y), z(a.z) {}

	Vector3(T nx, T ny, T nz) : x(nx), y(ny), z(nz) {}

	Vector3(const Vector3& startVec, const Vector3& endVec)
	{
		x = endVec.x - startVec.x;
		y = endVec.y - startVec.y;
		z = endVec.z - startVec.z;
	}


	Vector3 &operator =(const Vector3 &a) 
	{
        if (this == &a)
        {
            return *this;
        }
		x = a.x; y = a.y; z = a.z;
		return *this;
	}

	bool operator ==(const Vector3 &a) const 
	{
		return x==a.x && y==a.y && z==a.z;
	}

	bool operator !=(const Vector3 &a) const 
	{
		return x!=a.x || y!=a.y || z!=a.z;
	}


	void Zero() { x = y = z = 0.0f; }

	Vector3 Multiply(const Vector3& a) const 
	{
		return Vector3(x*a.x, y*a.y, z*a.z);
	}

	Vector3 operator -() const 
	{ 
		return Vector3(-x,-y,-z); 
	}


	Vector3 operator +(const Vector3 &a) const 
	{
		return Vector3(x + a.x, y + a.y, z + a.z);
	}

	Vector3 operator -(const Vector3 &a) const 
	{
		return Vector3(x - a.x, y - a.y, z - a.z);
	}

	Vector3 operator *(T a) const
	{
		return Vector3(x*a, y*a, z*a);
	}

	Vector3 operator /(T a) const
	{
		T oneOverA = 1.0 / a;
		return Vector3(x*oneOverA, y*oneOverA, z*oneOverA);
	}

	Vector3 &operator +=(const Vector3 &a) 
	{
		x += a.x; y += a.y; z += a.z;
		return *this;
	}

	Vector3 &operator -=(const Vector3 &a) 
	{
		x -= a.x; y -= a.y; z -= a.z;
		return *this;
	}

	Vector3 &operator *=(T a)
	{
		x *= a; y *= a; z *= a;
		return *this;
	}

	Vector3 &operator /=(T a) 
	{
		Real oneOverA = 1.0f / a;
		x *= oneOverA; y *= oneOverA; z *= oneOverA;
		return *this;
	}
    
    inline const T& operator [](size_t nIndex) const
    {
        assert(nIndex < 4);
        return *(&x+nIndex);
    }
    
    inline T& operator [](size_t nIndex)
    {
        assert(nIndex < 4);
        return *(&x+nIndex);
    }

    const Vector3& Normalize() const;

	//求向量的模
	const T Length() const;

	T Length();

	//模的平方
	T LengthSq() const;

    //乘积，对应分量相乘
	Vector3 operator *(const Vector3 &a) const
	{
		return Vector3(x*a.x , y*a.y , z*a.z);
	}

	T DotProduct(const Vector3 &a) const;
    
    static Vector3 CrossProduct(const Vector3 &a, const Vector3 &b);

	//根据入射光线和法线计算反射向量，必须要注意的是，入射光线和法线必须是单位向量
	static Vector3 Reflection(const Vector3& vecLight, const Vector3& vecNormal);

	static Vector3 Refraction(const Vector3& vecLight, const Vector3& vecNormal, T eta);
    
    // 插值函数
    static Vector3 Lerp(const Vector3& s, const Vector3& e, float t);

    //常量向量定义
	static const Vector3 ZERO;
	static const Vector3 UNIT_SCALE;
    static const Vector3 UNIT_X;
    static const Vector3 UNIT_Y;
    static const Vector3 UNIT_Z;
};

template <typename T>
inline T vectorMag(const Vector3<T> &a)
{
	return sqrt(a.x*a.x + a.y*a.y + a.z*a.z);
}

template <typename T>
inline Vector3<T> operator *(T k, const Vector3<T> &v)
{
	return Vector3<T>(k*v.x, k*v.y, k*v.z);
}

template <typename T>
T Distance(const Vector3<T> &a, const Vector3<T> &b);

template <typename T>
T DistanceSquared(const Vector3<T> &a, const Vector3<T> &b);

template <typename T>
extern const Vector3<T> kZeroVector;

typedef Vector3<float> Vector3f;
typedef Vector3<double> Vector3d;

NS_MATHUTIL_END

#endif 
