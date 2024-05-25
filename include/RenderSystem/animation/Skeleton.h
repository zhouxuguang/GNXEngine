//
//  Skeleton.h
//  GNXEditor
//
//  Created by zhouxuguang on 2024/5/19.
//

#ifndef GNX_ENGINE_SKELETON_INCLUDE_JSDGHFDDKF
#define GNX_ENGINE_SKELETON_INCLUDE_JSDGHFDDKF

#include "RSDefine.h"
#include "AnimationPose.h"
#include "MathUtil/Matrix4x4.h"

USING_NS_MATHUTIL;

NS_RENDERSYSTEM_BEGIN

class Skeleton 
{
public:
    Skeleton();
    Skeleton(const AnimationPose& bind, const std::vector<std::string>& names);
    ~Skeleton();
    
    void Set(const AnimationPose& bind, const std::vector<std::string>& names);
    AnimationPose& GetBindPose();
    AnimationPose& GetRestPose();
    std::vector<Matrix4x4f>& GetInvBindPoses();
    std::vector<std::string>& GetJointNames();
    std::string& GetJointName(unsigned int index);
    
private:
    //AnimationPose mRestPose;
    AnimationPose mBindPose;
    std::vector<Matrix4x4f> mInvBindPose;
    std::vector<std::string> mJointNames;
    
    void UpdateInverseBindPose();
};

typedef std::shared_ptr<Skeleton> SkeletonPtr;

NS_RENDERSYSTEM_END

#endif /* GNX_ENGINE_SKELETON_INCLUDE_JSDGHFDDKF */
