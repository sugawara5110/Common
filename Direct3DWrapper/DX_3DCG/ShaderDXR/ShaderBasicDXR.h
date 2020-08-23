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
"    ray.TMin = 0.001;\n"
     //光線の最大範囲
"    ray.TMax = 10000;\n"

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
"    TraceRay(gRtScene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, 0xFF, 0, 0, 0, ray, payload);\n"
"    float3 col = linearToSrgb(payload.color);\n"
"    gOutput[index] = float4(col, 1);\n"
"}\n"
//**************************************rayGen_Shader*******************************************************************//

//**************************************camMiss_Shader*******************************************************************//
//視線がヒットしなかった場合起動される
"[shader(\"miss\")]\n"
"void camMiss(inout RayPayload payload)\n"
"{\n"
"    payload.color = float3(0.0, 0.0, 0.0);\n"
"}\n"
//**************************************camMiss_Shader*******************************************************************//

//**************************************camHit_Shader*******************************************************************//
//視線がヒットした場合起動される
"[shader(\"closesthit\")]\n"
"void camHit(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attr)\n"
"{\n"
"    float3 hitPosition = HitWorldPosition();\n"
"    uint materialID = getMaterialID();\n"
"    uint mNo = material[materialID].materialNo;\n"
//UV計算
"    float2 triangleUV0 = getUV(attr, 0);\n"
"    float2 triangleUV1 = getUV(attr, 1);\n"
//法線の計算
"    float3 triangleNormal = getNormal(attr);\n"
//ノーマルマップからの法線出力
"    float3 normalMap = getNormalMap(triangleNormal, triangleUV0);\n"
//テクスチャ
"    float4 difTex = g_texDiffuse[materialID].SampleLevel(g_samLinear, triangleUV0, 0.0);\n"
"    float4 speTex = g_texSpecular[materialID].SampleLevel(g_samLinear, triangleUV1, 0.0);\n"

"    if(mNo == 0) {\n"//マテリアルがメタリックの場合
//光源への光線
"       difTex.xyz = EmissivePayloadCalculate(hitPosition, difTex.xyz, speTex.xyz, normalMap);\n"
//反射方向への光線
"       difTex.xyz = MetallicPayloadCalculate(hitPosition, difTex.xyz, normalMap);\n"
"    }\n"
//アルファテスト
"    if(material[materialID].alphaTest == 1.0f) {\n"
"       difTex.xyz = AlphaTest(hitPosition, difTex.xyz, difTex.w, 0);\n"
"    }\n"
"    payload.color = difTex.xyz;\n"
"}\n"
//**************************************camHit_Shader*******************************************************************//

//***********************************metallicHit_Shader*****************************************************************//
//camHitから飛ばされた光線がヒットした場合起動される
"[shader(\"closesthit\")]\n"
"void metallicHit(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attr)\n"
"{\n"
"    uint materialID = getMaterialID();\n"
"    float3 hitPosition = HitWorldPosition();\n"
//UV計算
"    float2 triangleUV0 = getUV(attr, 0);\n"
"    float2 triangleUV1 = getUV(attr, 1);\n"
//法線の計算
"    float3 triangleNormal = getNormal(attr);\n"
//ノーマルマップからの法線出力
"    float3 normalMap = getNormalMap(triangleNormal, triangleUV0);\n"
//ヒットした位置のテクスチャの色をpayload.color格納する
"    float4 difTex = g_texDiffuse[materialID].SampleLevel(g_samLinear, triangleUV0, 0.0);\n"
"    float4 speTex = g_texSpecular[materialID].SampleLevel(g_samLinear, triangleUV1, 0.0);\n"
//光源への光線
"    difTex.xyz = EmissivePayloadCalculate(hitPosition, difTex.xyz, speTex.xyz, normalMap);\n"
//アルファテスト
"    if(material[materialID].alphaTest == 1.0f) {\n"
"       difTex.xyz = AlphaTest(hitPosition, difTex.xyz, difTex.w, 1);\n"
"    }\n"
"    payload.color = difTex.xyz;\n"
"    payload.hit = true;\n"
"    payload.Alpha = difTex.w;\n"
"}\n"
//***********************************metallicHit_Shader*****************************************************************//

//***********************************metallicMiss_Shader****************************************************************//
//camHitから飛ばされた光線がヒットしなかった場合起動される
"[shader(\"miss\")]\n"
"void metallicMiss(inout RayPayload payload)\n"
"{\n"
"    payload.hit = false;\n"
"}\n"
//***********************************metallicMiss_Shader****************************************************************//

//***********************************emissiveHit_Shader*****************************************************************//
//camHitから飛ばされた光源への光線がヒットした場合起動される
"[shader(\"closesthit\")]\n"
"void emissiveHit(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attr)\n"
"{\n"
"    uint materialID = getMaterialID();\n"
"    uint mNo = material[materialID].materialNo;\n"

"    if(mNo == 1) {\n"//マテリアルがエミッシブの場合のみ処理
//UV計算
"       float2 triangleUV0 = getUV(attr, 0);\n"
"       float4 difTex = g_texDiffuse[materialID].SampleLevel(g_samLinear, triangleUV0, 0.0);\n"
//ヒットした位置のテクスチャの色をpayload.color格納する
"       payload.color = difTex.xyz;\n"
"       payload.hit = true;\n"
"    }\n"
"    else {\n"//エミッシブではなかった場合影になる
"       payload.color = GlobalAmbientColor.xyz;\n"
"       payload.hit = false;\n"
"    }\n"
"}\n"
//***********************************emissiveHit_Shader*****************************************************************//

//***********************************emissiveMiss_Shader****************************************************************//
//camHitから飛ばされた光源への光線がヒットしなかった場合起動される
"[shader(\"miss\")]\n"
"void emissiveMiss(inout RayPayload payload)\n"
"{\n"
"    payload.color = GlobalAmbientColor.xyz;\n"
"    payload.hit = false;\n"
"}\n";
//***********************************emissiveMiss_Shader****************************************************************//