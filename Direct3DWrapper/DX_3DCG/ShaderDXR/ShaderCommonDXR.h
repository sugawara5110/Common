///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                      ShaderCommonDXR.hlsl                                             //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

char* ShaderCommonDXR =

/////////////////////////////////////RGB �� sRGB�ɕϊ�/////////////////////////////////////////////
"float3 linearToSrgb(float3 c)\n"
"{\n"
"    float3 sq1 = sqrt(c);\n"
"    float3 sq2 = sqrt(sq1);\n"
"    float3 sq3 = sqrt(sq2);\n"
"    float3 srgb = 0.662002687 * sq1 + 0.684122060 * sq2 - 0.323583601 * sq3 - 0.0225411470 * c;\n"
"    return srgb;\n"
"}\n"

///////////////////////////////////////�q�b�g�ʒu�擾/////////////////////////////////////////////
"float3 HitWorldPosition()\n"
"{\n"
//     ���_           ���݂̃q�b�g�܂ł̋���      ����
"    return WorldRayOrigin() + RayTCurrent() * WorldRayDirection();\n"
"}\n"

////////////////Hit�̏d�S���g�p���āA���_���������Ԃ��ꂽhit�ʒu�̑������擾(�@��)///////////////
"float3 getCenterNormal(float3 vertexNormal[3], BuiltInTriangleIntersectionAttributes attr)\n"
"{\n"
"    return vertexNormal[0] +\n"
"        attr.barycentrics.x * (vertexNormal[1] - vertexNormal[0]) +\n"
"        attr.barycentrics.y * (vertexNormal[2] - vertexNormal[0]);\n"
"}\n"

////////////////////////////////�@���擾//////////////////////////////////////////////////////////
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

////////////////Hit�̏d�S���g�p���āA���_���������Ԃ��ꂽhit�ʒu�̑������擾(UV)////////////////
"float2 getCenterUV(float2 vertexUV[3], BuiltInTriangleIntersectionAttributes attr)\n"
"{\n"
"    return vertexUV[0] +\n"
"        attr.barycentrics.x * (vertexUV[1] - vertexUV[0]) +\n"
"        attr.barycentrics.y * (vertexUV[2] - vertexUV[0]);\n"
"}\n"

////////////////////////////////UV�擾//////////////////////////////////////////////////////////
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

/////////////////////////////�m�[�}���e�N�X�`������@���擾/////////////////////////////////////
"float3 getNormalMap(float3 normal, float2 uv, uint instanceID)\n"
"{\n"
"    NormalTangent tan;\n"
//�ڃx�N�g���v�Z
"    tan = GetTangent(normal, instance[instanceID].world, cameraUp);\n"//ShaderCG���֐�
//�@���e�N�X�`��
"    float4 Tnor = g_texNormal[instanceID].SampleLevel(g_samLinear, uv, 0.0);\n"
//�m�[�}���}�b�v�ł̖@���o��
"    return GetNormal(Tnor.xyz, tan.normal, tan.tangent);\n"//ShaderCG���֐�
"}\n"

///////////////////////�����֌������΂�, �q�b�g�����ꍇ���邳�����Z//////////////////////////
"float3 EmissivePayloadCalculate(in float3 hitPosition, in float3 difTexColor, float3 speTexColor, float3 normal)\n"
"{\n"
"    RayPayload emissivePayload;\n"
"    uint instanceID = InstanceID();\n"
"    LightOut emissiveColor = (LightOut)0;\n"
"    LightOut Out;\n"
"    RayDesc ray;\n"
//�q�b�g�����ʒu���X�^�[�g
"    ray.Origin = hitPosition;\n"
"    ray.TMin = 0.01;\n"
"    ray.TMax = 100000;\n"
"    for(uint i = 0; i < numEmissive.x; i++) {\n"
"        float3 lightVec = normalize(emissivePosition[i].xyz - hitPosition);\n"
"        ray.Direction = lightVec;\n"
"        TraceRay(gRtScene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES |\n"
"            RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH, 0xFF, 2, 0, 2, ray, emissivePayload);\n"

//�����v�Z
"        float3 SpeculerCol = material[instanceID].Speculer.xyz;\n"
"        float3 Diffuse = material[instanceID].Diffuse.xyz;\n"
"        float3 Ambient = material[instanceID].Ambient.xyz;\n"
"        float shininess = material[instanceID].shininess;\n"
"        Out = PointLightCom(SpeculerCol, Diffuse, Ambient, normal, emissivePosition[i], \n"//ShaderCG���֐�
"                            hitPosition, lightst[i], emissivePayload.color, cameraPosition.xyz, shininess);\n"

"        emissiveColor.Diffuse += Out.Diffuse;\n"
"        emissiveColor.Speculer += Out.Speculer;\n"
"    }\n"

"    difTexColor *= emissiveColor.Diffuse;\n"
"    speTexColor *= emissiveColor.Speculer;\n"

"    return difTexColor + speTexColor;\n"
"}\n"

///////////////////////���˕����֌������΂�, �q�b�g�����ꍇ�s�N�Z���l��Z///////////////////////
"float3 MetallicPayloadCalculate(in float3 hitPosition, in float3 difTexColor, float3 normal)\n"
"{\n"
"    RayPayload payload;\n"
"    if (!(RayFlags() & RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH))\n"
"    {\n"
"        RayDesc ray;\n"
//�q�b�g�����ʒu���X�^�[�g
"        ray.Origin = hitPosition;\n"
//�����x�N�g�� 
"        float3 eyeVec = WorldRayDirection();\n"
//���˃x�N�g��
"        float3 reflectVec = reflect(eyeVec, normalize(normal));\n"
"        ray.Direction = reflectVec;\n"//���˕�����Ray���΂�
"        ray.TMin = 0.01;\n"
"        ray.TMax = 100000;\n"
"        TraceRay(gRtScene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES |\n"
"            RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH, 0xFF, 1, 0, 1, ray, payload);\n"
"    }\n"
"    float3 outCol = float3(0.0f, 0.0f, 0.0f);\n"
"    if (payload.hit && payload.Alpha >= 1.0f) {\n"
"        outCol = difTexColor * payload.color;\n"//�q�b�g�����ꍇ�f�荞�݂Ƃ��ď�Z
"    }\n"
"    else {\n"
"        outCol = difTexColor;\n"//�q�b�g���Ȃ������ꍇ�f�荞�ݖ����Ō��̃s�N�Z����������
"    }\n"

"    return outCol;\n"
"}\n"

////////////////////////////////////�A���t�@�e�X�g//////////////////////////////////////
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
//�A���t�@�l�̔䗦�Ō��̐F�ƌ����Փː�̐F��z��
"    return payload.color * (1.0f - Alpha) + difTexColor * Alpha;\n"
"}\n";