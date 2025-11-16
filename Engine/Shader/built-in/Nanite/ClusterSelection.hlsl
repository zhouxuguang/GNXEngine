ByteAddressBuffer HierarchyBuffer:register(t0);
RWByteAddressBuffer OutResult:register(u1);

#define NANITE_MAX_BVH_NODE_FANOUT_BITS						2
#define NANITE_MAX_BVH_NODE_FANOUT_MASK						((1 << NANITE_MAX_BVH_NODE_FANOUT_BITS)-1)
#define NANITE_MAX_BVH_NODE_FANOUT							(1 << NANITE_MAX_BVH_NODE_FANOUT_BITS)
#define NANITE_BVH_NODE_ENABLE_MASK							((1 << NANITE_MAX_BVH_NODE_FANOUT)-1)
#define HIERARCHY_NODE_SLICE_SIZE	((4 + 4 + 4 + 1) * 4 * NANITE_MAX_BVH_NODE_FANOUT)

#define NANITE_MAX_CLUSTERS_PER_GROUP_BITS					9
#define NANITE_MAX_GROUP_PARTS_BITS							5
#define NANITE_MAX_RESOURCE_PAGES_BITS						16 // 2GB of 32kb root pages or 4GB of 64kb streaming pages

struct FHierarchyNodeSlice
{
	float4	LODBounds;
	float3	BoxBoundsCenter;
	float3	BoxBoundsExtent;
	float	MinLODError;
	float	MaxParentLODError;
	uint	ChildStartReference;	// Can be node (index) or cluster (page:cluster)
	uint	NumChildren;
	uint	StartPageIndex;
	uint	NumPages;
	bool	bEnabled;
	bool	bLoaded;
	bool	bLeaf;
};

uint BitFieldExtractU32(uint Data, uint Size, uint Offset)
{
	// Shift amounts are implicitly &31 in HLSL, so they should be optimized away on most platforms
	// In GLSL shift amounts < 0 or >= word_size are undefined, so we better be explicit
	Size &= 31;
	Offset &= 31;
	return (Data >> Offset) & ((1u << Size) - 1u);
}

FHierarchyNodeSlice UnpackHierarchyNodeSlice(uint4 RawData0, uint4 RawData1, uint4 RawData2, uint RawData3)
{
	const uint4 Misc0 = RawData1;
	const uint4 Misc1 = RawData2;
	const uint  Misc2 = RawData3;

	FHierarchyNodeSlice Node;
	Node.LODBounds				= asfloat(RawData0);

	Node.BoxBoundsCenter		= asfloat(Misc0.xyz);
	Node.BoxBoundsExtent		= asfloat(Misc1.xyz);

	Node.MinLODError			= f16tof32(Misc0.w);
	Node.MaxParentLODError		= f16tof32(Misc0.w >> 16);
	Node.ChildStartReference	= Misc1.w;						// When changing this, remember to also update StoreHierarchyNodeChildStartReference
	Node.bLoaded				= (Misc1.w != 0xFFFFFFFFu);

	Node.NumChildren			= BitFieldExtractU32(Misc2, NANITE_MAX_CLUSTERS_PER_GROUP_BITS, 0);
	Node.NumPages				= BitFieldExtractU32(Misc2, NANITE_MAX_GROUP_PARTS_BITS, NANITE_MAX_CLUSTERS_PER_GROUP_BITS);
	Node.StartPageIndex			= BitFieldExtractU32(Misc2, NANITE_MAX_RESOURCE_PAGES_BITS, NANITE_MAX_CLUSTERS_PER_GROUP_BITS + NANITE_MAX_GROUP_PARTS_BITS);
	Node.bEnabled				= Misc2 != 0u;
	Node.bLeaf					= Misc2 != 0xFFFFFFFFu;

	return Node;
}

#define HIERARCHY_NODE_SLICE_SIZE	((4 + 4 + 4 + 1) * 4 * NANITE_MAX_BVH_NODE_FANOUT)

FHierarchyNodeSlice GetHierarchyNodeSlice(ByteAddressBuffer InputBuffer, uint NodeIndex, uint ChildIndex)
{
	const uint BaseAddress	= NodeIndex * HIERARCHY_NODE_SLICE_SIZE;

	const uint4 RawData0	= InputBuffer.Load4(BaseAddress + 16 * ChildIndex);
	const uint4 RawData1	= InputBuffer.Load4(BaseAddress + (NANITE_MAX_BVH_NODE_FANOUT * 16) + 16 * ChildIndex);
	const uint4 RawData2	= InputBuffer.Load4(BaseAddress + (NANITE_MAX_BVH_NODE_FANOUT * 32) + 16 * ChildIndex);
	const uint  RawData3	= InputBuffer.Load( BaseAddress + (NANITE_MAX_BVH_NODE_FANOUT * 48) +  4 * ChildIndex);
	
	return UnpackHierarchyNodeSlice(RawData0, RawData1, RawData2, RawData3);
}

[numthreads(1, 1, 1)]//1 -> wave : 32 
void CS()
{
    //hierarchy node -> cluster :index
    //hierarchy node -> child node -> OutResult,
    uint offset = 0u;
    for (uint i = 0; i < 21; i ++) 
    {
        FHierarchyNodeSlice hierarchyNodeSlice = GetHierarchyNodeSlice(HierarchyBuffer, i, 0);
        OutResult.Store4(offset * 16, uint4(i, 0, hierarchyNodeSlice.ChildStartReference, hierarchyNodeSlice.bLeaf ? 1u : 2u)); 
        offset ++; 

        hierarchyNodeSlice = GetHierarchyNodeSlice(HierarchyBuffer, i, 1);
        OutResult.Store4(offset * 16, uint4(i, 1, hierarchyNodeSlice.ChildStartReference, hierarchyNodeSlice.bLeaf ? 1u : 2u)); 
        offset ++; 

        hierarchyNodeSlice = GetHierarchyNodeSlice(HierarchyBuffer, i, 2);
        OutResult.Store4(offset * 16, uint4(i, 2, hierarchyNodeSlice.ChildStartReference, hierarchyNodeSlice.bLeaf ? 1u : 2u)); 
        offset ++; 

        hierarchyNodeSlice = GetHierarchyNodeSlice(HierarchyBuffer, i, 3);
        OutResult.Store4(offset * 16, uint4(i, 3, hierarchyNodeSlice.ChildStartReference, hierarchyNodeSlice.bLeaf ? 1u : 2u)); 
        offset ++; 
    }
}