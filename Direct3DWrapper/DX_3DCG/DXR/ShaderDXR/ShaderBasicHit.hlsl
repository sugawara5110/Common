///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                           ShaderBasicHit.hlsl                                         //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "ShaderTraceRay.hlsl"

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

        difTex.xyz = PayloadCalculate_OneRay(payload.RecursionCnt, payload.hitPosition, difTex, speTex,
                                             normalMap, payload.hitInstanceId);

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
                                                         payload.throughput, payload.hitInstanceId, payload.Seed);
        }
    }
    payload.mNo = getMaterialCB().materialNo;
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