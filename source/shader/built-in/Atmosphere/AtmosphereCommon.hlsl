#include "AtmosphereDefine.hlsl"

/**
 * 以下几个clamp函数用以限制数值在其对应的数值域内
 */

float ClampCosine(float mu)
{
	return(clamp(mu, float(-1.0), float(1.0)));
}

float ClampDistance(float d)
{
	return(max(d, 0.0 * m));
}

float ClampRadius(AtmosphereParameters atmosphere, float r)
{
	return(clamp(r, atmosphere.bottom_radius, atmosphere.top_radius));
}

float SafeSqrt(float a)
{
	return(sqrt(max( a, 0.0 * m2 )));
}

/** Rayleigh phase function */
float PhaseFunctionR(float Mu) 
{
    return (3.0 / (16.0 * PI)) * (1.0 + Mu * Mu);
}

/** Mie phase function */
float PhaseFunctionM(float Mu, float MieG)
{
	return 1.5 * 1.0 / (4.0 * PI) * (1.0 - MieG * MieG) * pow( abs(1.0 + (MieG * MieG) - 2.0 * MieG * Mu), -3.0/2.0) * (1.0 + Mu * Mu) / (2.0 + MieG * MieG);
}

/**
 * 功能:
 *  将[0,1]的x映射到[0.5/n,1.0-0.5/n],其中n是纹理大小
 *  原因是防止在纹理边界部分采样产生一些外推值
 * 传入参数：
 *  x要映射的值,texture_size纹理大小
 **/
float GetTextureCoordFromUnitRange(float x, int texture_size)
{
	return(0.5 / float( texture_size ) + x * (1.0 - 1.0 / float(texture_size)));
}

/**
 * 功能:
 *  GetTextureCoordFromUnitRange的逆过程
 * 传入参数：
 *  u要映射的值,texture_size纹理大小
 **/
float GetUnitRangeFromTextureCoord(float u, int texture_size)
{
	return((u - 0.5 / float(texture_size )) / (1.0 - 1.0 / float(texture_size)));
}

/**
 * 功能:
 *  用求根公式求解二元一次方程x^2+2urx+r^2-t^2=0
 *  其中u(即下面的mu)是视线天顶角的cos值,而t(即下面的top_radius)是大气层最外层半径
 *  r是视点位置向量的z分量,求解的根是视点p沿视线到大气层顶层的距离
 * 传入参数：
 *  atmosphere大气模型参数(其中top_radius要用到),r为视点高度,mu是视线天顶角的cos值
 **/
float DistanceToTopAtmosphereBoundary(AtmosphereParameters atmosphere, float r, float mu)
{
	float discriminant = r * r * (mu * mu-1.0) + atmosphere.top_radius * atmosphere.top_radius;//判别式
	return(ClampDistance(-r * mu + SafeSqrt( discriminant ) ) ); /* 这里是方程解中"+"的那个根 */
}

/**
 * 功能:
 *  与函数DistanceToTopAtmosphereBoundary类似
 *  不过这里计算的是视点沿视线方向到地球表面交点的距离
 * 传入参数：
 *  atmosphere大气模型参数(其中top_radius要用到),r为视点高度,mu是视线天顶角的cos值
 **/
float DistanceToBottomAtmosphereBoundary(AtmosphereParameters atmosphere, float r, float mu)
{
	float discriminant = r*r*(mu*mu-1.0)+atmosphere.bottom_radius*atmosphere.bottom_radius;
	return(ClampDistance( -r * mu - SafeSqrt( discriminant ) ) ); /* 这里是方程解中"-"的那个根 */
}