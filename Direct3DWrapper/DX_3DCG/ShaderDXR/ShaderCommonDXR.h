///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                      ShaderCommonDXR.hlsl                                             //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

char* ShaderCommonDXR =

///////////////////////////////////////////MaterialID取得//////////////////////////////////////////
"uint getMaterialID()\n"
"{\n"
"    uint instanceID = InstanceID();\n"
"    return (instanceID >> 16) & 0x0000ffff;\n"
"}\n"

////////////////////////////////////////InstancingID取得///////////////////////////////////////////
"uint getInstancingID()\n"
"{\n"
"    uint instanceID = InstanceID();\n"
"    return instanceID & 0x0000ffff;\n"
"}\n"

/////////////////////////////////////RGB → sRGBに変換/////////////////////////////////////////////
"float3 linearToSrgb(in float3 c)\n"
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

///////////////////////////////////////深度値取得//////////////////////////////////////////////////
"float getDepth(in BuiltInTriangleIntersectionAttributes attr)\n"
"{\n"
"    uint indicesPerTriangle = 3;\n"
"    uint baseIndex = PrimitiveIndex() * indicesPerTriangle;\n"
"    uint materialID = getMaterialID();\n"

"    float3 vertex[3] = {\n"
"        Vertices[materialID][Indices[materialID][baseIndex + 0]].Pos,\n"
"        Vertices[materialID][Indices[materialID][baseIndex + 1]].Pos,\n"
"        Vertices[materialID][Indices[materialID][baseIndex + 2]].Pos\n"
"    };\n"

"    float3 v = vertex[0] + \n"
"        attr.barycentrics.x * (vertex[1] - vertex[0]) +\n"
"        attr.barycentrics.y * (vertex[2] - vertex[0]);\n"
"    float4 ver = float4(v, 1.0f);\n"

"    matrix m = wvp[getInstancingID()].wvp;\n"
"    float4 ver2 = mul(ver, m);\n"
"    return ver2.z / ver2.w;\n"
"}\n"

////////////////Hitの重心を使用して、頂点属性から補間されたhit位置の属性を取得(法線)///////////////
"float3 getCenterNormal(in float3 vertexNormal[3], in BuiltInTriangleIntersectionAttributes attr)\n"
"{\n"
"    return vertexNormal[0] +\n"
"        attr.barycentrics.x * (vertexNormal[1] - vertexNormal[0]) +\n"
"        attr.barycentrics.y * (vertexNormal[2] - vertexNormal[0]);\n"
"}\n"

////////////////////////////////法線取得//////////////////////////////////////////////////////////
"float3 getNormal(in BuiltInTriangleIntersectionAttributes attr)\n"
"{\n"
"    uint indicesPerTriangle = 3;\n"
"    uint baseIndex = PrimitiveIndex() * indicesPerTriangle;\n"
"    uint materialID = getMaterialID();\n"

"    float3 vertexNormals[3] = {\n"
"        Vertices[materialID][Indices[materialID][baseIndex + 0]].normal,\n"
"        Vertices[materialID][Indices[materialID][baseIndex + 1]].normal,\n"
"        Vertices[materialID][Indices[materialID][baseIndex + 2]].normal\n"
"    };\n"
"    return getCenterNormal(vertexNormals, attr);\n"
"}\n"

////////////////Hitの重心を使用して、頂点属性から補間されたhit位置の属性を取得(UV)////////////////
"float2 getCenterUV(in float2 vertexUV[3], in BuiltInTriangleIntersectionAttributes attr)\n"
"{\n"
"    return vertexUV[0] +\n"
"        attr.barycentrics.x * (vertexUV[1] - vertexUV[0]) +\n"
"        attr.barycentrics.y * (vertexUV[2] - vertexUV[0]);\n"
"}\n"

////////////////////////////////UV取得//////////////////////////////////////////////////////////
"float2 getUV(in BuiltInTriangleIntersectionAttributes attr, in uint uvNo)\n"
"{\n"
"    uint indicesPerTriangle = 3;\n"
"    uint baseIndex = PrimitiveIndex() * indicesPerTriangle;\n"
"    uint materialID = getMaterialID();\n"

