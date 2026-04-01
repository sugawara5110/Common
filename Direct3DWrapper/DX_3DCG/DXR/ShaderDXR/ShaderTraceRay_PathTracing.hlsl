///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                      ShaderTraceRay_PathTracing.hlsl                                  //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "ShaderCommon.hlsl"

///////////////////////G項/////////////////////////////////////////////////////////////////////
float G(in float3 hitPosition, in float3 normal, in RayPayload payload, in int emIndex)
{
    uint emInstanceID = (uint) emissiveNo[emIndex].x;
    MaterialCB emMcb = getMaterialCB2(emInstanceID);
    uint NeeLightSampleType = emMcb.NeeLightSampleType;
    
    float3 lightVec = payload.hitPosition - hitPosition;
    float3 light_normal = payload.normal;
    float3 hitnormal = normal;
    float3 Lvec = normalize(lightVec);
    float cosine1 = saturate(dot(-Lvec, light_normal));
    float cosine2 = saturate(dot(Lvec, hitnormal));
     
    float distance = length(lightVec);
    float distAtten = distance * distance;
    float g = cosine1 * cosine2 / distAtten;
    
    if (NeeLightSampleType == SOLID_ANGLE)
    {
        g = cosine2;
    }
    return g;
}

///////////////////////NeeGetLight///////////////////////////////////////////////////////////////
RayPayload NeeGetLight(in uint RecursionCnt, in float3 hitPosition, in float3 normal, 
                       inout int emIndex, inout uint Seed)
{
    int NumEmissive = (int) numEmissive.x;
/////光源サイズ合計
    float sumSize = AllLightArea(emIndex, hitPosition);
    
    if (useImageBasedLighting)
        sumSize += IBL_size;

/////乱数を生成
    float rnd = Rand(Seed);

/////光源毎のサイズから全光源の割合を計算,そこからインデックスを選択
    float sum_min = 0;
    float sum_max = 0;
    for (int i = 0; i < NumEmissive; i++)
    {
        sum_min = sum_max;
        sum_max += LightArea(i, hitPosition, true) / sumSize; //サイズの割合を累積
        if (sum_min <= rnd && rnd < sum_max)
        {
            emIndex = i;
            break;
        } //乱数が累積値の範囲に入ったらそのインデックス値を選択
    }

    RayPayload payload;
    RayDesc ray;
    
    float3 ePos;
    uint ray_flag;
    float3 RandomVector_Dir = float3(1.0f, 0.0f, 0.0f);
    bool emRay = false;
								
    if (emIndex >= 0)
    {
        uint emInstanceID = (uint) emissiveNo[emIndex].x;
        uint emInstancingID = getInstancingID2(emInstanceID);
        MaterialCB emMcb = getMaterialCB2(emInstanceID);
        uint NeeLightType = emMcb.NeeLightType;
    
        if (NeeLightType == RECTANGLE)
        {
            payload.hitPosition = sampleRectLight(emInstanceID, emInstancingID, Seed);
            payload.hit = true;
            payload.Seed = Seed;
        }
        if (NeeLightType == SPHERE)
        {
            payload.hitPosition = sampleSphereLight(emIndex, hitPosition, Seed);
            payload.hit = true;
            payload.Seed = Seed;
        }
        if (NeeLightType == OTHERS)
        {
            ePos = emissivePosition[emIndex].xyz;
            ray_flag = RAY_FLAG_CULL_FRONT_FACING_TRIANGLES;
            RandomVector_Dir = normalize(hitPosition - ePos);
            emRay = true;
        }
    }
    else
    {
        ePos = hitPosition;
        ray_flag = RAY_FLAG_SKIP_CLOSEST_HIT_SHADER;
        RandomVector_Dir = normalize(normal);
        emRay = true;
    }
								
    if (emRay)
    {
        ray.Direction = RandomVector(RandomVector_Dir, 1.0f, Seed); //2.0f全方向, 1.0f半球								
        payload.Seed = Seed;
        payload.hitPosition = ePos;
        payload.mNo = NEE; //処理分岐用

/////光源から点をランダムで取得
        traceRay(RecursionCnt, ray_flag, 0, 0, ray, payload);
        Seed = payload.Seed;
    }

    if (payload.hit)
    {
        float3 lightVec = payload.hitPosition - hitPosition;
        float targetDist = length(lightVec);
        float e = max(0.0001f, 0.0001f * targetDist);
        ray.Direction = normalize(lightVec);
        payload.hitPosition = hitPosition;
        payload.mNo = NEE; //処理分岐用
////////今の位置から取得した光源位置へ飛ばす
        traceRay(RecursionCnt, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, 0, 0, ray, payload);
        Seed = payload.Seed;
        if (payload.hit)
        {
            float hitDist = length(payload.hitPosition - hitPosition);
            if (hitDist < targetDist - e)
            {
                payload.hit = false;
                payload.color = float3(0, 0, 0);
            }
        }
    }
    
    return payload;
}

