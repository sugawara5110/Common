///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                      ShaderCommonDXR.hlsl                                             //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

char* ShaderCommonDXR =

/////////////////////////////////////RGB → sRGBに変換/////////////////////////////////////////////
"float3 linearToSrgb(float3 c)\n"
"{\n"
"    float3 sq1 = sqrt(c);\n"
"    float3 sq2 = sqrt(sq1);\n"
"    float3 sq3 = sqrt(sq2);\n"
"    float3 srgb = 0.662002687 * sq1 + 0.684122060 * sq2 - 0.323583601 * sq3 - 0.0225411470 * c;\n"
"    return srgb;\n"
"}\n"

///////////////////////////////////////ヒット位置取得/////////////////////////////////////////////
"float3 HitWorldPosition()\n"
"{\n"
//     原点           現在のヒットまでの距離      方向
"    return WorldRayOrigin() + RayTCurrent() * WorldRayDirection();\n"
"}\n"

////////////////Hitの重心を使用して、頂点属性から補間されたhit位置の属性を取得(法線)///////////////
"float3 getCenterNormal(float3 vertexNormal[3], BuiltInTriangleIntersectionAttributes attr)\n"
"{\n"
"    return vertexNormal[0] +\n"
"        attr.barycentrics.x * (vertexNormal[1] - vertexNormal[0]) +\n"
"        attr.barycentrics.y * (vertexNormal[2] - vertexNormal[0]);\n"
"}\n"

////////////////////////////////法線取得//////////////////////////////////////////////////////////
"float3 getNormal(uint instanceID, BuiltInTriangleIntersectionAttributes attr)\n"
"{\n"
"    uint indicesPerTriangle = 3;\n"
"    uint baseIndex = PrimitiveIndex() * indicesPerTriangle;\n"

"    float3 vertexNormals[3] = {\n"
"        Vertices[instanceID][Indices[instanceID][baseIndex + 0]].normal,\n"
"        Vertices[instanceID][Indices[instanceID][baseIndex + 1]].normal,\n"
"        Vertices[instanceID][Indices[instanceID][baseIndex + 2]].normal\n"
"    };\n"
"    return getCenterNormal(vertexNormals, attr);\n"
"}\n"

////////////////Hitの重心を使用して、頂点属性から補間されたhit位置の属性を取得(UV)////////////////
"float2 getCenterUV(float2 vertexUV[3], BuiltInTriangleIntersectionAttributes attr)\n"
"{\n"
"    return vertexUV[0] +\n"
"        attr.barycentrics.x * (vertexUV[1] - vertexUV[0]) +\n"
"        attr.barycentrics.y * (vertexUV[2] - vertexUV[0]);\n"
"}\n"

////////////////////////////////UV取得//////////////////////////////////////////////////////////
"float2 getUV(uint instanceID, BuiltInTriangleIntersectionAttributes attr, uint uvNo)\n"
"{\n"
"    uint indicesPerTriangle = 3;\n"
"    uint baseIndex = PrimitiveIndex() * indicesPerTriangle;\n"

"    float2 vertexUVs[3] = {\n"
"        Vertices[instanceID][Indices[instanceID][baseIndex + 0]].tex[uvNo],\n"
"        Vertices[instanceID][Indices[instanceID][baseIndex + 1]].tex[uvNo],\n"
"        Vertices[instanceID][Indices[instanceID][baseIndex + 2]].tex[uvNo]\n"
"    };\n"
"    return getCenterUV(vertexUVs, attr);\n"
"}\n"

/////////////////////////////ノーマルテクスチャから法線取得/////////////////////////////////////
"float3 getNormalMap(float3 normal, float2 uv, uint instanceID)\n"
"{\n"
"    NormalTangent tan;\n"
//接ベクトル計算
"    tan = GetTangent(normal, instance[instanceID].world, cameraUp);\n"//ShaderCG内関数
//法線テクスチャ
"    float4 Tnor = g_texNormal[instanceID].SampleLevel(g_samLinear, uv, 0.0);\n"
//ノーマルマップでの法線出力
"    return GetNormal(Tnor.xyz, tan.normal, tan.tangent);\n"//ShaderCG内関数
"}\n"

///////////////////////光源へ光線を飛ばす, ヒットした場合明るさが加算//////////////////////////
"float3 EmissivePayloadCalculate(in float3 hitPosition, in float3 difTexColor, float3 speTexColor, float3 normal)\n"
"{\n"
"    RayPayload emissivePayload;\n"
"    uint instanceID = InstanceID();\n"
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
"        float3 SpeculerCol = material[instanceID].Speculer.xyz;\n"
"        float3 Diffuse = material[instanceID].Diffuse.xyz;\n"
"        float3 Ambient = material[instanceID].Ambient.xyz;\n"
"        float shininess = material[instanceID].shininess;\n"
"        Out = PointLightCom(SpeculerCol, Diffuse, Ambient, normal, emissivePosition[i], \n"//ShaderCG内関数
"                            hitPosition, lightst[i], emissivePayload.color, cameraPosition.xyz, shininess);\n"

"        emissiveColor.Diffuse += Out.Diffuse;\n"
"        emissiveColor.Speculer += Out.Speculer;\n"
"    }\n"

"    difTexColor *= emissiveColor.Diffuse;\n"
"    speTexColor *= emissiveColor.Speculer;\n"

"    return difTexColor + speTexColor;\n"
"}\n"

///////////////////////反射方向へ光線を飛ばす, ヒットした場合ピクセル値乗算///////////////////////
"float3 MetallicPayloadCalculate(in float3 hitPosition, in float3 difTexColor, float3 normal)\n"
"{\n"
"    RayPayload payload;\n"
"    if (!(RayFlags() & RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH))\n"
"    {\n"
"        RayDesc ray;\n"
//ヒットした位置がスタート
"        ray.Origin = hitPosition;\n"
//視線ベクトル 
"        float3 eyeVec = WorldRayDirection();\n"
//反射ベクトル
"        float3 reflectVec = reflect(eyeVec, normalize(normal));\n"
"        ray.Direction = reflectVec;\n"//反射方向にRayを飛ばす
"        ray.TMin = 0.01;\n"
"        ray.TMax = 100000;\n"
"        TraceRay(gRtScene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES |\n"
"            RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH, 0xFF, 1, 0, 1, ray, payload);\n"
"    }\n"
"    float3 outCol = float3(0.0f, 0.0f, 0.0f);\n"
"    if (payload.hit && payload.Alpha >= 1.0f) {\n"
"        outCol = difTexColor * payload.color;\n"//ヒットした場合映り込みとして乗算
"    }\n"
"    else {\n"
"        outCol = difTexColor;\n"//ヒットしなかった場合映り込み無しで元のピクセル書き込み
"    }\n"

"    return outCol;\n"
"}\n"

////////////////////////////////////アルファテスト//////////////////////////////////////
"float3 AlphaTest(in float3 hitPosition, in float3 difTexColor, float Alpha, uint shaderIndex)\n"
"{\n"
"    RayPayload payload;\n"
"    if (!(RayFlags() & RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH))\n"
"    {\n"
"        RayDesc ray;\n"
"        ray.Origin = hitPosition;\n"
"        ray.TMin = 0.01;\n"
"        ray.TMax = 100000;\n"
"        ray.Direction = WorldRayDirection();\n"
"        TraceRay(gRtScene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES |\n"
"            RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH, 0xFF, shaderIndex, 0, shaderIndex, ray, payload);\n"
"    }\n"
//アルファ値の比率で元の色と光線衝突先の色を配合
"    return payload.color * (1.0f - Alpha) + difTexColor * Alpha;\n"
"}\n";