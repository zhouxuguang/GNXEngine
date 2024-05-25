Texture2D gInput            : register(t0);
RWTexture2D<unorm float4> gOutput : register(u1);


// Approximates luminance ("brightness") from an RGB value.  These weights are derived from
// experiment based on eye sensitivity to different wavelengths of light.
float CalcLuminance(float3 color)
{
    return dot(color, float3(0.299f, 0.587f, 0.114f));
}

[numthreads(16, 16, 1)]
void CS(int3 dispatchThreadID : SV_DispatchThreadID)
{
    int2 xy = dispatchThreadID.xy;
	float4 color = gInput[xy];

    float lume = CalcLuminance(color.rgb);

	gOutput[xy] = float4(lume, lume, lume, 1.0);
}