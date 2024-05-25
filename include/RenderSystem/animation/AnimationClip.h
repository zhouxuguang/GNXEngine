//
//  AnimationClip.h
//  GNXEditor
//
//  Created by zhouxuguang on 2024/5/17.
//

#ifndef GNX_ENGINE_ANIMATION_CLIP_INCLUDE_HHLFSD
#define GNX_ENGINE_ANIMATION_CLIP_INCLUDE_HHLFSD

#include "TransformTrack.h"
#include "AnimationPose.h"

NS_RENDERSYSTEM_BEGIN

// 动画片段
class AnimationClip
{
private:
    std::vector<TransformTrack> mTracks;   //每个骨骼一个track
    std::string mName;
    float mStartTime;
    float mEndTime;
    bool mLooping;
    float AdjustTimeToFitRange(float inTime);
public:
    AnimationClip();
    unsigned int GetIdAtIndex(unsigned int index);
    void SetIdAtIndex(unsigned int index, unsigned int id);
    unsigned int Size() const;
    
    /**
    根据时间生成插值的动画姿势 inTime 以秒为单位
     */
    float Sample(AnimationPose& outPose, float inTime);
    TransformTrack& operator[](unsigned int index);
    
    // 重新计算动画时长
    void RecalculateDuration();
    std::string& GetName();
    void SetName(const std::string& inNewName);
    float GetDuration() const;
    float GetStartTime() const;
    float GetEndTime() const;
    bool GetLooping() const;
    void SetLooping(bool inLooping);
};

typedef std::shared_ptr<AnimationClip> AnimationClipPtr;

NS_RENDERSYSTEM_END

#endif /* GNX_ENGINE_ANIMATION_CLIP_INCLUDE_HHLFSD */

