#ifndef GNX_VOLUMETRIC_COMMON_HLSL
#define GNX_VOLUMETRIC_COMMON_HLSL

#define M_PI 3.14159265359

// mu = cos(scattering angle); g = Cornette-Shanks anisotropy.
float RayleighPhaseFunction(float mu)
{
    return (3.0 / (16.0 * M_PI)) * (1.0 + mu * mu);
}

float MiePhaseFunction(float g, float mu)
{
    float k = 3.0 / (8.0 * M_PI) * (1.0 - g * g) / (2.0 + g * g);
    return k * (1.0 + mu * mu) / pow(1.0 + g * g - 2.0 * g * mu, 1.5);
}

float HenyeyGreenstein(float g, float costh)
{
    return (1.0 - g * g) / (4.0 * M_PI * pow(1.0 + g * g - 2.0 * g * costh, 3.0/2.0));
}

float Schlick(float k, float costh)
{
    return (1.0 - k * k) / (4.0 * M_PI * pow(1.0 - k * costh, 2.0));
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