"    float2 vertexUVs[3] = {\n"
"        Vertices[materialID][Indices[materialID][baseIndex + 0]].tex[uvNo],\n"
"        Vertices[materialID][Indices[materialID][baseIndex + 1]].tex[uvNo],\n"
"        Vertices[materialID][Indices[materialID][baseIndex + 2]].tex[uvNo]\n"
"    };\n"
"    return getCenterUV(vertexUVs, attr);\n"
"}\n"

/////////////////////////////ノーマルテクスチャから法線取得/////////////////////////////////////
"float3 getNormalMap(in float3 normal, float2 uv)\n"
"{\n"
"    NormalTangent tan;\n"
"    uint instancingID = getInstancingID();\n"
"    uint materialID = getMaterialID();\n"
//接ベクトル計算
"    tan = GetTangent(normal, (float3x3)wvp[getInstancingID()].world, cameraUp);\n"//ShaderCG内関数
//法線テクスチャ
"    float4 Tnor = g_texNormal[materialID].SampleLevel(g_samLinear, uv, 0.0);\n"
//ノーマルマップでの法線出力
"    return GetNormal(Tnor.xyz, tan.normal, tan.tangent);\n"//ShaderCG内関数
"}\n"

//////////////////////////////////////ピクセル値取得///////////////////////////////////////////
//////////////ディフェーズ
"float4 getDifPixel(in BuiltInTriangleIntersectionAttributes attr)\n"
"{\n"
"    uint materialID = getMaterialID();\n"
//UV計算
"    float2 UV = getUV(attr, 0);\n"
//ピクセル値
"    float4 difTex = g_texDiffuse[materialID].SampleLevel(g_samLinear, UV, 0.0);\n"
"    float4 add = material[materialID].AddObjColor;\n"
"    difTex.x = saturate(difTex.x + add.x);\n"
"    difTex.y = saturate(difTex.y + add.y);\n"
"    difTex.z = saturate(difTex.z + add.z);\n"
"    difTex.w = saturate(difTex.w + add.w);\n"
"    return difTex;\n"
"}\n"
//////////////ノーマル
"float3 getNorPixel(in BuiltInTriangleIntersectionAttributes attr)\n"
"{\n"
"    uint materialID = getMaterialID();\n"
//UV計算
"    float2 UV = getUV(attr, 0);\n"
//法線の計算
"    float3 triangleNormal = getNormal(attr);\n"
//ノーマルマップからの法線出力
"    return getNormalMap(triangleNormal, UV);\n"
"}\n"
//////////////スペキュラ
"float3 getSpePixel(in BuiltInTriangleIntersectionAttributes attr)\n"
"{\n"
"    uint materialID = getMaterialID();\n"
//UV計算
"    float2 UV = getUV(attr, 1);\n"
//ピクセル値
"    float4 spe = g_texSpecular[materialID].SampleLevel(g_samLinear, UV, 0.0);\n"
"    return spe.xyz;\n"
"}\n"

///////////////////////光源へ光線を飛ばす, ヒットした場合明るさが加算//////////////////////////
"float3 EmissivePayloadCalculate(in uint RecursionCnt, in float3 hitPosition, \n"
"                                in float3 difTexColor, in float3 speTexColor, in float3 normal)\n"
"{\n"
"    uint materialID = getMaterialID();\n"
"    uint mNo = material[materialID].materialNo;\n"
"    float3 ret = difTexColor;\n"

"    if(mNo != 2) {\n"//emissive以外

"       RayPayload payload;\n"
"       LightOut emissiveColor = (LightOut)0;\n"
"       LightOut Out;\n"
"       RayDesc ray;\n"
"       payload.hitPosition = hitPosition;\n"
"       ray.TMin = 0.01;\n"
"       ray.TMax = 100000;\n"
"       RecursionCnt++;\n"

