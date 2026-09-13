//
//  AtmosphereComponent.cpp
//  GNXEngine
//

#include "AtmosphereComponent.h"
#include "Runtime/BaseLib/include/LogService.h"

NS_RENDERSYSTEM_BEGIN

AtmosphereComponent::AtmosphereComponent()
{
}

AtmosphereComponent::~AtmosphereComponent()
{
}

bool AtmosphereComponent::Initialize(unsigned int numScatteringOrders)
{
    // 构建默认大气模型
    AtmosphereModel* model = CreateAtmoModel();
    if (!model)
    {
        LOG_ERROR("AtmosphereComponent: failed to create atmosphere model");
        return false;
    }

    const Atmosphere::AtmosphereParameters& params = model->GetAtmosphereParameters();

    // GNXEngine 的世界坐标为 Y-up。把地表原点放在行星北极点，
    // 地球中心沿 -Y 偏移一个内半径（单位：大气长度单位）。
    mEarthCenter = Vector3f(0.0f, -params.bottom_radius, 0.0f);

    mRenderer = std::make_shared<AtmosphereRenderer>();
    bool ok = mRenderer->Initialize(params, numScatteringOrders);

    delete model;
    return ok;
}

NS_RENDERSYSTEM_END
