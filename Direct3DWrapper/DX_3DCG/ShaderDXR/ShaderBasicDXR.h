///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                           ShaderBasicDXR.hlsl                                         //
///////////////////////////////////////////////////////////////////////////////////////////////////////////
char* ShaderBasicDXR =
"RWTexture2D<float4> gOutput : register(u0, space0);\n"
"RaytracingAccelerationStructure gRtScene : register(t0, space0);\n"
"StructuredBuffer<uint> Indices[] : register(t1, space1);\n"//無制限配列の場合,別なレジスタ空間にした方が(・∀・)ｲｲ!! みたい
"struct Vertex {\n"
"    float3 Pos;\n"
"    float3 normal;\n"
"    float2 tex;\n"
"};\n"
"StructuredBuffer<Vertex> Vertices[] : register(t2, space2);\n"
"cbuffer global : register(b0, space0)\n"
"{\n"
"    matrix projectionInv;\n"
"    matrix viewInv;\n"
"    float4 cameraUp;"
"    float4 cameraPosition;\n"
"    float4 lightPosition[256];\n"
"    float4 lightAmbientColor;\n"
"    float4 lightDiffuseColor;\n"
"};\n"
"struct Transform {\n"
"    matrix m;\n"
"};\n"
"ConstantBuffer<Transform> world[] : register(b1, space3);\n"

"Texture2D g_texDiffuse[] : register(t0, space10);\n"
"Texture2D g_texNormal[] : register(t1, space11);\n"
"SamplerState g_samLinear : register(s0, space12);\n"

"float3 linearToSrgb(float3 c)\n"
"{\n"
     //sRGBに変換
"    float3 sq1 = sqrt(c);\n"
"    float3 sq2 = sqrt(sq1);\n"
"    float3 sq3 = sqrt(sq2);\n"
"    float3 srgb = 0.662002687 * sq1 + 0.684122060 * sq2 - 0.323583601 * sq3 - 0.0225411470 * c;\n"
"    return srgb;\n"
"}\n"

"struct RayPayload\n"
"{\n"
"    float3 color;\n"
"};\n"

//**************************************rayGen_Shader*******************************************************************//
//最初に実行されるシェーダー, ピクセルの数だけ起動?
"[shader(\"raygeneration\")]\n"
"void rayGen()\n"
"{\n"
"    uint2 index = DispatchRaysIndex();\n"//この関数を実行しているピクセル座標を取得
"    uint2 dim = DispatchRaysDimensions();\n"//画面全体の幅と高さを取得
"    float2 screenPos = (index + 0.5f) / dim * 2.0f - 1.0f;\n"

"    RayDesc ray;\n"
     //光線の原点
"    ray.Origin = cameraPosition.xyz;\n"
     //光線の方向
"    float4 target = mul(projectionInv, float4(screenPos.x, -screenPos.y, 1, 1));\n"
"    ray.Direction = mul(viewInv, float4(target.xyz, 0));\n"
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

//ヒット位置取得
"float3 HitWorldPosition()\n"
"{\n"
     //     原点           現在のヒットまでの距離      方向
"    return WorldRayOrigin() + RayTCurrent() * WorldRayDirection();\n"
"}\n"

//Hitの重心を使用して、頂点属性から補間されたhit位置の属性を取得
"float3 getCenterNormal(float3 vertexNormal[3], BuiltInTriangleIntersectionAttributes attr)\n"
"{\n"
"    return vertexNormal[0] +\n"
"        attr.barycentrics.x * (vertexNormal[1] - vertexNormal[0]) +\n"
"        attr.barycentrics.y * (vertexNormal[2] - vertexNormal[0]);\n"
"}\n"

"float2 getCenterUV(float2 vertexUV[3], BuiltInTriangleIntersectionAttributes attr)\n"
"{\n"
"    return vertexUV[0] +\n"
"        attr.barycentrics.x * (vertexUV[1] - vertexUV[0]) +\n"
"        attr.barycentrics.y * (vertexUV[2] - vertexUV[0]);\n"
"}\n"

// Diffuse lighting calculation.
"float4 CalculateDiffuseLighting(float3 hitPosition, float3 normal)\n"
"{\n"
"    float3 pixelToLight = normalize(lightPosition[0].xyz - hitPosition);\n"

     //Diffuse contribution.
"    float fNDotL = max(0.0f, dot(pixelToLight, normal));\n"

"    return lightDiffuseColor * fNDotL;\n"
"}\n"

//**************************************camMiss_Shader*******************************************************************//
//ヒットしなかった場合起動される
"[shader(\"miss\")]\n"
"void camMiss(inout RayPayload payload)\n"
"{\n"
"    payload.color = float3(0.1, 0.1, 0.1);\n"
"}\n"
//**************************************camMiss_Shader*******************************************************************//

"struct MetallicPayload\n"
"{\n"
"    float3 color;\n"
"    bool hit;\n"
"};\n"
 
