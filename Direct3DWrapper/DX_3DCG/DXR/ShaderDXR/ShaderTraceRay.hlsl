///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                      ShaderTraceRay.hlsl                                              //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "ShaderTraceRay_PathTracing.hlsl"

///////////////////////光源へ光線を飛ばす, ヒットした場合明るさが加算//////////////////////////
float3 EmissivePayloadCalculate(in uint RecursionCnt, in float3 hitPosition, in float3 outDir,
                                in float3 difTexColor, in float3 speTexColor, in float3 normal)
{
    MaterialCB mcb = getMaterialCB();
    
    RayPayload payload;
    payload.hit = false;
    
    float3 emissiveColor = float3(0.0f, 0.0f, 0.0f);
    
    //マテリアル情報
    float3 Diffuse = mcb.Diffuse.xyz * difTexColor;
    
    //光源計算
    int NumEmissive = numEmissive.x;
    for (int i = 0; i < NumEmissive; i++)
    {
        if (emissivePosition[i].w == 1.0f)
        {
            //光源方向
            float3 lightVec = emissivePosition[i].xyz - hitPosition;
            float3 rDir = normalize(lightVec);
            
            RayDesc ray;
            ray.Direction = rDir;
            
            payload.hitPosition = hitPosition;
            payload.mNo = EMISSIVE;
            
            //光源までRayを飛ばす
            while (true)
            {
                traceRay(RecursionCnt, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, 0, 0, ray, payload);
                if (payload.EmissiveIndex == i || !materialIdent(payload.mNo, EMISSIVE))
                {
                    break;
                }
            }
            
            //実際にこの光源が見えている場合
            if (materialIdent(payload.mNo, EMISSIVE))
            {
                float3 ePos = payload.hitPosition;
                
                //実際にヒットした光源までの距離
                float3 actualLightVec = ePos - hitPosition;
                float actualDistance = length(actualLightVec);
                
                if (actualDistance <= 0.0f)
                    continue;
                
                float3 inDir = normalize(actualLightVec);
                // N・L
                float NoL = saturate(dot(normal, inDir));
                
                if (NoL <= 0.0f)
                    continue;
                
                //距離減衰
                float distAtten = 1.0f / max(actualDistance * actualDistance, 0.0001f);
                
                //光源から来た放射輝度
                float3 lightColor = payload.color;
                
                float3 diffuseBRDF = DiffuseBRDF(Diffuse);
                
                float3 specularBRDF = float3(0.0f, 0.0f, 0.0f);
                
                float3 refractionBTDF = float3(0.0f, 0.0f, 0.0f);
                
                if (materialIdent(getMaterialCB().materialNo, METALLIC))
                {
                    float no_use_pdf;
                    specularBRDF = SpecularBRDF_PDF(inDir, outDir, difTexColor, speTexColor, normal, no_use_pdf);
                }
                
                if (materialIdent(getMaterialCB().materialNo, TRANSLUCENCE))
                {
                    float in_eta = AIR_RefractiveIndex;
                    float out_eta = mcb.RefractiveIndex;

                    float norDir = dot(outDir, normal);

                    if (norDir < 0.0f)
                    { //法線が反対側の場合, 物質内部と判断
                        normal *= -1.0f;
                        in_eta = out_eta;
                        out_eta = AIR_RefractiveIndex;
                    }
                    float no_use_pdf;
                    refractionBTDF = RefractionBTDF_PDF(inDir, outDir, difTexColor, speTexColor, normal, in_eta, out_eta, no_use_pdf);
                }
                               
                emissiveColor += (diffuseBRDF + specularBRDF + refractionBTDF) * lightColor * NoL * distAtten;
            }
        }
    }
    return emissiveColor;
}

