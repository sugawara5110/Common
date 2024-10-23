///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                      ShaderTraceRay_PathTracing.hlsl                                  //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "ShaderCommon.hlsl"

///////////////////////G項/////////////////////////////////////////////////////////////////////
float G(in float3 hitPosition, in float3 normal, in RayPayload payload)
{
    float3 lightVec = payload.hitPosition - hitPosition;
    float3 light_normal = payload.normal;
    float3 hitnormal = normal;
    float3 Lvec = normalize(lightVec);
    float cosine1 = saturate(dot(-Lvec, light_normal));
    float cosine2 = saturate(dot(Lvec, hitnormal));
    float distance = length(lightVec);
    float distAtten = distance * distance;
    return cosine1 * cosine2 / distAtten;
}

///////////////////////NeeGetLight///////////////////////////////////////////////////////////////
RayPayload NeeGetLight(in uint RecursionCnt, in float3 hitPosition, in float3 normal, inout int emIndex)
{
	uint NumEmissive = numEmissive.x;
/////光源サイズ合計
	float sumSize = 0.0f;
	for (uint i = 0; i < NumEmissive; i++)
	{
		sumSize += emissiveNo[i].y;
	}
	if (useImageBasedLighting)
		sumSize += IBL_size;

/////乱数を生成
	uint rnd = Rand_integer() % 101;

/////光源毎のサイズから全光源の割合を計算,そこからインデックスを選択
	uint sum_min = 0;
	uint sum_max = 0;
	for (uint i = 0; i < NumEmissive; i++)
	{
		sum_min = sum_max;
		sum_max += (uint) (emissiveNo[i].y / sumSize * 100.0f); //サイズの割合を累積
		if (sum_min <= rnd && rnd < sum_max)
		{
			emIndex = i;
			break;
		} //乱数が累積値の範囲に入ったらそのインデックス値を選択
	}

	float3 ePos;
	uint ray_flag;

	if (emIndex >= 0)
	{
		ePos = emissivePosition[emIndex].xyz;
		ray_flag = RAY_FLAG_CULL_FRONT_FACING_TRIANGLES;
	}
	else
	{
		ePos = hitPosition;
		ray_flag = RAY_FLAG_SKIP_CLOSEST_HIT_SHADER;
	}

	RayDesc ray;
	ray.Direction = RandomVector(float3(1.0f, 0.0f, 0.0f), 2.0f); //2.0f全方向

	RayPayload payload;
	payload.hitPosition = ePos;
	payload.mNo = NEE; //処理分岐用

/////光源から点をランダムで取得
	traceRay(RecursionCnt, ray_flag, 0, 0, ray, payload);

	if (payload.hit)
	{
		float3 lightVec = payload.hitPosition - hitPosition;
		ray.Direction = normalize(lightVec);
		payload.hitPosition = hitPosition;
		payload.mNo = NEE; //処理分岐用
////////今の位置から取得した光源位置へ飛ばす
		traceRay(RecursionCnt, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, 0, 0, ray, payload);
	}
	return payload;
}

///////////////////////NextEventEstimation////////////////////////////////////////////////////
float3 NextEventEstimation(in float3 outDir, in uint RecursionCnt, in float3 hitPosition, 
                           in float4 difTexColor, in float3 speTexColor, in float3 normal,
                           in bool bsdf_f, in float in_eta, in float out_eta)
{
    float norDir = dot(outDir, normal);
    if (norDir < 0.0f)
    { //法線が反対側の場合, 物質内部と判断
        normal *= -1.0f;
    }
    
    int emIndex = -1;
    RayPayload neeP = NeeGetLight(RecursionCnt, hitPosition, normal, emIndex);

    float g = G(hitPosition, normal, neeP);

    float3 inDir = normalize(neeP.hitPosition - hitPosition);

    float3 local_inDir = worldToLocal(normal, inDir);
    float3 local_outDir = worldToLocal(normal, outDir);

    float pdf;
    float3 bsdf = BSDF(bsdf_f, local_inDir, local_outDir, difTexColor, speTexColor, local_normal, in_eta, out_eta, pdf);

    float PDF;
    if (emIndex >= 0)
    {
        PDF = LightPDF(emIndex);
    }
    else
    {
        PDF = IBL_PDF();
        g = 1.0f;
    }
    return saturate(bsdf * g) * neeP.color / PDF;
}

