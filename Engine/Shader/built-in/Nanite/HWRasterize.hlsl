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
	return Out;
}

void PS(VSOut In)
{
}