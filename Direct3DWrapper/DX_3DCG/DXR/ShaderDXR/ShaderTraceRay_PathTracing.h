///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                      ShaderTraceRay_PathTracing.hlsl                                  //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

char* ShaderTraceRay_PathTracing =

///////////////////////G項/////////////////////////////////////////////////////////////////////
"float G(in float3 hitPosition, in float3 normal, in RayPayload payload, uint emIndex)\n"
"{\n"
"    float3 lightVec = payload.hitPosition - hitPosition;\n"
"    float3 light_normal = payload.normal;\n"
"    float3 hitnormal = normal;\n"
"    float3 Lvec = normalize(lightVec);\n"
"    float cosine1 = saturate(dot(-Lvec, light_normal));\n"
"    float cosine2 = saturate(dot(Lvec, hitnormal));\n"
"    float distance = length(lightVec);\n"
"    float distAtten = distance * distance * lightst[emIndex].w;\n"
"    return cosine1 * cosine2 / distAtten;\n"
"}\n"

///////////////////////NeeGetLight///////////////////////////////////////////////////////////////
"RayPayload NeeGetLight(in uint RecursionCnt, in float3 hitPosition, in float3 normal, inout uint emIndex)\n"
"{\n"
"    uint NumEmissive = numEmissive.x;\n"
/////光源サイズ合計
"    float sumSize = 0.0f;\n"
"    for(uint i = 0; i < NumEmissive; i++){\n"
"       sumSize += emissiveNo[i].y;\n"
"    }\n"

/////乱数を生成
"    uint rnd = Rand_integer() % 101;\n"

/////光源毎のサイズから全光源の割合を計算,そこからインデックスを選択
"    uint sum_min = 0;\n"
"    uint sum_max = 0;\n"
"    for(uint i = 0; i < NumEmissive; i++){\n"
"       sum_min = sum_max;\n"
"       sum_max += (uint)(emissiveNo[i].y / sumSize * 100.0f);\n"//サイズの割合を累積
"       if(sum_min <= rnd && rnd < sum_max){emIndex = i; break;}\n"//乱数が累積値の範囲に入ったらそのインデックス値を選択
"    }\n"

"    float3 ePos = emissivePosition[emIndex].xyz;\n"

"    RayDesc ray;\n"
"    ray.Direction = RandomVector(float3(1.0f, 0.0f, 0.0f), 2.0f);\n"//2.0f全方向

"    RayPayload payload;\n"
"    payload.color = float3(0.0f, 0.0f, 0.0f);\n"
"    payload.hitPosition = ePos;\n"
"    payload.mNo = EMISSIVE | NEE;\n"//処理分岐用

/////光源から点をランダムで取得
"    traceRay(RecursionCnt, RAY_FLAG_CULL_FRONT_FACING_TRIANGLES, 1, 1, ray, payload);\n"

"    if(payload.hit){\n"
"       float3 lightVec = payload.hitPosition - hitPosition;\n"
"       ray.Direction = normalize(lightVec);\n"
"       payload.hitPosition = hitPosition;\n"
"       payload.mNo = EMISSIVE | NEE;\n"//処理分岐用
////////今の位置から取得した光源位置へ飛ばす
"       traceRay(RecursionCnt, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, 1, 1, ray, payload);\n"
"    }\n"
"    return payload;\n"
"}\n"

///////////////////////NextEventEstimation////////////////////////////////////////////////////
"float3 NextEventEstimation(in float3 outDir, in uint RecursionCnt, in float3 hitPosition, \n"
"                           in float3 difTexColor, in float3 speTexColor, in float3 normal)\n"
"{\n"
"    uint emIndex = 0;\n"
"    RayPayload neeP = NeeGetLight(RecursionCnt, hitPosition, normal, emIndex);\n"

"    float g = G(hitPosition, normal, neeP, emIndex);\n"

"    float3 inDir = normalize(neeP.hitPosition - hitPosition);\n"

"    float3 local_inDir = worldToLocal(normal, inDir);\n"
"    float3 local_outDir = worldToLocal(normal, outDir);\n"
"    if(local_outDir.z < 0){\n"
"       local_outDir = worldToLocal(normal * -1.0f, outDir);\n"
"    }\n"

"    const float3 local_normal = float3(0.0f, 0.0f, 1.0f);\n"

"    float pdf;\n"
"    float3 bsdf = SumBSDF(local_inDir, local_outDir, difTexColor, speTexColor, local_normal, pdf);\n"

"    float PDF = LightPDF(emIndex);\n"
"    return (bsdf * g / PDF) * neeP.color;\n"
"}\n"

