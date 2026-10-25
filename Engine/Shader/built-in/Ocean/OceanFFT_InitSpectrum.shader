#include "OceanFFT.hlsl"

RWTexture2DArray<float4> _InitialSpectrumTextures;

[numthreads(8, 8, 1)]
void CS_InitializeSpectrum(uint3 id : SV_DISPATCHTHREADID) 
{
    uint seed = id.x + _N * id.y + _N;
    seed += _Seed;

    float lengthScales[4] = { _LengthScale0, _LengthScale1, _LengthScale2, _LengthScale3 };

    for (uint i = 0; i < 4; ++i) 
    {
        float halfN = _N / 2.0f;

        float deltaK = 2.0f * PI / lengthScales[i];
        float2 K = (id.xy - halfN) * deltaK;
        float kLength = length(K);

        seed += i + hash(seed) * 10;
        float4 uniformRandSamples = float4(hash(seed), hash(seed * 2), hash(seed * 3), hash(seed * 4));
        float2 gauss1 = UniformToGaussian(uniformRandSamples.x, uniformRandSamples.y);
        float2 gauss2 = UniformToGaussian(uniformRandSamples.z, uniformRandSamples.w);

        if (_LowCutoff <= kLength && kLength <= _HighCutoff) 
        {
            float kAngle = atan2(K.y, K.x);
            float omega = Dispersion(kLength);

            float dOmegadk = DispersionDerivative(kLength);

            float spectrum = JONSWAP(omega, _Spectrums[i * 2]) * DirectionSpectrum(kAngle, omega, _Spectrums[i * 2]) * ShortWavesFade(kLength, _Spectrums[i * 2]);
            
            if (_Spectrums[i * 2 + 1].scale > 0)
                spectrum += JONSWAP(omega, _Spectrums[i * 2 + 1]) * DirectionSpectrum(kAngle, omega, _Spectrums[i * 2 + 1]) * ShortWavesFade(kLength, _Spectrums[i * 2 + 1]);
            
            _InitialSpectrumTextures[uint3(id.xy, i)] = float4(float2(gauss2.x, gauss1.y) * sqrt(2 * spectrum * abs(dOmegadk) / kLength * deltaK * deltaK), 0.0f, 0.0f);
        } 
        else 
        {
            _InitialSpectrumTextures[uint3(id.xy, i)] = 0.0f;
        }
    }
}

[numthreads(8, 8, 1)]
void CS_PackSpectrumConjugate(uint3 id : SV_DISPATCHTHREADID) 
{
    for (uint i = 0; i < 4; ++i) 
    {
        float2 h0 = _InitialSpectrumTextures[uint3(id.xy, i)].rg;
        float2 h0conj = _InitialSpectrumTextures[uint3((_N - id.x ) % _N, (_N - id.y) % _N, i)].rg;

        _InitialSpectrumTextures[uint3(id.xy, i)] = float4(h0, h0conj.x, -h0conj.y);
    }
}

[numthreads(8, 8, 1)]
void CS_UpdateSpectrumForFFT(uint3 id : SV_DISPATCHTHREADID) 
{
    
    float lengthScales[4] = { _LengthScale0, _LengthScale1, _LengthScale2, _LengthScale3 };

    for (int i = 0; i < 4; ++i) 
    {
        float4 initialSignal = _InitialSpectrumTextures[uint3(id.xy, i)];
        float2 h0 = initialSignal.xy;
        float2 h0conj = initialSignal.zw;

        float halfN = _N / 2.0f;
        float2 K = (id.xy - halfN) * 2.0f * PI / lengthScales[i];
        float kMag = length(K);
        float kMagRcp = rcp(kMag);

        if (kMag < 0.0001f) 
        {
            kMagRcp = 1.0f;
        }

        float w_0 = 2.0f * PI / _RepeatTime;
        float dispersion = floor(sqrt(_Gravity * kMag) / w_0) * w_0 * _FrameTime;

        float2 exponent = EulerFormula(dispersion);

        float2 htilde = ComplexMult(h0, exponent) + ComplexMult(h0conj, float2(exponent.x, -exponent.y));
        float2 ih = float2(-htilde.y, htilde.x);

        float2 displacementX = ih * K.x * kMagRcp;
        float2 displacementY = htilde;
        float2 displacementZ = ih * K.y * kMagRcp;

        float2 displacementX_dx = -htilde * K.x * K.x * kMagRcp;
        float2 displacementY_dx = ih * K.x;
        float2 displacementZ_dx = -htilde * K.x * K.y * kMagRcp;

        float2 displacementY_dz = ih * K.y;
        float2 displacementZ_dz = -htilde * K.y * K.y * kMagRcp;

        float2 htildeDisplacementX = float2(displacementX.x - displacementZ.y, displacementX.y + displacementZ.x);
        float2 htildeDisplacementZ = float2(displacementY.x - displacementZ_dx.y, displacementY.y + displacementZ_dx.x);
        
        float2 htildeSlopeX = float2(displacementY_dx.x - displacementY_dz.y, displacementY_dx.y + displacementY_dz.x);
        float2 htildeSlopeZ = float2(displacementX_dx.x - displacementZ_dz.y, displacementX_dx.y + displacementZ_dz.x);

        _SpectrumTextures[uint3(id.xy, i * 2)] = float4(htildeDisplacementX, htildeDisplacementZ);
        _SpectrumTextures[uint3(id.xy, i * 2 + 1)] = float4(htildeSlopeX, htildeSlopeZ);
    }
}