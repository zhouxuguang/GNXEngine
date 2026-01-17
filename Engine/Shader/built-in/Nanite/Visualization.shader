Texture2D<uint2> VisBuffer64 : register(t0);
RWTexture2D<float4> VisualizationBuffer : register(u1);

uint MurmurMix(uint Hash)
{
	Hash ^= Hash >> 16;
	Hash *= 0x85ebca6b;
	Hash ^= Hash >> 13;
	Hash *= 0xc2b2ae35;
	Hash ^= Hash >> 16;
	return Hash;
}

float3 IntToColor(uint Index)
{
	uint Hash = MurmurMix(Index);
	float3 Color = float3
	(
		(Hash >>  0) & 255,
		(Hash >>  8) & 255,
		(Hash >> 16) & 255
	);
	return Color * (1.0f / 255.0f);
}

[numthreads(8, 8, 1)]
void CS(uint3 dispatchThreadID : SV_DispatchThreadID, uint3 GID : SV_GroupID)
{
    uint2 threadID = dispatchThreadID.xy;
	if (any(threadID >= uint2(1400u, 480u)))
	{
        return;
    }

    float3 result = float3(0.0f, 0.0f, 0.0f);

    uint2 pixelValue = VisBuffer64[threadID];
    uint packedData = pixelValue.x;
    if (packedData != 0)
    {
        uint pageIndex = (packedData >> 8) - 1;
        uint clusterIndex = (packedData & 0xFF) - 1;
        result = IntToColor(clusterIndex);
        result = result * 0.8 + 0.2;
    }

    VisualizationBuffer[dispatchThreadID.xy] = float4(result, 1.0f);
}
