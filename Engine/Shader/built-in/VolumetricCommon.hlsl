#ifndef GNX_VOLUMETRIC_COMMON_HLSL
#define GNX_VOLUMETRIC_COMMON_HLSL

// mu = cos(scattering angle); g = Cornette-Shanks anisotropy.
float RayleighPhaseFunction(float mu)
{
    return (3.0 / (16.0 * 3.1415926)) * (1.0 + mu * mu);
}

float MiePhaseFunction(float g, float mu)
{
    float k = 3.0 / (8.0 * 3.1415926) * (1.0 - g * g) / (2.0 + g * g);
    return k * (1.0 + mu * mu) / pow(1.0 + g * g - 2.0 * g * mu, 1.5);
}

float BeerLambertTransmittance(float opticalDepth)
{
    return exp(-opticalDepth);
}

float3 BeerLambertTransmittance(float3 opticalDepth)
{
    return exp(-opticalDepth);
}

#endif
