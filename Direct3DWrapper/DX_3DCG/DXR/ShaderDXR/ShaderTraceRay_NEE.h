///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                      ShaderTraceRay_NEE.hlsl                                          //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

char* ShaderTraceRay_NEE =

///////////////////////NEE/////////////////////////////////////////////////////////////////////
"float3 Nee(in float3 hitPosition)\n"
"{\n"
"    uint NumEmissive = numEmissive.x;\n"
"    uint emIndex = Rand_integer() % NumEmissive;\n"
"    float3 ePos = emissivePosition[emIndex].xyz;\n"

"    RayDesc ray;\n"
"    ray.TMin = TMin_TMax.x;\n"
"    ray.TMax = TMin_TMax.y;\n"
"    ray.Direction = RandomVector(float3(1.0f, 0.0f, 0.0f), 2.0f);\n"

"    RayPayload payload;\n"
"    payload.hitPosition = ePos;\n"
"    bool loop = true;\n"
/////光源から点をランダムで取得
"    while(loop){\n"
"       payload.mNo = EMISSIVE | NEE;\n"//処理分岐用
"       ray.Origin = payload.hitPosition;\n"
"       TraceRay(gRtScene, RAY_FLAG_CULL_FRONT_FACING_TRIANGLES, \n"
"                0xFF, 1, 0, 1, ray, payload);\n"
"       loop = payload.reTry;\n"
"    }\n"

"    float3 normal1 = payload.normal;\n"
"    float3 ret = float3(0.0f, 0.0f, 0.0f);\n"

"    if(payload.hit){\n"
"       float3 lightVec = payload.hitPosition - hitPosition;\n"
"       ray.Direction = normalize(lightVec);\n"
"       payload.hitPosition = hitPosition;\n"
"       loop = true;\n"
////////今の位置から取得した光源位置へ飛ばす
"       while(loop){\n"
"          payload.mNo = EMISSIVE | NEE;\n"//処理分岐用
"          ray.Origin = payload.hitPosition;\n"
"          TraceRay(gRtScene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, \n"
"                   0xFF, 1, 0, 1, ray, payload);\n"
"          loop = payload.reTry;\n"
"       }\n"
"       float3 normal2 = payload.normal;\n"
"       float3 Lvec = normalize(lightVec);\n"
"       float cosine1 = abs(dot(-lightVec, normal1));\n"
"       float cosine2 = abs(dot(lightVec, normal2));\n"
"       float distance = length(lightVec);\n"
"       float distAtten = 1.0f / (distance * distance);\n"//後で考える
"       float G = cosine1 * cosine2 * distAtten;\n"
"       ret = G * payload.color;\n"
"    }\n"
"    return ret;\n"
"}\n"

///////////////////////光源へ光線を飛ばす, ヒットした場合明るさが加算//////////////////////////
"float3 EmissivePayloadCalculate_NEE(in uint RecursionCnt, in float3 hitPosition, \n"
"                                    in float3 difTexColor, in float3 speTexColor, in float3 normal)\n"
"{\n"
"    uint materialID = getMaterialID();\n"
"    MaterialCB mcb = material[materialID];\n"
"    uint mNo = mcb.materialNo;\n"
"    float3 ret = difTexColor;\n"//光源だった場合、映り込みがそのまま出る・・・あとで変更する

"    bool mf = materialIdent(mNo, EMISSIVE);\n"
"    if(!mf) {\n"//emissive以外

"       RayPayload payload;\n"
"       payload.hit = false;\n"
"       LightOut emissiveColor = (LightOut)0;\n"
"       LightOut Out;\n"
"       RayDesc ray;\n"
"       payload.hitPosition = hitPosition;\n"
"       ray.TMin = TMin_TMax.x;\n"
"       ray.TMax = TMin_TMax.y;\n"
"       RecursionCnt++;\n"
"       payload.RecursionCnt = RecursionCnt;\n"

"       float3 SpeculerCol = mcb.Speculer.xyz;\n"
"       float3 Diffuse = mcb.Diffuse.xyz;\n"
"       float3 Ambient = mcb.Ambient.xyz + GlobalAmbientColor.xyz;\n"
"       float shininess = mcb.shininess;\n"

"       if(RecursionCnt <= maxRecursion) {\n"
//点光源計算
"          float LightArea = LightArea_RandNum.x;\n"
"          uint RandNum = LightArea_RandNum.y;\n"

"          float3 dif = float3(0.0f, 0.0f, 0.0f);\n"
"          float3 spe = float3(0.0f, 0.0f, 0.0f);\n"

"          for(uint k = 0; k < RandNum; k++){\n"

"             ray.Direction = RandomVector(normal, LightArea);\n"
"             payload.mNo = EMISSIVE | NEE_PATHTRACER;\n"//処理分岐用

"             bool loop = true;\n"
"             payload.hitPosition = hitPosition;\n"
"             payload.EmissiveIndex = 0;\n"
"             while(loop){\n"
"                ray.Origin = payload.hitPosition;\n"
"                TraceRay(gRtScene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, \n"
"                         0xFF, 1, 0, 1, ray, payload);\n"
"                loop = payload.reTry;\n"
"             }\n"

"             uint emInd = payload.EmissiveIndex;\n"
"             float4 emissiveHitPos = emissivePosition[emInd];\n"
"             emissiveHitPos.xyz = payload.hitPosition;\n"

"             float3 col = payload.color * Nee(hitPosition);\n"
"             if(payload.hit)col = Nee(hitPosition);\n"
"             Out = PointLightComNoDistance(SpeculerCol, Diffuse, Ambient, normal, emissiveHitPos, \n"//ShaderCG内関数
"                                           hitPosition, col, cameraPosition.xyz, shininess);\n"

"             dif += Out.Diffuse;\n"
"             spe += Out.Speculer;\n"
"          }\n"
"          emissiveColor.Diffuse += (dif / (float)RandNum);\n"
"          emissiveColor.Speculer += (spe / (float)RandNum);\n"
"          float PI2 = (2 * PI) * 0.4f;\n"//後で考える
"          emissiveColor.Diffuse *= PI2;\n"
"          emissiveColor.Speculer *= PI2;\n"
"       }\n"
//最後にテクスチャの色に掛け合わせ
"       difTexColor *= emissiveColor.Diffuse;\n"
"       speTexColor *= emissiveColor.Speculer;\n"
"       ret = difTexColor + speTexColor;\n"
"    }\n"
"    return ret;\n"
"}\n";