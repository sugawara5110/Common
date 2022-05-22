///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                      ShaderCommonDXR.hlsl                                             //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

char* ShaderCommonDXR =

///////////////////////////////////////////Material識別////////////////////////////////////////////
"bool materialIdent(uint matNo, uint MaterialBit)\n"
"{\n"
"    return (matNo & MaterialBit) == MaterialBit;\n"
"}\n"

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

///////////////////////////////////////頂点取得////////////////////////////////////////////////////
"Vertex3 getVertex()\n"
"{\n"
"    uint indicesPerTriangle = 3;\n"
"    uint baseIndex = PrimitiveIndex() * indicesPerTriangle;\n"
"    uint materialID = getMaterialID();\n"

"    Vertex3 ver = {\n"
"        Vertices[materialID][Indices[materialID][baseIndex + 0]],\n"
"        Vertices[materialID][Indices[materialID][baseIndex + 1]],\n"
"        Vertices[materialID][Indices[materialID][baseIndex + 2]]\n"
"    };\n"
"    return ver;\n"
"}\n"

///////////////////////////////////////深度値取得//////////////////////////////////////////////////
"float getDepth(in BuiltInTriangleIntersectionAttributes attr, Vertex3 v3)\n"
"{\n"
"    float3 vertex[3] = {\n"
"        v3.v[0].Pos,\n"
"        v3.v[1].Pos,\n"
"        v3.v[2].Pos\n"
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
"float3 getNormal(in BuiltInTriangleIntersectionAttributes attr, Vertex3 v3)\n"
"{\n"
"    float3 vertexNormals[3] = {\n"
"        v3.v[0].normal,\n"
"        v3.v[1].normal,\n"
"        v3.v[2].normal\n"
"    };\n"
"    return getCenterNormal(vertexNormals, attr);\n"
"}\n"
////////////////////////////////接ベクトル取得//////////////////////////////////////////////////////////
"float3 get_tangent(in BuiltInTriangleIntersectionAttributes attr, Vertex3 v3)\n"
"{\n"
"    float3 vertexTangents[3] = {\n"
"        v3.v[0].tangent,\n"
"        v3.v[1].tangent,\n"
"        v3.v[2].tangent\n"
"    };\n"
"    return getCenterNormal(vertexTangents, attr);\n"
"}\n"

////////////////Hitの重心を使用して、頂点属性から補間されたhit位置の属性を取得(UV)////////////////
"float2 getCenterUV(in float2 vertexUV[3], in BuiltInTriangleIntersectionAttributes attr)\n"
"{\n"
"    return vertexUV[0] +\n"
"        attr.barycentrics.x * (vertexUV[1] - vertexUV[0]) +\n"
"        attr.barycentrics.y * (vertexUV[2] - vertexUV[0]);\n"
"}\n"

////////////////////////////////UV取得//////////////////////////////////////////////////////////
"float2 getUV(in BuiltInTriangleIntersectionAttributes attr, in uint uvNo, Vertex3 v3)\n"
"{\n"
"    float2 vertexUVs[3] = {\n"
"        v3.v[0].tex[uvNo],\n"
"        v3.v[1].tex[uvNo],\n"
"        v3.v[2].tex[uvNo]\n"
"    };\n"
"    return getCenterUV(vertexUVs, attr);\n"
"}\n"

/////////////////////////////ノーマルテクスチャから法線取得/////////////////////////////////////
"float3 getNormalMap(in float3 normal, in float2 uv, in float3 tangent)\n"
"{\n"
"    NormalTangent tan;\n"
"    uint instancingID = getInstancingID();\n"
"    uint materialID = getMaterialID();\n"
//接ベクトル計算
"    tan = GetTangent(normal, (float3x3)wvp[getInstancingID()].world, tangent);\n"//ShaderCG内関数
//法線テクスチャ
"    float4 Tnor = g_texNormal[materialID].SampleLevel(g_samLinear, uv, 0.0);\n"
//ノーマルマップでの法線出力
"    return GetNormal(Tnor.xyz, tan.normal, tan.tangent);\n"//ShaderCG内関数
"}\n"

//////////////////////////////////////ピクセル値取得///////////////////////////////////////////
//////////////ディフェーズ
"float4 getDifPixel(in BuiltInTriangleIntersectionAttributes attr, Vertex3 v3)\n"
"{\n"
"    uint materialID = getMaterialID();\n"
//UV計算
"    float2 UV = getUV(attr, 0, v3);\n"
//ピクセル値
"    float4 difTex = g_texDiffuse[materialID].SampleLevel(g_samLinear, UV, 0.0);\n"
"    float4 add = wvp[getInstancingID()].AddObjColor;\n"
"    difTex.x = saturate(difTex.x + add.x);\n"
"    difTex.y = saturate(difTex.y + add.y);\n"
"    difTex.z = saturate(difTex.z + add.z);\n"
"    difTex.w = saturate(difTex.w + add.w);\n"
"    return difTex;\n"
"}\n"
//////////////ノーマル
"float3 getNorPixel(in BuiltInTriangleIntersectionAttributes attr, Vertex3 v3)\n"
"{\n"
"    uint materialID = getMaterialID();\n"
//UV計算
"    float2 UV = getUV(attr, 0, v3);\n"
//法線の計算
"    float3 triangleNormal = getNormal(attr, v3);\n"
//接ベクトル計算
"    float3 tan = get_tangent(attr, v3);\n"
//ノーマルマップからの法線出力
"    return getNormalMap(triangleNormal, UV, tan);\n"
"}\n"
//////////////スペキュラ
"float3 getSpePixel(in BuiltInTriangleIntersectionAttributes attr, Vertex3 v3)\n"
"{\n"
"    uint materialID = getMaterialID();\n"
//UV計算
"    float2 UV = getUV(attr, 1, v3);\n"
//ピクセル値
"    float4 spe = g_texSpecular[materialID].SampleLevel(g_samLinear, UV, 0.0);\n"
"    return spe.xyz;\n"
"}\n"

