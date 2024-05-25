

#ifndef __MATHUTIL_H_INCLUDED__
#define __MATHUTIL_H_INCLUDED__

#if !defined(_USE_MATH_DEFINES)
#define _USE_MATH_DEFINES
#endif
#include <math.h>

#ifdef __SSE__
#include <xmmintrin.h>
#endif

#include "Math3DCommon.h"


const float kPi = 3.14159265f;
const float k2Pi = kPi * 2.0f;
const float kPiOver2 = kPi / 2.0f;
const float k1OverPi = 1.0f / kPi;
const float k1Over2Pi = 1.0f / k2Pi;
const float kPiOver180 = kPi / 180.0f;
const float k180OverPi = 180.0f / kPi;


extern float wrapPi(float theta);


extern float safeAcos(float x);


inline float	degToRad(float deg) { return deg * kPiOver180; }
inline float	radToDeg(float rad) { return rad * k180OverPi; }


inline void sinCos(float *returnSin, float *returnCos, float theta) {


	*returnSin = sin(theta);
	*returnCos = cos(theta);
}

inline float	fovToZoom(float fov) { return 1.0f / tan(fov * .5f); }
inline float	zoomToFov(float zoom) { return 2.0f * atan(1.0f / zoom); }

//Clamp函数
template <class T>
inline const T Clamp (const T& value, const T& low, const T& high)
{
	return std::min (std::max(value,low), high);
}


float GetClamp(float x,float fMin,float fMax);

//快速正弦函数
inline MATH3D_API double FastSin(double x);

//生成整数随机数
inline int Rand_Int(void);

inline float Rand_Float(void);

inline void Rand_Seed(const unsigned int seed);

class MATH3D_API MathUtil
{
public:
	static Real ACos (Real fValue);
	static Real ASin (Real fValue);
	static Real ATan (Real fValue);
	static Real ATan2 (Real fY, Real fX);
	static Real Ceil (Real fValue);
	static Real Cos (Real fValue);
	static Real Exp (Real fValue);
	static Real FAbs (Real fValue);
	static Real Floor (Real fValue);
	static Real FMod (Real fX, Real fY);
	static Real InvSqrt (Real fValue);		//1/sqrt(x)
	static Real Log (Real fValue);
	static Real Pow (Real fBase, Real fExponent);
	static Real Sin (Real fValue);
	static Real Sqr (Real fValue);
	static Real Sqrt (Real fValue);
	static Real Tan (Real fValue);
    
    static bool IsNaN(Real f);

	//快速计算函数
	static float FastInvSqrt (float fValue);
	static double FastInvSqrt (double dValue);
	static Real FastSin(Real fValue);//fValue参数的单位是度
	static Real FastCos(Real fValue);//fValue参数的单位是度
};

#endif 
