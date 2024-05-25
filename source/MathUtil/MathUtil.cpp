

#include <math.h>
#include <stdlib.h>

#include "MathUtil.h"
#include "Vector3.h"

USING_NS_MATHUTIL

const Vector3 kZeroVector(0.0f, 0.0f, 0.0f);


float wrapPi(float theta)
{
	theta += kPi;
	theta -= floor(theta * k1Over2Pi) * k2Pi;
	theta -= kPi;
	return theta;
}

float safeAcos(float x)
{


	if (x <= -1.0f)
	{
		return kPi;
	}

	if (x >= 1.0f)
	{
		return 0.0f;
	}

	return acos(x);
}

double FastSin(double x)
{
	return x - 3796201.0/pow(2.0,24.0)*x*x*x + 0.00833220803*x*x*x*x*x - 0.000195168955*x*x*x*x*x*x*x;
}

inline int Rand_Int(void)
{
	return rand();
}

inline float Rand_Float(void)
{
	return ((float)rand()/(float)RAND_MAX);
}

inline void Rand_Seed(const unsigned int seed)
{
	srand(seed);
}

//初始化正弦和余弦表
double SinTable[361] = {0};
double CosTable[361] = {0};
bool g_bTableInit = false;

void TableInit()
{
	for (int i = 0; i < 360; i ++)
	{
		double dbAngle = i * M_PI / 180.0;
		SinTable[i] = sin(dbAngle);
		CosTable[i] = cos(dbAngle);
	}

	SinTable[360] = SinTable[359];
	CosTable[360] = CosTable[359];

	g_bTableInit = true;
}



Real MathUtil::ACos(Real fValue)
{
	if ( -(Real)1.0 < fValue )
	{
		if ( fValue < (Real)1.0 )
			return (Real)acos((double)fValue);
		else
			return (Real)0.0;
	}
	else
	{
		return M_PI;
	}
}

Real MathUtil::ASin(Real fValue)
{
	if ( -(Real)1.0 < fValue )
	{
		if ( fValue < (Real)1.0 )
			return (Real)acos((double)fValue);
		else
			return (Real)0.0;
	}
	else
	{
		return M_PI;
	}
}

bool MathUtil::IsNaN(Real f)
{
    // std::isnan() is C99, not supported by all compilers
    // However NaN always fails this next test, no other number does.
    return f != f;
    //return std::isnan(f);
}

double MathUtil::FastInvSqrt(double dValue)
{
	double dHalf = 0.5*dValue;
	long long i = *(long long*)&dValue;
	i = 0x5fe6ec85e7de30da - (i >> 1);
	dValue = *(double*)&i;
	dValue = dValue*(1.5 - dHalf*dValue*dValue);
	return dValue;
}

float MathUtil::FastInvSqrt(float fValue)
{
	float fHalf = 0.5f*fValue;
	int i = *(int*)&fValue;
	i = 0x5f3759df - (i >> 1);
	fValue = *(float*)&i;
	fValue = fValue*(1.5f - fHalf*fValue*fValue);
	return fValue;
}

inline bool b2IsValid(float x)
{
    int ix = *reinterpret_cast<int*>(&x);
    return (ix & 0x7f800000) != 0x7f800000;
}

Real MathUtil::ATan(Real fValue)
{
	return atan(fValue);
}

Real MathUtil::FastSin(Real fValue)
{
	if (!g_bTableInit)
	{
		TableInit();
	}

	fValue = fmod(fValue,360);

	//负数转换为正数
	if (fValue < 0)
	{
		fValue += 360.0;
	}

	int nValueInt = (int)fValue;
	double thetaFrac = fValue - nValueInt;

	//线性插值
	return SinTable[nValueInt] + thetaFrac*(SinTable[nValueInt+1] - SinTable[nValueInt]);
	
}

Real MathUtil::FastCos(Real fValue)
{
	if (!g_bTableInit)
	{
		TableInit();
	}

	fValue = fmod(fValue,360);

	//负数转换为正数
	if (fValue < 0)
	{
		fValue += 360.0;
	}

	int nValueInt = (int)fValue;
	double thetaFrac = fValue - nValueInt;

	//线性插值
	return CosTable[nValueInt] + thetaFrac*(CosTable[nValueInt+1] - CosTable[nValueInt]);
}

float GetClamp(float x,float fMin,float fMax)
{
	if (x < fMin)
	{
		x = fMin;
	}

	else if (x > fMax)
	{
		x = fMax;
	}

	return x;
}
