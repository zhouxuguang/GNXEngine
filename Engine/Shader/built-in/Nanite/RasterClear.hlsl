RWTexture2D<uint2> VisBuffer64;

[numthreads(8, 8, 8)]
void CS(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    VisBuffer64[dispatchThreadID.xy] = uint2(0, 0xFFFFFFFFu);
}
