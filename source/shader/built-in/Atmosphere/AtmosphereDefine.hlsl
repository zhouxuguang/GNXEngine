#ifndef GNX_ENGINE_ATMOSPHERE_COMMON_DEFINE_INCLUDE
#define GNX_ENGINE_ATMOSPHERE_COMMON_DEFINE_INCLUDE

#define PI 3.14159265358979323846

// 光学长度纹理size
#define TRANSMITTANCE_TEXTURE_WIDTH 256
#define TRANSMITTANCE_TEXTURE_HEIGHT 64

// 内散射(r,mu,mu_s,nu)的size
#define SCATTERING_TEXTURE_R_SIZE 32
#define SCATTERING_TEXTURE_MU_SIZE 128
#define SCATTERING_TEXTURE_MU_S_SIZE 32
#define SCATTERING_TEXTURE_NU_SIZE 8

// 辐照度纹理size
#define IRRADIANCE_TEXTURE_WIDTH 64
#define IRRADIANCE_TEXTURE_HEIGHT 16

// 辐照度纹理大小
#define IRRADIANCE_TEXTURE_SIZE float2(IRRADIANCE_TEXTURE_WIDTH, IRRADIANCE_TEXTURE_HEIGHT)

//大气层密度剖面
struct DensityProfileLayer 
{
	[[vk::offset(0)]] float width;
	[[vk::offset(4)]] float exp_term;
	[[vk::offset(8)]] float exp_scale;
	[[vk::offset(12)]] float linear_term;
	[[vk::offset(16)]] float constant_term;
};

struct DensityProfile 
{
	DensityProfileLayer layers[2];
};

//大气层参数模型
struct AtmosphereParameters
{
	// 太阳相关参数
	// 大气层顶部的太阳辐照度
	[[vk::offset(0)]] float3 solar_irradiance;
	// 太阳角半径
  	[[vk::offset(12)]] float sun_angular_radius;

	// 星球相关参数
  	// 星球中心到大气层底部的距离, 即地球半径
  	[[vk::offset(16)]] float bottom_radius;
  	// 星球中心到大气层顶部的距离
  	[[vk::offset(20)]] float top_radius;

	// Rayleigh散射系数
  	// 空气分子的密度分布,[0,1]
  	[[vk::offset(32)]] DensityProfile rayleigh_density;
  	// 海拔为h处的rayleigh散射系数 = rayleigh_scattering * rayleigh_density
  	[[vk::offset(96)]] float3 rayleigh_scattering;

	// Mie散射
  	// 气溶胶的密度剖面,[0,1]
  	[[vk::offset(112)]] DensityProfile mie_density;
	// mie散射系数 = mie_scattering * mie_density
  	[[vk::offset(176)]] float3 mie_scattering;
  	// 气溶胶的消光系数 = mie_extinction * mie_density
  	[[vk::offset(192)]] float3 mie_extinction;
  	// 气溶胶的Cornette-Shanks相位函数中的非对称参数
  	[[vk::offset(204)]] float mie_phase_function_g;
  	// 吸收光线的空气分子的密度分布,[0,1]
  	[[vk::offset(208)]] DensityProfile absorption_density;
  	// 吸收光线的空气消光系数 = absorption_extinction * absorption_density
  	[[vk::offset(272)]] float3 absorption_extinction;
  	// 地面的平均反照率(反射系数)
  	[[vk::offset(288)]] float3 ground_albedo;
  	// 太阳天最大天顶角的cos值(cos因此最小)，用于后面大气散射的预计算
  	[[vk::offset(300)]] float mu_s_min;
};

#endif