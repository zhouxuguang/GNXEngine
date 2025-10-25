#ifndef RENDERSYSTEM_ANIMATION_POSE_DGHDFGVH_INCLUDE_H
#define RENDERSYSTEM_ANIMATION_POSE_DGHDFGVH_INCLUDE_H

#include "RenderSystem/RSDefine.h"
#include "MathUtil/Matrix4x4.h"
#include "Transform.h"

NS_RENDERSYSTEM_BEGIN

//动画姿势类，可以表达bind pose以及插值后的姿势
class AnimationPose
{
private:
    std::vector<Transform> mJoints;   //关节的数组
    std::vector<int> mParents;          //父节点数组，存储编号
    Transform GetGlobalTransform1(unsigned int index);   //这里动画姿势插值的时候使用，动画数据都是局部变换，需要变到全局变换
public:
    AnimationPose();
    AnimationPose(const AnimationPose& p);
    AnimationPose& operator=(const AnimationPose& p);
    AnimationPose(unsigned int numJoints);
    void Resize(unsigned int size);
    unsigned int Size();
    Transform GetLocalTransform(unsigned int index);
    void SetLocalTransform(unsigned int index, const Transform& transform);
    Transform GetGlobalTransform(unsigned int index);
    Transform operator[](unsigned int index);
    void GetMatrixPalette(std::vector<Matrix4x4f>& outMatrixPalette);
    int GetParent(unsigned int index);
    void SetParent(unsigned int index, int parent);

    bool operator==(const AnimationPose& other);
    bool operator!=(const AnimationPose& other);
};

NS_RENDERSYSTEM_END

#endif
