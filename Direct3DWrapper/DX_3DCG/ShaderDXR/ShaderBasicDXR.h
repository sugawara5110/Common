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
"    float4 emissivePosition[256];\n"
"    float4 numEmissive;\n"//.xのみ
"    float4 lightst[256];\n"//レンジ, 減衰1, 減衰2, 減衰3
"    float4 GlobalAmbientColor;\n"
"};\n"
"struct Instance {\n"
"    matrix world;\n"
"    uint materialNo;\n"//0:metallic, 1:emissive
"};\n"
"ConstantBuffer<Instance> instance[] : register(b1, space3);\n"

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
"    bool hit;\n"
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
     //光線の原点, ここが光線スタート, 視線から始まる
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

//**************************************camMiss_Shader*******************************************************************//
//視線がヒットしなかった場合起動される
"[shader(\"miss\")]\n"
"void camMiss(inout RayPayload payload)\n"
"{\n"
"    payload.color = float3(0.0, 0.0, 0.0);\n"
"}\n"
//**************************************camMiss_Shader*******************************************************************//

"float3 getNormalMap(float3 normal, float2 uv, uint instanceID)\n"
"{\n"
"    NormalTangent tan;\n"
//接ベクトル計算
"    tan = GetTangent(normal, instance[instanceID].world, cameraUp);\n"
//法線テクスチャ
"    float4 Tnor = g_texNormal[instanceID].SampleLevel(g_samLinear, uv, 0.0);\n"
//ノーマルマップでの法線出力
"    return GetNormal(Tnor.xyz, tan.normal, tan.tangent);\n"
"}\n"

//光源へ光線を飛ばす, ヒットした場合明るさが加算
"float3 EmissivePayloadCalculate(in float3 hitPosition, in float3 difTexColor, float3 normal)\n"
"{\n"
"    RayPayload emissivePayload;\n"
"    LightOut emissiveColor = (LightOut)0;\n"
"    LightOut Out;\n"
"    RayDesc ray;\n"
//ヒットした位置がスタート
"    ray.Origin = hitPosition;\n"
"    ray.TMin = 0.01;\n"
"    ray.TMax = 100000;\n"
"    for(uint i = 0; i < numEmissive.x; i++) {\n"
"        float3 lightVec = normalize(emissivePosition[i].xyz - hitPosition);\n"
"        ray.Direction = lightVec;\n"
"        TraceRay(gRtScene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES |\n"
"            RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH, 0xFF, 2, 0, 2, ray, emissivePayload);\n"

//光源計算
"        float3 SpeculerCol = float3(0.5f, 0.5f, 0.5f);\n"
"        float3 Diffuse = float3(0.5f, 0.5f, 0.5f);\n"
"        float3 Ambient = float3(0.1f, 0.1f, 0.1f);\n"
"        Out = PointLightCom(SpeculerCol, Diffuse, Ambient, normal, emissivePosition[i], \n"
"                            hitPosition, lightst[i], emissivePayload.color, cameraPosition.xyz, 4.0f);\n"

"        emissiveColor.Diffuse += Out.Diffuse;\n"
"        emissiveColor.Speculer += Out.Speculer;\n"
"    }\n"

"    difTexColor *= emissiveColor.Diffuse;\n"
"    float3 speCol = float3(1.0f, 1.0f, 1.0f);\n"
"    speCol *= emissiveColor.Speculer;\n"

"    return difTexColor + speCol;\n"
"}\n"

//**************************************camHit_Shader*******************************************************************//
//視線がヒットした場合起動される
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
"    float4 color = g_texDiffuse[instanceID].SampleLevel(g_samLinear, triangleUV, 0.0);\n"

//光源への光線
"    color.xyz = EmissivePayloadCalculate(hitPosition, color.xyz, normalMap);\n"

"    RayPayload metallicPayload;\n"
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
"    if (metallicPayload.hit) {\n"
"        payload.color = color.xyz * metallicPayload.color;\n"//ヒットした場合映り込みとして加算
"    }\n"
"    else {\n"
"        payload.color = color;\n"
"    }\n"
"}\n"
//**************************************camHit_Shader*******************************************************************//

//***********************************metallicHit_Shader*****************************************************************//
//camHitから飛ばされた光線がヒットした場合起動される
"[shader(\"closesthit\")]\n"
"void metallicHit(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attr)\n"
"{\n"
"    uint instanceID = InstanceID();\n"
"    float3 hitPosition = HitWorldPosition();\n"

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
//ヒットした位置のテクスチャの色をpayload.color格納する
"    float4 difTex = g_texDiffuse[instanceID].SampleLevel(g_samLinear, triangleUV, 0.0);\n"
"    payload.color = difTex.xyz;\n"
"    payload.color = EmissivePayloadCalculate(hitPosition, payload.color, normalMap);\n"
"    payload.hit = true;\n"
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
"    uint instanceID = InstanceID();\n"
"    uint mNo = instance[instanceID].materialNo;\n"

"    if(mNo == 1) {\n"//マテリアルがエミッシブの場合のみ処理
"       uint indicesPerTriangle = 3;\n"
"       uint baseIndex = PrimitiveIndex() * indicesPerTriangle;\n"

"       float2 vertexUVs[3] = {\n"
"           Vertices[instanceID][Indices[instanceID][baseIndex + 0]].tex,\n"
"           Vertices[instanceID][Indices[instanceID][baseIndex + 1]].tex,\n"
"           Vertices[instanceID][Indices[instanceID][baseIndex + 2]].tex\n"
"       };\n"
//UV計算
"       float2 triangleUV = getCenterUV(vertexUVs, attr);\n"
"       float4 difTex = g_texDiffuse[instanceID].SampleLevel(g_samLinear, triangleUV, 0.0);\n"
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