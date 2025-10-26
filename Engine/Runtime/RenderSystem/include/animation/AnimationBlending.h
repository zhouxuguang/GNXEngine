//
//  AnimationBlending.h
//  GNXEditor
//
//  Created by zhouxuguang on 2024/5/22.
//

#ifndef GNXENGINE_ANIMATION_BLENDING_INCLUDE_HJSHDFHSD
#define GNXENGINE_ANIMATION_BLENDING_INCLUDE_HJSHDFHSD

#include "RSDefine.h"
#include "AnimationPose.h"
#include "AnimationClip.h"
#include "Skeleton.h"

NS_RENDERSYSTEM_BEGIN

// 动画混合
class AnimationBlending
{
public:
    // 动画姿态的常规混合
    static void Blend(AnimationPose& output, AnimationPose& a, AnimationPose& b, float t, int blendRoot);
    
private:
    static bool IsInHierarchy(AnimationPose& pose, uint32_t parent, uint32_t search);
};

NS_RENDERSYSTEM_END

#endif /* GNXENGINE_ANIMATION_BLENDING_INCLUDE_HJSHDFHSD */
