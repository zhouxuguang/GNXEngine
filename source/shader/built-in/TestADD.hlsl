
StructuredBuffer<float> gInputA : register(t0);
StructuredBuffer<float> gInputB : register(t1);
RWStructuredBuffer<float> gOutput : register(u2);


[numthreads(32, 1, 1)]
void CS(int3 dtid : SV_DispatchThreadID)
{
	gOutput[dtid.x] = gInputA[dtid.x] + gInputB[dtid.x];
}