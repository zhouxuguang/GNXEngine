//
//  AtmosphereComponent.h
//  GNXEngine
//
//  大气散射组件：把预计算大气散射作为一个独立的场景组件。
//  挂载到场景节点后，延迟渲染器在渲染时会收集该组件并渲染天空。
//

#ifndef GNX_ENGINE_ATMOSPHERE_COMPONENT_INCLUDE_HJFHJ
#define GNX_ENGINE_ATMOSPHERE_COMPONENT_INCLUDE_HJFHJ

#include "../Component.h"
#include "AtmosphereConstant.h"
#include "AtmosphereRenderer.h"
#include "Runtime/MathUtil/include/Vector3.h"

NS_RENDERSYSTEM_BEGIN

class RENDERSYSTEM_API AtmosphereComponent : public Component
{
public:
    AtmosphereComponent();
    ~AtmosphereComponent() override;

    ComponentType GetComponentType() const override
    {
        return ComponentType::Atmosphere;
    }

    // 构建默认大气模型并初始化 GPU 预计算资源
    bool Initialize(const Atmosphere::AtmosphereParameters& params, unsigned int numScatteringOrders = 4);

    bool IsInitialized() const
    {
        return mRenderer && mRenderer->IsInitialized();
    }

    AtmosphereRenderer* GetRenderer() const
    {
        return mRenderer.get();
    }

    // 曝光
    void SetExposure(float exposure)
    {
        mExposure = exposure;
    }

    float GetExposure() const
    {
        return mExposure;
    }

    // 白点
    void SetWhitePoint(const Vector3f& wp)
    {
        mWhitePoint = wp;
    }

    const Vector3f& GetWhitePoint() const
    {
        return mWhitePoint;
    }

    // 太阳方向（世界空间，从地表指向太阳，单位向量）
    void SetSunDirection(const Vector3f& dir)
    {
        mSunDirection = dir;
    }

    const Vector3f& GetSunDirection() const
    {
        return mSunDirection;
    }

    // 世界坐标和相机覆盖位置均使用米。相机覆盖值可由双精度地球相机逐帧更新。
    void SetPlanetCenter(const Vector3d& center) { mPlanetCenter = center; }
    const Vector3d& GetPlanetCenter() const { return mPlanetCenter; }
    void SetCameraWorldPosition(const Vector3d& position) { mCameraWorldPosition = position; mHasCameraWorldPosition = true; }
    void ClearCameraWorldPosition() { mHasCameraWorldPosition = false; }
    bool HasCameraWorldPosition() const { return mHasCameraWorldPosition; }
    const Vector3d& GetCameraWorldPosition() const { return mCameraWorldPosition; }

    // 在初始化前配置天空资源和额外 UBO；重新预计算时配置保持有效。
    void SetSkyShaderAsset(const std::string& asset) { mSkyShaderAsset = asset; }
    void SetSkyExtraUniformBuffer(const std::string& name, RenderCore::UniformBufferPtr buffer)
    { mSkyExtraUniformName = name; mSkyExtraUniformBuffer = std::move(buffer); }

private:
    AtmosphereRendererPtr mRenderer;

    // 默认曝光：着色器输出线性 HDR，由管线末端 PostProcessing(ACES) 色调映射，
    // 取 5.0 时整体亮度与参考实现（exposure=10 配合自带指数曲线）一致。
    float mExposure = 5.0f;
    Vector3f mWhitePoint{1.0f, 1.0f, 1.0f};
    Vector3f mSunDirection{0.0f, 1.0f, 0.0f};
    Vector3d mPlanetCenter{0.0, 0.0, 0.0};
    Vector3d mCameraWorldPosition{0.0, 0.0, 0.0};
    bool mHasCameraWorldPosition = false;
    std::string mSkyShaderAsset = "Atmosphere/AtmosphereShader";
    std::string mSkyExtraUniformName;
    RenderCore::UniformBufferPtr mSkyExtraUniformBuffer;
};

template<> struct ComponentTypeOf<AtmosphereComponent>
{
    static constexpr ComponentType Value = ComponentType::Atmosphere;
};

NS_RENDERSYSTEM_END

#endif // GNX_ENGINE_ATMOSPHERE_COMPONENT_INCLUDE_HJFHJ
