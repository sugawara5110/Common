///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                           ShaderRayGen.hlsl                                           //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "ShaderCommon.hlsl"

//共通
void rayGenIn()
{
    uint2 pixelPos = DispatchRaysIndex().xy; //この関数を実行しているピクセル座標を取得
    uint2 dim = DispatchRaysDimensions(); //画面全体の幅と高さを取得
    float2 screenPos = (pixelPos + 0.5f) / dim * 2.0f - 1.0f;

    RayPayload payload;
	
    uint pixelID = pixelPos.y * dim.x + pixelPos.x;
    payload.Seed = pcg_hash(pixelID ^ pcg_hash(SeedFrame));
    
    float rand = Rand(payload.Seed);
    payload.wavelength_choice = (rand < 0.333f) ? 0 : (rand < 0.666f ? 1 : 2);
    payload.wavelength_mask_sw = true;

    RayDesc ray;
//光線の原点, ここが光線スタート, 視線から始まる
    ray.Origin = cameraPosition.xyz;
//光線の方向
    float4 world = mul(float4(screenPos.x, -screenPos.y, 0, 1), projectionToWorld);
    world.xyz /= world.w;
    ray.Direction = normalize(world.xyz - ray.Origin);

//TraceRay(AccelerationStructure, 
//         RAY_FLAG, 
//         InstanceInclusionMask, 
//         HitGroupIndex, 使用するhitShaderのIndex
//         MultiplierForGeometryContributionToShaderIndex, 
//         MissShaderIndex, 使用するmissShaderのIndex
//         RayDesc, 
//         Payload);
//この関数からRayがスタートする
//payloadに各hit, miss シェーダーで計算された値が格納される
    payload.RecursionCnt = 0;
    payload.hitPosition = ray.Origin;
    gDepthOut[pixelPos] = 1.0f;
    payload.depth = 1.0f;
    payload.normal = float3(0.0f, 0.0f, 0.0f);
    payload.hitInstanceId = -1;
    payload.throughput = float3(1.0f, 1.0f, 1.0f);
    payload.mNo = DIFFUSE;

    traceRay(payload.RecursionCnt, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, 0, 0, ray, payload);

    gDepthOut[pixelPos] = payload.depth;

    gNormalMap[pixelPos] = float4(payload.normal, 1.0f);

    gInstanceIdMap[pixelPos] = payload.hitInstanceId;

    float3 col = payload.color;
    
    if (any(isnan(col)) || any(isinf(col)))
    {
        col = float3(0, 0, 0);
    }

    col = max(0.0f, col);

    if (traceMode != 0)
    {
        float4 prev_screenPos4 = mul(float4(payload.hitPosition, 1.0f), prevViewProjection);
        float2 prev_screenPos = (prev_screenPos4.xy / prev_screenPos4.z * payload.depth);
        prev_screenPos.y *= -1.0f;
        uint2 prevInd = (prev_screenPos + 1.0f) * dim * 0.5f;
        float prevDepth = gPrevDepthOut[prevInd];
        float3 prevNor = gPrevNormalMap[prevInd].xyz;
        float crruentDepth = gDepthOut[pixelPos];
        float3 crruentNor = gNormalMap[pixelPos].xyz;

        float frameReset = frameReset_DepthRange_NorRange.x;
        float DepthRange = frameReset_DepthRange_NorRange.y;
        float NorRange = frameReset_DepthRange_NorRange.z;

        if (abs(prevDepth - crruentDepth) <= DepthRange && dot(prevNor, crruentNor) >= NorRange)
        {
            gFrameIndexMap[pixelPos]++;
        }
        else
        {
            gFrameIndexMap[pixelPos] = 0;
        }

        if (frameReset == 1.0f)
            gFrameIndexMap[pixelPos] = 0;

        const float CMA_Ratio = 1.0f / ((float) gFrameIndexMap[pixelPos] + 1.0f);
        float3 prev = gOutput[pixelPos];
        float3 le = lerp(prev, col, CMA_Ratio);
        gOutput[pixelPos] = float4(le, 1.0f);
    }
    else
    {
        gOutput[pixelPos] = float4(col, 1.0f);
    }
}

//通常
[shader("raygeneration")]
void rayGen()
{
	rayGenIn();
}

//InstanceIdMapテスト用
[shader("raygeneration")]
void rayGen_InstanceIdMapTest()
{
	rayGenIn();
	uint2 index = DispatchRaysIndex();
	int id = gInstanceIdMap[index];
	int numInstance = numEmissive.y;
	float3 ret = float3(0.0f, 0.0f, 0.0f);

	int step = 4096 / numInstance;
	int color3 = step;

	for (int i = 0; i < numInstance; i++)
	{
		int r = color3 >> 8;
		int g = (color3 >> 4) & 0x00f;
		int b = color3 & 0x00f;
		float rf = float(r) / 15.0f;
		float gf = float(g) / 15.0f;
		float bf = float(b) / 15.0f;
		if (id == i)
		{
			ret = float3(rf, gf, bf);
			break;
		}
		color3 += step;
	}

	gOutput[index] = float4(ret, 1.0f);
}
