

#include <math.h>
#include <stdlib.h>

#include "MathUtil.h"
#include "Vector3.h"

USING_NS_MATHUTIL

//const Vector3 kZeroVector(0.0f, 0.0f, 0.0f);


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

//��ʼ�����Һ����ұ�
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

	//����ת��Ϊ����
	if (fValue < 0)
	{
		fValue += 360.0;
	}

	int nValueInt = (int)fValue;
	double thetaFrac = fValue - nValueInt;

	//���Բ�ֵ
	return SinTable[nValueInt] + thetaFrac*(SinTable[nValueInt+1] - SinTable[nValueInt]);
	
}

Real MathUtil::FastCos(Real fValue)
{
	if (!g_bTableInit)
	{
		TableInit();
	}

	fValue = fmod(fValue,360);

	//����ת��Ϊ����
	if (fValue < 0)
	{
		fValue += 360.0;
	}

	int nValueInt = (int)fValue;
	double thetaFrac = fValue - nValueInt;

	//���Բ�ֵ
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

namespace
{

struct RGB9E5Data
{
	unsigned int R : 9;
	unsigned int G : 9;
	unsigned int B : 9;
	unsigned int E : 5;
};

// B is the exponent bias (15)
constexpr int g_sharedexp_bias = 15;

// N is the number of mantissa bits per component (9)
constexpr int g_sharedexp_mantissabits = 9;

// number of mantissa bits per component pre-biased
constexpr int g_sharedexp_biased_mantissabits = g_sharedexp_bias + g_sharedexp_mantissabits;

// Emax is the maximum allowed biased exponent value (31)
constexpr int g_sharedexp_maxexponent = 31;

constexpr float g_sharedexp_max =
((static_cast<float>(1 << g_sharedexp_mantissabits) - 1) /
	static_cast<float>(1 << g_sharedexp_mantissabits)) *
	static_cast<float>(1 << (g_sharedexp_maxexponent - g_sharedexp_bias));

template <typename destType, typename sourceType>
destType bitCast(const sourceType& source)
{
	size_t copySize = std::min(sizeof(destType), sizeof(sourceType));
	destType output;
	memcpy(&output, &source, copySize);
	return output;
}

}  // anonymous namespace

uint32_t convertRGBFloatToRGB9E5(float red, float green, float blue)
{
	const float red_c = std::max<float>(0, std::min(g_sharedexp_max, red));
	const float green_c = std::max<float>(0, std::min(g_sharedexp_max, green));
	const float blue_c = std::max<float>(0, std::min(g_sharedexp_max, blue));

	const float max_c = std::max<float>({ red_c, green_c, blue_c });
	const float exp_p =
		std::max<float>(-g_sharedexp_bias - 1, floor(log(max_c))) + 1 + g_sharedexp_bias;
	const int max_s = static_cast<int>(
		floor((max_c / (pow(2.0f, exp_p - g_sharedexp_biased_mantissabits))) + 0.5f));
	const int exp_s =
		static_cast<int>((max_s < pow(2.0f, g_sharedexp_mantissabits)) ? exp_p : exp_p + 1);
	const float pow2_exp = pow(2.0f, static_cast<float>(exp_s) - g_sharedexp_biased_mantissabits);

	RGB9E5Data output;
	output.R = static_cast<unsigned int>(floor((red_c / pow2_exp) + 0.5f));
	output.G = static_cast<unsigned int>(floor((green_c / pow2_exp) + 0.5f));
	output.B = static_cast<unsigned int>(floor((blue_c / pow2_exp) + 0.5f));
	output.E = exp_s;

	return bitCast<unsigned int>(output);
}

void convertRGB9E5toRGBFloat(uint32_t input, float* red, float* green, float* blue)
{
	const RGB9E5Data* inputData = reinterpret_cast<const RGB9E5Data*>(&input);

	const float pow2_exp =
		pow(2.0f, static_cast<float>(inputData->E) - g_sharedexp_biased_mantissabits);

	*red = inputData->R * pow2_exp;
	*green = inputData->G * pow2_exp;
	*blue = inputData->B * pow2_exp;
}
