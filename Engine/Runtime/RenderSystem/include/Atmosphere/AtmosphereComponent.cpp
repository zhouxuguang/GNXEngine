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

bool AtmosphereComponent::Initialize(const Atmosphere::AtmosphereParameters& params,
                                     unsigned int numScatteringOrders)
{
    mRenderer = std::make_shared<AtmosphereRenderer>();
    mRenderer->SetSkyShaderAsset(mSkyShaderAsset);
    mRenderer->SetSkyExtraUniformBuffer(mSkyExtraUniformName, mSkyExtraUniformBuffer);
    return mRenderer->Initialize(params, numScatteringOrders);
}

NS_RENDERSYSTEM_END
