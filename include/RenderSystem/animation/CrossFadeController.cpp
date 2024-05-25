//
//  CrossFadeController.cpp
//  GNXEditor
//
//  Created by zhouxuguang on 2024/5/22.
//

#include "CrossFadeController.h"
#include "AnimationBlending.h"

NS_RENDERSYSTEM_BEGIN

CrossFadeController::CrossFadeController() 
{
    mClip = 0;
    mTime = 0.0f;
    mWasSkeletonSet = false;
}

CrossFadeController::CrossFadeController(Skeleton& skeleton) 
{
    mClip = 0;
    mTime = 0.0f;
    SetSkeleton(skeleton);
}

void CrossFadeController::SetSkeleton(Skeleton& skeleton) 
{
    mSkeleton = skeleton;
    mPose = mSkeleton.GetRestPose();
    mWasSkeletonSet = true;
}

void CrossFadeController::Play(AnimationClip* target)
{
    mTargets.clear();
    mClip = target;
    mPose = mSkeleton.GetRestPose();
    mTime = target->GetStartTime();
}

void CrossFadeController::FadeTo(AnimationClip* target, float fadeTime)
{
    if (mClip == 0) 
    {
        Play(target);
        return;
    }

    if (mTargets.size() >= 1) 
    {
        if (mTargets[mTargets.size() - 1].mClip == target) 
        {
            return;
        }
    }
    else 
    {
        if (mClip == target) 
        {
            return;
        }
    }

    mTargets.push_back(CrossFadeTarget(target, mSkeleton.GetRestPose(), fadeTime));
}

void CrossFadeController::Update(float dt) 
{
    if (mClip == 0 || !mWasSkeletonSet) 
    {
        return;
    }

    unsigned int numTargets = (unsigned int)mTargets.size();
    for (unsigned int i = 0; i < numTargets; ++i)
    {
        if (mTargets[i].mElapsed >= mTargets[i].mDuration) 
        {
            mClip = mTargets[i].mClip;
            mTime = mTargets[i].mTime;
            mPose = mTargets[i].mPose;

            mTargets.erase(mTargets.begin() + i);
            break;
        }
    }
    numTargets = (unsigned int)mTargets.size();
    mPose = mSkeleton.GetRestPose();
    mTime = mClip->Sample(mPose, mTime + dt);

    for (unsigned int i = 0; i < numTargets; ++i) 
    {
        CrossFadeTarget& target = mTargets[i];
        target.mTime = target.mClip->Sample(target.mPose, target.mTime + dt);
        target.mElapsed += dt;
        float t = target.mElapsed / target.mDuration;
        if (t > 1.0f) { t = 1.0f; }

        AnimationBlending::Blend(mPose, mPose, target.mPose, t, -1);
    }
}

AnimationPose& CrossFadeController::GetCurrentPose()
{
    return mPose;
}

AnimationClip* CrossFadeController::GetCurrentClip() const
{
    return mClip;
}

NS_RENDERSYSTEM_END
