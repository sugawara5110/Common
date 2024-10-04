///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                           ShaderBasicHit.hlsl                                         //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

//�ʏ�
[shader("closesthit")]
void basicHit(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attr)
{
	payload.hitPosition = HitWorldPosition();
	Vertex3 v3 = getVertex();
	float4 difTex = getDifPixel(attr, v3);
	float3 normalMap = getNorPixel(attr, v3);
	float3 speTex = getSpePixel(attr, v3);

	if (traceMode == 0)
	{
		if (materialIdent(payload.mNo, EMISSIVE))
			return;
////////�����ւ̌���
		difTex.xyz = EmissivePayloadCalculate(payload.RecursionCnt, payload.hitPosition,
                                             difTex.xyz, speTex, normalMap);

////////�@���؂�ւ�
		float3 r_eyeVec = -WorldRayDirection(); //�����ւ̃x�N�g��
		float norDir = dot(r_eyeVec, normalMap);
		if (norDir < 0.0f)
			normalMap *= -1.0f;

////////�t���l���v�Z
		float fresnel = saturate(dot(r_eyeVec, normalMap));

////////���˕����ւ̌���
		difTex.xyz = MetallicPayloadCalculate(payload.RecursionCnt, payload.hitPosition,
                                             difTex.xyz, normalMap, payload.hitInstanceId, fresnel);

////////������
		difTex.xyz = Translucent(payload.RecursionCnt, payload.hitPosition,
                                difTex, normalMap, fresnel);

		payload.depth = getDepth(attr, v3);
		payload.normal = normalMap;

		payload.color = difTex.xyz;
		payload.hit = true;
		payload.Alpha = difTex.w;
	}
	else
	{
////////PathTracing
		payload.normal = normalMap;
		payload.depth = getDepth(attr, v3);
		payload.hitInstanceId = (int) getInstancingID();

		if (!materialIdent(payload.mNo, NEE))
		{
			payload.color = PayloadCalculate_PathTracing(payload.RecursionCnt, payload.hitPosition,
                                                       difTex, speTex, normalMap,
                                                       payload.throughput, payload.hitInstanceId);
		}
	}
}

//�@���}�b�v�e�X�g�p
[shader("closesthit")]
void basicHit_normalMapTest(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attr)
{
	payload.hitPosition = HitWorldPosition();
	Vertex3 v3 = getVertex();
//�e�N�X�`���擾
	float4 difTex = getDifPixel(attr, v3);
	float3 normalMap = getNorPixel(attr, v3);

//�[�x�擾
	payload.depth = getDepth(attr, v3);
//�@���擾
	payload.normal = normalMap;

	payload.color = normalMap;
	payload.hit = true;
	payload.Alpha = difTex.w;
}