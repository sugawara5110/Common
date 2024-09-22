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
"    return sin(dot(v2, float2(12.9898, 78.233)) * (SeedFrame % 100 + 1) * 0.001 + Seed + SeedFrame) * 43758.5453;\n"
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

///////////////////////////////////////////DiffuseBRDF/////////////////////////////////////////////
"float3 DiffuseBRDF(float3 diffuse)\n"
"{\n"
"    return diffuse / PI;\n"
"}\n"

///////////////////////////////////////////CosinePDF///////////////////////////////////////////////
"float CosinePDF(float dotNL)\n"
"{\n"
"    return dotNL / PI;\n"
"}\n"

///////////////////////////////////////////GGX_PDF/////////////////////////////////////////////////
"float GGX_PDF(float NDF, float dotNH, float dotVH)\n"
"{\n"
"    return NDF * dotNH / (4 * dotVH);\n"
"}\n"

///////////////////////////////////////////GGX_GeometrySchlick/////////////////////////////////////
"float GGX_GeometrySchlick(float dotNX, float roughness)\n"
"{\n"
"    float a = roughness * roughness;\n"
"    float k = a / 2.0f;\n"
"    return dotNX / (dotNX * (1 - k) + k);\n"
"}\n"

///////////////////////////////////////////GGX_Distribution////////////////////////////////////////
"float GGX_Distribution(float3 N, float3 H, float roughness)\n"
"{\n"
"    float a = roughness * roughness;\n"
"    float a2 = a * a;\n"
"    float dotNH = max(0.0, dot(N, H));\n"
"    float dotNH2 = dotNH * dotNH;\n"

"    float d = (dotNH2 * (a2 - 1.0f) + 1.0f);\n"
"    d *= PI * d;\n"

"    return a2 / d;\n"
"}\n"

///////////////////////////////////////////GGX_GeometrySmith///////////////////////////////////////
"float GGX_GeometrySmith(float3 N, float3 V, float3 L, float roughness)\n"
"{\n"
"    float dotNV = abs(dot(N, V));\n"
"    float dotNL = abs(dot(N, L));\n"
"    return GGX_GeometrySchlick(dotNV, roughness) * GGX_GeometrySchlick(dotNL, roughness);\n"
"}\n"

///////////////////////////////////////////FresnelSchlick/////////////////////////////////////////
"float3 FresnelSchlick(float dotVH, float3 F0)\n"
"{\n"
"    return F0 + (1 - F0) * pow(1 - dotVH, 5.0);\n"
"}\n"

///////////////////////////////////////////SpecularBRDF///////////////////////////////////////////
"float3 SpecularBRDF(float D, float G, float3 F, float3 V, float3 L, float3 N)\n"
"{\n"
"    float dotNL = abs(dot(N, L));\n"
"    float dotNV = abs(dot(N, V));\n"
"    return (D * G * F) / (4 * dotNV * dotNL + 0.001f);\n"
"}\n"

///////////////////////////////////////////DiffSpeBSDF/////////////////////////////////////////////
"float3 DiffSpeBSDF(float3 inDir, float3 outDir, float3 difTexColor, float3 speTexColor, float3 normal, out float PDF)\n"
"{\n"
"    const uint materialID = getMaterialID();\n"
"    const MaterialCB mcb = material[materialID];\n"
"    const float3 Diffuse = mcb.Diffuse.xyz;\n"
"    const float3 Speculer = mcb.Speculer.xyz;\n"
"    const float roughness = mcb.roughness;\n"

"    if (dot(normal, inDir) <= 0)\n"
"    {\n"
"        PDF = 1.0f;\n"
"        return float3(0, 0, 0);\n"
"    }\n"

"    const float sum_diff = Diffuse.x + Diffuse.y + Diffuse.z;\n"
"    const float sum_spe = Speculer.x + Speculer.y + Speculer.z;\n"
"    const float sum = sum_diff + sum_spe;\n"
"    const float diff_threshold = sum_diff / sum;\n"
"    const float spe_threshold = sum_spe / sum;\n"

"    const float3 H = normalize(inDir + outDir);\n"

"    const float dotNL = abs(dot(normal, inDir));\n"
"    const float dotNH = abs(dot(normal, H));\n"
"    const float dotVH = abs(dot(outDir, H));\n"

