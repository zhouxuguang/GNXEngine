RWTexture2D<uint2> VisBuffer64;

[numthreads(8, 8, 1)]
void CS(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    if (any(dispatchThreadID.xy >= uint2(1400u, 480u)))
	{
        return;
    }

    VisBuffer64[dispatchThreadID.xy] = uint2(0, 0xFFFFFFFFu);
}