"float3 getNormalMap(float3 normal, float2 uv, uint instanceID)\n"
"{\n"
"    NormalTangent tan;\n"
//接ベクトル計算
"    tan = GetTangent(normal, world[instanceID].m, cameraUp);\n"
//法線テクスチャ
"    float4 Tnor = g_texNormal[instanceID].SampleLevel(g_samLinear, uv, 0.0);\n"
//ノーマルマップでの法線出力
"    return GetNormal(Tnor.xyz, tan.normal, tan.tangent);\n"
"}\n"

//**************************************camHit_Shader*******************************************************************//
//ヒットした場合起動される
"[shader(\"closesthit\")]\n"
"void camHit(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attr)\n"
"{\n"
"    float3 hitPosition = HitWorldPosition();\n"
"    uint instanceID = InstanceID();\n"

"    uint indicesPerTriangle = 3;\n"
"    uint baseIndex = PrimitiveIndex() * indicesPerTriangle;\n"

"    float3 vertexNormals[3] = {\n"
"        Vertices[instanceID][Indices[instanceID][baseIndex + 0]].normal,\n"
"        Vertices[instanceID][Indices[instanceID][baseIndex + 1]].normal,\n"
"        Vertices[instanceID][Indices[instanceID][baseIndex + 2]].normal\n"
"    };\n"
"    float2 vertexUVs[3] = {\n"
"        Vertices[instanceID][Indices[instanceID][baseIndex + 0]].tex,\n"
"        Vertices[instanceID][Indices[instanceID][baseIndex + 1]].tex,\n"
"        Vertices[instanceID][Indices[instanceID][baseIndex + 2]].tex\n"
"    };\n"

//UV計算
"    float2 triangleUV = getCenterUV(vertexUVs, attr);\n"
//法線の計算
"    float3 triangleNormal = getCenterNormal(vertexNormals, attr);\n"
//ノーマルマップからの法線出力, world変換
"    float3 normalMap = getNormalMap(triangleNormal, triangleUV, instanceID);\n"
//ベーステクスチャ
"    float4 difTex = g_texDiffuse[instanceID].SampleLevel(g_samLinear, triangleUV, 0.0);\n"

"    float4 diffuseColor = CalculateDiffuseLighting(hitPosition, normalMap);\n"
"    float4 color = lightAmbientColor + diffuseColor;\n"//自身のテクスチャの色
"    color = difTex;\n"
"    MetallicPayload metallicPayload;\n"
"    if (!(RayFlags() & RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH))\n"
"    {\n"
"        RayDesc ray;\n"
//ヒットした位置がスタート
"        ray.Origin = hitPosition;\n"
//視線ベクトル 
"        float3 eyeVec = WorldRayDirection();\n"
//反射ベクトル
"        float3 reflectVec = reflect(eyeVec, normalize(normalMap));\n"
"        ray.Direction = reflectVec;\n"//反射方向にRayを飛ばす
"        ray.TMin = 0.01;\n"
"        ray.TMax = 100000;\n"
"        TraceRay(gRtScene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES |\n"
"            RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH, 0xFF, 1, 0, 1, ray, metallicPayload);\n"
"    }\n"
//metallicPayload.colorにはヒットしたジオメトリのテクスチャの色が格納されてる
"    if (metallicPayload.hit) {\n"
"        payload.color = color.xyz * metallicPayload.color;\n"
"    }\n"
"    else {\n"
"        payload.color = color;\n"
"    }\n"
"}\n"
//**************************************camHit_Shader*******************************************************************//

//***********************************metallicHit_Shader*****************************************************************//
//camHitから飛ばされたRayがヒットした場合起動される
"[shader(\"closesthit\")]\n"
"void metallicHit(inout MetallicPayload payload, in BuiltInTriangleIntersectionAttributes attr)\n"
"{\n"
"    uint instanceID = InstanceID();\n"

"    uint indicesPerTriangle = 3;\n"
"    uint baseIndex = PrimitiveIndex() * indicesPerTriangle;\n"

"    float2 vertexUVs[3] = {\n"
"        Vertices[instanceID][Indices[instanceID][baseIndex + 0]].tex,\n"
"        Vertices[instanceID][Indices[instanceID][baseIndex + 1]].tex,\n"
"        Vertices[instanceID][Indices[instanceID][baseIndex + 2]].tex\n"
"    };\n"
//UV計算
"    float2 triangleUV = getCenterUV(vertexUVs, attr);\n"
"    float4 difTex = g_texDiffuse[instanceID].SampleLevel(g_samLinear, triangleUV, 0.0);\n"
//ヒットした位置のテクスチャの色をpayload.color格納する
"    payload.color = difTex.xyz;\n"
"    payload.hit = true;\n"
"}\n"
//***********************************metallicHit_Shader*****************************************************************//

//***********************************metallicMiss_Shader****************************************************************//
"[shader(\"miss\")]\n"
"void metallicMiss(inout MetallicPayload payload)\n"
"{\n"
"    payload.hit = false;\n"
"}\n";
//***********************************metallicMiss_Shader****************************************************************//