#ifndef GNX_NEW_SKY_ATMOSPHERE_COMMON_HLSL
#define GNX_NEW_SKY_ATMOSPHERE_COMMON_HLSL

// Hillaire, EGSR 2020 / UE SkyAtmosphere. The public engine parameters remain
// in metres; all ray intersections and optical integrals below use kilometres.
#include "AtmosphereDefine.hlsl"

#define NEW_SKY_TRANSMITTANCE_WIDTH 256
#define NEW_SKY_TRANSMITTANCE_HEIGHT 64
#define NEW_SKY_MULTI_WIDTH 32
#define NEW_SKY_MULTI_HEIGHT 32
#define NEW_SKY_VIEW_WIDTH 192
#define NEW_SKY_VIEW_HEIGHT 104
#define NEW_SKY_AERIAL_WIDTH 32
#define NEW_SKY_AERIAL_HEIGHT 32
#define NEW_SKY_AERIAL_DEPTH 16
#define NEW_SKY_DEFAULT_AERIAL_RANGE_KM 96.0
// UE 5.3 sampling defaults (r.SkyAtmosphere.*). The effective sample count
// lerps from min to max with the marched distance in km.
#define NEW_SKY_SAMPLE_COUNT_MIN 2.0
#define NEW_SKY_SKYVIEW_SAMPLE_COUNT_MIN 4.0
#define NEW_SKY_SAMPLE_COUNT_MAX 32.0
#define NEW_SKY_DISTANCE_TO_SAMPLE_COUNT_MAX_KM 150.0
#define NEW_SKY_AERIAL_SAMPLES_PER_SLICE 2.0
#define NEW_SKY_SAMPLE_OFFSET 0.3

cbuffer AtmosphereParametersCB : register(b0) { AtmosphereParameters ATMOSPHERE; };
#ifdef NEW_SKY_USE_VIEW
cbuffer AtmosphereViewCB : register(b1)
{
    float4x4 inv_view_proj;
    float4 camera_pos_exposure;
    float4 sun_direction_pad;
    float4 sun_size_pad;
    float4 white_point_pad;
};
#endif
#ifdef NEW_SKY_USE_PLANET
cbuffer AtmospherePlanetCB : register(b2)
{
    float4 ground_radii_top_height;
    float4 camera_up_altitude;
};
#endif
#ifdef NEW_SKY_USE_PASS
cbuffer NewSkyLutCB : register(b3)
{
    // x: aerial slice (0..15); y: aerial range in km (96); z: slice count (16).
    float4 new_sky_lut_params;
};
#endif

#ifdef NEW_SKY_USE_TRANSMITTANCE
Texture2D new_sky_transmittance_texture;
SamplerState new_sky_transmittance_textureSam;
#endif
#ifdef NEW_SKY_USE_MULTISCATTERING
Texture2D new_sky_multiscattering_texture;
SamplerState new_sky_multiscattering_textureSam;
#endif
#ifdef NEW_SKY_USE_SKYVIEW_TEXTURE
Texture2D new_sky_view_texture;
SamplerState new_sky_view_textureSam;
#endif
#ifdef NEW_SKY_USE_AERIAL_TEXTURE
Texture3D new_sky_aerial_texture;
SamplerState new_sky_aerial_textureSam;
#endif

struct NewSkyVertexOutput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

NewSkyVertexOutput NewSkyFullscreenVertex(uint vertexID)
{
    NewSkyVertexOutput o;
    o.position = float4(-1.0 + float((vertexID & 1) << 2),
                        -1.0 + float((vertexID & 2) << 1), 0.0, 1.0);
    o.uv = float2(vertexID == 1 ? 2.0 : 0.0,
                  vertexID == 2 ? 2.0 : 0.0);
#ifdef TEXCOORD_FLIP
    o.uv.y = 1.0 - o.uv.y;
#endif
    return o;
}

