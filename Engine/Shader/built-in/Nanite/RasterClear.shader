RWTexture2D<uint2> VisBuffer64 : register(u0);

struct FQueueState
{
	uint mTotalClusterCount;
	uint mClusterWriteOffset;
};
RWStructuredBuffer<FQueueState> QueueState : register(u1);
RWByteAddressBuffer WorkArg0 : register(u2);
RWByteAddressBuffer WorkArg1 : register(u3);

[numthreads(8, 8, 1)]
void CS(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    if (any(dispatchThreadID.xy >= uint2(1400u, 480u)))
	{
        return;
    }

    if (dispatchThreadID.x == 0 && dispatchThreadID.y == 0)
    {
        QueueState[0].mTotalClusterCount = 0;
        QueueState[0].mClusterWriteOffset = 0;

        WorkArg0.Store4(0, uint4(0, 0, 0, 0));
        WorkArg0.Store4(16, uint4(0, 0, 1, 0));
        WorkArg1.Store4(0, uint4(0, 0, 0, 0));
        WorkArg1.Store4(16, uint4(0, 0, 1, 0));
    }

    VisBuffer64[dispatchThreadID.xy] = uint2(0, 0xFFFFFFFFu);
}
