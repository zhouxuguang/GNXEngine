ByteAddressBuffer ClusterPageData : register(t0);

struct PrimitiveAttributesPacked
{
	nointerpolation uint4 PixelValue_ViewId_SwapVW_Mip_ArrayIndex_LevelOffset_ViewRect : TEXCOORD1;
};

struct VSOut
{
	PrimitiveAttributesPacked PrimitivePacked;
	float4 Position								: SV_Position;
};

VSOut VS(
	uint VertexID		: SV_VertexID,
	uint VisibleIndex	: SV_InstanceID
	)
{
	VSOut Out;
	Out.Position = float4(0.0f, 0.0f, 0.0f, 1.0f);
	uint pageIndex = 0u;
	uint clusterIndex = 0u;
	uint pageCount = ClusterPageData.Load(0);
	uint pageBaseAddressOffset = ClusterPageData.Load(4 + pageIndex * 4);    // page的基地址
	uint clusterCountOnPage = ClusterPageData.Load(pageBaseAddressOffset);   // page中cluster的个数
	uint clusterDataOffset = ClusterPageData.Load(pageBaseAddressOffset + 4 + clusterIndex * 4);   // cluster数据的局部偏移
	uint clusterBaseAddressOffset = pageBaseAddressOffset + 4 + clusterCountOnPage * 4 + clusterDataOffset;  // cluster数据的基地址

	uint clusterIndexOffset = ClusterPageData.Load(clusterBaseAddressOffset);   // 1112
	uint clusterIndexCount = ClusterPageData.Load(clusterBaseAddressOffset + 4); // 378

	uint currentVertexIndexOffset = clusterBaseAddressOffset + clusterDataOffset;
	uint currentVertexIndexDataOffset = currentVertexIndexOffset  + VertexID * 4;
	uint currentVertexIndex = ClusterPageData.Load(currentVertexIndexDataOffset);
	float3 pos = asfloat(ClusterPageData.Load3(clusterBaseAddressOffset + 8 + currentVertexIndex * 12));
	if (VertexID == 0)
	{
		Out.Position = float4(pos, 1.0);
	}
	else
	{
		Out.Position = float4(float(pageBaseAddressOffset), float(clusterCountOnPage), 
			float(clusterIndexOffset), float(clusterIndexCount));
	}
	return Out;
}

void PS(VSOut In)
{
}