float NewSkyBottomKm() { return ATMOSPHERE.bottom_radius * 0.001; }
float NewSkyTopKm() { return ATMOSPHERE.top_radius * 0.001; }

#ifdef NEW_SKY_USE_PLANET
float3 NewSkyUp()
{
    float3 up = camera_up_altitude.xyz;
    return dot(up, up) > 1e-8 ? normalize(up) : float3(0.0, 1.0, 0.0);
}

float NewSkyProxyHeightKm()
{
    // The 1 m offset keeps a ground-level downward ray outside the ground.
    return NewSkyBottomKm() + max(camera_up_altitude.w * 0.001, 0.0) + 0.001;
}

float3 NewSkyProxyCameraKm() { return NewSkyUp() * NewSkyProxyHeightKm(); }
#endif

float NewSkyDensityLayer(DensityProfileLayer layer, float heightM)
{
    return saturate(layer.exp_term * exp(layer.exp_scale * heightM)
                  + layer.linear_term * heightM + layer.constant_term);
}

float NewSkyDensity(DensityProfile profile, float altitudeKm)
{
    float h = max(altitudeKm, 0.0) * 1000.0;
    return h < profile.layers[0].width
        ? NewSkyDensityLayer(profile.layers[0], h)
        : NewSkyDensityLayer(profile.layers[1], h);
}

struct NewSkyMedium
{
    float3 rayleigh;
    float3 mie;
    float3 scattering;
    float3 extinction;
};

NewSkyMedium NewSkySampleMedium(float3 positionKm)
{
    float altitudeKm = max(length(positionKm) - NewSkyBottomKm(), 0.0);
    float rayDensity = NewSkyDensity(ATMOSPHERE.rayleigh_density, altitudeKm);
    float mieDensity = NewSkyDensity(ATMOSPHERE.mie_density, altitudeKm);
    float absorptionDensity = NewSkyDensity(ATMOSPHERE.absorption_density, altitudeKm);
    NewSkyMedium m;
    m.rayleigh = ATMOSPHERE.rayleigh_scattering * (1000.0 * rayDensity);
    m.mie = ATMOSPHERE.mie_scattering * (1000.0 * mieDensity);
    m.scattering = m.rayleigh + m.mie;
    m.extinction = m.rayleigh
                 + ATMOSPHERE.mie_extinction * (1000.0 * mieDensity)
                 + ATMOSPHERE.absorption_extinction * (1000.0 * absorptionDensity);
    return m;
}

bool NewSkyRaySphere(float3 originKm, float3 ray, float radiusKm,
                     out float nearT, out float farT)
{
    float b = dot(originKm, ray);
    float c = dot(originKm, originKm) - radiusKm * radiusKm;
    float discriminant = b * b - c;
    if (discriminant < 0.0)
    {
        nearT = -1.0;
        farT = -1.0;
        return false;
    }
    float s = sqrt(max(discriminant, 0.0));
    nearT = -b - s;
    farT = -b + s;
    return farT > 0.0;
}

float NewSkyGroundDistance(float3 originKm, float3 ray)
{
    float nearT, farT;
    if (!NewSkyRaySphere(originKm, ray, NewSkyBottomKm(), nearT, farT))
        return -1.0;
    return nearT > 0.00001 ? nearT : -1.0;
}

// For a camera in orbit, enter the top sphere before evaluating the medium.
bool NewSkyEnterAtmosphere(inout float3 originKm, float3 ray,
                           inout float maxDistanceKm)
{
    if (length(originKm) <= NewSkyTopKm() - 0.0001)
        return true;
    float nearT, farT;
    if (!NewSkyRaySphere(originKm, ray, NewSkyTopKm(), nearT, farT))
        return false;
    float entry = max(nearT, 0.0) + 0.0001;
    if (maxDistanceKm >= 0.0 && entry >= maxDistanceKm)
        return false;
    originKm += ray * entry;
    if (maxDistanceKm >= 0.0)
        maxDistanceKm -= entry;
    return true;
}

