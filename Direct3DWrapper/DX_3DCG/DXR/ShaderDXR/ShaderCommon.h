///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                      ShaderCommon.hlsl                                                //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

char* ShaderCommon =

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
"}\n";