"       float3 SpeculerCol = material[materialID].Speculer.xyz;\n"
"       float3 Diffuse = material[materialID].Diffuse.xyz;\n"
"       float3 Ambient = material[materialID].Ambient.xyz;\n"
"       float shininess = material[materialID].shininess;\n"

"       if(RecursionCnt <= maxRecursion) {\n"
//点光源計算
"          for(uint i = 0; i < numEmissive.x; i++) {\n"
"              if(emissivePosition[i].w == 1.0f) {\n"
"                 float3 lightVec = normalize(emissivePosition[i].xyz - hitPosition);\n"
"                 ray.Direction = lightVec;\n"
"                 payload.instanceID = (uint)emissiveNo[i].x;\n"
"                 bool loop = true;\n"
"                 payload.hitPosition = hitPosition;\n"
"                 while(loop){\n"
"                    payload.mNo = 2;\n"//処理分岐用
"                    ray.Origin = payload.hitPosition;\n"
"                    TraceRay(gRtScene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, \n"
"                             0xFF, 1, 0, 1, ray, payload);\n"
"                    loop = payload.reTry;\n"
"                 }\n"

"                 Out = PointLightCom(SpeculerCol, Diffuse, Ambient, normal, emissivePosition[i], \n"//ShaderCG内関数
"                                     hitPosition, lightst[i], payload.color, cameraPosition.xyz, shininess);\n"

"                 emissiveColor.Diffuse += Out.Diffuse;\n"
"                 emissiveColor.Speculer += Out.Speculer;\n"
"              }\n"
"          }\n"
//平行光源計算
"          if(dLightst.x == 1.0f){\n"
"             payload.hitPosition = hitPosition;\n"
"             ray.Direction = -dDirection.xyz;\n"
"             bool loop = true;\n"
"             while(loop){\n"
"                payload.mNo = 3;\n"//処理分岐用
"                ray.Origin = payload.hitPosition;\n"
"                TraceRay(gRtScene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, \n"
"                         0xFF, 1, 0, 1, ray, payload);\n"
"                loop = payload.reTry;\n"
"             }\n"

"             Out = DirectionalLightCom(SpeculerCol, Diffuse, Ambient, normal, dLightst, dDirection.xyz, \n"//ShaderCG内関数
"                                       payload.color, hitPosition, cameraPosition.xyz, shininess);\n"

"             emissiveColor.Diffuse += Out.Diffuse;\n"
"             emissiveColor.Speculer += Out.Speculer;\n"
"          }\n"
"       }\n"
//最後にテクスチャの色に掛け合わせ
"       difTexColor *= emissiveColor.Diffuse;\n"
"       speTexColor *= emissiveColor.Speculer;\n"
"       ret = difTexColor + speTexColor;\n"
"    }\n"
"    return ret;\n"
"}\n"

///////////////////////反射方向へ光線を飛ばす, ヒットした場合ピクセル値乗算///////////////////////
"float3 MetallicPayloadCalculate(in uint RecursionCnt, in float3 hitPosition, \n"
"                                in float3 difTexColor, in float3 normal)\n"
"{\n"
"    uint materialID = getMaterialID();\n"
"    uint mNo = material[materialID].materialNo;\n"
"    float3 ret = difTexColor;\n"

"    if(mNo == 0 || mNo == 3) {\n"//METALLIC

"       RayPayload payload;\n"
"       RecursionCnt++;\n"
"       payload.RecursionCnt = RecursionCnt;\n"
"       RayDesc ray;\n"
//視線ベクトル 
"       float3 eyeVec = WorldRayDirection();\n"
//反射ベクトル
"       float3 reflectVec = reflect(eyeVec, normalize(normal));\n"
"       ray.Direction = reflectVec;\n"//反射方向にRayを飛ばす
"       ray.TMin = 0.01;\n"
"       ray.TMax = 100000;\n"

