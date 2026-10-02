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
    mParameters = params;
    mNumScatteringOrders = numScatteringOrders;
    mHasParameters = true;
    if (mAlgorithm == AtmosphereAlgorithm::SkyAtmosphere)
    {
        mSkyRenderer = std::make_shared<SkyAtmosphereRenderer>();
        mSkyRenderer->SetCompositeShaderAsset(mNewSkyShaderAsset);
        mSkyRenderer->SetSkyExtraUniformBuffer(mSkyExtraUniformName, mSkyExtraUniformBuffer);
        mSkyRenderer->SetPlanetEllipsoidRadii(mPlanetEllipsoidRadii);
        return mSkyRenderer->Initialize(params);
    }
    mRenderer = std::make_shared<AtmosphereRenderer>();
    mRenderer->SetSkyShaderAsset(mSkyShaderAsset);
    mRenderer->SetSkyExtraUniformBuffer(mSkyExtraUniformName, mSkyExtraUniformBuffer);
    mRenderer->SetPlanetEllipsoidRadii(mPlanetEllipsoidRadii);
    return mRenderer->Initialize(params, numScatteringOrders);
}

void AtmosphereComponent::SetAlgorithm(AtmosphereAlgorithm algorithm)
{
    if (algorithm == mAlgorithm) return;
    mAlgorithm = algorithm;
    if (!mHasParameters) return;
    if (algorithm == AtmosphereAlgorithm::SkyAtmosphere && !mSkyRenderer)
    {
        mSkyRenderer = std::make_shared<SkyAtmosphereRenderer>();
        mSkyRenderer->SetCompositeShaderAsset(mNewSkyShaderAsset);
        mSkyRenderer->SetSkyExtraUniformBuffer(mSkyExtraUniformName, mSkyExtraUniformBuffer);
        mSkyRenderer->SetPlanetEllipsoidRadii(mPlanetEllipsoidRadii);
        if (!mSkyRenderer->Initialize(mParameters))
            LOG_ERROR("AtmosphereComponent: Sky Atmosphere initialization failed");
    }
    else if (algorithm == AtmosphereAlgorithm::LegacyPrecomputed && !mRenderer)
    {
        mRenderer = std::make_shared<AtmosphereRenderer>();
        mRenderer->SetSkyShaderAsset(mSkyShaderAsset);
        mRenderer->SetSkyExtraUniformBuffer(mSkyExtraUniformName, mSkyExtraUniformBuffer);
        mRenderer->SetPlanetEllipsoidRadii(mPlanetEllipsoidRadii);
        if (!mRenderer->Initialize(mParameters, mNumScatteringOrders))
            LOG_ERROR("AtmosphereComponent: legacy atmosphere initialization failed");
    }
}

bool AtmosphereComponent::IsPrecomputed() const
{
    return mAlgorithm == AtmosphereAlgorithm::SkyAtmosphere
        ? (mSkyRenderer && mSkyRenderer->IsPrecomputed())
        : (mRenderer && mRenderer->IsPrecomputed());
}

bool AtmosphereComponent::IsReadyForRendering() const
{
    return IsPrecomputed() && (mAlgorithm == AtmosphereAlgorithm::LegacyPrecomputed ||
                               (mSkyRenderer && mSkyRenderer->IsDynamicReady()));
}

void AtmosphereComponent::Precompute(CommandBufferPtr commandBuffer)
{
    if (mAlgorithm == AtmosphereAlgorithm::SkyAtmosphere)
    {
        if (mSkyRenderer) mSkyRenderer->Precompute(commandBuffer);
    }
    else if (mRenderer) mRenderer->Precompute(commandBuffer);
}

void AtmosphereComponent::UpdateViewParams(const Camera* camera,
    const Vector3d& cameraWorldPosition, const Vector3d& planetCenter,
    const Vector3f& sunDirection, float exposure, const Vector3f& whitePoint)
{
    if (mAlgorithm == AtmosphereAlgorithm::SkyAtmosphere)
    {
        if (mSkyRenderer) mSkyRenderer->UpdateViewParams(camera, cameraWorldPosition,
            planetCenter, sunDirection, exposure, whitePoint);
    }
    else if (mRenderer) mRenderer->UpdateViewParams(camera, cameraWorldPosition,
        planetCenter, sunDirection, exposure, whitePoint);
}

void AtmosphereComponent::UpdatePlanetParams(const Vector3d& up, double altitude)
{
    if (mAlgorithm == AtmosphereAlgorithm::SkyAtmosphere)
    {
        if (mSkyRenderer) mSkyRenderer->UpdatePlanetParams(up, altitude);
    }
    else if (mRenderer) mRenderer->UpdatePlanetParams(up, altitude);
}

void AtmosphereComponent::UpdateDynamicLuts(CommandBufferPtr commandBuffer)
{
    if (mAlgorithm == AtmosphereAlgorithm::SkyAtmosphere && mSkyRenderer)
        mSkyRenderer->UpdateDynamicLuts(commandBuffer);
}

void AtmosphereComponent::RenderSky(RenderEncoderPtr encoder)
{
    if (mAlgorithm == AtmosphereAlgorithm::LegacyPrecomputed && mRenderer)
        mRenderer->RenderSky(encoder);
}

void AtmosphereComponent::RenderPlanetAtmosphere(RenderEncoderPtr encoder,
    RCTexturePtr sceneColor, RCTexturePtr sceneDepth)
{
    if (mAlgorithm == AtmosphereAlgorithm::SkyAtmosphere)
    {
        if (mSkyRenderer) mSkyRenderer->RenderComposite(encoder, sceneColor, sceneDepth);
    }
    else if (mRenderer) mRenderer->RenderPlanetAtmosphere(encoder, sceneColor, sceneDepth);
}

NS_RENDERSYSTEM_END
