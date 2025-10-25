//
//  AnimationBlending.cpp
//  GNXEditor
//
//  Created by zhouxuguang on 2024/5/22.
//

#include "AnimationBlending.h"

NS_RENDERSYSTEM_BEGIN

void AnimationBlending::Blend(AnimationPose& output, AnimationPose& a, AnimationPose& b, float t, int blendRoot)
{
    uint32_t numJoints = output.Size();
    for (uint32_t i = 0; i < numJoints; ++i)
    {
        if (blendRoot >= 0)
        {
            if (!IsInHierarchy(output, (uint32_t)blendRoot, i))
            {
                continue;
            }
        }

        output.SetLocalTransform(i, a.GetLocalTransform(i).Lerp(b.GetLocalTransform(i), t));
    }
}

bool AnimationBlending::IsInHierarchy(AnimationPose& pose, uint32_t parent, uint32_t search)
{
    if (search == parent)
    {
        return true;
    }
    int p = pose.GetParent(search);

    while (p >= 0)
    {
        if (p == (int)parent)
        {
            return true;
        }
        p = pose.GetParent(p);
    }

    return false;
}

NS_RENDERSYSTEM_END
