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
"void EmissiveHit(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attr)\n"
"{\n"
"    Vertex3 v3 = getVertex();\n"
"    float4 difTex = getDifPixel(attr, v3);\n"
"    float3 normalMap = getNorPixel(attr, v3);\n"
"    payload.normal = normalMap;\n"
"    payload.hitPosition = HitWorldPosition();\n"
"    payload.hit = false;\n"
"    payload.reTry = false;\n"
"    payload.color = float3(0.0f, 0.0f, 0.0f);\n"

"    if(difTex.w < 1.0f){\n"
"       payload.reTry = true;\n"
"       return;\n"
"    }\n"

"    payload.hitInstanceId = (int)getInstancingID(); \n"
"    payload.EmissiveIndex = getEmissiveIndex();\n"
"    payload.hit = true;\n"
"    payload.color = difTex.xyz;\n"
"}\n";
