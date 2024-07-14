///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                           ShaderBasicHit.hlsl                                         //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

char* ShaderBasicHit =

//通常
"[shader(\"closesthit\")]\n"
"void basicHit(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attr)\n"
"{\n"
"    payload.hitPosition = HitWorldPosition();\n"
"    Vertex3 v3 = getVertex();\n"
//テクスチャ取得
"    float4 difTex = getDifPixel(attr, v3);\n"
"    float3 normalMap = getNorPixel(attr, v3);\n"
"    float3 speTex = getSpePixel(attr, v3);\n"

"    payload.reTry = false;\n"

//光源への光線
"    difTex.xyz = EmissivePayloadCalculate(payload.RecursionCnt, payload.hitPosition, \n"
"                                          difTex.xyz, speTex, normalMap);\n"

//法線切り替え
"    float3 r_eyeVec = -WorldRayDirection();\n"//視線へのベクトル
"    float norDir = dot(r_eyeVec, normalMap);\n"
"    if(norDir < 0.0f)normalMap *= -1.0f;\n"

//フレネル計算
"    float fresnel = saturate(dot(r_eyeVec, normalMap));\n"

//反射方向への光線
"    difTex.xyz = MetallicPayloadCalculate(payload.RecursionCnt, payload.hitPosition, \n"
"                                          difTex.xyz, normalMap, payload.hitInstanceId, fresnel);\n"

//半透明
"    difTex.xyz = Translucent(payload.RecursionCnt, payload.hitPosition, \n"
"                             difTex, normalMap, fresnel);\n"

//深度取得
"    payload.depth = getDepth(attr, v3);\n"
//法線取得
"    payload.normal = normalMap;\n"

"    payload.color = difTex.xyz;\n"
"    payload.hit = true;\n"
"    payload.Alpha = difTex.w;\n"
"}\n"

//法線マップテスト用
"[shader(\"closesthit\")]\n"
"void basicHit_normalMapTest(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attr)\n"
"{\n"
"    payload.hitPosition = HitWorldPosition();\n"
"    Vertex3 v3 = getVertex();\n"
//テクスチャ取得
"    float4 difTex = getDifPixel(attr, v3);\n"
"    float3 normalMap = getNorPixel(attr, v3);\n"

"    payload.reTry = false;\n"
//深度取得
"    payload.depth = getDepth(attr, v3);\n"
//法線取得
"    payload.normal = normalMap;\n"

"    payload.color = normalMap;\n"
"    payload.hit = true;\n"
"    payload.Alpha = difTex.w;\n"
"}\n";