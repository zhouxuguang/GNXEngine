
#ifndef __MATH3D_QUATERNION_H_INCLUDED__
#define __MATH3D_QUATERNION_H_INCLUDED__

//四元数接口

#include "Math3DCommon.h"
#include "Vector3.h"
#include "Matrix3x3.h"

NS_MATHUTIL_BEGIN


template <typename T>
class MATH3D_API Quaternion 
{
public:
	T	m_dfW, m_dfX, m_dfY, m_dfZ;

	//构造函数
	Quaternion();

	Quaternion(T w, T x, T y, T z);

	Quaternion(const Quaternion& rhs);

	Quaternion & operator=(const Quaternion& rhs);
    
    ~Quaternion();

	//从旋转矩阵构建
	void FromRotateMatrix(const Matrix3x3<T>& mRot);

	//转换到旋转矩阵
	void ToRotateMatrix(Matrix3x3<T>& mRot) const;

	//从轴角对构建
	void FromAngleAxis(const T fAngle,const Vector3<T>& vecAxis);

	//转换到轴角对
	void ToAngleAxis(T& fAngle, Vector3<T>& vecAxis) const;
    
    //从欧拉角构建 yaw (Z), pitch (Y), roll (X)
    void FromEulerAngles(T yaw, T pitch, T roll);
    
    //从两个方向的单位向量创建四元数
    void FromToRotation(const Vector3<T>& from, const Vector3<T>& to);

	//长度
	T Norm() const;
    
    // 归一化
    Quaternion Normalized() const;
    
    void Normalize();
    
    Quaternion operator-() const;
    
    Quaternion operator +(const Quaternion &other) const;

	Quaternion operator *(const Quaternion &other) const;

	Quaternion &operator *=(const Quaternion &other);
    
    Vector3<T> operator* (const Vector3<T>& v) const;
    
    Quaternion operator* (float scale) const;
    
    bool operator==(const Quaternion& right) const;

    // 点积
	T DotProduct(const Quaternion &other) const;
    
    // 相反旋转的四元数
    static Quaternion Inverse(const Quaternion& rotation);
    
    //四元数混合
    static Quaternion Mix(const Quaternion& from, const Quaternion& to, float t);

	//球面线性插值
	static Quaternion Slerp(const Quaternion &p, const Quaternion &q, T t);

	//共轭
	static Quaternion Conjugate(const Quaternion &q);

	//求幂
	static Quaternion Pow(const Quaternion &q, T fExponent);
    
    //根据观察方向和向上方向确定旋转四元数
    static Quaternion LookRotation(const Vector3<T>& direcion, const Vector3<T>& up);
    
    //
    static const Quaternion IDENTITY;
};

typedef Quaternion<float> Quaternionf;
typedef Quaternion<double> Quaterniond;

NS_MATHUTIL_END
 

#endif // #ifndef __QUATERNION_H_INCLUDED__
