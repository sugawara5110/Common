///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                           Shader2ndHit.hlsl                                           //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

char* Shader2ndHit =

"[shader(\"closesthit\")]\n"
"void s2ndHit(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attr)\n"
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
"    payload.hitInstanceId = (int)getInstancingID(); \n"

"    if(difTex.w < 1.0f){\n"
"       payload.reTry = true;\n"
"       return;\n"
"    }\n"
//////���s����
"    if(materialIdent(payload.mNo, DIRECTIONLIGHT)){\n"
"       if(materialIdent(mNo, DIRECTIONLIGHT)){\n"//���s���������}�e���A����?
"          payload.color = dLightColor.xyz;\n"
"          return;\n"
"       }\n"
"    }\n"
//////�����ȊO
"    if(!materialIdent(payload.mNo, NEE) && traceMode != 0){\n"
"       payload.color = PayloadCalculate_PathTracing(payload.RecursionCnt, payload.hitPosition, \n"
"                                                    difTex.xyz, speTex, normalMap, \n"
"                                                    payload.throughput, payload.hitInstanceId);\n"
"    }\n"
"}\n";