"       if (RecursionCnt <= maxRecursion) {\n"
"           bool loop = true;\n"
"           payload.hitPosition = hitPosition;\n"
"           while(loop){\n"
"              ray.Origin = payload.hitPosition;\n"
"              TraceRay(gRtScene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, \n"
"                       0xFF, 0, 0, 0, ray, payload);\n"
"              loop = payload.reTry;\n"
"           }\n"
"       }\n"
"       float3 outCol = float3(0.0f, 0.0f, 0.0f);\n"
"       if (payload.hit && payload.Alpha > 0.0f) {\n"
"           outCol = difTexColor * payload.color;\n"//ヒットした場合映り込みとして乗算
"       }\n"
"       else {\n"
"           outCol = difTexColor;\n"//ヒットしなかった場合映り込み無しで元のピクセル書き込み
"       }\n"
"       ret = outCol;\n"
"    }\n"
"    return ret;\n"
"}\n"

////////////////////////////////////////半透明//////////////////////////////////////////
"float3 Translucent(in uint RecursionCnt, in float3 hitPosition, in float4 difTexColor, in float3 normal)\n"
"{\n"
"    uint materialID = getMaterialID();\n"
"    uint mNo = material[materialID].materialNo;\n"
"    float3 ret = difTexColor.xyz;\n"

"    if(mNo == 5) {\n"

"       float Alpha = difTexColor.w;\n"
"       RayPayload payload;\n"
"       RecursionCnt++;\n"
"       payload.RecursionCnt = RecursionCnt;\n"
"       RayDesc ray; \n"
"       ray.TMin = 0.01;\n"
"       ray.TMax = 100000;\n"
//視線ベクトル 
"       float3 eyeVec = WorldRayDirection();\n"
"       ray.Direction = normalize(eyeVec + -normal * material[materialID].RefractiveIndex);\n"

"       if (RecursionCnt <= maxRecursion) {\n"
"           bool loop = true;\n"
"           payload.hitPosition = hitPosition;\n"
"           while(loop){\n"
"              ray.Origin = payload.hitPosition;\n"
"              TraceRay(gRtScene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, \n"
"                       0xFF, 0, 0, 0, ray, payload);\n"
"              loop = payload.reTry; \n"
"           }\n"
"       }\n"
//アルファ値の比率で元の色と光線衝突先の色を配合
"       ret = payload.color * (1.0f - Alpha) + difTexColor * Alpha;\n"
"    }\n"
"    return ret;\n"
"}\n"

////////////////////////////////////アルファテスト//////////////////////////////////////
"bool AlphaTestSw(in float Alpha)\n"
"{\n"
"    bool ret = true;\n"
"    uint materialID = getMaterialID();\n"
"    float test = material[materialID].alphaTest;\n"
"    if(test == 1.0f && Alpha <= 0.0f)ret = false;\n"
"    return ret;\n"
"}\n"

"float3 AlphaTest(in uint RecursionCnt, in float3 hitPosition, in float4 difTexColor)\n"
"{\n"
"    uint materialID = getMaterialID();\n"
"    uint mNo = material[materialID].materialNo;\n"
"    float3 ret = difTexColor.xyz;\n"

"    if(material[materialID].alphaTest == 1.0f && mNo != 5) {\n"

"       float Alpha = difTexColor.w;\n"
"       RayPayload payload;\n"
"       RecursionCnt++;\n"
"       payload.RecursionCnt = RecursionCnt;\n"
"       RayDesc ray; \n"
"       ray.TMin = 0.01;\n"
"       ray.TMax = 100000;\n"
"       ray.Direction = WorldRayDirection();\n"

"       if (RecursionCnt <= maxRecursion) {\n"
"           bool loop = true;\n"
"           payload.hitPosition = hitPosition;\n"
"           while(loop){\n"
"              ray.Origin = payload.hitPosition;\n"
"              TraceRay(gRtScene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, \n"
"                       0xFF, 0, 0, 0, ray, payload);\n"
"              loop = payload.reTry; \n"
"           }\n"
"       }\n"
//アルファ値の比率で元の色と光線衝突先の色を配合
"       ret = payload.color * (1.0f - Alpha) + difTexColor * Alpha;\n"
"    }\n"
"    return ret;\n"
"}\n";