///////////////////////光源へ光線を飛ばす, ヒットした場合明るさが加算//////////////////////////
"float3 EmissivePayloadCalculate(in uint RecursionCnt, in float3 hitPosition, \n"
"                                in float3 difTexColor, in float3 speTexColor, in float3 normal)\n"
"{\n"
"    uint materialID = getMaterialID();\n"
"    MaterialCB mcb = material[materialID];\n"
"    uint mNo = mcb.materialNo;\n"
"    float3 ret = difTexColor;\n"

"    bool mf = materialIdent(mNo, EMISSIVE);\n"
"    if(!mf) {\n"//emissive以外

"       RayPayload payload;\n"
"       LightOut emissiveColor = (LightOut)0;\n"
"       LightOut Out;\n"
"       RayDesc ray;\n"
"       payload.hitPosition = hitPosition;\n"
"       ray.TMin = TMin_TMax.x;\n"
"       ray.TMax = TMin_TMax.y;\n"
"       RecursionCnt++;\n"

"       float3 SpeculerCol = mcb.Speculer.xyz;\n"
"       float3 Diffuse = mcb.Diffuse.xyz;\n"
"       float3 Ambient = mcb.Ambient.xyz;\n"
"       float shininess = mcb.shininess;\n"

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
"                    payload.mNo = EMISSIVE;\n"//処理分岐用
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
"                payload.mNo = DIRECTIONLIGHT | METALLIC;\n"//処理分岐用
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

"    if(materialIdent(mNo, METALLIC)) {\n"//METALLIC

"       RayPayload payload;\n"
"       RecursionCnt++;\n"
"       payload.RecursionCnt = RecursionCnt;\n"
"       RayDesc ray;\n"
//視線ベクトル 
"       float3 eyeVec = WorldRayDirection();\n"
//反射ベクトル
"       float3 reflectVec = reflect(eyeVec, normalize(normal));\n"
"       ray.Direction = reflectVec;\n"//反射方向にRayを飛ばす
"       ray.TMin = TMin_TMax.x;\n"
"       ray.TMax = TMin_TMax.y;\n"

"       if (RecursionCnt <= maxRecursion) {\n"
"           payload.hitPosition = hitPosition;\n"
"           ray.Origin = payload.hitPosition;\n"
"           TraceRay(gRtScene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, \n"
"                    0xFF, 0, 0, 0, ray, payload);\n"
"       }\n"
"       float3 outCol = float3(0.0f, 0.0f, 0.0f);\n"
"       if (payload.hit) {\n"
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
"    MaterialCB mcb = material[materialID];\n"
"    uint mNo = mcb.materialNo;\n"
"    float3 ret = difTexColor.xyz;\n"

"    if(materialIdent(mNo, TRANSLUCENCE)) {\n"

"       float Alpha = difTexColor.w;\n"
"       RayPayload payload;\n"
"       RecursionCnt++;\n"
"       payload.RecursionCnt = RecursionCnt;\n"
"       RayDesc ray; \n"
"       ray.TMin = TMin_TMax.x;\n"
"       ray.TMax = TMin_TMax.y;\n"
//視線ベクトル 
"       float3 eyeVec = WorldRayDirection();\n"
"       ray.Direction = normalize(eyeVec + -normal * mcb.RefractiveIndex);\n"

"       if (RecursionCnt <= maxRecursion) {\n"
"           payload.hitPosition = hitPosition;\n"
"           ray.Origin = payload.hitPosition;\n"
"           TraceRay(gRtScene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, \n"
"                    0xFF, 0, 0, 0, ray, payload);\n"
"       }\n"
//アルファ値の比率で元の色と光線衝突先の色を配合
"       ret = payload.color * (1.0f - Alpha) + difTexColor * Alpha;\n"
"    }\n"
"    return ret;\n"
"}\n"

////////////////////////////////////アルファブレンド//////////////////////////////////////
"float3 AlphaBlend(in uint RecursionCnt, in float3 hitPosition, in float4 difTexColor)\n"
"{\n"
"    uint materialID = getMaterialID();\n"
"    MaterialCB mcb = material[materialID];\n"
"    uint mNo = mcb.materialNo;\n"
"    float3 ret = difTexColor.xyz;\n"
"    float blend = mcb.AlphaBlend;\n"
"    float Alpha = difTexColor.w;\n"

"    bool mf = materialIdent(mNo, TRANSLUCENCE);\n"
"    if(blend == 1.0f && !mf && Alpha < 1.0f) {\n"

"       RayPayload payload;\n"
"       RecursionCnt++;\n"
"       payload.RecursionCnt = RecursionCnt;\n"
"       RayDesc ray; \n"
"       ray.TMin = TMin_TMax.x;\n"
"       ray.TMax = TMin_TMax.y;\n"
"       ray.Direction = WorldRayDirection();\n"

"       if (RecursionCnt <= maxRecursion) {\n"
"           payload.hitPosition = hitPosition;\n"
"           ray.Origin = payload.hitPosition;\n"
"           TraceRay(gRtScene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, \n"
"                    0xFF, 0, 0, 0, ray, payload);\n"
"       }\n"
//アルファ値の比率で元の色と光線衝突先の色を配合
"       ret = payload.color * (1.0f - Alpha) + difTexColor * Alpha;\n"
"    }\n"
"    return ret;\n"
"}\n";