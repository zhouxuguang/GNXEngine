#ifndef MATH3DLIB_MATRIX440_INCLUDE_04DDDDD
#define MATH3DLIB_MATRIX440_INCLUDE_04DDDDD

#include "Matrix3x3.h"
#include "Vector4.h"

NS_MATHUTIL_BEGIN

//4x4矩阵类

template <typename T>
class MATH3D_API Matrix4x4
{
public:
    //identity matix
    Matrix4x4(void);

    Matrix4x4(
        T m00, T m01, T m02, T m03,
        T m10, T m11, T m12, T m13,
        T m20, T m21, T m22, T m23,
        T m30, T m31, T m32, T m33 );

    explicit Matrix4x4(T *pfMatValues);

    Matrix4x4(const Matrix4x4& rtMat);

    ~Matrix4x4(void);

    Matrix4x4 &operator = (const Matrix4x4& rhs);

    Matrix4x4 operator +(T fValue);

    Matrix4x4 operator -(T fValue);

    Matrix4x4 operator *(T fValue);

    Matrix4x4 operator /(T fValue);

    inline const T* operator [](size_t nIndex) const
    {
        return m_pValue[nIndex];
    }

    inline T* operator [](size_t nIndex)
    {
        return m_pValue[nIndex];
    }
    
    //操作符重载
    Vector3<T> operator * ( const Vector3<T> &v ) const;
    Vector4<T> operator * (const Vector4<T>& other);

    Matrix4x4 operator * (const Matrix4x4& other);
    
    Matrix4x4 operator * (const Matrix4x4& other) const;

    Matrix4x4& operator *= (const Matrix4x4& other);
    
    Matrix4x4 operator + (const Matrix4x4& other) const;


    //创建单位矩阵
    void MakeIdentity();

    //是否单位矩阵
    bool IsIdentity() const;

    T *GetValues()
    {
        return m_adfValues;
    }

    const T* GetValues() const
    {
        return m_adfValues;
    }

    void SetValues(T* pfValues)
    {
        memcpy(m_adfValues, pfValues, sizeof(T)*16);
    }

    //行列式的值
    T Determinant() const;

    Matrix4x4 Inverse() const;

    //转置
    Matrix4x4 Transpose() const;
    
    //获得3新矩阵
    Matrix3x3<T> GetMatrix3() const;
    
    /**
     *  创建透视投影矩阵
     *
     *  @param fieldOfView 垂直视角
     *  @param aspectRatio 宽高比
     *  @param zNearPlane  近裁剪面
     *  @param zFarPlane   远裁剪面
     */
    static Matrix4x4 CreatePerspective(T fieldOfView, T aspectRatio, T zNearPlane, T zFarPlane);
    
    /**
     *  创建透视投影矩阵
     *
     *  @param left       左边边界
     *  @param right     右边边界
     *  @param bottom    下边界
     *  @param top           上边界
     *  @param zNearPlane  近裁剪面
     *  @param zFarPlane  远裁剪面
     */
    static Matrix4x4 CreateFrustum(T left, T right, T bottom, T top, T zNearPlane, T zFarPlane);
    
    
    /**
     *  创建正交投影矩阵
     *
     *  @param left       左边边界
     *  @param right     右边边界
     *  @param bottom    下边界
     *  @param top           上边界
     *  @param zNearPlane  近裁剪面
     *  @param zFarPlane  远裁剪面
     */
    static Matrix4x4 CreateOrthographic(T left, T right, T bottom, T top, T zNearPlane, T zFarPlane);
    
    /**
     *  创建缩放矩阵
     *
     *  @param scale       缩放比例
     */
    static Matrix4x4 CreateScale(const Vector3<T>& scale);
    
    /**
     *  创建缩放矩阵
     *
     *  @param xScale       缩放比例
     *  @param yScale       缩放比例
     *  @param zScale       缩放比例
     */
    static Matrix4x4 CreateScale(T xScale, T yScale, T zScale);
    
    //创建旋转矩阵
    static Matrix4x4 CreateRotation(const Vector3<T>& axis, T angle);
    
    static Matrix4x4 CreateRotationX(T angle);
    
    static Matrix4x4 CreateRotationY(T angle);
    
    static Matrix4x4 CreateRotationZ(T angle);
    
    static Matrix4x4 CreateRotation(T x, T y, T z, T angle);
    
    //创建平移矩阵
    static Matrix4x4 CreateTranslate(const Vector3<T>& dist);
    
    static Matrix4x4 CreateTranslate(T x, T y, T z);
    
    //创建观察矩阵
    static Matrix4x4 CreateLookAt(const Vector3<T>& eyePosition, const Vector3<T>& targetPosition, const Vector3<T>& up);
    
    static Matrix4x4 CreateLookAt(T eyePositionX, T eyePositionY, T eyePositionZ,
                             T targetCenterX, T targetCenterY, T targetCenterZ,
                             T upX, T upY, T upZ);

private:

    //矩阵元素
    union
    {
        T m_adfValues[16];
        T m_pValue[4][4];
    };
    
    static const size_t MATRIX4_SIZE;
    
    //特殊的矩阵
public:
    static const Matrix4x4 IDENTITY;
    static const Matrix4x4 ZERO;
    
};

typedef Matrix4x4<float> Matrix4x4f;
typedef Matrix4x4<double> Matrix4x4d;

NS_MATHUTIL_END


#endif

