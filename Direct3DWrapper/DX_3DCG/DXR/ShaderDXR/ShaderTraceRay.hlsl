///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                      ShaderTraceRay.hlsl                                              //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "ShaderTraceRay_PathTracing.hlsl"
#include "../../Core/ShaderCG/ShaderCalculateLighting.hlsl"

///////////////////////光源へ光線を飛ばす, ヒットした場合明るさが加算//////////////////////////
float3 EmissivePayloadCalculate(in uint RecursionCnt, in float3 hitPosition, 
                                in float3 difTexColor, in float3 speTexColor, in float3 normal)
{
    MaterialCB mcb = getMaterialCB();
    uint mNo = mcb.materialNo;

    RayPayload payload;
    payload.hit = false;
    LightOut emissiveColor = (LightOut) 0;
    LightOut Out;
    RayDesc ray;

    float3 SpeculerCol = mcb.Speculer.xyz;
    float3 Diffuse = mcb.Diffuse.xyz;
    float3 Ambient = mcb.Ambient.xyz + GlobalAmbientColor.xyz;
    float shininess = mcb.shininess;

////////光源計算
    uint NumEmissive = numEmissive.x;
    for (uint i = 0; i < NumEmissive; i++)
    {
        if (emissivePosition[i].w == 1.0f)
        {
            float3 lightVec = normalize(emissivePosition[i].xyz - hitPosition);

            ray.Direction = lightVec;

            payload.hitPosition = hitPosition;
            payload.mNo = EMISSIVE; //処理分岐用

            traceRay(RecursionCnt, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, 0, 0, ray, payload);
            
            if (!materialIdent(payload.mNo, EMISSIVE))
            {
                payload.color = float3(0, 0, 0);
            }
            float4 emissiveHitPos = emissivePosition[i];
            emissiveHitPos.xyz = payload.hitPosition;

            Out = PointLightCom(SpeculerCol, Diffuse, Ambient, normal, emissiveHitPos, //ShaderCG内関数
                                  hitPosition, lightst[i], payload.color, cameraPosition.xyz, shininess);

            emissiveColor.Diffuse += Out.Diffuse;
            emissiveColor.Speculer += Out.Speculer;
        }
    }
////////最後にテクスチャの色に掛け合わせ
    difTexColor *= emissiveColor.Diffuse;
    speTexColor *= emissiveColor.Speculer;
    return difTexColor + speTexColor;
}

///////////////////////反射方向へ光線を飛ばす, ヒットした場合ピクセル値乗算///////////////////////
float3 MetallicPayloadCalculate(in uint RecursionCnt, in float3 hitPosition, 
                                in float3 difTexColor, in float3 normal, inout int hitInstanceId)
{
    uint mNo = getMaterialCB().materialNo;
    float3 ret = difTexColor;

    hitInstanceId = (int) getInstancingID(); //自身のID書き込み

    if (materialIdent(mNo, METALLIC))
    { //METALLIC

        RayPayload payload;
        RayDesc ray;
//視線ベクトル 
        float3 eyeVec = WorldRayDirection();
//反射ベクトル
        float3 reflectVec = reflect(eyeVec, normalize(normal));
        ray.Direction = reflectVec; //反射方向にRayを飛ばす
        payload.hitPosition = hitPosition;

        traceRay(RecursionCnt, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, 0, 0, ray, payload);

        float3 outCol = float3(0.0f, 0.0f, 0.0f);
        if (payload.hit)
        {
            float3 refCol = payload.color;

            outCol = difTexColor * refCol; //ヒットした場合映り込みとして乗算
            hitInstanceId = payload.hitInstanceId; //ヒットしたID書き込み
            int hitmNo = payload.mNo;
            if (materialIdent(hitmNo, EMISSIVE))
            {
                outCol = refCol;
            }
        }
        else
        {
            outCol = difTexColor; //ヒットしなかった場合映り込み無しで元のピクセル書き込み
        }
        ret = outCol;
    }
    return ret;
}

////////////////////////////////////////半透明//////////////////////////////////////////
float3 Translucent(in uint RecursionCnt, in float3 hitPosition, in float4 difTexColor, in float3 normal)
{
    MaterialCB mcb = getMaterialCB();
    uint mNo = mcb.materialNo;
    float3 ret = difTexColor.xyz;
    float Alpha = difTexColor.w;

    if (materialIdent(mNo, TRANSLUCENCE) && Alpha < 1.0f)
    {

        float Alpha = difTexColor.w;
        RayPayload payload;
        RayDesc ray;
        
        float in_eta = AIR_RefractiveIndex;
        float out_eta = mcb.RefractiveIndex;
        
//視線ベクトル 
        float3 r_eyeVec = -WorldRayDirection(); //視線へのベクトル
        float norDir = dot(r_eyeVec, normal);
        if (norDir < 0.0f)
        {
            normal *= -1.0f;
            in_eta = mcb.RefractiveIndex;
            out_eta = AIR_RefractiveIndex;
        }
        
        float3 eyeVec = WorldRayDirection();
        float eta = in_eta / out_eta;
        ray.Direction = refract(eyeVec, normalize(normal), eta);
        payload.hitPosition = hitPosition;

        traceRay(RecursionCnt, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, 0, 0, ray, payload);

//アルファ値の比率で元の色と光線衝突先の色を配合
        ret = payload.color * (1.0f - Alpha) + difTexColor * Alpha;
    }
    return ret;
}

////////////////////////////////////////ONE_RAY//////////////////////////////////////////
float3 PayloadCalculate_OneRay(in uint RecursionCnt, in float3 hitPosition,
                               in float4 difTex, in float3 speTex, in float3 normalMap,
                               inout int hitInstanceId)
{
////////光源への光線
    difTex.xyz = EmissivePayloadCalculate(RecursionCnt, hitPosition, difTex.xyz, speTex, normalMap);

////////反射方向への光線
    difTex.xyz = MetallicPayloadCalculate(RecursionCnt, hitPosition, difTex.xyz, normalMap, hitInstanceId);

////////半透明
    difTex.xyz = Translucent(RecursionCnt, hitPosition, difTex, normalMap);

    return difTex.xyz;
}