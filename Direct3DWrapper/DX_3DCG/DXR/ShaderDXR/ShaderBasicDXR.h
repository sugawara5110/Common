///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                           ShaderBasicDXR.hlsl                                         //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

char* ShaderBasicDXR =

//**************************************rayGen_Shader*******************************************************************//
//最初に実行されるシェーダー, ピクセルの数だけ起動?
"[shader(\"raygeneration\")]\n"
"void rayGen()\n"
"{\n"
"    uint2 index = DispatchRaysIndex();\n"//この関数を実行しているピクセル座標を取得
"    uint2 dim = DispatchRaysDimensions();\n"//画面全体の幅と高さを取得
"    float2 screenPos = (index + 0.5f) / dim * 2.0f - 1.0f;\n"

"    RayDesc ray;\n"
     //光線の原点, ここが光線スタート, 視線から始まる
"    ray.Origin = cameraPosition.xyz;\n"
     //光線の方向
"    float4 world = mul(float4(screenPos.x, -screenPos.y, 0, 1), projectionToWorld);\n"
"    world.xyz /= world.w;\n"
"    ray.Direction = normalize(world.xyz - ray.Origin);\n"
     //光線の最小範囲
"    ray.TMin = TMin_TMax.x;\n"
     //光線の最大範囲
"    ray.TMax = TMin_TMax.y;\n"

"    RayPayload payload;\n"
     //TraceRay(AccelerationStructure, 
     //         RAY_FLAG, 
     //         InstanceInclusionMask, 
     //         HitGroupIndex, 使用するhitShaderのIndex
     //         MultiplierForGeometryContributionToShaderIndex, 
     //         MissShaderIndex, 使用するmissShaderのIndex
     //         RayDesc, 
     //         Payload);
     //この関数からRayがスタートする
     //payloadに各hit, miss シェーダーで計算された値が格納される
"    payload.RecursionCnt = 1;\n"
"    bool loop = true;\n"
"    payload.hitPosition = ray.Origin;\n"
"    gDepthOut[index] = 1.0f;\n"
"    payload.depth = -1.0f;\n"
"    payload.reTry = false;\n"

"    while(loop){\n"
"       ray.Origin = payload.hitPosition;\n"
"       TraceRay(gRtScene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, 0xFF, 0, 0, 0, ray, payload);\n"
"       loop = payload.reTry;\n"
"    }\n"

"    if(payload.depth != -1.0f)"
"       gDepthOut[index] = payload.depth;\n"
"    float3 col = linearToSrgb(payload.color);\n"
"    float3 colsatu;\n"
"    colsatu.x = saturate(col.x);\n"
"    colsatu.y = saturate(col.y);\n"
"    colsatu.z = saturate(col.z);\n"
"    gOutput[index] = float4(colsatu, 1);\n"
"}\n"
//**************************************rayGen_Shader*******************************************************************//

//*************************************anyBasicHit_Shader***************************************************************//
"[shader(\"anyhit\")]\n"
"void anyBasicHit(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attr)\n"
"{\n"
//テクスチャ取得
"    Vertex3 v3 = getVertex();\n"
"    float4 difTex = getDifPixel(attr, v3);\n"
"    float Alpha = difTex.w;\n"

"    if (Alpha <= 0.0f)\n"
"    {\n"
"        IgnoreHit();\n"
"    }\n"
"}\n"
//*************************************anyBasicHit_Shader***************************************************************//

//**************************************basicMiss_Shader****************************************************************//
"[shader(\"miss\")]\n"
"void basicMiss(inout RayPayload payload)\n"
"{\n"
"    payload.color = float3(0.0, 0.0, 0.0);\n"
"    payload.hit = false;\n"
"    payload.reTry = false;\n"
"}\n"
//**************************************basicMiss_Shader****************************************************************//

//**************************************basicHit_Shader*****************************************************************//
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
//深度取得
"    if(payload.depth == -1.0f) {\n"
"       payload.depth = getDepth(attr, v3);\n"
"    }\n"
//光源への光線
"    difTex.xyz = EmissivePayloadCalculate(payload.RecursionCnt, payload.hitPosition, difTex.xyz, speTex, normalMap);\n"
//反射方向への光線
"    difTex.xyz = MetallicPayloadCalculate(payload.RecursionCnt, payload.hitPosition, difTex.xyz, normalMap);\n"
//半透明
"    difTex.xyz = Translucent(payload.RecursionCnt, payload.hitPosition, difTex, normalMap);\n"
//アルファブレンド
"    difTex.xyz = AlphaBlend(payload.RecursionCnt, payload.hitPosition, difTex);\n"

"    payload.color = difTex.xyz;\n"
"    payload.hit = true;\n"
"    payload.Alpha = difTex.w;\n"
"}\n"
//**************************************basicHit_Shader*****************************************************************//

//***********************************emissiveHit_Shader*****************************************************************//
"[shader(\"closesthit\")]\n"
"void emissiveHit(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attr)\n"
"{\n"
"    uint materialID = getMaterialID();\n"
"    uint mNo = material[materialID].materialNo;\n"
"    Vertex3 v3 = getVertex();\n"
"    float4 difTex = getDifPixel(attr, v3);\n"
"    payload.hitPosition = HitWorldPosition();\n"
"    payload.reTry = false;\n"
//ヒットした位置のテクスチャの色をpayload.color格納する
//////点光源
"    if(payload.mNo == 2 && mNo == 2) {\n"
"       payload.color = difTex.xyz;\n"
"       if(InstanceID() != payload.instanceID || difTex.w <= 0.0f) {\n"
"          payload.reTry = true;\n"//目標の点光源以外の場合素通り
"       }\n"
"    }\n"
//////平行光源
"    if(payload.mNo == 3) {\n"
"       if(mNo == 3 || mNo == 4) {\n"//平行光源発生マテリアルか?
"          payload.color = dLightColor.xyz;\n"
"       }\n"
"       if(mNo == 2) {\n"//点光源の場合素通り
"          payload.reTry = true;\n"
"       }\n"
"    }\n"
//////影
"    if(mNo != 2 && payload.mNo == 2 || mNo < 2 && payload.mNo == 3) {\n"
"       if(difTex.w >= 1.0f) {\n"
"          payload.color = GlobalAmbientColor.xyz;\n"
"       }\n"
"       else {\n"
"          payload.reTry = true;\n"
"       }\n"
"    }\n"
"}\n"
//***********************************emissiveHit_Shader*****************************************************************//

//***********************************emissiveMiss_Shader****************************************************************//
"[shader(\"miss\")]\n"
"void emissiveMiss(inout RayPayload payload)\n"
"{\n"
"    payload.color = GlobalAmbientColor.xyz;\n"
"    payload.reTry = false;\n"
"}\n";
//***********************************emissiveMiss_Shader****************************************************************//