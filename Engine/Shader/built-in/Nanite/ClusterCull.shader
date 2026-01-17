#include "../GNXEngineCommon.hlsl"

ByteAddressBuffer MainAndPostNodeAndClusterBatches : register(t0);
ByteAddressBuffer ClusterPageData : register(t1);
RWByteAddressBuffer WorkArg : register(u2);
RWByteAddressBuffer OutVisibleClustersSWHW : register(u3); //

cbuffer GlobalData
{
	float4x4 MATRIX_Model;
	uint4 Misc0;
	float4 Nanite_ViewOrigin;
	float4 Nanite_ViewForward; //=>FNaniteView.ViewForward
}

struct ClusterInfo
{
	uint BaseAddress;
	uint IndexDataOffset;
	uint IndexCount;
	float4 LODBounds;//bounding sphere
	float LODError;//
	float EdgeLength;//
};

ClusterInfo GetClusterinfo(uint inPageIndex, uint inClusterIndex)
{
	uint pageBaseAddressOffset = ClusterPageData.Load(4u+inPageIndex*4u);
	uint clusterCountOnPage = ClusterPageData.Load(pageBaseAddressOffset);//4u
	uint clusterDataOffset = ClusterPageData.Load(pageBaseAddressOffset+4u+4u*inClusterIndex);
	uint clusterBaseAddressOffset = pageBaseAddressOffset+4u+clusterCountOnPage*4u+clusterDataOffset;
	uint clusterIndexDataOffset = ClusterPageData.Load(clusterBaseAddressOffset);
	uint clusterIndexCount=ClusterPageData.Load(clusterBaseAddressOffset+4u);
	uint4 clusterLODBounds=ClusterPageData.Load4(clusterBaseAddressOffset+8u);
	uint clusterLODErrorAndEdgeLength=ClusterPageData.Load(clusterBaseAddressOffset+24u);
	ClusterInfo clusterInfo;
	clusterInfo.BaseAddress=clusterBaseAddressOffset;
	clusterInfo.IndexDataOffset=clusterIndexDataOffset;
	clusterInfo.IndexCount=clusterIndexCount;
	clusterInfo.LODBounds=asfloat(clusterLODBounds);
	clusterInfo.LODError=f16tof32(clusterLODErrorAndEdgeLength);
	clusterInfo.EdgeLength=f16tof32(clusterLODErrorAndEdgeLength>>16);
	return clusterInfo;
}

float2 GetProjectionScales(float4 sphere)
{
	if (MATRIX_P[3][3] >= 1.0f)
	{
		//not ortho
		return float2(1.0f, 1.0f);
	}

	float3 center = sphere.xyz;
	float radius = sphere.w;
	
	float distanceToSphereCenterSq = dot(center, center);
	float distanceToSphereCenter = sqrt(distanceToSphereCenterSq);
	
	float zVS = dot(Nanite_ViewForward.xyz, center);//center
	
	float xVSSq = distanceToSphereCenterSq - zVS * zVS;
	float xVS = sqrt(max(0.0f, xVSSq));

	float distanceToTangentPointSq = distanceToSphereCenterSq - radius * radius;
	float distanceToTangentPoint = sqrt(max(0.0f, distanceToTangentPointSq));

	float sinTheta = radius / distanceToSphereCenter;
	float cosTheta = distanceToTangentPoint / distanceToSphereCenter;

	float a = (-sinTheta * xVS + cosTheta * zVS) / distanceToSphereCenter;
	float b = (sinTheta * xVS + cosTheta * zVS) / distanceToSphereCenter;

	float minZ = max(0.1f, zVS - radius);
	float maxZ = max(0.1f, zVS + radius);
	
	if (zVS + radius > 0.1f)
	{
		return float2(minZ * a, maxZ * b);
	}
	
	return float2(0.0f, 0.0f);
}

[numthreads(1, 1, 1)]//1 -> wave : 32 
void CS()
{
	uint clusterCount = WorkArg.Load2(0).y;
	uint visibleClusterCount = 0;
	for (uint i = 0; i < clusterCount; i ++) 
	{
		uint2 packedCluster = MainAndPostNodeAndClusterBatches.Load2(i*8u);
		uint pageIndex = packedCluster.x;
		uint clusterIndex = packedCluster.y;//MipLevel,LODBounds,LODError,EdgeLength => SW,HW,Visible(false)
		ClusterInfo clusterInfo = GetClusterinfo(pageIndex, clusterIndex);

		float3 boundingSphere = clusterInfo.LODBounds.xyz;
		float4 boundingSpherePositionWS = mul(float4(boundingSphere, 1.0f), MATRIX_Model);
		boundingSpherePositionWS = float4(boundingSpherePositionWS.xyz - Nanite_ViewOrigin.xyz, 1.0f);

		//QEM : Quadric Error Metric
		float2 projectionScales = GetProjectionScales(float4(boundingSpherePositionWS.xyz, clusterInfo.LODBounds.w));
		float lodScale = Nanite_ViewOrigin.w;
		float lodScaleHW = Nanite_ViewForward.w;
		if (projectionScales.x > clusterInfo.LODError * lodScale)
		{
			if (projectionScales.x < abs(clusterInfo.EdgeLength) * lodScaleHW)
			{
				//hw
			}
			else
			{
				//sw
			}
			OutVisibleClustersSWHW.Store2(visibleClusterCount * 8u, packedCluster);
			visibleClusterCount++;
		}
	}
	WorkArg.Store(4, visibleClusterCount);
}