///////////////////////NextEventEstimation////////////////////////////////////////////////////
float3 NextEventEstimation(in float3 outDir, in uint RecursionCnt, in float3 hitPosition, 
                           in float3 difTexColor, in float3 speTexColor, in float3 normal,
                           in int bsdf_f, in float in_eta, in float out_eta, 
                           inout uint Seed, out bool hit)
{
    float norDir = dot(outDir, normal);
    if (norDir < 0.0f)
    { //法線が反対側の場合, 物質内部と判断
        normal *= -1.0f;
    }
    
    int emIndex = -1;
    RayPayload neeP = NeeGetLight(RecursionCnt, hitPosition, normal, emIndex, Seed);

    float g = 1.0f;

    float3 inDir = normalize(neeP.hitPosition - hitPosition);

    float3 local_inDir = worldToLocal(normal, inDir);
    float3 local_outDir = worldToLocal(normal, outDir);

    float pdf;
    float3 bsdf = BSDF(bsdf_f, local_inDir, local_outDir, difTexColor, speTexColor, local_normal, in_eta, out_eta, pdf);

    float PDF;
    
    if (emIndex >= 0)
    {
        PDF = LightPDF(emIndex, hitPosition);
        g = G(hitPosition, normal, neeP, emIndex);
        if (bsdf_f == 2)
        {
            bsdf *= PI;
        }
    }
    else
    {
        PDF = IBL_PDF();
    }
     
    hit = neeP.hit;
    return bsdf * g * neeP.color / PDF;
}

///////////////////////PathTracing////////////////////////////////////////////////////////////
RayPayload PathTracing(in float3 outDir, in uint RecursionCnt, in float3 hitPosition, 
                       in float3 difTexColor, in float3 speTexColor, in float3 normal, 
                       in float3 throughput, in uint matNo,
                       out int bsdf_f, out float in_eta, out float out_eta, inout uint seed)
{
    RayPayload payload;
    payload.Seed = seed;
    payload.hitPosition = hitPosition;

    float rouPDF = min(max(max(throughput.x, throughput.y), throughput.z), 1.0f);

    MaterialCB mcb = getMaterialCB();
    uint mNo = mcb.materialNo;
    float roughness = mcb.roughness;

    float3 rDir = float3(0.0f, 0.0f, 0.0f);

    in_eta = AIR_RefractiveIndex;
    out_eta = mcb.RefractiveIndex;

    float norDir = dot(outDir, normal);
    if (norDir < 0.0f)
    { //法線が反対側の場合, 物質内部と判断
        normal *= -1.0f;
        in_eta = mcb.RefractiveIndex;
        out_eta = AIR_RefractiveIndex;
    }

    bsdf_f = 2;
    float rnd = Rand(payload.Seed);
    const float ft = 1.0f - FresnelSchlick3(outDir, speTexColor, normal, TRANSLUCENCE);
    
    if (ft > rnd && materialIdent(mNo, TRANSLUCENCE))
    {
        bsdf_f = 0;
        float eta = in_eta / out_eta; //eta = 入射前物質の屈折率 / 入射後物質の屈折率

        float3 eyeVec = -outDir;
        
        if (roughness <= 0.0f)
        {
            rDir = refract(eyeVec, normal, eta);
        }
        else
        {
            float3 H = SampleGGX(normal, roughness, payload.Seed);
            rDir = refract(eyeVec, H, eta);
        }
    }
    else if (materialIdent(mNo, METALLIC))
    {
        bsdf_f = 1;
        float3 eyeVec = -outDir;
        float3 H = float3(0, 0, 0);
        if (roughness <= 0.0f)
        {
            rDir = reflect(eyeVec, normal);
            H = normalize(rDir + outDir);
        }
        else
        {
            H = SampleGGX(normal, roughness, payload.Seed);
            rDir = reflect(eyeVec, H);
        }
    }
        
    if (materialIdent(mNo, DIFFUSE))
    {
        bsdf_f = 2;
        rDir = SampleHemisphereCosine(normal, payload.Seed);
    }

    RayDesc ray;
    ray.Direction = rDir;
    
    float3 local_inDir = worldToLocal(normal, rDir);
    float3 local_outDir = worldToLocal(normal, outDir);
    
    float PDF = 0.0f;
    float cosine = abs(dot(local_normal, local_inDir));
    
    float3 bsdf = BSDF(bsdf_f, local_inDir, local_outDir, difTexColor, speTexColor, local_normal, in_eta, out_eta, PDF);

    throughput *= (bsdf * cosine / PDF / rouPDF);
   
    payload.throughput = throughput;
    
    /////確率的に処理を打ち切り
    rnd = Rand(payload.Seed);
    if (rnd > rouPDF)
    {
        payload.throughput = float3(0.0f, 0.0f, 0.0f);
        payload.color = float3(0.0f, 0.0f, 0.0f);
        payload.hit = false;
        seed = payload.Seed;
        return payload;
    }

    payload.hitPosition = hitPosition;
    payload.mNo = matNo; //処理分岐用

    traceRay(RecursionCnt, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, 0, 0, ray, payload); //透過マテリアルはcpp側でフラグ処理済み
    
    payload.throughput = throughput;
    
    seed = payload.Seed;
    
    return payload;
}

