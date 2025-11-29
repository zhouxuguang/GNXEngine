#include "../GNXEngineCommon.hlsl"

ByteAddressBuffer ClusterPageData;
ByteAddressBuffer MainAndPostNodeAndClusterBatches;

struct PrimitiveAttributesPacked
{
	nointerpolation uint4 PixelValue_ViewId_SwapVW_Mip_ArrayIndex_LevelOffset_ViewRect : TEXCOORD1;
};

struct VSOut
{
	PrimitiveAttributesPacked PrimitivePacked;
	float4 Position	: SV_Position;
};

VSOut VS(uint vertexID : SV_VertexID, uint VisibleIndex : SV_InstanceID)
{
	VSOut Out;

    uint2 packedCluster = MainAndPostNodeAndClusterBatches.Load2(VisibleIndex * 8u);
	uint pageIndex = packedCluster.x;
	uint clusterIndex = packedCluster.y;

	uint pageCount = ClusterPageData.Load(0);
	uint pageBaseAddressOffset = ClusterPageData.Load(4 + pageIndex * 4);    // page的基地址
	uint clusterCountOnPage = ClusterPageData.Load(pageBaseAddressOffset);   // page中cluster的个数
	uint clusterDataOffset = ClusterPageData.Load(pageBaseAddressOffset + 4 + clusterIndex * 4);   // cluster数据的局部偏移
	uint clusterBaseAddressOffset = pageBaseAddressOffset + 4 + clusterCountOnPage * 4 + clusterDataOffset;  // cluster数据的基地址

	uint clusterIndexOffset = ClusterPageData.Load(clusterBaseAddressOffset);   // 1112
	uint clusterIndexCount = ClusterPageData.Load(clusterBaseAddressOffset + 4); // 378

	uint currentVertexIndexOffset = clusterBaseAddressOffset + clusterIndexOffset;
	uint currentVertexIndexDataOffset = currentVertexIndexOffset + vertexID * 4;
	uint currentVertexIndex = ClusterPageData.Load(currentVertexIndexDataOffset);
	float3 pos = asfloat(ClusterPageData.Load3(clusterBaseAddressOffset + 8 + currentVertexIndex * 12));

	Out.Position = float4(0.0f, 0.0f, 0.0f, 1.0f);
	if (vertexID < clusterIndexCount)
	{
		float4 posW = float4(pos, 1.0);
		posW = mul(posW, MATRIX_V);
		posW = mul(posW, MATRIX_P);

		Out.Position = posW;
		Out.PrimitivePacked.PixelValue_ViewId_SwapVW_Mip_ArrayIndex_LevelOffset_ViewRect.x = (pageIndex + 1) << 8 | (clusterIndex + 1);
	}
	return Out;
}

uint2 PS(VSOut In) : SV_Target
{
	uint2 screenCoord = (uint2)In.Position.xy;
	uint pixelValue = In.PrimitivePacked.PixelValue_ViewId_SwapVW_Mip_ArrayIndex_LevelOffset_ViewRect.x;
	return uint2(0xFFFFFFFFu, pixelValue);
}