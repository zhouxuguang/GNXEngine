//
//  SkeletonAnimation.h
//  GNXEditor
//
//  Created by zhouxuguang on 2024/5/19.
//

#ifndef GNX_ENGINE_RENDERSYSTEM_SKELETON_ANIMATION_INCLUDE_SDJJ
#define GNX_ENGINE_RENDERSYSTEM_SKELETON_ANIMATION_INCLUDE_SDJJ

#include "RSDefine.h"
#include "AnimationPose.h"
#include "MathUtil/Matrix4x4.h"
#include "Skeleton.h"
#include "Component.h"
#include "SceneNode.h"
#include "animation/AnimationClip.h"
#include "skinnedMesh/SkinnedMeshRenderer.h"

USING_NS_MATHUTIL;

NS_RENDERSYSTEM_BEGIN

// 骨骼动画播放器
class SkeletonAnimation : public Component
{
public:
    AnimationPose mAnimatedPose;                  // 当前的姿势
    std::vector<Matrix4x4f> mPosePalette;         // 姿势矩阵
    unsigned int mClip = 0;                        //当前运行哪个动画片段
    float mPlayback = 0.0;                          // 当前播放的时间
    std::vector<AnimationClipPtr> mAnimationClips;  //动画片段集合
    SkeletonPtr mSkeleton = nullptr;                 //当前动画的骨骼
    bool mCPUSkin = true;
    
private:
    virtual void Update(float deltaTime) 
    {
        mPlayback = mAnimationClips[mClip]->Sample(mAnimatedPose, mPlayback + deltaTime);
        printf("SkeletonAnimation Update mPlayback = %f\n", mPlayback);
        
        // 拿到skinnedMesh
        SkinnedMeshRenderer * skinnedRenderer = mSceneNode->QueryComponentT<SkinnedMeshRenderer>();
        assert(skinnedRenderer);
        
        SkinnedMeshPtr skinnedMesh = skinnedRenderer->GetSharedMesh();
        assert(skinnedMesh);
        
        if (mCPUSkin)
        {
            // 更新mesh
            skinnedMesh->CPUSkin(*mSkeleton, mAnimatedPose);
        }
        else
        {
            skinnedMesh->GPUSkin(*mSkeleton, mAnimatedPose);
        }
    }
};

NS_RENDERSYSTEM_END

#endif /* GNX_ENGINE_RENDERSYSTEM_SKELETON_ANIMATION_INCLUDE_SDJJ */
