#include "AnimationClip.h"
#include <math.h>

NS_RENDERSYSTEM_BEGIN

AnimationClip::AnimationClip()
{
    mName = "No name given";
    mStartTime = 0.0f;
    mEndTime = 0.0f;
    mLooping = true;
}

float AnimationClip::Sample(AnimationPose& outPose, float time)
{
    if (GetDuration() == 0.0f) 
    {
        return 0.0f;
    }
    time = AdjustTimeToFitRange(time);

    unsigned int size = (unsigned int)mTracks.size();
    for (unsigned int i = 0; i < size; ++i) 
    {
        unsigned int joint = mTracks[i].GetId();
        Transform local = outPose.GetLocalTransform(joint);  // 参考姿势
        Transform animated = mTracks[i].Sample(local, time, mLooping);
        outPose.SetLocalTransform(joint, animated);
        
//        printf("animated i = %d, position x = %f, y = %f, z = %f, rotation w = %f, x = %f, y = %f, z = %f, scale x = %f, y = %f, z = %f\n",
//               i, animated.position.x, animated.position.y, animated.position.z,
//               animated.rotation.m_dfW, animated.rotation.m_dfX, animated.rotation.m_dfY, animated.rotation.m_dfZ,
//               animated.scale.x, animated.scale.y, animated.scale.z);
    }
    return time;
}

float AnimationClip::AdjustTimeToFitRange(float inTime)
{
    if (mLooping) 
    {
        float duration = mEndTime - mStartTime;
        if (duration <= 0) 
        {
            return 0.0f;
        }
        inTime = fmodf(inTime - mStartTime, mEndTime - mStartTime);
        if (inTime < 0.0f) 
        {
            inTime += mEndTime - mStartTime;
        }
        inTime = inTime + mStartTime;
    }
    else 
    {
        if (inTime < mStartTime) 
        {
            inTime = mStartTime;
        }
        if (inTime > mEndTime) 
        {
            inTime = mEndTime;
        }
    }
    return inTime;
}

void AnimationClip::RecalculateDuration()
{
    mStartTime = 0.0f;
    mEndTime = 0.0f;
    bool startSet = false;
    bool endSet = false;
    unsigned int tracksSize = (unsigned int)mTracks.size();
    for (unsigned int i = 0; i < tracksSize; ++i) 
    {
        if (mTracks[i].IsValid()) 
        {
            float trackStartTime = mTracks[i].GetStartTime();
            float trackEndTime = mTracks[i].GetEndTime();

            if (trackStartTime < mStartTime || !startSet) 
            {
                mStartTime = trackStartTime;
                startSet = true;
            }

            if (trackEndTime > mEndTime || !endSet) 
            {
                mEndTime = trackEndTime;
                endSet = true;
            }
        }
    }
}

TransformTrack& AnimationClip::operator[](unsigned int joint)
{
    for (unsigned int i = 0, size = (unsigned int)mTracks.size(); i < size; ++i)
    {
        if (mTracks[i].GetId() == joint) 
        {
            return mTracks[i];
        }
    }

    mTracks.push_back(TransformTrack());
    mTracks[mTracks.size() - 1].SetId(joint);
    return mTracks[mTracks.size() - 1];
}

std::string& AnimationClip::GetName()
{
    return mName;
}

void AnimationClip::SetName(const std::string& inNewName)
{
    mName = inNewName;
}

unsigned int AnimationClip::GetIdAtIndex(unsigned int index)
{
    return mTracks[index].GetId();
}

void AnimationClip::SetIdAtIndex(unsigned int index, unsigned int id)
{
    return mTracks[index].SetId(id);
}

unsigned int AnimationClip::Size() const
{
    return (unsigned int)mTracks.size();
}

float AnimationClip::GetDuration() const
{
    return mEndTime - mStartTime;
}

float AnimationClip::GetStartTime() const
{
    return mStartTime;
}

float AnimationClip::GetEndTime() const
{
    return mEndTime;
}

bool AnimationClip::GetLooping() const
{
    return mLooping;
}

void AnimationClip::SetLooping(bool inLooping)
{
    mLooping = inLooping;
}

NS_RENDERSYSTEM_END
