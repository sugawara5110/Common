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
"    float3 ret = difTexColor;\n"//光源だった場合、映り込みがそのまま出る・・・あとで変更する

"    bool mf = materialIdent(mNo, EMISSIVE);\n"
"    if(!mf) {\n"//emissive以外

"       RayPayload payload;\n"
"       payload.hit = false;\n"
"       LightOut emissiveColor = (LightOut)0;\n"
"       LightOut Out;\n"
"       RayDesc ray;\n"
"       payload.hitPosition = hitPosition;\n"

"       float3 SpeculerCol = mcb.Speculer.xyz;\n"
"       float3 Diffuse = mcb.Diffuse.xyz;\n"
"       float3 Ambient = mcb.Ambient.xyz + GlobalAmbientColor.xyz;\n"
"       float shininess = mcb.shininess;\n"

//点光源計算
"       uint NumEmissive = numEmissive.x;\n"
"       for(uint i = 0; i < NumEmissive; i++) {\n"
"           if(emissivePosition[i].w == 1.0f) {\n"
"              float3 lightVec = normalize(emissivePosition[i].xyz - hitPosition);\n"

"              ray.Direction = lightVec;\n"
"              payload.mNo = EMISSIVE;\n"//処理分岐用

"              payload.hitPosition = hitPosition;\n"

"              traceRay(RecursionCnt, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, 1, 1, ray, payload);\n"

"              float4 emissiveHitPos = emissivePosition[i];\n"
"              emissiveHitPos.xyz = payload.hitPosition;\n"

"              Out = PointLightCom(SpeculerCol, Diffuse, Ambient, normal, emissiveHitPos, \n"//ShaderCG内関数
"                                  hitPosition, lightst[i], payload.color, cameraPosition.xyz, shininess);\n"

"              emissiveColor.Diffuse += Out.Diffuse;\n"
"              emissiveColor.Speculer += Out.Speculer;\n"
"           }\n"
"       }\n"
//平行光源計算
"       if(dLightst.x == 1.0f){\n"
"          payload.hitPosition = hitPosition;\n"
"          ray.Direction = -dDirection.xyz;\n"
"          payload.mNo = DIRECTIONLIGHT;\n"//処理分岐用

"          traceRay(RecursionCnt, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, 1, 1, ray, payload);\n"

"          Out = DirectionalLightCom(SpeculerCol, Diffuse, Ambient, normal, dLightst, dDirection.xyz, \n"//ShaderCG内関数
"                                    payload.color, hitPosition, cameraPosition.xyz, shininess);\n"

"          emissiveColor.Diffuse += Out.Diffuse;\n"
"          emissiveColor.Speculer += Out.Speculer;\n"
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
"                                in float3 difTexColor, in float3 normal, inout int hitInstanceId, in float fresnel)\n"
"{\n"
"    uint materialID = getMaterialID();\n"
"    uint mNo = material[materialID].materialNo;\n"
"    float3 ret = difTexColor;\n"

"    hitInstanceId = (int)getInstancingID(); \n"//自身のID書き込み

"    if(materialIdent(mNo, METALLIC)) {\n"//METALLIC

"       RayPayload payload;\n"
"       payload.hit = false;\n"
"       RayDesc ray;\n"
//視線ベクトル 
"       float3 eyeVec = WorldRayDirection();\n"
//反射ベクトル
"       float3 reflectVec = reflect(eyeVec, normalize(normal));\n"
"       ray.Direction = reflectVec;\n"//反射方向にRayを飛ばす
"       payload.hitPosition = hitPosition;\n"

"       traceRay(RecursionCnt, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, 0, 0, ray, payload);\n"

"       float3 outCol = float3(0.0f, 0.0f, 0.0f);\n"
"       if (payload.hit) {\n"
"           float3 refCol = payload.color * (1.0f - fresnel);\n"

"           outCol = difTexColor * refCol;\n"//ヒットした場合映り込みとして乗算
"           hitInstanceId = payload.hitInstanceId;\n"//ヒットしたID書き込み
"           int hitmNo = payload.mNo;\n"
"           if(materialIdent(hitmNo, EMISSIVE)){\n"
"              outCol = refCol;\n"
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
"float3 Translucent(in uint RecursionCnt, in float3 hitPosition, in float4 difTexColor, in float3 normal, in float fresnel)\n"
"{\n"
"    uint materialID = getMaterialID();\n"
"    MaterialCB mcb = material[materialID];\n"
"    uint mNo = mcb.materialNo;\n"
"    float3 ret = difTexColor.xyz;\n"
"    float Alpha = difTexColor.w;\n"

"    if(materialIdent(mNo, TRANSLUCENCE) && Alpha < 1.0f) {\n"

"       float Alpha = difTexColor.w;\n"
"       RayPayload payload;\n"
"       RayDesc ray; \n"
//視線ベクトル 
"       float3 eyeVec = WorldRayDirection();\n"
"       ray.Direction = refract(eyeVec, normalize(normal), mcb.RefractiveIndex);\n"
"       payload.hitPosition = hitPosition;\n"

"       traceRay(RecursionCnt, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, 0, 0, ray, payload);\n"

//アルファ値の比率で元の色と光線衝突先の色を配合
"       ret = payload.color * fresnel * (1.0f - Alpha) + difTexColor * Alpha;\n"
"    }\n"
"    return ret;\n"
"}\n";