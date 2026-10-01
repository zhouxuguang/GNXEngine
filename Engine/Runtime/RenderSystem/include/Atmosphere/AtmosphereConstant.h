#ifndef ATMOSPHERE_CONSTANTS_H
#define ATMOSPHERE_CONSTANTS_H

#include "../RSDefine.h"
#include "Runtime/MathUtil/include/Vector3.h"
#include "Runtime/MathUtil/include/Vector2.h"
#include "Runtime/MathUtil/include/SimdMath.h"
#include "Runtime/MathUtil/include/Matrix4x4.h"

USING_NS_MATHUTIL

NS_RENDERSYSTEM_BEGIN

namespace Atmosphere
{

// 光学长度纹理size
constexpr uint32_t TRANSMITTANCE_TEXTURE_WIDTH   = 256;
constexpr uint32_t TRANSMITTANCE_TEXTURE_HEIGHT  = 64;

// 内散射(r,mu,mu_s,nu)的size
constexpr uint32_t SCATTERING_TEXTURE_R_SIZE     = 32;
constexpr uint32_t SCATTERING_TEXTURE_MU_SIZE    = 128;
constexpr uint32_t SCATTERING_TEXTURE_MU_S_SIZE  = 32;
constexpr uint32_t SCATTERING_TEXTURE_NU_SIZE    = 8;

// 内散射积分纹理size,3D纹理
constexpr uint32_t SCATTERING_TEXTURE_WIDTH      = SCATTERING_TEXTURE_NU_SIZE * SCATTERING_TEXTURE_MU_S_SIZE;
constexpr uint32_t SCATTERING_TEXTURE_HEIGHT     = SCATTERING_TEXTURE_MU_SIZE;
constexpr uint32_t SCATTERING_TEXTURE_DEPTH      = SCATTERING_TEXTURE_R_SIZE;

// 辐照度纹理size
constexpr uint32_t IRRADIANCE_TEXTURE_WIDTH      = 64;
constexpr uint32_t IRRADIANCE_TEXTURE_HEIGHT     = 16;

static constexpr double kLambdaR = 680.0;
static constexpr double kLambdaG = 550.0;
static constexpr double kLambdaB = 440.0;

static constexpr double kPi = 3.1415926;
// 太阳角半径

// 太阳立体角

// 长度单位


// 大气密度剖面：width 为米，exp_scale 和 linear_term 为每米。
struct DECLARE_ALIGNED(16) DensityProfileLayer
{
    float width;
    float exp_term;
    float exp_scale;
    float linear_term;
    float constant_term;
};

struct DensityProfile
{
    DensityProfileLayer layers[2];
};

// 大气参数：半径为米；散射和消光系数为每米。
struct AtmosphereParameters
{
    // 大气层顶部的太阳辐照度
    Vector3f solar_irradiance;
    // 太阳角半径
    float sun_angular_radius;
    // 星球中心到大气层底部的距离, 即地球半径
    float bottom_radius;
    // 星球中心到大气层顶部的距离
    float top_radius;

    // 空气分子的密度分布,[0,1]
    DensityProfile rayleigh_density;
    // 海拔为h处的rayleigh散射系数 = rayleigh_scattering * rayleigh_density
    Vector3f rayleigh_scattering;

    // 气溶胶的密度剖面,[0,1]
    DensityProfile mie_density;
    // mie散射系数 = mie_scattering * mie_density
    Vector3f mie_scattering;
    float padding1;
    // 气溶胶的消光系数 = mie_extinction * mie_density
    Vector3f mie_extinction;
    // 气溶胶的Cornette-Shanks相位函数中的非对称参数
    float mie_phase_function_g;
    // 吸收光线的空气分子的密度分布,[0,1]
    DensityProfile absorption_density;
    // 吸收光线的空气消光系数 = absorption_extinction * absorption_density
    Vector3f absorption_extinction;
    float padding2;
    // 地面的平均反照率(反射系数)
    Vector3f ground_albedo;
    // 太阳天最大天顶角的cos值(cos因此最小)，用于后面大气散射的预计算
    float mu_s_min;
};

static void ValidateAtmosphereParametersOffsets()
{
    static_assert(offsetof(AtmosphereParameters, solar_irradiance) == 0, "solar_irradiance offset error");
    static_assert(offsetof(AtmosphereParameters, sun_angular_radius) == 12, "sun_angular_radius offset error");
    static_assert(offsetof(AtmosphereParameters, bottom_radius) == 16, "bottom_radius offset error");
    static_assert(offsetof(AtmosphereParameters, top_radius) == 20, "top_radius offset error");
    static_assert(offsetof(AtmosphereParameters, rayleigh_density) == 32, "rayleigh_density offset error");
    static_assert(offsetof(AtmosphereParameters, rayleigh_scattering) == 96, "rayleigh_scattering offset error");
    static_assert(offsetof(AtmosphereParameters, mie_density) == 112, "mie_density offset error");
    static_assert(offsetof(AtmosphereParameters, mie_scattering) == 176, "mie_scattering offset error");
    static_assert(offsetof(AtmosphereParameters, mie_extinction) == 192, "mie_extinction offset error");
    static_assert(offsetof(AtmosphereParameters, mie_phase_function_g) == 204, "mie_phase_function_g offset error");
    static_assert(offsetof(AtmosphereParameters, absorption_density) == 208, "absorption_density offset error");
    static_assert(offsetof(AtmosphereParameters, absorption_extinction) == 272, "absorption_extinction offset error");
    static_assert(offsetof(AtmosphereParameters, ground_albedo) == 288, "ground_albedo offset error");
    static_assert(offsetof(AtmosphereParameters, mu_s_min) == 300, "mu_s_min offset error");
    static_assert(sizeof(AtmosphereParameters) == 304, "AtmosphereParameters Size mismatch");
}

// 视角数据均以行星中心为原点，长度单位为米。
struct DECLARE_ALIGNED(16) AtmosphereViewParams
{
    Matrix4x4f inv_view_proj;       // 不含世界平移的逆视图投影矩阵
    simd_float4 camera_pos_exposure; // xyz=相机相对行星中心(米), w=曝光
    simd_float4 sun_direction_pad;
    simd_float4 sun_size_pad;
    simd_float4 white_point_pad;
};

struct DECLARE_ALIGNED(16) AtmospherePlanetParams
{
    simd_float4 ground_radii_top_height; // xyz=椭球半轴(米), w=大气厚度(米)
    simd_float4 camera_up_altitude;      // xyz=双精度计算的大地法线, w=相机大地高(米)
};

static_assert(sizeof(AtmospherePlanetParams) == 32);

static void ValidateAtmosphereViewParamsOffsets()
{
    static_assert(offsetof(AtmosphereViewParams, camera_pos_exposure) == 64);
    static_assert(offsetof(AtmosphereViewParams, sun_direction_pad) == 80);
    static_assert(offsetof(AtmosphereViewParams, sun_size_pad) == 96);
    static_assert(offsetof(AtmosphereViewParams, white_point_pad) == 112);
    static_assert(sizeof(AtmosphereViewParams) == 128);
}

// 预计算散射 Pass 的参数（与 ComputeSingleScattering/ScatteringDensity/MultipleScattering
// 中 ScatteringCB 一一对应）
struct AtmosphereScatteringParams
{
    int layer;              // 当前散射层（3D 纹理切片）
    int scattering_order;   // 当前散射重数
    int pad0;
    int pad1;
};

}

NS_RENDERSYSTEM_END

#endif // ATMOSPHERE_CONSTANTS_H
