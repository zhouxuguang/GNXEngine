// Scene composition for Hillaire/UE sky atmosphere. The scene color is HDR;
// RGB LUT values are irradiance-normalized until multiplied in the LUT passes.
#define NEW_SKY_USE_VIEW 1
#define NEW_SKY_USE_PLANET 1
#define NEW_SKY_USE_TRANSMITTANCE 1
#define NEW_SKY_USE_MULTISCATTERING 1
#define NEW_SKY_USE_SKYVIEW_TEXTURE 1
#define NEW_SKY_USE_AERIAL_TEXTURE 1
#include "NewSkyAtmosphereCommon.hlsl"

Texture2D scene_color_texture;
SamplerState scene_color_textureSam;
Texture2D scene_depth_texture;
SamplerState scene_depth_textureSam;

[shader("vertex")]
NewSkyVertexOutput VS(uint vertexID : SV_VertexID)
{
    return NewSkyFullscreenVertex(vertexID);
}

[shader("pixel")]
float4 PS(NewSkyVertexOutput input) : SV_Target0
{
    int2 pixel = int2(input.position.xy);
    float3 scene = scene_color_texture.Load(int3(pixel, 0)).rgb;
    float depth = scene_depth_texture.Load(int3(pixel, 0)).r;
#ifdef USE_REVERSE_Z
    bool hasSurface = depth > 0.0;
#else
    bool hasSurface = depth < 1.0;
#endif
    float3 sun = normalize(sun_direction_pad.xyz);
    float3 worldRay = NewSkyScreenRay(input.uv);
    float exposure = camera_pos_exposure.w;

    if (hasSurface)
    {
        float3 cameraToSurfaceM = NewSkyScreenPoint(input.uv, depth);
        float distanceKm = length(cameraToSurfaceM) * 0.001;
        float3 proxyRay = worldRay;
        if (distanceKm > NEW_SKY_DEFAULT_AERIAL_RANGE_KM ||
            NewSkyProxyHeightKm() > NewSkyTopKm())
        {
            float3 endpointM = camera_pos_exposure.xyz + cameraToSurfaceM;
            float endpointAltitudeKm = NewSkyGeodeticHeightM(endpointM,
                ground_radii_top_height.xyz) * 0.001;
            float mu = NewSkyProxyCosine(worldRay, NewSkyUp(), distanceKm,
                NewSkyBottomKm() + max(endpointAltitudeKm, 0.0));
            proxyRay = NewSkyProxyRay(worldRay, NewSkyUp(), mu);
        }
        float4 aerial = NewSkyAerialForRay(input.uv, proxyRay,
            distanceKm, sun);
        return float4(scene * aerial.a + aerial.rgb * exposure, 1.0);
    }

    // The map uses an actual WGS84 ellipsoid. The LUTs use the spherical
    // atmosphere at the camera's geodetic height and up direction.
    float groundDistanceM;
    if (NewSkyIntersectEllipsoid(camera_pos_exposure.xyz, worldRay,
                                  ground_radii_top_height.xyz, groundDistanceM))
    {
        float distanceKm = groundDistanceM * 0.001;
        float3 up = NewSkyUp();
        float mu = NewSkyGroundProxyCosine(worldRay, up, distanceKm);
        float3 proxyRay = NewSkyProxyRay(worldRay, up, mu);
        float3 proxyGround = NewSkyProxyCameraKm() + proxyRay * distanceKm;
        float3 actualGroundM = camera_pos_exposure.xyz + worldRay * groundDistanceM;
        float3 actualNormal = normalize(actualGroundM /
            (ground_radii_top_height.xyz * ground_radii_top_height.xyz));
        float3 groundRadiance = NewSkyLambertSurface(proxyGround, actualNormal,
                                                     ATMOSPHERE.ground_albedo, sun);
        float4 aerial = NewSkyAerialForRay(input.uv, proxyRay,
            distanceKm, sun);
        return float4((groundRadiance * aerial.a + aerial.rgb) * exposure, 1.0);
    }

    float3 sky = NewSkySkyRadiance(worldRay, sun);
    return float4(scene + sky * exposure, 1.0);
}
