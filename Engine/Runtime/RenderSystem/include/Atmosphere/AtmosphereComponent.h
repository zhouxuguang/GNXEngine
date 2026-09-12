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

    ComponentType GetComponentType() const override { return ComponentType::Atmosphere; }

    // 构建默认大气模型并初始化 GPU 预计算资源
    bool Initialize(unsigned int numScatteringOrders = 4);

    bool IsInitialized() const { return mRenderer && mRenderer->IsInitialized(); }

    AtmosphereRenderer* GetRenderer() const { return mRenderer.get(); }

    // 曝光
    void SetExposure(float exposure) { mExposure = exposure; }
    float GetExposure() const { return mExposure; }

    // 白点
    void SetWhitePoint(const Vector3f& wp) { mWhitePoint = wp; }
    const Vector3f& GetWhitePoint() const { return mWhitePoint; }

    // 太阳方向（世界空间，从地表指向太阳，单位向量）
    void SetSunDirection(const Vector3f& dir) { mSunDirection = dir; }
    const Vector3f& GetSunDirection() const { return mSunDirection; }

    // 地球中心（大气单位）
    void SetEarthCenter(const Vector3f& center) { mEarthCenter = center; }
    const Vector3f& GetEarthCenter() const { return mEarthCenter; }

    // 场景“额外几何体”（球体 + 地面）：用于演示大气光柱与地面着色，由着色器外部传入。
    // 单位说明：均为“大气单位”（1 大气单位 = Atmosphere::kLengthUnitInMeters 米）。
    // 默认值与参考实现一致：球心 (0,0,1000m)、半径 1000m、地面着色反照率 (0,0,0.04)。
    void SetSceneGeometry(const Atmosphere::AtmosphereSceneGeometry& geometry) { mSceneGeometry = geometry; }
    const Atmosphere::AtmosphereSceneGeometry& GetSceneGeometry() const { return mSceneGeometry; }

private:
    AtmosphereRendererPtr mRenderer;

    // 默认曝光：着色器输出线性 HDR，由管线末端 PostProcessing(ACES) 色调映射，
    // 取 5.0 时整体亮度与参考实现（exposure=10 配合自带指数曲线）一致。
    float mExposure = 5.0f;
    Vector3f mWhitePoint{1.0f, 1.0f, 1.0f};
    Vector3f mSunDirection{0.0f, 0.0f, 1.0f};
    // 默认地球中心：-(bottom_radius / length_unit) = -(6360000 / 1000)
    Vector3f mEarthCenter{0.0f, 0.0f, -6360.0f};
    Atmosphere::AtmosphereSceneGeometry mSceneGeometry;
};

template<> struct ComponentTypeOf<AtmosphereComponent>
{
    static constexpr ComponentType Value = ComponentType::Atmosphere;
};

NS_RENDERSYSTEM_END

#endif // GNX_ENGINE_ATMOSPHERE_COMPONENT_INCLUDE_HJFHJ
