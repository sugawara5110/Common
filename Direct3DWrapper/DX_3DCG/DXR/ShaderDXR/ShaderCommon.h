///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                      ShaderCommon.hlsl                                                //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

char* ShaderCommon =

///////////////////////////////////////////traceRay////////////////////////////////////////////////
"void traceRay(in uint RecursionCnt, \n"
"              in uint RayFlags, \n"
"              in uint HitGroupIndex, \n"
"              in uint MissShaderIndex, \n"
"              in RayDesc ray, \n"
"              inout RayPayload payload)\n"
"{\n"
"    ray.TMin = TMin_TMax.x;\n"
"    ray.TMax = TMin_TMax.y;\n"
"    payload.color = float3(0.0f, 0.0f, 0.0f);\n"
"    RecursionCnt++;\n"
"    payload.RecursionCnt = RecursionCnt;\n"
"    payload.EmissiveIndex = 0;\n"

"    if(RecursionCnt <= maxRecursion) {\n"
"       bool loop = true;\n"
"       while(loop){\n"
"          ray.Origin = payload.hitPosition;\n"
"          TraceRay(gRtScene, RayFlags, 0xFF, HitGroupIndex, 0, MissShaderIndex, ray, payload);\n"
"          loop = payload.reTry;\n"
"       }\n"
"    }\n"
"}\n"

///////////////////////////////////////////ランダムfloat///////////////////////////////////////////
"float Rand_float(float2 v2)\n"
"{\n"
"    Seed++;\n"
"    uint frameIndex = gFrameIndexMap[DispatchRaysIndex().xy];\n"
"    return sin(dot(v2, float2(12.9898, 78.233)) * (frameIndex % 100 + 1) * 0.001 + Seed + frameIndex) * 43758.5453;\n"
"}\n"

///////////////////////////////////////////ランダム整数////////////////////////////////////////////
"uint Rand_integer()\n"
"{\n"
"    float2 index = (float2)DispatchRaysIndex().xy;\n"
"    return (uint)(abs(Rand_float(index)));\n"
"}\n"

///////////////////////////////////////////ランダム少数////////////////////////////////////////////
"float Rand_frac(float2 v2)\n"
"{\n"
"    return frac(Rand_float(v2));\n"
"}\n"

///////////////////////////////////////////ランダムベクトル////////////////////////////////////////
"float3 RandomVector(float3 v, float area)\n"
"{\n"
"    float2 index = (float2)DispatchRaysIndex().xy;\n"
"    float rand1 = Rand_frac(index);\n"
"    float rand2 = Rand_frac(index + 0.5f);\n"

//ランダムなベクトルを生成
"    float z = area * rand1 - 1.0f;\n"
"    float phi = PI * (2.0f * rand2 - 1.0f);\n"
"    float sq = sqrt(1.0f - z * z);\n"
"    float x = sq * cos(phi);\n"
"    float y = sq * sin(phi);\n"
"    float3 randV = float3(x, y, z);\n"

"    return -localToWorld(v, randV);\n"
"}\n"

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

///////////////////////////////////////ヒット位置取得/////////////////////////////////////////////
"float3 HitWorldPosition()\n"
"{\n"
//     原点           現在のヒットまでの距離      レイを飛ばした方向
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

///////////////////////////////////////////DiffuseBRDF/////////////////////////////////////////////
"float3 DiffuseBRDF(float3 diffuse)\n"
"{\n"
"    return diffuse / PI;\n"
"}\n"

///////////////////////////////////////////SpecularBRDF////////////////////////////////////////////
"float3 SpecularPhongBRDF(float3 Specular, float3 normal, float3 viewDir, float3 lightDir, float shininess)\n"
"{\n"
"    float norm = (shininess + 2.0f) / (2 * PI);\n"
"    float3 halfDir = normalize(viewDir + lightDir);\n"
"    float dotNH = abs(dot(normal, halfDir));\n"
"    return Specular * pow(dotNH, shininess) * norm;\n"
"}\n"

///////////////////////////////////////////CosinePDF///////////////////////////////////////////////
"float CosinePDF(float3 normal, float3 dir)\n"
"{\n"
"    const float dotNL = abs(dot(normal, dir));\n"
"    return dotNL / PI;\n"
"}\n"

///////////////////////////////////////////LightPDF////////////////////////////////////////////////
"float LightPDF(uint emIndex)\n"
"{\n"
"    float NumEmissive = (float)numEmissive.x;\n"
"    float emSize = emissiveNo[emIndex].y;\n"
"    return 1.0f / (NumEmissive * emSize);\n"
"}\n"

///////////////////////////////////////////radiusPDF///////////////////////////////////////////////
"float radiusPDF()\n"
"{\n"
"    return 1.0f / (2 * PI);\n"
"}\n"

///////////////////////////////////////////sumBRDF/////////////////////////////////////////////////
"float3 sumBRDF(float3 inDir, float3 outDir, float3 difTexColor, float3 speTexColor, float3 normal)\n"
"{\n"
"    uint materialID = getMaterialID();\n"
"    MaterialCB mcb = material[materialID];\n"
"    float3 Diffuse = mcb.Diffuse.xyz * difTexColor;\n"
"    float3 Speculer = mcb.Speculer.xyz * speTexColor;\n"
//"    float3 Ambient = mcb.Ambient.xyz + GlobalAmbientColor.xyz;\n"
"    float shininess = mcb.shininess;\n"

"    float3 local_inDir = worldToLocal(normal, inDir);\n"
"    float3 local_outDir = worldToLocal(normal, outDir);\n"

"    float3 difBRDF = DiffuseBRDF(Diffuse);\n"
"    float3 speBRDF = SpecularPhongBRDF(Speculer, normal, local_outDir, local_inDir, shininess);\n"

"    return difBRDF + speBRDF;\n"
"}\n";
