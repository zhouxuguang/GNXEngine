//
//  CrossFadeController.h
//  GNXEditor
//
//  Created by zhouxuguang on 2024/5/22.
//

#ifndef GNXENGINE_CROSSFADE_CONTROLLER_INCLUDE_NDSJFSDDJ
#define GNXENGINE_CROSSFADE_CONTROLLER_INCLUDE_NDSJFSDDJ

#include "RSDefine.h"
#include "AnimationClip.h"
#include "AnimationPose.h"
#include "Skeleton.h"

NS_RENDERSYSTEM_BEGIN

struct CrossFadeTarget 
{
    AnimationPose mPose;
    AnimationClip* mClip;
    float mTime;
    float mDuration;
    float mElapsed;

    inline CrossFadeTarget() : mClip(0), mTime(0.0f), mDuration(0.0f), mElapsed(0.0f) { }
    inline CrossFadeTarget(AnimationClip* target, AnimationPose& pose, float duration) :
        mClip(target), mTime(target->GetStartTime()), mPose(pose), mDuration(duration), mElapsed(0.0f) { }
};

class CrossFadeController 
{
private:
    std::vector<CrossFadeTarget> mTargets;
    AnimationClip* mClip;
    float mTime;
    AnimationPose mPose;
    Skeleton mSkeleton;
    bool mWasSkeletonSet;
public:
    CrossFadeController();
    CrossFadeController(Skeleton& skeleton);
    void SetSkeleton(Skeleton& skeleton);
    void Play(AnimationClip* target);
    void FadeTo(AnimationClip* target, float fadeTime);
    void Update(float dt);
    AnimationPose& GetCurrentPose();
    AnimationClip* GetCurrentClip() const;
};

NS_RENDERSYSTEM_END

#endif /* GNXENGINE_CROSSFADE_CONTROLLER_INCLUDE_NDSJFSDDJ */
