ByteAddressBuffer HierarchyBuffer : register(t0);
RWByteAddressBuffer OutResult : register(u1);
RWByteAddressBuffer OutRasterBinMeta : register(u2);
RWByteAddressBuffer OutRasterBinData : register(u3);

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
    //hierarchy node -> child node -> OutResult
    //0->1091 : 1092 uint => 52 uint bvh node => 4 child [13 uint]
	uint currentArgOffset = 0u, currentArgCount = 1u;
	uint nextArgOffset = 0u, nextArgCount = 0u;
	uint offset = 0u;
	uint currentNodeIndex = 0u;
	bool isNextArgOffsetInited = false;
	while (true)
	{
		FHierarchyNodeSlice hierarchyNodeSlice = GetHierarchyNodeSlice(HierarchyBuffer, currentNodeIndex, 0u);
		if (!hierarchyNodeSlice.bLeaf && hierarchyNodeSlice.ChildStartReference != 0u)
		{
			if (!isNextArgOffsetInited)
			{
				isNextArgOffsetInited = true;
				nextArgOffset = offset;
			}
			OutResult.Store4(offset * 16, uint4(hierarchyNodeSlice.ChildStartReference, hierarchyNodeSlice.NumPages, 0, 0));
			nextArgCount ++;
			offset ++;
		}

		hierarchyNodeSlice = GetHierarchyNodeSlice(HierarchyBuffer, currentNodeIndex, 1u);
		if (!hierarchyNodeSlice.bLeaf && hierarchyNodeSlice.ChildStartReference != 0u)
		{
			if (!isNextArgOffsetInited)
			{
				isNextArgOffsetInited = true;
				nextArgOffset = offset;
			}
			OutResult.Store4(offset * 16, uint4(hierarchyNodeSlice.ChildStartReference, hierarchyNodeSlice.NumPages, 0, 0));
			nextArgCount ++;
			offset ++;
		}

		hierarchyNodeSlice = GetHierarchyNodeSlice(HierarchyBuffer, currentNodeIndex, 2u);
		if (!hierarchyNodeSlice.bLeaf && hierarchyNodeSlice.ChildStartReference != 0u)
		{
			if (!isNextArgOffsetInited)
			{
				isNextArgOffsetInited = true;
				nextArgOffset = offset;
			}
			OutResult.Store4(offset * 16, uint4(hierarchyNodeSlice.ChildStartReference, hierarchyNodeSlice.NumPages, 0, 0));
			nextArgCount ++;
			offset ++;
		}

		hierarchyNodeSlice = GetHierarchyNodeSlice(HierarchyBuffer, currentNodeIndex, 3u);
		if (!hierarchyNodeSlice.bLeaf && hierarchyNodeSlice.ChildStartReference != 0u)
		{
			if (!isNextArgOffsetInited)
			{
				isNextArgOffsetInited = true;
				nextArgOffset = offset;
			}
			OutResult.Store4(offset * 16, uint4(hierarchyNodeSlice.ChildStartReference, hierarchyNodeSlice.NumPages, 0, 0));
			nextArgCount ++;
			offset ++;
		}

		currentArgCount --;
		if (currentArgCount == 0u)
		{
			if (nextArgCount == 0u)
			{
				break;
			}
			currentArgOffset = nextArgOffset;
			currentArgCount = nextArgCount;
			currentNodeIndex = OutResult.Load4(currentArgOffset * 16).x;
			nextArgOffset = nextArgCount;
			nextArgCount = 0u;
			isNextArgOffsetInited = false;
		}else{
			currentArgOffset ++;
			currentNodeIndex = OutResult.Load(currentArgOffset * 16).x;
		}
	}

	uint4 xxxx = 0u;
	for (uint i = 0u; i < 21u; i ++)
	{
		for (uint j = 0u; j < 4u; j ++)
		{
			FHierarchyNodeSlice hierarchyNodeSlice = GetHierarchyNodeSlice(HierarchyBuffer, i, j);
			OutResult.Store4(offset * 16, uint4(i, j, hierarchyNodeSlice.ChildStartReference, hierarchyNodeSlice.NumPages));
			offset ++;

			if (10 == hierarchyNodeSlice.NumPages)
			{
				xxxx = uint4(i, j, hierarchyNodeSlice.ChildStartReference, hierarchyNodeSlice.NumPages);
			}
		}
	}

	uint pageIndex = xxxx.z >> 8;
	uint clusterOffset = xxxx.z & 0xFFu;
	FHierarchyNodeSlice hierarchyNodeSlice = GetHierarchyNodeSlice(HierarchyBuffer, xxxx.x, xxxx.y);
	OutResult.Store4(offset * 16, uint4(xxxx.x, xxxx.y, pageIndex, clusterOffset));
	offset ++;
	OutResult.Store4(offset * 16, uint4(xxxx.x, xxxx.y, xxxx.w, hierarchyNodeSlice.NumChildren));
	offset ++;

	//offset, count
	OutRasterBinMeta.Store(0, uint4(pageIndex, clusterOffset, hierarchyNodeSlice.NumChildren, 0u));
	//cluster index
	for (uint i = clusterOffset; i<hierarchyNodeSlice.NumChildren; i++)
	{
		OutRasterBinData.Store(0, uint4(pageIndex, clusterOffset, hierarchyNodeSlice.NumChildren, 0u));
	}

	OutResult.Store4(0, uint4(384u, hierarchyNodeSlice.NumChildren, 0u, 0u));

	// OutResult.Store(offset * 4, nextArgOffset);
	// offset ++;
	// OutResult.Store(offset * 4, nextArgCount);
	// offset ++;
}