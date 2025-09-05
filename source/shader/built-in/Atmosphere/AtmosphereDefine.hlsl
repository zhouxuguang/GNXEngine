
#define m 1.0          //米
#define nm 1.0         //纳米
#define rad 1.0         //弧度
#define sr 1.0        //立体弧度
#define watt 1.0       //瓦特

#define PI 3.14159265358979323846

#define km (1000.0 * m) //千米
#define m2 (m * m)  //平方米

struct DensityProfileLayer //大气层密度剖面
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

struct AtmosphereParameters //大气层参数模型
{
	// 大气层顶部的太阳辐照度
	float3 solar_irradiance;
	// 太阳角半径
  	float sun_angular_radius;
  	// 星球中心到大气层底部的距离, 即地球半径
  	float bottom_radius;
  	// 星球中心到大气层顶部的距离
  	float top_radius;
  	// 空气分子的密度分布,[0,1]
  	DensityProfile rayleigh_density;
  	// 海拔为h处的rayleigh散射系数 = rayleigh_scattering * rayleigh_density
  	float3 rayleigh_scattering;
  	// 气溶胶的密度剖面,[0,1]
  	DensityProfile mie_density;
	// mie散射系数 = mie_scattering * mie_density
  	float3 mie_scattering;
  	// 气溶胶的消光系数 = mie_extinction * mie_density
  	float3 mie_extinction;
  	// 气溶胶的Cornette-Shanks相位函数中的非对称参数
  	float mie_phase_function_g;
  	// 吸收光线的空气分子的密度分布,[0,1]
  	DensityProfile absorption_density;
  	// 吸收光线的空气消光系数 = absorption_extinction * absorption_density
  	float3 absorption_extinction;
  	// 地面的平均反照率(反射系数)
  	float3 ground_albedo;
  	// 太阳天最大天顶角的cos值(cos因此最小)，用于后面大气散射的预计算
  	float mu_s_min;
};