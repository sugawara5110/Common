///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                      ShaderTraceRay.hlsl                                              //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

char* ShaderTraceRay =

///////////////////////光源へ光線を飛ばす, ヒットした場合明るさが加算//////////////////////////
"float3 EmissivePayloadCalculate(in uint RecursionCnt, in float3 hitPosition, \n"
"                                in float3 difTexColor, in float3 speTexColor, in float3 normal)\n"
"{\n"
"    uint materialID = getMaterialID();\n"
"    MaterialCB mcb = material[materialID];\n"
"    uint mNo = mcb.materialNo;\n"
"    float3 ret = difTexColor;\n"

"    bool mf = materialIdent(mNo, EMISSIVE);\n"
"    if(!mf) {\n"//emissive以外

"       RayPayload payload;\n"
"       LightOut emissiveColor = (LightOut)0;\n"
"       LightOut Out;\n"
"       RayDesc ray;\n"
"       payload.hitPosition = hitPosition;\n"
"       ray.TMin = TMin_TMax.x;\n"
"       ray.TMax = TMin_TMax.y;\n"
"       RecursionCnt++;\n"

"       float3 SpeculerCol = mcb.Speculer.xyz;\n"
"       float3 Diffuse = mcb.Diffuse.xyz;\n"
"       float3 Ambient = mcb.Ambient.xyz + GlobalAmbientColor.xyz;\n"
"       float shininess = mcb.shininess;\n"

"       if(RecursionCnt <= maxRecursion) {\n"
//点光源計算
"          for(uint i = 0; i < numEmissive.x; i++) {\n"
"              if(emissivePosition[i].w == 1.0f) {\n"
"                 float3 lightVec = normalize(emissivePosition[i].xyz - hitPosition);\n"
"                 ray.Direction = lightVec;\n"
"                 payload.instanceID = (uint)emissiveNo[i].x;\n"
"                 bool loop = true;\n"
"                 payload.hitPosition = hitPosition;\n"
"                 while(loop){\n"
"                    payload.mNo = EMISSIVE;\n"//処理分岐用
"                    ray.Origin = payload.hitPosition;\n"
"                    TraceRay(gRtScene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, \n"
"                             0xFF, 1, 0, 1, ray, payload);\n"
"                    loop = payload.reTry;\n"
"                 }\n"

"                 Out = PointLightCom(SpeculerCol, Diffuse, Ambient, normal, emissivePosition[i], \n"//ShaderCG内関数
"                                     hitPosition, lightst[i], payload.color, cameraPosition.xyz, shininess);\n"

"                 emissiveColor.Diffuse += Out.Diffuse;\n"
"                 emissiveColor.Speculer += Out.Speculer;\n"
"              }\n"
"          }\n"
//平行光源計算
"          if(dLightst.x == 1.0f){\n"
"             payload.hitPosition = hitPosition;\n"
"             ray.Direction = -dDirection.xyz;\n"
"             bool loop = true;\n"
"             while(loop){\n"
"                payload.mNo = DIRECTIONLIGHT | METALLIC;\n"//処理分岐用
"                ray.Origin = payload.hitPosition;\n"
"                TraceRay(gRtScene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, \n"
"                         0xFF, 1, 0, 1, ray, payload);\n"
"                loop = payload.reTry;\n"
"             }\n"

"             Out = DirectionalLightCom(SpeculerCol, Diffuse, Ambient, normal, dLightst, dDirection.xyz, \n"//ShaderCG内関数
"                                       payload.color, hitPosition, cameraPosition.xyz, shininess);\n"

"             emissiveColor.Diffuse += Out.Diffuse;\n"
"             emissiveColor.Speculer += Out.Speculer;\n"
"          }\n"
"       }\n"
//最後にテクスチャの色に掛け合わせ
"       difTexColor *= emissiveColor.Diffuse;\n"
"       speTexColor *= emissiveColor.Speculer;\n"
"       ret = difTexColor + speTexColor;\n"
"    }\n"
"    return ret;\n"
"}\n"

///////////////////////反射方向へ光線を飛ばす, ヒットした場合ピクセル値乗算///////////////////////
"float3 MetallicPayloadCalculate(in uint RecursionCnt, in float3 hitPosition, \n"
"                                in float3 difTexColor, in float3 normal, inout int hitInstanceId)\n"
"{\n"
"    uint materialID = getMaterialID();\n"
"    uint mNo = material[materialID].materialNo;\n"
"    float3 ret = difTexColor;\n"

"    hitInstanceId = (int)getInstancingID(); \n"//自身のID書き込み

"    if(materialIdent(mNo, METALLIC)) {\n"//METALLIC

