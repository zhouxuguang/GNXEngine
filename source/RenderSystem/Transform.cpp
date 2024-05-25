//
//  Transform.cpp
//  GNXEngine
//
//  Created by zhouxuguang on 2022/10/3.
//

#include "Transform.h"

NS_RENDERSYSTEM_BEGIN

//联合转换
// a = parent transform, b = child (or current) transform
static Transform CombineTransforms(const Transform& a, const Transform& b)
{
    Transform out;
 
    out.scale = a.scale * b.scale; // vec3 * vec3, parent scale times child scale
    out.rotation = b.rotation *  a.rotation; // quat * quat, Quaternions multiply in reverse, this is parent times child
 
    // parent scale times child position, rotated by parent rotation:
    out.position = a.rotation * (a.scale * b.position); // (vec3 * vec3) * quat, quaternions multiply in reverse
    out.position = a.position + out.position; // ve3 + vec3, combine positions
 
    return out;
}

//获得反向的转换
static Transform LocalInverse(Transform t)
{
    Quaternionf invRotation = Quaternionf::Inverse(t.rotation);
 
    Vector3f invScale = Vector3f(0, 0, 0);
    if (t.scale.x != 0)
    {
        // Do epsilon comparison here
        invScale.x = 1.0 / t.scale.x;
    }
    if (t.scale.y != 0)
    {
        // Do epsilon comparison here
        invScale.y = 1.0 / t.scale.y;
    }
    if (t.scale.z != 0)
    {
        // Do epsilon comparison here
        invScale.z = 1.0 / t.scale.z;
    }
 
    Vector3f invTranslation = invRotation * (invScale * (t.position * -1.0));

    Transform result;
    result.position = invTranslation;
    result.rotation = invRotation;
    result.scale = invScale;
 
    return result;
}

//转换点
static Vector3f transformPoint(const Transform& a, const Vector3f& b)
{
    Vector3f out;
    out = a.rotation * (a.scale * b);
    out = a.position + out;
    return out;
}

static Transform mix(const Transform& a, const Transform& b, float t)
{
    Quaternion bRot = b.rotation;
    
    if (bRot.DotProduct(a.rotation) < 0.0f)
    {
        bRot = Quaternionf::Inverse(bRot);
    }
    return Transform
    (
        Vector3f::Lerp(a.position, b.position, t),
        Quaternionf::Slerp(a.rotation, bRot, t),
        Vector3f::Lerp(a.scale, b.scale, t)
    );
}

static Matrix4x4f transformToMat4(const Transform& t)
{
    // First, extract the rotation basis of the transform
    Vector3 x = t.rotation * Vector3f(1, 0, 0);
    Vector3 y = t.rotation * Vector3f(0, 1, 0);
    Vector3 z = t.rotation * Vector3f(0, 0, 1);

    // Next, scale the basis vectors
    x = x * t.scale.x;
    y = y * t.scale.y;
    z = z * t.scale.z;

    // Extract the position of the transform
    Vector3 p = t.position;

    // Create matrix
    return Matrix4x4f(
        x.x, y.x, z.x, p.x, // X basis (& Scale)
        x.y, y.y, z.y, p.y, // Y basis (& scale)
        x.z, y.z, z.z, p.z, // Z basis (& scale)
        0, 0, 0, 1  // Position
    );
}

Transform mat4ToTransform(const Matrix4x4f& _this)
{
    Transform out;

    /* 提取平移部分 */
    out.position.x = _this[0][3];
    out.position.y = _this[1][3];
    out.position.z = _this[2][3];
    
    /* 提取出矩阵的3列. */
    Vector3f vCols[3] = 
    {
        Vector3f(_this[0][0],_this[1][0],_this[2][0]),
        Vector3f(_this[0][1],_this[1][1],_this[2][1]),
        Vector3f(_this[0][2],_this[1][2],_this[2][2])
    };
    
    /* 提取缩放因子 */
    out.scale.x = vCols[0].Length();
    out.scale.y = vCols[1].Length();
    out.scale.z = vCols[2].Length();
    
    /* 根据行列式是否为负，缩放进行反号 */
    if (_this.Determinant() < 0) out.scale = -out.scale;
    
    /* 将缩放从矩阵中移除 */
    if(out.scale.x) vCols[0] /= out.scale.x;
    if(out.scale.y) vCols[1] /= out.scale.y;
    if(out.scale.z) vCols[2] /= out.scale.z;
    
    // 构建 3x3 旋转矩阵
    Matrix3x3f m(vCols[0].x, vCols[1].x, vCols[2].x,
                 vCols[0].y, vCols[1].y, vCols[2].y,
                 vCols[0].z, vCols[1].z, vCols[2].z);
    
    // 从3x3矩阵构建四元数
    out.rotation.FromRotateMatrix(m);

    return out;
}

Transform::Transform()
{
    position = Vector3f(0, 0, 0);
    rotation = Quaternionf(1, 0, 0, 0);
    scale = Vector3f(1, 1, 1);
}

Transform::Transform(const Vector3f& p, const Quaternionf& r, const Vector3f& s) : position(p), rotation(r), scale(s)
{
    //
}

Transform::~Transform()
{
    //
}

Transform Transform::Inverse()
{
    return LocalInverse(*this);
}

void Transform::Combine(const Transform& parentTransform)
{
    *this = CombineTransforms(parentTransform, *this);
}

Transform Transform::Lerp(const Transform& other, float t) const
{
    return mix(*this, other, t);
}

Vector3f Transform::TransformPoint(const Vector3f& point) const
{
    return transformPoint(*this, point);
}

Matrix4x4f Transform::TransformToMat4() const
{
    return transformToMat4(*this);
}

void Transform::TransformFromMat4(const Matrix4x4f& m)
{
    *this = mat4ToTransform(m);
}

bool Transform::operator==(const Transform& b) const
{
    return position == b.position &&
            rotation == b.rotation &&
            scale == b.scale;
}

bool Transform::operator!=(const Transform& b) const
{
    return !(*this == b);
}

NS_RENDERSYSTEM_END
