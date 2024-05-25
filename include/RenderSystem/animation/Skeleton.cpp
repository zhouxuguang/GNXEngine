//
//  Skeleton.cpp
//  GNXEditor
//
//  Created by zhouxuguang on 2024/5/19.
//

#include "Skeleton.h"

NS_RENDERSYSTEM_BEGIN

Skeleton::Skeleton() { }

Skeleton::Skeleton(const AnimationPose& bind, const std::vector<std::string>& names)
{
    Set(bind, names);
}

Skeleton::~Skeleton()
{
}

void Skeleton::Set(const AnimationPose& bind, const std::vector<std::string>& names)
{
    mBindPose = bind;
    mJointNames = names;
    UpdateInverseBindPose();
}

void Skeleton::UpdateInverseBindPose() 
{
    unsigned int size = mBindPose.Size();
    mInvBindPose.resize(size);

    for (unsigned int i = 0; i < size; ++i) 
    {
        Transform world = mBindPose.GetGlobalTransform(i);
        Matrix4x4f worldMatrix = world.TransformToMat4();
        mInvBindPose[i] = worldMatrix.Inverse();
    }
}

AnimationPose& Skeleton::GetBindPose()
{
    return mBindPose;
}

AnimationPose& Skeleton::GetRestPose()
{
    return mBindPose;
}

std::vector<Matrix4x4f>& Skeleton::GetInvBindPoses()
{
    return mInvBindPose;
}

std::vector<std::string>& Skeleton::GetJointNames() 
{
    return mJointNames;
}

std::string& Skeleton::GetJointName(unsigned int idx) 
{
    return mJointNames[idx];
}

NS_RENDERSYSTEM_END
