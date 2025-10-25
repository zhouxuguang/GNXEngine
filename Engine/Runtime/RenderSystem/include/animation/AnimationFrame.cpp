//
//  AnimationFrame.cpp
//  GNXEditor
//
//  Created by zhouxuguang on 2024/5/17.
//

#include "AnimationFrame.h"
#include <math.h>

NS_RENDERSYSTEM_BEGIN

namespace TrackHelpers 
{
    inline float Interpolate(float a, float b, float t) 
    {
        return a + (b - a) * t;
    }

    inline Vector3f Interpolate(const Vector3f& a, const Vector3f& b, float t)
    {
        return Vector3f::Lerp(a, b, t);
    }

    inline Quaternionf Interpolate(const Quaternionf& a, const Quaternionf& b, float t)
    {
        Quaternionf result = Quaternionf::Mix(a, b, t);
        if (a.DotProduct(b) < 0)
        {
            // Neighborhood
            result = Quaternionf::Mix(a, -b, t);
        }
        return result.Normalized(); //NLerp, not slerp
    }

    // Hermite helpers
    inline float AdjustHermiteResult(float f) 
    {
        return f;
    }

    inline Vector3f AdjustHermiteResult(const Vector3f& v)
    {
        return v;
    }

    inline Quaternionf AdjustHermiteResult(const Quaternionf& q)
    {
        return q.Normalized();
    }

    inline void Neighborhood(const float& a, float& b) { }
    inline void Neighborhood(const Vector3f& a, Vector3f& b) { }
    inline void Neighborhood(const Quaternionf& a, Quaternionf& b)
    {
        if (a.DotProduct(b) < 0)
        {
            b = -b;
        }
    }
} // End Track Helpers namespace

template<typename T, int N>
AnimationTrack<T, N>::AnimationTrack()
{
    mInterpolation = Interpolation::Linear;
}

template<typename T, int N>
float AnimationTrack<T, N>::GetStartTime()
{
    return mFrames[0].mTime;
}

template<typename T, int N>
float AnimationTrack<T, N>::GetEndTime()
{
    return mFrames[mFrames.size() - 1].mTime;
}

template<typename T, int N>
T AnimationTrack<T, N>::Sample(float time, bool looping)
{
    if (mInterpolation == Interpolation::Constant) 
    {
        return SampleConstant(time, looping);
    }
    else if (mInterpolation == Interpolation::Linear) 
    {
        return SampleLinear(time, looping);
    }
    return SampleCubic(time, looping);
}

template<typename T, int N>
AnimationFrame<N>& AnimationTrack<T, N>::operator[](unsigned int index)
{
    return mFrames[index];
}

template<typename T, int N>
void AnimationTrack<T, N>::Resize(unsigned int size)
{
    mFrames.resize(size);
}

template<typename T, int N>
unsigned int AnimationTrack<T, N>::Size()
{
    return (unsigned int)mFrames.size();
}

template<typename T, int N>
Interpolation AnimationTrack<T, N>::GetInterpolation()
{
    return mInterpolation;
}

template<typename T, int N>
void AnimationTrack<T, N>::SetInterpolation(Interpolation interpolation)
{
    mInterpolation = interpolation;
}

template<typename T, int N>
T AnimationTrack<T, N>::Hermite(float t, const T& p1, const T& s1, const T& _p2, const T& s2)
{
    float tt = t * t;
    float ttt = tt * t;

    T p2 = _p2;
    TrackHelpers::Neighborhood(p1, p2);

    float h1 = 2.0f * ttt - 3.0f * tt + 1.0f;
    float h2 = -2.0f * ttt + 3.0f * tt;
    float h3 = ttt - 2.0f * tt + t;
    float h4 = ttt - tt;

    T result = p1 * h1 + p2 * h2 + s1 * h3 + s2 * h4;
    return TrackHelpers::AdjustHermiteResult(result);
}

template<typename T, int N>
int AnimationTrack<T, N>::FrameIndex(float time, bool looping)
{
    unsigned int size = (unsigned int)mFrames.size();
    if (size <= 1) 
    {
        return -1;
    }
    if (looping) 
    {
        float startTime = mFrames[0].mTime;
        float endTime = mFrames[size - 1].mTime;
        float duration = endTime - startTime;

        time = fmodf(time - startTime, endTime - startTime);
        if (time < 0.0f) 
        {
            time += endTime - startTime;
        }
        time = time + startTime;
    }
    else 
    {
        if (time <= mFrames[0].mTime) {
            return 0;
        }
        if (time >= mFrames[size - 2].mTime) {
            return (int)size - 2;
        }
    }
    for (int i = (int)size - 1; i >= 0; --i) 
    {
        if (time >= mFrames[i].mTime) 
        {
            return i;
        }
    }
    // 应该不会走到这里
    return -1;
} // End of FrameIndex

template<typename T, int N>
float AnimationTrack<T, N>::AdjustTimeToFitTrack(float time, bool looping)
{
    unsigned int size = (unsigned int)mFrames.size();
    if (size <= 1) { return 0.0f; }

    float startTime = mFrames[0].mTime;
    float endTime = mFrames[size - 1].mTime;
    float duration = endTime - startTime;
    if (duration <= 0.0f) { return 0.0f; }
    if (looping) {
        time = fmodf(time - startTime, endTime - startTime);
        if (time < 0.0f) {
            time += endTime - startTime;
        }
        time = time + startTime;
    }
    else {
        if (time <= mFrames[0].mTime) { time = startTime; }
        if (time >= mFrames[size - 1].mTime) { time = endTime; }
    }

    return time;
}

