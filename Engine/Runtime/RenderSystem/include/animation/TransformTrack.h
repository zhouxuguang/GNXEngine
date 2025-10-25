//
//  TransformTrack.h
//  GNXEditor
//
//  Created by zhouxuguang on 2024/5/17.
//

#ifndef GNX_ENGINE_TRANSFORM_TRACK_INCLUDE_HHLFSD
#define GNX_ENGINE_TRANSFORM_TRACK_INCLUDE_HHLFSD

#include "RSDefine.h"
#include "AnimationFrame.h"
#include "Transform.h"

NS_RENDERSYSTEM_BEGIN

// 每一个骨骼节点变换的轨迹
class TransformTrack
{
private:
    unsigned int mId;
    VectorTrackPtr mPosition;
    QuaternionTrackPtr mRotation;
    VectorTrackPtr mScale;
public:
    TransformTrack();
    ~TransformTrack();
    unsigned int GetId();
    void SetId(unsigned int id);
    VectorTrackPtr GetPositionTrack();
    QuaternionTrackPtr GetRotationTrack();
    VectorTrackPtr GetScaleTrack();
    float GetStartTime();
    float GetEndTime();
    bool IsValid();
    Transform Sample(const Transform& ref, float time, bool looping);
};

NS_RENDERSYSTEM_END

#endif /* GNX_ENGINE_TRANSFORM_TRACK_INCLUDE_HHLFSD */