"       RayPayload payload;\n"
"       RecursionCnt++;\n"
"       payload.RecursionCnt = RecursionCnt;\n"
"       RayDesc ray;\n"
//視線ベクトル 
"       float3 eyeVec = WorldRayDirection();\n"
//反射ベクトル
"       float3 reflectVec = reflect(eyeVec, normalize(normal));\n"
"       ray.Direction = reflectVec;\n"//反射方向にRayを飛ばす
"       ray.TMin = TMin_TMax.x;\n"
"       ray.TMax = TMin_TMax.y;\n"

"       if (RecursionCnt <= maxRecursion) {\n"
"           payload.hitPosition = hitPosition;\n"
"           ray.Origin = payload.hitPosition;\n"
"           TraceRay(gRtScene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, \n"
"                    0xFF, 0, 0, 0, ray, payload);\n"
"       }\n"
"       float3 outCol = float3(0.0f, 0.0f, 0.0f);\n"
"       if (payload.hit) {\n"
"           outCol = difTexColor * payload.color;\n"//ヒットした場合映り込みとして乗算
"           hitInstanceId = payload.hitInstanceId;\n"//ヒットしたID書き込み
"           int hitmNo = payload.mNo;\n"
"           if(materialIdent(hitmNo, EMISSIVE)){\n"
"              outCol = payload.color;\n"
"           }\n"
"       }\n"
"       else {\n"
"           outCol = difTexColor;\n"//ヒットしなかった場合映り込み無しで元のピクセル書き込み
"       }\n"
"       ret = outCol;\n"
"    }\n"
"    return ret;\n"
"}\n"

////////////////////////////////////////半透明//////////////////////////////////////////
"float3 Translucent(in uint RecursionCnt, in float3 hitPosition, in float4 difTexColor, in float3 normal)\n"
"{\n"
"    uint materialID = getMaterialID();\n"
"    MaterialCB mcb = material[materialID];\n"
"    uint mNo = mcb.materialNo;\n"
"    float3 ret = difTexColor.xyz;\n"

"    if(materialIdent(mNo, TRANSLUCENCE)) {\n"

"       float Alpha = difTexColor.w;\n"
"       RayPayload payload;\n"
"       RecursionCnt++;\n"
"       payload.RecursionCnt = RecursionCnt;\n"
"       RayDesc ray; \n"
"       ray.TMin = TMin_TMax.x;\n"
"       ray.TMax = TMin_TMax.y;\n"
//視線ベクトル 
"       float3 eyeVec = WorldRayDirection();\n"
"       ray.Direction = normalize(eyeVec + -normal * mcb.RefractiveIndex);\n"

"       if (RecursionCnt <= maxRecursion) {\n"
"           payload.hitPosition = hitPosition;\n"
"           ray.Origin = payload.hitPosition;\n"
"           TraceRay(gRtScene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, \n"
"                    0xFF, 0, 0, 0, ray, payload);\n"
"       }\n"
//アルファ値の比率で元の色と光線衝突先の色を配合
"       ret = payload.color * (1.0f - Alpha) + difTexColor * Alpha;\n"
"    }\n"
"    return ret;\n"
"}\n"

////////////////////////////////////アルファブレンド//////////////////////////////////////
"float3 AlphaBlend(in uint RecursionCnt, in float3 hitPosition, in float4 difTexColor)\n"
"{\n"
"    uint materialID = getMaterialID();\n"
"    MaterialCB mcb = material[materialID];\n"
"    uint mNo = mcb.materialNo;\n"
"    float3 ret = difTexColor.xyz;\n"
"    float blend = mcb.AlphaBlend;\n"
"    float Alpha = difTexColor.w;\n"

"    bool mf = materialIdent(mNo, TRANSLUCENCE);\n"
"    if(blend == 1.0f && !mf && Alpha < 1.0f) {\n"

"       RayPayload payload;\n"
"       RecursionCnt++;\n"
"       payload.RecursionCnt = RecursionCnt;\n"
"       RayDesc ray; \n"
"       ray.TMin = TMin_TMax.x;\n"
"       ray.TMax = TMin_TMax.y;\n"
"       ray.Direction = WorldRayDirection();\n"

"       if (RecursionCnt <= maxRecursion) {\n"
"           payload.hitPosition = hitPosition;\n"
"           ray.Origin = payload.hitPosition;\n"
"           TraceRay(gRtScene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, \n"
"                    0xFF, 0, 0, 0, ray, payload);\n"
"       }\n"
//アルファ値の比率で元の色と光線衝突先の色を配合
"       ret = payload.color * (1.0f - Alpha) + difTexColor * Alpha;\n"
"    }\n"
"    return ret;\n"
"}\n";