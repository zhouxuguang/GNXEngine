//
//  Transform.h
//  GNXEngine
//
//  Created by zhouxuguang on 2022/10/3.
//

#ifndef GNX_ENGINE_TRANSFORM_INCLUDE_HPP
#define GNX_ENGINE_TRANSFORM_INCLUDE_HPP

#include "RSDefine.h"
#include "MathUtil/Vector3.h"
#include "MathUtil/Quaternion.h"
#include "MathUtil/Matrix4x4.h"

//变换的类

NS_RENDERSYSTEM_BEGIN

USING_NS_MATHUTIL

class Transform
{
public:
    Transform();
    Transform(const Vector3f& p, const Quaternionf& r, const Vector3f& s);
    ~Transform();
    
    Transform Inverse();
    
    // 结合变换，parentTransform 父节点的变换
    void Combine(const Transform& parentTransform);
    
    // 和另外一个变换进行线性插值
    Transform Lerp(const Transform& other, float t) const;
    
    // 变换顶点坐标
    Vector3f TransformPoint(const Vector3f& point) const;
    
    // 将变换转换为矩阵
    Matrix4x4f TransformToMat4() const;
    
    // 从变换矩阵恢复变换信息
    void TransformFromMat4(const Matrix4x4f& m);
    
    bool operator==(const Transform& b) const;
    bool operator!=(const Transform& b) const;
    
    Vector3f position;       //平移
    Quaternionf rotation;    //旋转
    Vector3f scale;          //缩放
};

NS_RENDERSYSTEM_END

#endif /* GNX_ENGINE_TRANSFORM_INCLUDE_HPP */
