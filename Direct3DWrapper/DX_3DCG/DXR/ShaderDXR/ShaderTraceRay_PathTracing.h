///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                      ShaderTraceRay_PathTracing.hlsl                                  //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

char* ShaderTraceRay_PathTracing =

///////////////////////光源へ光線を飛ばす, ヒットした場合明るさが加算//////////////////////////
"float3 EmissivePayloadCalculate_PathTracing(in uint RecursionCnt, in float3 hitPosition, \n"
"                                            in float3 difTexColor, in float3 speTexColor, in float3 normal)\n"
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
"       RayDesc ray;\n"
"       payload.hitPosition = hitPosition;\n"
"       RecursionCnt++;\n"
"       payload.RecursionCnt = RecursionCnt;\n"

"       float3 SpeculerCol = mcb.Speculer.xyz;\n"
"       float3 Diffuse = mcb.Diffuse.xyz;\n"
"       float3 Ambient = mcb.Ambient.xyz + GlobalAmbientColor.xyz;\n"
"       float shininess = mcb.shininess;\n"

"       if(RecursionCnt <= maxRecursion) {\n"
//点光源計算
"          float LightArea = LightArea_RandNum.x;\n"

"          ray.Direction = RandomVector(normal, LightArea);\n"
"          payload.mNo = EMISSIVE;\n"//処理分岐用

"          payload.hitPosition = hitPosition;\n"
"          payload.EmissiveIndex = 0;\n"

"          traceRay(RecursionCnt, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, 1, 1, ray, payload);\n"

"          uint emInd = payload.EmissiveIndex;\n"
"          float4 emissiveHitPos = emissivePosition[emInd];\n"
"          emissiveHitPos.xyz = payload.hitPosition;\n"

"          emissiveColor = PointLightComNoDistance(SpeculerCol, Diffuse, Ambient, normal, emissiveHitPos, \n"//ShaderCG内関数
"                                                  hitPosition, payload.color, cameraPosition.xyz, shininess);\n"

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