float NewSkyRayleighPhase(float c)
{
    return 3.0 * (1.0 + c * c) / (16.0 * PI);
}

float NewSkyMiePhase(float c)
{
    // Standard forward-scattering HG; c = view ray dot direction to sun.
    float g = clamp(ATMOSPHERE.mie_phase_function_g, -0.99, 0.99);
    float d = max(1.0 + g * g - 2.0 * g * c, 0.0001);
    return (1.0 - g * g) / (4.0 * PI * d * sqrt(d));
}

void NewSkyTransmittanceParams(float2 uv, out float radiusKm, out float mu)
{
    float bottom = NewSkyBottomKm();
    float top = NewSkyTopKm();
    float H = sqrt(max(top * top - bottom * bottom, 0.0));
    float rho = H * saturate(uv.y);
    radiusKm = sqrt(rho * rho + bottom * bottom);
    float dMin = top - radiusKm;
    float dMax = rho + H;
    float d = dMin + saturate(uv.x) * (dMax - dMin);
    mu = d <= 1e-6 ? 1.0 : clamp((H * H - rho * rho - d * d)
                              / (2.0 * radiusKm * d), -1.0, 1.0);
}

float2 NewSkyTransmittanceUv(float radiusKm, float mu)
{
    float bottom = NewSkyBottomKm();
    float top = NewSkyTopKm();
    float H = sqrt(max(top * top - bottom * bottom, 0.0));
    float rho = sqrt(max(radiusKm * radiusKm - bottom * bottom, 0.0));
    float discriminant = radiusKm * radiusKm * (mu * mu - 1.0) + top * top;
    float d = max(-radiusKm * mu + sqrt(max(discriminant, 0.0)), 0.0);
    float dMin = top - radiusKm;
    float dMax = rho + H;
    return saturate(float2((d - dMin) / max(dMax - dMin, 1e-6),
                           rho / max(H, 1e-6)));
}

#ifdef NEW_SKY_USE_TRANSMITTANCE
float3 NewSkySunTransmittance(float3 positionKm, float3 sunDirection)
{
    if (NewSkyGroundDistance(positionKm, sunDirection) > 0.0)
        return float3(0.0, 0.0, 0.0);
    float radiusKm = length(positionKm);
    if (radiusKm >= NewSkyTopKm())
        return float3(1.0, 1.0, 1.0);
    float mu = dot(positionKm, sunDirection) / max(radiusKm, 1e-6);
    float2 uv = NewSkyTransmittanceUv(radiusKm, mu);
    return new_sky_transmittance_texture.SampleLevel(
        new_sky_transmittance_textureSam, uv, 0).rgb;
}
#endif

#ifdef NEW_SKY_USE_MULTISCATTERING
float3 NewSkyMultipleScattering(float3 positionKm, float3 sunDirection)
{
    float radiusKm = length(positionKm);
    float mu = dot(positionKm, sunDirection) / max(radiusKm, 1e-6);
    float2 uv = saturate(float2(0.5 + 0.5 * mu,
        (radiusKm - NewSkyBottomKm()) / max(NewSkyTopKm() - NewSkyBottomKm(), 1e-6)));
    return new_sky_multiscattering_texture.SampleLevel(
        new_sky_multiscattering_textureSam, uv, 0).rgb;
}
#endif

#ifdef NEW_SKY_USE_TRANSMITTANCE
struct NewSkyIntegration
{
    float3 luminance;       // Per unit solar irradiance.
    float3 transmittance;
    float3 uniformTransfer; // Unit isotropic illumination transfer, for the MS LUT.
    bool groundHit;
};

