//
//  TransformTrack.cpp
//  GNXEditor
//
//  Created by zhouxuguang on 2024/5/17.
//

#include "TransformTrack.h"

NS_RENDERSYSTEM_BEGIN

TransformTrack::TransformTrack() 
{
    mId = 0;
    mPosition = std::make_shared<FastVectorTrack>();
    mRotation = std::make_shared<FastQuaternionTrack>();
    mScale = std::make_shared<FastVectorTrack>();
}

TransformTrack::~TransformTrack()
{
}

unsigned int TransformTrack::GetId() 
{
    return mId;
}

void TransformTrack::SetId(unsigned int id) 
{
    mId = id;
}

VectorTrackPtr TransformTrack::GetPositionTrack()
{
    return mPosition;
}

QuaternionTrackPtr TransformTrack::GetRotationTrack()
{
    return mRotation;
}

VectorTrackPtr TransformTrack::GetScaleTrack()
{
    return mScale;
}

bool TransformTrack::IsValid() 
{
    return mPosition->Size() > 1 || mRotation->Size() > 1 || mScale->Size() > 1;
}

float TransformTrack::GetStartTime() 
{
    float result = 0.0f;
    bool isSet = false;

    if (mPosition->Size() > 1)
    {
        result = mPosition->GetStartTime();
        isSet = true;
    }
    if (mRotation->Size() > 1)
    {
        float rotationStart = mRotation->GetStartTime();
        if (rotationStart < result || !isSet)
        {
            result = rotationStart;
            isSet = true;
        }
    }
    if (mScale->Size() > 1)
    {
        float scaleStart = mScale->GetStartTime();
        if (scaleStart < result || !isSet)
        {
            result = scaleStart;
            isSet = true;
        }
    }

    return result;
}

float TransformTrack::GetEndTime() 
{
    float result = 0.0f;
    bool isSet = false;

    if (mPosition->Size() > 1)
    {
        result = mPosition->GetEndTime();
        isSet = true;
    }
    if (mRotation->Size() > 1)
    {
        float rotationEnd = mRotation->GetEndTime();
        if (rotationEnd > result || !isSet) {
            result = rotationEnd;
            isSet = true;
        }
    }
    if (mScale->Size() > 1)
    {
        float scaleEnd = mScale->GetEndTime();
        if (scaleEnd > result || !isSet)
        {
            result = scaleEnd;
            isSet = true;
        }
    }

    return result;
}

Transform TransformTrack::Sample(const Transform& ref, float time, bool looping)
{
    Transform result = ref; // Assign default values
    if (mPosition->Size() > 1)
    {
        // Only assign if animated
        result.position = mPosition->Sample(time, looping);
    }
    if (mRotation->Size() > 1) 
    { 
        // Only assign if animated
        result.rotation = mRotation->Sample(time, looping);
    }
    if (mScale->Size() > 1)
    {
        // Only assign if animated
        result.scale = mScale->Sample(time, looping);
    }
    return result;
}

NS_RENDERSYSTEM_END