"    float3 F0 = 0.08.xxx;\n"
"    F0 = lerp(F0 * speTexColor, difTexColor, (spe_threshold).xxx);\n"

"    const float NDF = GGX_Distribution(normal, H, roughness);\n"
"    const float G = GGX_GeometrySmith(normal, outDir, inDir, roughness);\n"
"    const float3 F = FresnelSchlick(max(dot(outDir, H), 0), F0);\n"
"    const float3 kD = (1 - F) * (1 - spe_threshold);\n"

"    const float3 speBRDF = SpecularBRDF(NDF, G, F, outDir, inDir, normal);\n"
"    const float spePDF = GGX_PDF(NDF, dotNH, dotVH);\n"
"    const float3 diffBRDF = DiffuseBRDF(difTexColor);\n"
"    const float diffPDF = CosinePDF(dotNL);\n"
"    const float3 sumBSDF = (diffBRDF * kD + speBRDF) * dotNL;\n"
"    const float sumPDF = diff_threshold * diffPDF + spe_threshold * spePDF;\n"

"    if (sumPDF <= 0){\n"
"        PDF = 1.0f;\n"
"        return float3(0, 0, 0);\n"
"    }\n"

"    PDF = sumPDF;\n"
"    return sumBSDF;\n"
"}\n"

///////////////////////////////////////////RefractionBTDF///////////////////////////////////////////
"float3 RefractionBTDF(float D, float G, float3 F, float3 V, float3 L, float3 N, float3 H, float in_eta, float out_eta)\n"
"{\n"
"	 const float dotNL = abs(dot(N, L));\n"
"	 const float dotNV = abs(dot(N, V));\n"
"	 const float dotHLnotAbs = dot(H, L);\n"
"	 const float dotHL = abs(dotHLnotAbs);\n"
"	 const float dotHVnotAbs = dot(H, V);\n"
"	 const float dotHV = abs(dotHVnotAbs);\n"

"	 const float a = dotHL * dotHV / (dotNL * dotNV);\n"
"	 const float3 b = out_eta * out_eta * (1 - F) * G * D / pow((in_eta * dotHLnotAbs + out_eta * dotHVnotAbs), 2);\n"
"	 return a * b;\n"
"}\n"

///////////////////////////////////////////RefSpeBSDF//////////////////////////////////////////////
"float3 RefSpeBSDF(float3 inDir, float3 outDir, float4 difTexColor, float3 N, float3 H, float in_eta, float out_eta, out float PDF)\n"
"{\n"
"    const float Alpha = difTexColor.w;\n"
"    const float speRatio = Alpha;\n" 

"    const float dotNL = abs(dot(N, inDir));\n"
"    const float dotNV = abs(dot(N, outDir));\n"
"    const float dotNH = abs(dot(N, H));\n"
"    const float dotVH = abs(dot(outDir, H));\n"
"    const float dotLH = abs(dot(inDir, H));\n"

"    const uint materialID = getMaterialID();\n"
"    const MaterialCB mcb = material[materialID];\n"
"    const float3 Speculer = mcb.Speculer.xyz;\n"
"    const float roughness = mcb.roughness;\n"

"    float3 F0 = 0.08.xxx * Speculer;\n"
"    float3 F = FresnelSchlick(max(dot(H, outDir), 0), F0);\n"

"    float NDF = GGX_Distribution(N, H, roughness);\n"
"    float G = GGX_GeometrySmith(N, outDir, inDir, roughness);\n"

"    float3 speBRDF = SpecularBRDF(NDF, G, F, outDir, inDir, N);\n"
"    float spePDF = GGX_PDF(NDF, dotNH, dotVH);\n"
"    float3 refrBTDF = RefractionBTDF(NDF, G, F, outDir, inDir, N, H, in_eta, out_eta);\n"
"    float refrPDF = GGX_PDF(NDF, dotNH, dotVH);\n"
"    const float3 sumBSDF = (speBRDF + refrBTDF * difTexColor.xyz) * dotNL;\n"
"    const float sumPDF = speRatio * spePDF + (1 - speRatio) * refrPDF;\n"

"    if (sumPDF <= 0){\n"
"        PDF = 1.0f;\n"
"        return float3(0, 0, 0);\n"
"    }\n"

"    PDF = sumPDF;\n"
"    return sumBSDF;\n"
"}\n";