///////////////////////PathTracing////////////////////////////////////////////////////////////
"RayPayload PathTracing(in float3 outDir, in uint RecursionCnt, in float3 hitPosition, \n"
"                       in float3 difTexColor, in float3 speTexColor, in float3 normal, \n"
"                       in float3 throughput, in uint matNo)\n"
"{\n"
"    RayPayload payload;\n"
"    payload.hitPosition = hitPosition;\n"
"    payload.hit = false;\n"

"    float rouPDF = min(max(max(throughput.x, throughput.y), throughput.z), 1.0f);\n"
/////確率的に処理を打ち切り これやらないと白っぽくなる
"    uint rnd = Rand_integer() % 101;\n"
"    if(rnd > (uint)(rouPDF * 100.0f)){\n"
"       payload.throughput = float3(0.0f, 0.0f, 0.0f);\n"
"       payload.color = float3(0.0f, 0.0f, 0.0f);\n"
"       return payload;\n"
"    }\n"

"    uint materialID = getMaterialID();\n"
"    MaterialCB mcb = material[materialID];\n"
"    float3 Diffuse = mcb.Diffuse.xyz;\n"
"    float3 Speculer = mcb.Speculer.xyz;\n"
"    uint mNo = mcb.materialNo;\n"
"    float roughness = mcb.roughness;\n"

"    float sum_diff = Diffuse.x + Diffuse.y + Diffuse.z;\n"
"    float sum_spe = Speculer.x + Speculer.y + Speculer.z;\n"
"    float sum = sum_diff + sum_spe;\n"
"    uint diff_threshold = (uint)(sum_diff / sum * 100.0f);\n"

"    float3 rDir = float3(0.0f, 0.0f, 0.0f);\n"

"    if(diff_threshold < rnd && materialIdent(mNo, METALLIC)){\n"//Speculer
"       float3 eyeVec = -outDir;\n"
"       float3 reflectVec = reflect(eyeVec, normal);\n"
"       float Area = roughness * roughness;\n"
"       rDir = RandomVector(reflectVec, Area);\n"
"    }\n"
"    else{\n"//Diffuse
"       rDir = RandomVector(normal, 1.0f);\n"//1.0f半球
"    }\n"

"    RayDesc ray;\n"
"    ray.Direction = rDir;\n"

"    float3 local_inDir = worldToLocal(normal, ray.Direction);\n"
"    float3 local_outDir = worldToLocal(normal, outDir);\n"

"    const float3 local_normal = float3(0.0f, 0.0f, 1.0f);\n"

"    float PDF;\n"
"    float3 bsdf = SumBSDF(local_inDir, local_outDir, difTexColor, speTexColor, local_normal, PDF);\n"
"    float cosine = saturate(dot(local_normal, local_inDir));\n"

"    throughput *= (bsdf * cosine / PDF);\n"

"    payload.throughput = throughput / rouPDF;\n"

"    payload.hitPosition = hitPosition;\n"
"    payload.mNo = matNo;\n"//処理分岐用

"    traceRay(RecursionCnt, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, 1, 1, ray, payload);\n"

"    return payload;\n"
"}\n"

///////////////////////PayloadCalculate_PathTracing///////////////////////////////////////////
"float3 PayloadCalculate_PathTracing(in uint RecursionCnt, in float3 hitPosition, \n"
"                                    in float3 difTexColor, in float3 speTexColor, in float3 normal, \n"
"                                    inout float3 throughput)\n"
"{\n"
"    float3 ret = difTexColor;\n"//光源だった場合、映り込みがそのまま出る・・・あとで変更する

"    uint materialID = getMaterialID();\n"
"    MaterialCB mcb = material[materialID];\n"
"    uint mNo = mcb.materialNo;\n"

"    bool mf = materialIdent(mNo, EMISSIVE);\n"
"    if(!mf) {\n"//emissive以外

"       float3 outDir = -WorldRayDirection();\n"

////////PathTracing
"       uint matNo = EMISSIVE | NEE_PATHTRACER;\n"
"       if(traceMode == 1)matNo = EMISSIVE;\n"

"       RayPayload pathPay = PathTracing(outDir, RecursionCnt, hitPosition, difTexColor, speTexColor, normal, throughput, matNo);\n"

////////NextEventEstimation
"       if(traceMode == 2){\n"
"          float3 neeCol = NextEventEstimation(outDir, RecursionCnt, hitPosition, difTexColor, speTexColor, normal);\n"
"          ret = pathPay.color + neeCol * throughput;\n"
"       }\n"
"       else{\n"
"          if(pathPay.hit){\n"
"             ret = pathPay.color * pathPay.throughput;\n"//光源ヒット時のみthroughputを乗算し値を返す
"          }\n"
"          else{\n"
"             ret = pathPay.color;\n"//光源ヒットがない場合はそのまま値を返す
"          }\n"
"       }\n"

"       throughput = pathPay.throughput;\n"//throughputの更新
"    }\n"
"    return ret;\n"
"}\n";