#include "AnimationPose.h"

NS_RENDERSYSTEM_BEGIN

AnimationPose::AnimationPose() { }

AnimationPose::AnimationPose(unsigned int numJoints)
{
    Resize(numJoints);
}

AnimationPose::AnimationPose(const AnimationPose& p)
{
    *this = p;
}

AnimationPose& AnimationPose::operator=(const AnimationPose& p)
{
    if (&p == this) 
    {
        return *this;
    }

    // 分配足够大小的内存
    if (mParents.size() != p.mParents.size())
    {
        mParents.resize(p.mParents.size());
    }
    if (mJoints.size() != p.mJoints.size()) 
    {
        mJoints.resize(p.mJoints.size());
    }

    if (mParents.size() != 0) 
    {
        memcpy(mParents.data(), p.mParents.data(), sizeof(int) * mParents.size());
    }
    if (mJoints.size() != 0) 
    {
        memcpy(mJoints.data(), p.mJoints.data(), sizeof(Transform) * mJoints.size());
    }

    return *this;
}

void AnimationPose::Resize(unsigned int size)
{
    mParents.resize(size);
    mJoints.resize(size);
}

unsigned int AnimationPose::Size()
{
    return (unsigned int)mJoints.size();
}

Transform AnimationPose::GetLocalTransform(unsigned int index)
{
    return mJoints[index];
}

void AnimationPose::SetLocalTransform(unsigned int index, const Transform& transform)
{
    mJoints[index] = transform;
}

Transform AnimationPose::GetGlobalTransform(unsigned int index)
{
    Transform result = mJoints[index];
//    for (int parent = mParents[index]; parent >= 0; parent = mParents[parent])
//    {
//        result.Combine(mJoints[parent]);
//    }

    return result;
}

Transform AnimationPose::operator[](unsigned int index)
{
    return GetGlobalTransform(index);
}

Transform AnimationPose::GetGlobalTransform1(unsigned int index)
{
    Transform result = mJoints[index];
    for (int parent = mParents[index]; parent >= 0; parent = mParents[parent])
    {
        result.Combine(mJoints[parent]);
    }

    return result;
}

void AnimationPose::GetMatrixPalette(std::vector<Matrix4x4f>& outMatrixPalette)
{
#if 0
    unsigned int size = Size();
    if (outMatrixPalette.size() != size)
    {
        outMatrixPalette.resize(size);
    }

    for (unsigned int i = 0; i < size; ++i)
    {
        Transform t = GetGlobalTransform1(i);
        outMatrixPalette[i] = t.TransformToMat4();
    }
#else
    int size = (int)Size();
    if ((int)outMatrixPalette.size() != size) { outMatrixPalette.resize(size); }

    int i = 0;
    for (; i < size; ++i) 
    {
        int parent = mParents[i];
        
        // 如果父节点的索引大于当前节点索引，那么就退化为未优化的版本
        if (parent > i) { break; }

        Matrix4x4f global = mJoints[i].TransformToMat4();
        if (parent >= 0)
        {
            global = outMatrixPalette[parent] * global;
        }
        outMatrixPalette[i] = global;
    }

    // 退化的版本
    for (; i < size; ++i)
    {
        Transform t = GetGlobalTransform1(i);
        outMatrixPalette[i] = t.TransformToMat4();
    }
#endif
}

int AnimationPose::GetParent(unsigned int index)
{
    return mParents[index];
}

void AnimationPose::SetParent(unsigned int index, int parent)
{
    mParents[index] = parent;
}

bool AnimationPose::operator==(const AnimationPose& other)
{
    if (mJoints.size() != other.mJoints.size()) 
    {
        return false;
    }
    if (mParents.size() != other.mParents.size()) 
    {
        return false;
    }
    unsigned int size = (unsigned int)mJoints.size();
    for (unsigned int i = 0; i < size; ++i) 
    {
        Transform thisLocal = mJoints[i];
        Transform otherLocal = other.mJoints[i];

        int thisParent = mParents[i];
        int otherParent = other.mParents[i];

        if (thisParent != otherParent) 
        {
            return false;
        }

        if (thisLocal != otherLocal) 
        {
            return false;
        }
    }
    return true;
}

bool AnimationPose::operator!=(const AnimationPose& other)
{
    return !(*this == other);
}

NS_RENDERSYSTEM_END