///////////////////////反射方向へ光線を飛ばす, ヒットした場合ピクセル値乗算///////////////////////
float3 MetallicPayloadCalculate(in uint RecursionCnt, in float3 hitPosition, in float3 outDir,
                                in float3 difTexColor, in float3 normal, inout int hitInstanceId, 
                                inout uint Seed)
{
    MaterialCB mcb = getMaterialCB();
    uint mNo = mcb.materialNo;

    float3 ret = difTexColor;

    hitInstanceId = (int) getInstancingID();

    if (materialIdent(mNo, METALLIC))
    {
        RayPayload payload;
        RayDesc ray;

        payload.hitPosition = hitPosition;
        payload.Seed = Seed;

        float3 eyeVec = -outDir;

        //反射方向
        float3 reflectVec;

        float roughness = mcb.roughness;

        if (roughness <= 0.0f)
        {
            //完全鏡面
            reflectVec = reflect(eyeVec, normalize(normal));
        }
        else
        {
            //GGXによる粗い反射
            float3 H = SampleGGX(
                normalize(normal),
                roughness,
                payload.Seed);

            reflectVec = reflect(eyeVec, H);
        }

        ray.Direction = normalize(reflectVec);

        //反射方向へRayを飛ばす
        traceRay(
            RecursionCnt,
            RAY_FLAG_CULL_BACK_FACING_TRIANGLES,
            0,
            0,
            ray,
            payload);

        //戻ってきた色
        float3 outCol = float3(0.0f, 0.0f, 0.0f);

        if (payload.hit)
        {
            float3 refCol = payload.color;

            // ヒットした場合、映り込みとして乗算
            outCol = difTexColor * refCol;

            hitInstanceId = payload.hitInstanceId;

            int hitmNo = payload.mNo;

            if (materialIdent(hitmNo, EMISSIVE))
            {
                outCol = refCol;
            }
        }
        else
        {
            // ヒットしなかった場合
            outCol = difTexColor;
        }

        ret = outCol;
    }

    return ret;
}

////////////////////////////////////////半透明//////////////////////////////////////////
float3 Translucent(in uint RecursionCnt, in float3 hitPosition, in float3 outDir, 
                   in float4 difTexColor, in float3 speTexColor, in float3 normal)
{
    MaterialCB mcb = getMaterialCB();
    uint mNo = mcb.materialNo;

    float3 ret = difTexColor.xyz;
    float transparency = 1.0f - difTexColor.w;

    if (materialIdent(mNo, TRANSLUCENCE) && transparency > 0.0f)
    {
        float in_eta = AIR_RefractiveIndex;
        float out_eta = mcb.RefractiveIndex;

        float norDir = dot(outDir, normal);

        if (norDir < 0.0f)
        { //法線が反対側の場合, 物質内部と判断
            normal *= -1.0f;
            in_eta = out_eta;
            out_eta = AIR_RefractiveIndex;
        }

        float eta = in_eta / out_eta; //eta = 入射前物質の屈折率 / 入射後物質の屈折率

        float3 eyeVec = -outDir;

        float F = FresnelSchlick3(outDir, difTexColor.xyz, speTexColor, normal);

        float3 refractDir = refract(eyeVec, normal, eta);

        float3 refractColor = float3(0.0f, 0.0f, 0.0f);

        // refract() がゼロなら全反射
        if (length(refractDir) > 0.0f)
        {
            RayPayload refractPayload;
            RayDesc refractRay;
            refractRay.Direction = normalize(refractDir);

            refractPayload.hitPosition = hitPosition;

            traceRay(
                RecursionCnt,
                RAY_FLAG_CULL_BACK_FACING_TRIANGLES,
                0,
                0,
                refractRay,
                refractPayload);

            refractColor = refractPayload.color;
        }
        else
        {
            // 全反射
            F = 1.0f;
        }

        // 反射 + 屈折
        float3 glassColor =
            difTexColor.xyz * F +
            refractColor * (1.0f - F);

        // Alpha
        ret = glassColor * transparency +
            difTexColor.xyz * (1.0f - transparency);
    }
    
    return ret;
}

////////////////////////////////////////ONE_RAY//////////////////////////////////////////
float3 PayloadCalculate_OneRay(in uint RecursionCnt, in float3 hitPosition,
                               in float4 difTex, in float3 speTex, in float3 normalMap,
                               inout int hitInstanceId, inout uint Seed)
{
    float3 outDir = -WorldRayDirection();
    
    difTex.xyz = EmissivePayloadCalculate(RecursionCnt, hitPosition, outDir, difTex.xyz, speTex, normalMap);

    difTex.xyz = MetallicPayloadCalculate(RecursionCnt, hitPosition, outDir, difTex.xyz, normalMap, hitInstanceId, Seed);

    difTex.xyz = Translucent(RecursionCnt, hitPosition, outDir, difTex, speTex, normalMap);

    return difTex.xyz;
}