NewSkyIntegration NewSkyIntegrate(float3 originKm, float3 ray,
                                  float3 sunDirection, float maxDistanceKm,
                                  float sampleCount, bool isotropicPhase,
                                  bool includeMultiScattering,
                                  bool includeGroundBounce,
                                  float minSampleCount = 0.0,
                                  float maxSampleCount = 0.0)
{
    NewSkyIntegration result;
    result.luminance = float3(0.0, 0.0, 0.0);
    result.transmittance = float3(1.0, 1.0, 1.0);
    result.uniformTransfer = float3(0.0, 0.0, 0.0);
    result.groundHit = false;
    if (!NewSkyEnterAtmosphere(originKm, ray, maxDistanceKm))
        return result;

    float nearTop, farTop;
    if (!NewSkyRaySphere(originKm, ray, NewSkyTopKm(), nearTop, farTop))
        return result;
    float endT = farTop;
    float groundT = NewSkyGroundDistance(originKm, ray);
    if (groundT > 0.0 && groundT < endT)
    {
        endT = groundT;
        result.groundHit = true;
    }
    if (maxDistanceKm >= 0.0 && maxDistanceKm < endT)
    {
        endT = maxDistanceKm;
        result.groundHit = false;
    }

    // UE 5.3 IntegrateSingleScatteredLuminance: adaptive count with quadratic
    // sample distribution, or the fixed linear scheme for LUT passes.
    const bool variableCount = maxSampleCount > 0.0;
    if (variableCount)
        sampleCount = lerp(minSampleCount, maxSampleCount,
            saturate(endT / NEW_SKY_DISTANCE_TO_SAMPLE_COUNT_MAX_KM));
    sampleCount = max(sampleCount, 1.0);
    float sampleCountFloor = variableCount ? floor(sampleCount) : sampleCount;
    sampleCountFloor = max(sampleCountFloor, 1.0);
    float endTFloor = endT;
    if (variableCount)
        endTFloor = endT * sampleCountFloor / sampleCount;

    float c = clamp(dot(ray, sunDirection), -1.0, 1.0);
    float rayPhase = isotropicPhase ? 1.0 / (4.0 * PI) : NewSkyRayleighPhase(c);
    float miePhase = isotropicPhase ? 1.0 / (4.0 * PI) : NewSkyMiePhase(c);
    [loop]
    for (int i = 0; float(i) < sampleCount; ++i)
    {
        float t0, t1;
        if (variableCount)
        {
            t0 = float(i) / sampleCountFloor;
            t1 = float(i + 1) / sampleCountFloor;
            t0 *= t0;
            t1 *= t1;
            t0 *= endTFloor;
            t1 = t1 > 1.0 ? endT : endTFloor * t1;
        }
        else
        {
            t0 = endT * float(i) / sampleCount;
            t1 = endT * float(i + 1) / sampleCount;
        }
        float segmentKm = t1 - t0;
        float3 p = originKm + ray * lerp(t0, t1, NEW_SKY_SAMPLE_OFFSET);
        NewSkyMedium medium = NewSkySampleMedium(p);
        float3 opticalStep = medium.extinction * segmentKm;
        float3 stepTransmittance = exp(-opticalStep);
        float3 segment = (1.0 - stepTransmittance) / max(medium.extinction, 1e-6);
        float3 sunTransmittance = NewSkySunTransmittance(p, sunDirection);
        float3 source = sunTransmittance *
            (medium.rayleigh * rayPhase + medium.mie * miePhase);
#ifdef NEW_SKY_USE_MULTISCATTERING
        if (includeMultiScattering)
            source += medium.scattering * NewSkyMultipleScattering(p, sunDirection);
#endif
        result.luminance += result.transmittance * source * segment;
        result.uniformTransfer += result.transmittance * medium.scattering * segment;
        result.transmittance *= stepTransmittance;
        if (max(result.transmittance.x, max(result.transmittance.y,
            result.transmittance.z)) < 0.01)
            break;
    }
    if (includeGroundBounce && result.groundHit)
    {
        float3 p = originKm + ray * endT;
        float3 up = normalize(p);
        float sunCos = max(dot(up, sunDirection), 0.0);
        result.luminance += result.transmittance * ATMOSPHERE.ground_albedo
            * NewSkySunTransmittance(p + up * 0.001, sunDirection)
            * (sunCos / PI);
    }
    return result;
}
#endif

