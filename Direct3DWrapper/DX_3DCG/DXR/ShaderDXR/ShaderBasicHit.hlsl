///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                           ShaderBasicHit.hlsl                                         //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

//通常
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
////////光源への光線
		difTex.xyz = EmissivePayloadCalculate(payload.RecursionCnt, payload.hitPosition,
                                             difTex.xyz, speTex, normalMap);

////////法線切り替え
		float3 r_eyeVec = -WorldRayDirection(); //視線へのベクトル
		float norDir = dot(r_eyeVec, normalMap);
		if (norDir < 0.0f)
			normalMap *= -1.0f;

////////フレネル計算
		float fresnel = saturate(dot(r_eyeVec, normalMap));

////////反射方向への光線
		difTex.xyz = MetallicPayloadCalculate(payload.RecursionCnt, payload.hitPosition,
                                             difTex.xyz, normalMap, payload.hitInstanceId, fresnel);

////////半透明
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

//法線マップテスト用
[shader("closesthit")]
void basicHit_normalMapTest(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attr)
{
	payload.hitPosition = HitWorldPosition();
	Vertex3 v3 = getVertex();
//テクスチャ取得
	float4 difTex = getDifPixel(attr, v3);
	float3 normalMap = getNorPixel(attr, v3);

//深度取得
	payload.depth = getDepth(attr, v3);
//法線取得
	payload.normal = normalMap;

	payload.color = normalMap;
	payload.hit = true;
	payload.Alpha = difTex.w;
}