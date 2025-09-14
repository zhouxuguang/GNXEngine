#ifndef ATMOSPHERE_CONSTANTS_H
#define ATMOSPHERE_CONSTANTS_H

#include "RSDefine.h"
#include "MathUtil/Vector3.h"

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
static constexpr double kSunAngularRadius = 0.00935 / 2.0;
// 太阳立体角
static constexpr double kSunSolidAngle = kPi * kSunAngularRadius * kSunAngularRadius;
// 长度单位
static constexpr double kLengthUnitInMeters = 1000.0;

//大气层密度剖面
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

//大气层参数模型
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

}

NS_RENDERSYSTEM_END

#endif // ATMOSPHERE_CONSTANTS_H