#ifdef NEW_SKY_USE_VIEW
float3 NewSkyScreenRay(float2 uv)
{
#ifdef TEXCOORD_FLIP
    uv.y = 1.0 - uv.y;
#endif
    float4 clip = float4(uv * 2.0 - 1.0, 0.5, 1.0);
    float4 rayPoint = mul(clip, inv_view_proj);
    return normalize(rayPoint.xyz / rayPoint.w);
}

float3 NewSkyScreenPoint(float2 uv, float depth)
{
#ifdef TEXCOORD_FLIP
    uv.y = 1.0 - uv.y;
#endif
    float4 surfacePoint = mul(float4(uv * 2.0 - 1.0, depth, 1.0), inv_view_proj);
    return surfacePoint.xyz / surfacePoint.w;
}
#endif

float2 NewSkyFromUnitToSubUv(float2 uv, float2 size)
{
    return (uv * (size - 1.0) + 0.5) / size;
}

float2 NewSkyFromSubToUnitUv(float2 uv, float2 size)
{
    return saturate((uv * size - 0.5) / (size - 1.0));
}

float NewSkyHorizonZenith(float viewRadiusKm)
{
    float beta = acos(sqrt(max(viewRadiusKm * viewRadiusKm
        - NewSkyBottomKm() * NewSkyBottomKm(), 0.0)) / viewRadiusKm);
    return PI - beta;
}

float3 NewSkySkyUvToLocalRay(float2 subUv, float viewRadiusKm)
{
    float2 uv = NewSkyFromSubToUnitUv(subUv,
        float2(NEW_SKY_VIEW_WIDTH, NEW_SKY_VIEW_HEIGHT));
    float horizon = NewSkyHorizonZenith(viewRadiusKm);
    float theta;
    if (uv.y < 0.5)
    {
        float a = 1.0 - 2.0 * uv.y;
        theta = horizon * (1.0 - a * a);
    }
    else
    {
        float a = 2.0 * uv.y - 1.0;
        theta = horizon + (PI - horizon) * a * a;
    }
    float phi = 2.0 * PI * uv.x;
    return float3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
}

float2 NewSkyLocalRayToSkyUv(float3 localRay, float viewRadiusKm)
{
    float theta = acos(clamp(localRay.z, -1.0, 1.0));
    float horizon = NewSkyHorizonZenith(viewRadiusKm);
    float v = theta <= horizon
        ? 0.5 * (1.0 - sqrt(saturate(1.0 - theta / horizon)))
        : 0.5 + 0.5 * sqrt(saturate((theta - horizon) / max(PI - horizon, 1e-6)));
    float phi = atan2(localRay.y, localRay.x);
    float u = frac(phi / (2.0 * PI) + 1.0);
    return NewSkyFromUnitToSubUv(float2(u, v),
        float2(NEW_SKY_VIEW_WIDTH, NEW_SKY_VIEW_HEIGHT));
}

float3 NewSkyToSunLocal(float3 ray, float3 up, float3 sunDirection)
{
    float3 east = sunDirection - dot(sunDirection, up) * up;
    if (dot(east, east) < 1e-8)
        east = abs(up.y) < 0.9 ? cross(float3(0.0, 1.0, 0.0), up)
                               : cross(float3(1.0, 0.0, 0.0), up);
    east = normalize(east);
    float3 north = normalize(cross(up, east));
    return float3(dot(ray, east), dot(ray, north), dot(ray, up));
}

float NewSkyAerialRangeKm()
{
    // Keep composition independent of b3; the renderer allocates a fixed 96 km LUT.
    return NEW_SKY_DEFAULT_AERIAL_RANGE_KM;
}

