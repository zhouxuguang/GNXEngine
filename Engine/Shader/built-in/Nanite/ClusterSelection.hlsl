#include "../GNXEngineCommon.hlsl"

ByteAddressBuffer HierarchyBuffer : register(t0);
RWByteAddressBuffer OutResult : register(u1);
RWByteAddressBuffer OutRasterBinMeta : register(u2);
RWByteAddressBuffer OutMainAndPostNodeAndClusterBatches : register(u3);

cbuffer GlobalData
{
	float4x4 MATRIX_Model;
	uint4 Misc0;
	float4 Nanite_ViewOrigin;
}

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

void StoreCluster(inout uint clusterOffset, FHierarchyNodeSlice hierarchyNodeSlice)
{
	uint clusterCount = hierarchyNodeSlice.NumChildren;
	uint pageIndex = hierarchyNodeSlice.ChildStartReference >> 8;
	uint localClusterOffset = hierarchyNodeSlice.ChildStartReference & 0xFFu;
	uint localClusterPageIndexEnd = localClusterOffset + clusterCount;
	for (uint localClusterPageIndex = localClusterOffset; localClusterPageIndex < localClusterPageIndexEnd; localClusterPageIndex ++)
	{
		OutMainAndPostNodeAndClusterBatches.Store2(clusterOffset * 8u, uint2(pageIndex, localClusterPageIndex));
		clusterOffset ++;
	}
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

float2 GetProjectionScales(float4 sphere)
{
	if (MATRIX_P[3][3] >= 1.0f)
	{
		//not ortho
		return float2(1.0f, 1.0f);
	}
	//translated world
	//min z(0.1) * 0.1,max(0.9) z * 100.0
	
	return float2(0.0f, 0.0f);
}

bool ShouldVisitChild(FHierarchyNodeSlice hierarchyNodeSlice)
{
	float3 boundingSphere = hierarchyNodeSlice.LODBounds.xyz;
	float4 boundingSpherePositionWS = mul(float4(boundingSphere, 1.0f), MATRIX_Model);
	boundingSpherePositionWS = float4(boundingSpherePositionWS.xyz - Nanite_ViewOrigin.xyz, 1.0f);

	//QEM : Quadric Error Metric
	float2 projectionScales = GetProjectionScales(float4(boundingSpherePositionWS.xyz, hierarchyNodeSlice.LODBounds.w));
	float lodScale = Nanite_ViewOrigin.w;
	float threshold = lodScale * hierarchyNodeSlice.MaxParentLODError;
	if (projectionScales.x <= threshold)
	{
		// projectionScales.y>minLODError
		return true;
	}
	return false;
}

struct NextClusterSelectionArgs
{
	uint mNextArgOffset;
	uint mNextArgCount;
	bool mIsNextArgOffsetInited;
};

uint VisitBVHNode(FHierarchyNodeSlice hierarchyNodeSlice,
	inout NextClusterSelectionArgs nextClusterSelectionArgs,
	inout uint nodeOffset, inout uint clusterOffset)
{
	bool bShouldVisitChild = ShouldVisitChild(hierarchyNodeSlice);
	if (!hierarchyNodeSlice.bLeaf && hierarchyNodeSlice.ChildStartReference != 0u)
	{
		if (!nextClusterSelectionArgs.mIsNextArgOffsetInited)
		{
			nextClusterSelectionArgs.mIsNextArgOffsetInited = true;
			nextClusterSelectionArgs.mNextArgOffset = nodeOffset;
		}
		OutResult.Store4(nodeOffset * 16, uint4(hierarchyNodeSlice.ChildStartReference, hierarchyNodeSlice.NumPages, 0u, 0u));
		nextClusterSelectionArgs.mNextArgCount ++;
		nodeOffset ++;
	}

	else
	{
		if (Misc0.x == hierarchyNodeSlice.NumPages)
		{
			StoreCluster(clusterOffset, hierarchyNodeSlice);
			return hierarchyNodeSlice.NumChildren;
		}
	}

	return 0u;
}

[numthreads(1, 1, 1)]//1 -> wave : 32 
void CS()
{
    //hierarchy node -> cluster :index
    //hierarchy node -> child node -> OutResult
    //0->1091 : 1092 uint => 52 uint bvh node => 4 child [13 uint]
	NextClusterSelectionArgs nextClusterSelectionArgs;
	nextClusterSelectionArgs.mIsNextArgOffsetInited = false;
	nextClusterSelectionArgs.mNextArgOffset = 0u;
	nextClusterSelectionArgs.mNextArgCount = 0u;

	uint clusterOffset = 0u;
	uint totalClusterCount = 0u;

	uint currentArgOffset = 0u, currentArgCount = 1u;
	uint offset = 0u;
	uint currentNodeIndex = 0u;
	while (true)
	{
		FHierarchyNodeSlice hierarchyNodeSlice = GetHierarchyNodeSlice(HierarchyBuffer, currentNodeIndex, 0u);
		totalClusterCount += VisitBVHNode(hierarchyNodeSlice, nextClusterSelectionArgs, offset, clusterOffset);

		hierarchyNodeSlice = GetHierarchyNodeSlice(HierarchyBuffer, currentNodeIndex, 1u);
		totalClusterCount += VisitBVHNode(hierarchyNodeSlice, nextClusterSelectionArgs, offset, clusterOffset);

		hierarchyNodeSlice = GetHierarchyNodeSlice(HierarchyBuffer, currentNodeIndex, 2u);
		totalClusterCount += VisitBVHNode(hierarchyNodeSlice, nextClusterSelectionArgs, offset, clusterOffset);

		hierarchyNodeSlice = GetHierarchyNodeSlice(HierarchyBuffer, currentNodeIndex, 3u);
		totalClusterCount += VisitBVHNode(hierarchyNodeSlice, nextClusterSelectionArgs, offset, clusterOffset);

		currentArgCount --;
		if (currentArgCount == 0u)
		{
			if (nextClusterSelectionArgs.mNextArgCount == 0u)
			{
				break;
			}
			currentArgOffset = nextClusterSelectionArgs.mNextArgOffset;
			currentArgCount = nextClusterSelectionArgs.mNextArgCount;
			currentNodeIndex = OutResult.Load4(currentArgOffset * 16).x;

			nextClusterSelectionArgs.mNextArgOffset = nextClusterSelectionArgs.mNextArgCount;
			nextClusterSelectionArgs.mNextArgCount = 0u;
			nextClusterSelectionArgs.mIsNextArgOffsetInited = false;
		}
		else
		{
			currentArgOffset ++;
			currentNodeIndex = OutResult.Load(currentArgOffset * 16).x;
		}
	}

	OutResult.Store4(0, uint4(384u, totalClusterCount, 0u, 0u));
}