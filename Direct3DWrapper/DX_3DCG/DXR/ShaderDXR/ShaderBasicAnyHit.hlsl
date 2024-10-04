///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                           ShaderBasicAnyHit.hlsl                                      //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

[shader("anyhit")]
void anyBasicHit(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attr)
{
//�e�N�X�`���擾
	Vertex3 v3 = getVertex();
	float4 difTex = getDifPixel(attr, v3);
	float Alpha = difTex.w;

	if (Alpha <= 0.0f)
	{
		IgnoreHit();
	}
}
