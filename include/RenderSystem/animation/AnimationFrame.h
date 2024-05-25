//
//  AnimationFrame.h
//  GNXEditor
//
//  Created by zhouxuguang on 2024/5/17.
//

#ifndef GNXENGINE_ANIMATION_FRAME_INCLUDE_GDFGDFHB_INCLUDE
#define GNXENGINE_ANIMATION_FRAME_INCLUDE_GDFGDFHB_INCLUDE

#include "RenderSystem/RSDefine.h"
#include "MathUtil/Matrix4x4.h"
#include "MathUtil/Quaternion.h"
#include "MathUtil/Vector3.h"
#include "Transform.h"

NS_RENDERSYSTEM_BEGIN

//动画帧的数据
template<uint32_t N>
class AnimationFrame
{
public:
    float mValue[N];
    float mIn[N];
    float mOut[N];
    float mTime;
};

//typedef AnimationFrame<1> ScalarFrame;
//typedef AnimationFrame<3> VectorFrame;
//typedef AnimationFrame<4> QuaternionFrame;

// 动画插值类型
enum class Interpolation
{
    Constant,
    Linear,
    Cubic
};

//动画轨迹数据，动画帧的集合
template<typename T, int N>
class AnimationTrack
{
protected:
    std::vector<AnimationFrame<N>> mFrames;
    Interpolation mInterpolation;
private:
    T SampleConstant(float time, bool looping);
    T SampleLinear(float time, bool looping);
    T SampleCubic(float time, bool looping);
    T Hermite(float time, const T& point1, const T& slope1, const T& point2, const T& slope2);
    virtual int FrameIndex(float time, bool looping);
    float AdjustTimeToFitTrack(float time, bool looping);
    T Cast(float* value);
public:
    AnimationTrack();
    void Resize(unsigned int size);
    unsigned int Size();
    Interpolation GetInterpolation();
    void SetInterpolation(Interpolation interpolation);
    float GetStartTime();
    float GetEndTime();
    T Sample(float time, bool looping);
    AnimationFrame<N>& operator[](unsigned int index);
    
    virtual void UpdateIndexLookupTable(){}
};

using ScalarTrack = AnimationTrack<float, 1>;
using VectorTrack = AnimationTrack<Vector3f, 3>;
using QuaternionTrack = AnimationTrack<Quaternionf, 4>;

using VectorTrackPtr = std::shared_ptr<VectorTrack>;
using QuaternionTrackPtr = std::shared_ptr<QuaternionTrack>;

// 快速动画轨迹的类
template<typename T, int N>
class FastAnimationTrack : public AnimationTrack<T, N>
{
private:
    std::vector<unsigned int> mSampledFrames;
    virtual int FrameIndex(float time, bool looping) override;
public:
    virtual void UpdateIndexLookupTable() override;
};

using FastScalarTrack = FastAnimationTrack<float, 1>;
using FastVectorTrack = FastAnimationTrack<Vector3f, 3>;
using FastQuaternionTrack = FastAnimationTrack<Quaternionf, 4>;

using FastVectorTrackPtr = std::shared_ptr<FastVectorTrack>;
using FastQuaternionTrackPtr = std::shared_ptr<FastQuaternionTrack>;

NS_RENDERSYSTEM_END

#endif /* GNXENGINE_ANIMATION_FRAME_INCLUDE_GDFGDFHB_INCLUDE */