///////////////////////PayloadCalculate_PathTracing///////////////////////////////////////////
float3 PayloadCalculate_PathTracing(in uint RecursionCnt, in float3 hitPosition, 
                                    in float3 difTexColor, in float3 speTexColor, in float3 normal, 
                                    in float3 throughput, inout int hitInstanceId, inout uint Seed)
{
    float3 ret = float3(0.0f, 0.0f, 0.0f);

    hitInstanceId = (int) getInstancingID();

    MaterialCB mcb = getMaterialCB();
    uint mNo = mcb.materialNo;
    float roughness = mcb.roughness;
    
    float3 outDir = -WorldRayDirection();

    uint matNo = NEE_PATHTRACER;
    if (traceMode == 1)
        matNo = EMISSIVE;

    int bsdf_f;
    float in_eta;
    float out_eta;
    
    RayPayload pathPay = PathTracing(outDir, RecursionCnt, hitPosition, difTexColor, speTexColor,
                                     normal, throughput, matNo, bsdf_f, in_eta, out_eta, Seed);

    if (traceMode == 2)
    {
        /////NextEventEstimation
        bool neeHit;
        float3 neeCol = NextEventEstimation(outDir, RecursionCnt, hitPosition, difTexColor, speTexColor,
                                            normal, bsdf_f, in_eta, out_eta, Seed, neeHit);
        
        bool f = roughness <= 0.0f && materialIdent(mNo, METALLIC) || //完全鏡面
             materialIdent(mNo, TRANSLUCENCE); //透過
        
        if (pathPay.hit)
        {
            if (f)
            {
                ret = pathPay.color * pathPay.throughput; //完全反射と透過は通常パストレと同じ寄与
            }
            else
            {
                ret = float3(0.0f, 0.0f, 0.0f);
            }
        }
        else
        {
            ret = pathPay.color; //光源ヒットがない場合はそのまま値を返す
        }
        
        if (neeHit && !f)
        {
            ret += neeCol * throughput; //通常NEE処理
        }
    }
    else
    {
        /////PathTracing
        if (pathPay.hit)
        {
            ret = pathPay.color * pathPay.throughput; //光源ヒット時のみthroughputを乗算し値を返す
        }
        else
        {
            ret = pathPay.color; //光源ヒットがない場合はそのまま値を返す
        }
    }
    hitInstanceId = pathPay.hitInstanceId;

    return ret;
}