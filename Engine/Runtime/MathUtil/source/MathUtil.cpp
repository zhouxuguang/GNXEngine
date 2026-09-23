

#include <math.h>
#include <stdlib.h>
#include <mutex>
#include <cstring>

#include "MathUtil.h"
#include "Vector3.h"

NS_MATHUTIL_BEGIN

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

int Rand_Int(void)
{
	return rand();
}

float Rand_Float(void)
{
	return ((float)rand()/(float)RAND_MAX);
}

void Rand_Seed(const unsigned int seed)
{
	srand(seed);
}

namespace
{

double SinTable[361] = {0};
double CosTable[361] = {0};
std::once_flag g_tableInitFlag;

void TableInit()
{
	for (int i = 0; i < 360; i ++)
	{
		double dbAngle = i * M_PI / 180.0;
		SinTable[i] = sin(dbAngle);
		CosTable[i] = cos(dbAngle);
	}

	SinTable[360] = SinTable[0];
	CosTable[360] = CosTable[0];
}

} // namespace


Real MathUtil::ACos(Real fValue)
{
	const Real clamped = Clamp(fValue, Real(-1), Real(1));
	return static_cast<Real>(acos(static_cast<double>(clamped)));
}

Real MathUtil::ASin(Real fValue)
{
	// Keep the same saturating boundary semantics as ACos, without a second
	// hand-written range-check implementation.
	const Real clamped = Clamp(fValue, Real(-1), Real(1));
	return static_cast<Real>(asin(static_cast<double>(clamped)));
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
	// Copy the representation instead of type-punning through a pointer.
	// The latter violates strict-aliasing and can miscompile under /O2/LTO.
	long long i = 0;
	std::memcpy(&i, &dValue, sizeof(i));
	i = 0x5fe6ec85e7de30da - (i >> 1);
	std::memcpy(&dValue, &i, sizeof(dValue));
	dValue = dValue*(1.5 - dHalf*dValue*dValue);
	return dValue;
}

float MathUtil::FastInvSqrt(float fValue)
{
	float fHalf = 0.5f*fValue;
	int i = 0;
	std::memcpy(&i, &fValue, sizeof(i));
	i = 0x5f3759df - (i >> 1);
	std::memcpy(&fValue, &i, sizeof(fValue));
	fValue = fValue*(1.5f - fHalf*fValue*fValue);
	return fValue;
}

Real MathUtil::ATan(Real fValue)
{
	return atan(fValue);
}

Real MathUtil::FastSin(Real fValue)
{
	// BUG 修复：原来用非原子的 bool g_bTableInit 做懒初始化，多线程并发调用会数据竞争。
	// 改用 std::call_once 保证只初始化一次且线程安全。
	std::call_once(g_tableInitFlag, TableInit);

	fValue = fmod(fValue,360);

	if (fValue < 0)
	{
		fValue += 360.0;
	}

	int nValueInt = (int)fValue;
	// fmod 归一化后 fValue 可能因浮点舍入恰好等于 360.0（例如极小负数 +360.0），
	// 此时 nValueInt = 360，下一行会读取 SinTable[361] —— 越界（合法下标 0..360），
	// 越界读到的内存若为 NaN/Inf 还会污染插值结果（FastCos 即会返回 NaN）。
	// 钳到 359 后 thetaFrac == 1，插值结果仍精确等于 SinTable[360]，语义不变。
	if (nValueInt > 359)
	{
		nValueInt = 359;
	}
	double thetaFrac = fValue - nValueInt;

	return SinTable[nValueInt] + thetaFrac*(SinTable[nValueInt+1] - SinTable[nValueInt]);
	
}

Real MathUtil::FastCos(Real fValue)
{
	std::call_once(g_tableInitFlag, TableInit);

	fValue = fmod(fValue,360);

	if (fValue < 0)
	{
		fValue += 360.0;
	}

	int nValueInt = (int)fValue;
	// 同 FastSin：fValue 可能被舍入为 360.0，导致读取 CosTable[361] 越界（并可能返回 NaN）。
	if (nValueInt > 359)
	{
		nValueInt = 359;
	}
	double thetaFrac = fValue - nValueInt;

	return CosTable[nValueInt] + thetaFrac*(CosTable[nValueInt+1] - CosTable[nValueInt]);
}

float GetClamp(float x,float fMin,float fMax)
{
	return Clamp(x, fMin, fMax);
}

namespace
{

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

}  // anonymous namespace

uint32_t convertRGBFloatToRGB9E5(float red, float green, float blue)
{
	const float red_c = std::max<float>(0, std::min(g_sharedexp_max, red));
	const float green_c = std::max<float>(0, std::min(g_sharedexp_max, green));
	const float blue_c = std::max<float>(0, std::min(g_sharedexp_max, blue));

	const float max_c = std::max<float>({ red_c, green_c, blue_c });

	// 全零（或极小）输入编码为 0
	if (max_c < 1e-32f)
	{
		return 0;
	}

	// 共享指数必须使用以 2 为底的对数（log2）。
	// BUG 修复：原来这里用的是自然对数 log()，导致对较大的 HDR 值严重低估指数，
	// 9 bit 尾数溢出被截断，编码结果错误（例如 100.0 -> 36.0，8192.0 -> 0.0）。
	//
	// 使用 frexp 精确拆分：max_c = mantissa * 2^exp，其中 mantissa∈[0.5,1)，
	// 因此 floor(log2(max_c)) == exp - 1。相比 log2() 浮点近似，在 2 的整数次幂
	// 附近不会因为舍入误差导致 floor 取值错误。
	int binExp = 0;
	frexp(max_c, &binExp);
	const float exp_p =
		std::max<float>(-g_sharedexp_bias - 1, static_cast<float>(binExp - 1)) + 1 + g_sharedexp_bias;
	const int max_s = static_cast<int>(
		floor((max_c / (pow(2.0f, exp_p - g_sharedexp_biased_mantissabits))) + 0.5f));
	const int exp_s =
		static_cast<int>((max_s < pow(2.0f, g_sharedexp_mantissabits)) ? exp_p : exp_p + 1);
	const float pow2_exp = pow(2.0f, static_cast<float>(exp_s) - g_sharedexp_biased_mantissabits);

	// 防止浮点舍入导致 9 bit 尾数溢出（field 只有 9 bit，赋值会截断）
	auto quantize = [pow2_exp](float c) -> unsigned int {
		unsigned int v = static_cast<unsigned int>(floor((c / pow2_exp) + 0.5f));
		return v < 511u ? v : 511u;
	};
	const uint32_t r = quantize(red_c);
	const uint32_t g = quantize(green_c);
	const uint32_t b = quantize(blue_c);
	const uint32_t e = static_cast<uint32_t>(exp_s) & 0x1fu;
	return r | (g << 9) | (b << 18) | (e << 27);
}

void convertRGB9E5toRGBFloat(uint32_t input, float* red, float* green, float* blue)
{
	const uint32_t r = input & 0x1ffu;
	const uint32_t g = (input >> 9) & 0x1ffu;
	const uint32_t b = (input >> 18) & 0x1ffu;
	const uint32_t e = (input >> 27) & 0x1fu;
	const float pow2_exp =
		pow(2.0f, static_cast<float>(e) - g_sharedexp_biased_mantissabits);

	*red = r * pow2_exp;
	*green = g * pow2_exp;
	*blue = b * pow2_exp;
}

NS_MATHUTIL_END