template<> float AnimationTrack<float, 1>::Cast(float* value)
{
    return value[0];
}

template<> Vector3f AnimationTrack<Vector3f, 3>::Cast(float* value)
{
    return Vector3f(value[0], value[1], value[2]);
}

template<> Quaternionf AnimationTrack<Quaternionf, 4>::Cast(float* value)
{
    Quaternionf r = Quaternionf(value[0], value[1], value[2], value[3]);
    return r.Normalized();
}

template<typename T, int N>
T AnimationTrack<T, N>::SampleConstant(float time, bool looping)
{
    int frame = FrameIndex(time, looping);
    if (frame < 0 || frame >= (int)mFrames.size()) {
        return T();
    }

    return Cast(&mFrames[frame].mValue[0]);
}

template<typename T, int N>
T AnimationTrack<T, N>::SampleLinear(float time, bool looping)
{
    int thisFrame = FrameIndex(time, looping);
    if (thisFrame < 0 || thisFrame >= (int)(mFrames.size() - 1)) {
        return T();
    }
    int nextFrame = thisFrame + 1;

    float trackTime = AdjustTimeToFitTrack(time, looping);
    float frameDelta = mFrames[nextFrame].mTime - mFrames[thisFrame].mTime;
    if (frameDelta <= 0.0f) {
        return T();
    }
    float t = (trackTime - mFrames[thisFrame].mTime) / frameDelta;

    T start = Cast(&mFrames[thisFrame].mValue[0]);
    T end = Cast(&mFrames[nextFrame].mValue[0]);

    return TrackHelpers::Interpolate(start, end, t);
}

template<typename T, int N>
T AnimationTrack<T, N>::SampleCubic(float time, bool looping)
{
    int thisFrame = FrameIndex(time, looping);
    if (thisFrame < 0 || thisFrame >= (int)(mFrames.size() - 1)) {
        return T();
    }
    int nextFrame = thisFrame + 1;

    float trackTime = AdjustTimeToFitTrack(time, looping);
    float frameDelta = mFrames[nextFrame].mTime - mFrames[thisFrame].mTime;
    if (frameDelta <= 0.0f) {
        return T();
    }
    float t = (trackTime - mFrames[thisFrame].mTime) / frameDelta;

    T point1 = Cast(&mFrames[thisFrame].mValue[0]);
    T slope1;// = mFrames[thisFrame].mOut * frameDelta;
    memcpy(&slope1, mFrames[thisFrame].mOut, N * sizeof(float));
    slope1 = slope1 * frameDelta;

    T point2 = Cast(&mFrames[nextFrame].mValue[0]);
    T slope2;// = mFrames[nextFrame].mIn[0] * frameDelta;
    memcpy(&slope2, mFrames[nextFrame].mIn, N * sizeof(float));
    slope2 = slope2 * frameDelta;

    return Hermite(t, point1, slope1, point2, slope2);
}

//这个显式实例化要放在类模板函数的最后
template class AnimationTrack<float, 1>;
template class AnimationTrack<Vector3f, 3>;
template class AnimationTrack<Quaternionf, 4>;

template<typename T, int N>
int FastAnimationTrack<T, N>::FrameIndex(float time, bool looping)
{
    // 最少需要两帧
    std::vector<AnimationFrame<N>>& frames = this->mFrames;

    unsigned int size = (unsigned int)frames.size();
    if (size <= 1) { return -1; }

    // 计算出归一化的时间
    if (looping)
    {
        float startTime = frames[0].mTime;
        float endTime = frames[size - 1].mTime;
        float duration = endTime - startTime;
        time = fmodf(time - startTime, endTime - startTime);
        if (time < 0.0f) {
            time += endTime - startTime;
        }
        time = time + startTime;
    }
    else 
    {
        if (time <= frames[0].mTime) 
        {
            return 0;
        }
        if (time >= frames[size - 2].mTime) 
        {
            return (int)size - 2;
        }
    }
    float duration = this->GetEndTime() - this->GetStartTime();
    unsigned int numSamples = 60 + (unsigned int)(duration * 60.0f);
    float t = time / duration;

    // 计算出当前时间对应的索引
    unsigned int index = (unsigned int)(t * (float)numSamples);
    if (index >= mSampledFrames.size()) 
    {
        return -1;
    }
    return (int)mSampledFrames[index];
}

template<typename T, int N>
void FastAnimationTrack<T, N>::UpdateIndexLookupTable()
{
    // 一个有效的动画至少要两帧
    int numFrames = (int)this->mFrames.size();
    if (numFrames <= 1) 
    {
        return;
    }
    
    // 计算出总的采样个数
    float duration = this->GetEndTime() - this->GetStartTime();
    unsigned int numSamples = 60 + (unsigned int)(duration * 60.0f);
    mSampledFrames.resize(numSamples);
    
    // 计算当前采样的最近的帧索引
    for (int i = 0; i < numSamples; ++i)
    {
        float t = (float)i / (float)(numSamples - 1);
        float time = t * duration + this->GetStartTime();

        unsigned int frameIndex = 0;
        for (int j = numFrames - 1; j >= 0; --j) 
        {
            if (time >= this->mFrames[j].mTime) 
            {
                frameIndex = (unsigned int)j;
                
                // 帧索引不能是最后一个
                if ((int)frameIndex >= numFrames - 2)
                {
                    frameIndex = numFrames - 2;
                }
                break;
            }
        }
        mSampledFrames[i] = frameIndex;
    }
}


template class FastAnimationTrack<float, 1>;
template class FastAnimationTrack<Vector3f, 3>;
template class FastAnimationTrack<Quaternionf, 4>;

NS_RENDERSYSTEM_END
