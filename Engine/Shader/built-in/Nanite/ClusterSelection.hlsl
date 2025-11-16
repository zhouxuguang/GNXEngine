ByteAddressBuffer HierarchyBuffer:register(t0);
RWByteAddressBuffer OutResult:register(u1);

[numthreads(1, 1, 1)]//1 -> wave : 32 
void CS()
{
    //hierarchy node -> cluster :index
    //hierarchy node -> child node -> OutResult,
    OutResult.Store(0, HierarchyBuffer.Load(0));
}