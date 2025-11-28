Texture2D<uint64_t> VisBuffer64 : register(t0);
RWTexture2D<float4> VisualizationBuffer : register(u1);

// int3 dispatchThreadID : SV_DispatchThreadID
[numthreads(8, 8, 8)]
void CS(uint3 DTID : SV_DispatchThreadID, uint3 GID : SV_GroupID)
{
    float3 result = float3(0.0f,0.0f,0.0f);
    // for (uint x = 0u; x < 1400u; x++)
    // {
    //     for (uint y = 0u; y < 480u; y++)
    //     {
            
    //     }
    // }

    VisualizationBuffer[DTID.xy] = float4(1.0, 1.0, 0.0, 1.0f);
}