///////////////////////PathTracing////////////////////////////////////////////////////////////
RayPayload PathTracing(in float3 outDir, in uint RecursionCnt, in float3 hitPosition, 
                       in float4 difTexColor, in float3 speTexColor, in float3 normal, 
                       in float3 throughput, in uint matNo,
                       out bool bsdf_f, out float in_eta, out float out_eta)
{
    RayPayload payload;
    payload.hitPosition = hitPosition;

    float rouPDF = min(max(max(throughput.x, throughput.y), throughput.z), 1.0f);
/////確率的に処理を打ち切り これやらないと白っぽくなる
    uint rnd = Rand_integer() % 101;
    if (rnd > (uint) (rouPDF * 100.0f))
    {
        payload.throughput = float3(0.0f, 0.0f, 0.0f);
        payload.color = float3(0.0f, 0.0f, 0.0f);
        payload.hit = false;
        return payload;
    }

    uint materialID = getMaterialID();
    MaterialCB mcb = material[materialID];
    float3 Diffuse = mcb.Diffuse.xyz;
    float3 Speculer = mcb.Speculer.xyz;
    uint mNo = mcb.materialNo;
    float roughness = mcb.roughness;

    float sum_diff = Diffuse.x + Diffuse.y + Diffuse.z;
    float sum_spe = Speculer.x + Speculer.y + Speculer.z;
    float sum = sum_diff + sum_spe;
    uint diff_threshold = (uint) (sum_diff / sum * 100.0f);

    float Alpha = difTexColor.w;

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

    bsdf_f = true;
    rnd = Rand_integer() % 101;
    if ((uint) (Alpha * 100.0f) < rnd && materialIdent(mNo, TRANSLUCENCE))
    { //透過

//////////////eta = 入射前物質の屈折率 / 入射後物質の屈折率
        float eta = in_eta / out_eta;

        float3 eyeVec = -outDir;
        float3 refractVec = refract(eyeVec, normal, eta);
        float Area = roughness * roughness;
        
        if (roughness <= 0.0f)
        {
            rDir = refractVec;
        }
        else
        {
            rDir = RandomVector(refractVec, Area);
        }
    }
    else
    {
        bsdf_f = false;
        rnd = Rand_integer() % 101;
        if (diff_threshold < rnd && materialIdent(mNo, METALLIC))
        { //Speculer
            float3 eyeVec = -outDir;
            float3 reflectVec = reflect(eyeVec, normal);
            float Area = roughness * roughness;
            if (roughness <= 0.0f)
            {
                rDir = reflectVec;
            }
            else
            {
                rDir = RandomVector(reflectVec, Area);
            }
        }
        else
        { //Diffuse
            rDir = RandomVector(normal, 1.0f); //1.0f半球
        }
    }

    RayDesc ray;
    ray.Direction = rDir;

    float3 local_inDir = worldToLocal(normal, ray.Direction);
    float3 local_outDir = worldToLocal(normal, outDir);

    float PDF = 0.0f;
    float cosine = abs(dot(local_normal, local_inDir));
    
    float3 bsdf = BSDF(bsdf_f, local_inDir, local_outDir, difTexColor, speTexColor, local_normal, in_eta, out_eta, PDF);

    throughput *= (bsdf * cosine / PDF / rouPDF);

    payload.throughput = throughput;

    payload.hitPosition = hitPosition;
    payload.mNo = matNo; //処理分岐用

    traceRay(RecursionCnt, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, 0, 0, ray, payload); //透過マテリアルはcpp側でフラグ処理済み
    
    payload.throughput = throughput;
 
/////NEE実行時かつ『NONREFLECTION』はパストレでの光源衝突は寄与しない

    if (payload.hit && matNo == NEE_PATHTRACER && materialIdent(mNo, NONREFLECTION))
    {
        payload.color = float3(0.0f, 0.0f, 0.0f);
    }
    
    return payload;
}

///////////////////////PayloadCalculate_PathTracing///////////////////////////////////////////
float3 PayloadCalculate_PathTracing(in uint RecursionCnt, in float3 hitPosition, 
                                    in float4 difTexColor, in float3 speTexColor, in float3 normal, 
                                    in float3 throughput, inout int hitInstanceId)
{
    float3 ret = difTexColor.xyz;

    hitInstanceId = (int) getInstancingID();

    uint materialID = getMaterialID();
    MaterialCB mcb = material[materialID];
    uint mNo = mcb.materialNo;

    float3 outDir = -WorldRayDirection();

/////PathTracing
    uint matNo = NEE_PATHTRACER;
    if (traceMode == 1)
        matNo = EMISSIVE;

    bool bsdf_f;
    float in_eta;
    float out_eta;
    RayPayload pathPay = PathTracing(outDir, RecursionCnt, hitPosition, difTexColor, speTexColor, normal, throughput, matNo,
                                     bsdf_f, in_eta, out_eta);

/////NextEventEstimation
    if (traceMode == 2)
    {
        float3 neeCol = NextEventEstimation(outDir, RecursionCnt, hitPosition, difTexColor, speTexColor, normal,
                                            bsdf_f, in_eta, out_eta);
        
        if (pathPay.hit && !materialIdent(mNo, NONREFLECTION))
        {
            ret = pathPay.color * pathPay.throughput; //光源ヒットかつ反射透過の場合, 通常パストレと同じ寄与
        }
        else
        {
            ret = pathPay.color + neeCol * pathPay.throughput; //通常NEE処理
        }
    }
    else
    {
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