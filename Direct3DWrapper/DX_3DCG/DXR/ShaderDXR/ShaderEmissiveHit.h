///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                           ShaderEmissiveHit.hlsl                                      //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

char* ShaderEmissiveHit =

"uint getEmissiveIndex(){\n"
"   uint ret = 0;\n"
"   for(uint i = 0; i < 256; i++){\n"
"      if(emissiveNo[i].x == InstanceID()){\n"
"         ret = i;\n"
"         break;\n"
"      }\n"
"   }\n"
"   return ret;\n"
"}\n"

"[shader(\"closesthit\")]\n"
"void emissiveHit(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attr)\n"
"{\n"
"    uint materialID = getMaterialID();\n"
"    uint mNo = material[materialID].materialNo;\n"
"    Vertex3 v3 = getVertex();\n"
"    float4 difTex = getDifPixel(attr, v3);\n"
"    float3 normalMap = getNorPixel(attr, v3);\n"
"    float3 speTex = getSpePixel(attr, v3);\n"
"    payload.normal = normalMap;\n"
"    payload.hitPosition = HitWorldPosition();\n"
"    payload.hit = false;\n"
"    payload.reTry = false;\n"
"    payload.color = float3(0.0f, 0.0f, 0.0f);\n"

"    if(difTex.w < 1.0f){\n"
"       payload.reTry = true;\n"
"       return;\n"
"    }\n"
//ヒットした位置のテクスチャの色をpayload.color格納する
//////点光源
"    if(materialIdent(payload.mNo, EMISSIVE) && materialIdent(mNo, EMISSIVE)){\n"
"       payload.EmissiveIndex = getEmissiveIndex();\n"
"       payload.hit = true;\n"
"       if(!materialIdent(payload.mNo, NEE_PATHTRACER))payload.color = difTex.xyz;\n"
"       return;\n"
"    }\n"
//////平行光源
"    if(materialIdent(payload.mNo, DIRECTIONLIGHT)){\n"
"       if(materialIdent(mNo, DIRECTIONLIGHT)){\n"//平行光源発生マテリアルか?
"          payload.color = dLightColor.xyz;\n"
"          return;\n"
"       }\n"
"       if(materialIdent(mNo, EMISSIVE)){\n"//点光源の場合素通り
"          payload.reTry = true;\n"
"          return;\n"
"       }\n"
"    }\n"
//////光源以外
"    if(!materialIdent(payload.mNo, NEE) && traceMode != 0){\n"
"       payload.color = PayloadCalculate_PathTracing(payload.RecursionCnt, payload.hitPosition, \n"
"                                                    difTex.xyz, speTex, normalMap, payload.throughput);\n"
"    }\n"
"}\n";