#ifdef NEW_SKY_USE_AERIAL_TEXTURE
float4 NewSkySampleAerial(float2 uv, float distanceKm)
{
    float rangeKm = NewSkyAerialRangeKm();
    float w = sqrt(saturate(distanceKm / rangeKm));
    float4 aerial = new_sky_aerial_texture.SampleLevel(
        new_sky_aerial_textureSam, float3(uv, w), 0);
    float slice = w * float(NEW_SKY_AERIAL_DEPTH);
    float weight = saturate(slice * slice * 2.0);
    aerial.rgb *= weight;
    aerial.a = 1.0 - weight * (1.0 - aerial.a);
    return aerial;
}
#endif

float3 NewSkyProxyRay(float3 worldRay, float3 up, float mu)
{
    float3 horizontal = worldRay - dot(worldRay, up) * up;
    if (dot(horizontal, horizontal) < 1e-8)
        horizontal = abs(up.y) < 0.9 ? cross(up, float3(0.0, 1.0, 0.0))
                                       : cross(up, float3(1.0, 0.0, 0.0));
    horizontal = normalize(horizontal);
    return horizontal * sqrt(max(1.0 - mu * mu, 0.0)) + up * mu;
}

#ifdef NEW_SKY_USE_PLANET
float NewSkyProxyCosine(float3 worldRay, float3 up, float distanceKm,
                        float endpointRadiusKm)
{
    float direct = dot(worldRay, up);
    if (distanceKm < 1.0)
        return direct;
    float cameraRadius = NewSkyProxyHeightKm();
    float fitted = (endpointRadiusKm * endpointRadiusKm - cameraRadius * cameraRadius
                  - distanceKm * distanceKm) / (2.0 * cameraRadius * distanceKm);
    return clamp(lerp(direct, clamp(fitted, -1.0, 1.0),
                      smoothstep(1.0, 20.0, distanceKm)), -1.0, 1.0);
}

float NewSkyGroundProxyCosine(float3 worldRay, float3 up, float distanceKm)
{
    return NewSkyProxyCosine(worldRay, up, distanceKm, NewSkyBottomKm());
}
#endif

float NewSkyGeodeticHeightM(float3 positionM, float3 radiiM)
{
    // Closest ellipsoid point (the same iterative fit used by the old planet
    // composite), evaluated only for scene-depth pixels with distant terrain.
    float3 radii2 = radiiM * radiiM;
    float shortest = min(radiiM.x, min(radiiM.y, radiiM.z));
    float lambda = (length(positionM) - shortest) * shortest;
    [unroll]
    for (int i = 0; i < 6; ++i)
    {
        float3 q = max(radii2 + lambda, float3(1.0, 1.0, 1.0));
        float3 n = positionM * positionM * radii2;
        float f = dot(n / (q * q), float3(1.0, 1.0, 1.0)) - 1.0;
        float df = -2.0 * dot(n / (q * q * q), float3(1.0, 1.0, 1.0));
        lambda -= f / min(df, -1e-20);
    }
    float3 surface = positionM * radii2 / (radii2 + lambda);
    float3 normal = normalize(surface / radii2);
    return dot(positionM - surface, normal);
}

bool NewSkyIntersectEllipsoid(float3 originM, float3 ray, float3 radiiM,
                             out float distanceM)
{
    float3 o = originM / radiiM;
    float3 d = ray / radiiM;
    float a = dot(d, d);
    float b = dot(o, d);
    float c = dot(o, o) - 1.0;
    float discriminant = b * b - a * c;
    if (discriminant < 0.0)
    {
        distanceM = -1.0;
        return false;
    }
    float root = sqrt(max(discriminant, 0.0));
    float nearT = (-b - root) / a;
    float farT = (-b + root) / a;
    distanceM = nearT > 0.0 ? nearT : farT;
    return distanceM > 0.0;
}

#ifdef NEW_SKY_USE_SKYVIEW_TEXTURE
float3 NewSkyApproximateSkyIrradiance(float3 sunDirection)
{
    float3 up = NewSkyUp();
    float3 localUp = NewSkyToSunLocal(up, up, sunDirection);
    float2 uv = NewSkyLocalRayToSkyUv(localUp, NewSkyProxyHeightKm());
    // PI times a representative sky radiance; the Lambert BRDF divides by PI.
    return PI * new_sky_view_texture.SampleLevel(new_sky_view_textureSam, uv, 0).rgb;
}

float3 NewSkyLambertSurface(float3 surfaceKm, float3 normal,
                            float3 albedo, float3 sunDirection)
{
    float3 direct = ATMOSPHERE.solar_irradiance
        * NewSkySunTransmittance(surfaceKm + normal * 0.001, sunDirection)
        * max(dot(normal, sunDirection), 0.0);
    float3 skyIrradiance = NewSkyApproximateSkyIrradiance(sunDirection);
    return albedo * (direct + skyIrradiance) / PI;
}
#endif

#ifdef NEW_SKY_USE_AERIAL_TEXTURE
float4 NewSkyAerialForRay(float2 uv, float3 worldRay,
                          float distanceKm, float3 sunDirection)
{
    // UE 5.3 ray marches opaque pixels when the camera is outside the
    // atmosphere (ForceRayMarching) or when FastApplyOnOpaque is disabled.
    // The map's planet-scale horizon needs the marched path: extrapolating
    // the AP LUT's last slice past 96 km breaks the fade into the horizon.
    if (NewSkyProxyHeightKm() > NewSkyTopKm()
        || distanceKm > NEW_SKY_DEFAULT_AERIAL_RANGE_KM)
    {
        NewSkyIntegration full = NewSkyIntegrate(NewSkyProxyCameraKm(),
            worldRay, sunDirection, distanceKm, 0.0, false, true, false,
            NEW_SKY_SAMPLE_COUNT_MIN, NEW_SKY_SAMPLE_COUNT_MAX);
        float alpha = dot(full.transmittance,
            float3(1.0 / 3.0, 1.0 / 3.0, 1.0 / 3.0));
        return float4(full.luminance * ATMOSPHERE.solar_irradiance, alpha);
    }
    return NewSkySampleAerial(uv, distanceKm);
}
#endif

#ifdef NEW_SKY_USE_SKYVIEW_TEXTURE
float3 NewSkySkyRadiance(float3 worldRay, float3 sunDirection)
{
    float3 proxyCamera = NewSkyProxyCameraKm();
    float3 transmittance;
    float3 radiance;
    if (NewSkyProxyHeightKm() > NewSkyTopKm())
    {
        NewSkyIntegration full = NewSkyIntegrate(proxyCamera, worldRay,
            sunDirection, -1.0, 0.0, false, true, false,
            NEW_SKY_SAMPLE_COUNT_MIN, NEW_SKY_SAMPLE_COUNT_MAX);
        radiance = full.luminance * ATMOSPHERE.solar_irradiance;
        transmittance = full.transmittance;
    }
    else
    {
        float3 localRay = NewSkyToSunLocal(worldRay, NewSkyUp(), sunDirection);
        float2 skyUv = NewSkyLocalRayToSkyUv(localRay, NewSkyProxyHeightKm());
        radiance = new_sky_view_texture.SampleLevel(
            new_sky_view_textureSam, skyUv, 0).rgb;
        transmittance = NewSkySunTransmittance(proxyCamera, sunDirection);
    }
    if (dot(worldRay, sunDirection) > sun_size_pad.y)
    {
        float solidAngle = PI * max(ATMOSPHERE.sun_angular_radius
            * ATMOSPHERE.sun_angular_radius, 1e-8);
        radiance += transmittance * ATMOSPHERE.solar_irradiance / solidAngle;
    }
    return radiance;
}
#endif

#endif
