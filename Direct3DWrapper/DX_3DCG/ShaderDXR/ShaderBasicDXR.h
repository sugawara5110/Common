///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                           ShaderBasicDXR.hlsl                                         //
///////////////////////////////////////////////////////////////////////////////////////////////////////////
char* ShaderBasicDXR =
"RWTexture2D<float4> gOutput : register(u0, space0);\n"
"RaytracingAccelerationStructure gRtScene : register(t0, space0);\n"
"StructuredBuffer<uint> Indices[] : register(t1, space1);\n"//�������z��̏ꍇ,�ʂȃ��W�X�^��Ԃɂ�������(�E�́E)��!! �݂���
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
     //sRGB�ɕϊ�
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
//�ŏ��Ɏ��s�����V�F�[�_�[, �s�N�Z���̐������N��?
"[shader(\"raygeneration\")]\n"
"void rayGen()\n"
"{\n"
"    uint2 index = DispatchRaysIndex();\n"//���̊֐������s���Ă���s�N�Z�����W���擾
"    uint2 dim = DispatchRaysDimensions();\n"//��ʑS�̂̕��ƍ������擾
"    float2 screenPos = (index + 0.5f) / dim * 2.0f - 1.0f;\n"

"    RayDesc ray;\n"
     //�����̌��_
"    ray.Origin = cameraPosition.xyz;\n"
     //�����̕���
"    float4 target = mul(projectionInv, float4(screenPos.x, -screenPos.y, 1, 1));\n"
"    ray.Direction = mul(viewInv, float4(target.xyz, 0));\n"
     //�����̍ŏ��͈�
"    ray.TMin = 0.001;\n"
     //�����̍ő�͈�
"    ray.TMax = 10000;\n"

"    RayPayload payload;\n"
     //TraceRay(AccelerationStructure, 
     //         RAY_FLAG, 
     //         InstanceInclusionMask, 
     //         HitGroupIndex, �g�p����hitShader��Index
     //         MultiplierForGeometryContributionToShaderIndex, 
     //         MissShaderIndex, �g�p����missShader��Index
     //         RayDesc, 
     //         Payload);
     //���̊֐�����Ray���X�^�[�g����
     //payload�Ɋehit, miss �V�F�[�_�[�Ōv�Z���ꂽ�l���i�[�����
"    TraceRay(gRtScene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, 0xFF, 0, 0, 0, ray, payload);\n"
"    float3 col = linearToSrgb(payload.color);\n"
"    gOutput[index] = float4(col, 1);\n"
"}\n"
//**************************************rayGen_Shader*******************************************************************//

//�q�b�g�ʒu�擾
"float3 HitWorldPosition()\n"
"{\n"
     //     ���_           ���݂̃q�b�g�܂ł̋���      ����
"    return WorldRayOrigin() + RayTCurrent() * WorldRayDirection();\n"
"}\n"

//Hit�̏d�S���g�p���āA���_���������Ԃ��ꂽhit�ʒu�̑������擾
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
//�q�b�g���Ȃ������ꍇ�N�������
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
//�ڃx�N�g���v�Z
"    tan = GetTangent(normal, world[instanceID].m, cameraUp);\n"
//�@���e�N�X�`��
"    float4 Tnor = g_texNormal[instanceID].SampleLevel(g_samLinear, uv, 0.0);\n"
//�m�[�}���}�b�v�ł̖@���o��
"    return GetNormal(Tnor.xyz, tan.normal, tan.tangent);\n"
"}\n"

//**************************************camHit_Shader*******************************************************************//
//�q�b�g�����ꍇ�N�������
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

//UV�v�Z
"    float2 triangleUV = getCenterUV(vertexUVs, attr);\n"
//�@���̌v�Z
"    float3 triangleNormal = getCenterNormal(vertexNormals, attr);\n"
//�m�[�}���}�b�v����̖@���o��, world�ϊ�
"    float3 normalMap = getNormalMap(triangleNormal, triangleUV, instanceID);\n"
//�x�[�X�e�N�X�`��
"    float4 difTex = g_texDiffuse[instanceID].SampleLevel(g_samLinear, triangleUV, 0.0);\n"

"    float4 diffuseColor = CalculateDiffuseLighting(hitPosition, normalMap);\n"
"    float4 color = lightAmbientColor + diffuseColor;\n"//���g�̃e�N�X�`���̐F
"    color = difTex;\n"
"    MetallicPayload metallicPayload;\n"
"    if (!(RayFlags() & RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH))\n"
"    {\n"
"        RayDesc ray;\n"
//�q�b�g�����ʒu���X�^�[�g
"        ray.Origin = hitPosition;\n"
//�����x�N�g�� 
"        float3 eyeVec = WorldRayDirection();\n"
//���˃x�N�g��
"        float3 reflectVec = reflect(eyeVec, normalize(normalMap));\n"
"        ray.Direction = reflectVec;\n"//���˕�����Ray���΂�
"        ray.TMin = 0.01;\n"
"        ray.TMax = 100000;\n"
"        TraceRay(gRtScene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES |\n"
"            RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH, 0xFF, 1, 0, 1, ray, metallicPayload);\n"
"    }\n"
//metallicPayload.color�ɂ̓q�b�g�����W�I���g���̃e�N�X�`���̐F���i�[����Ă�
"    if (metallicPayload.hit) {\n"
"        payload.color = color.xyz * metallicPayload.color;\n"
"    }\n"
"    else {\n"
"        payload.color = color;\n"
"    }\n"
"}\n"
//**************************************camHit_Shader*******************************************************************//

//***********************************metallicHit_Shader*****************************************************************//
//camHit�����΂��ꂽRay���q�b�g�����ꍇ�N�������
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
//UV�v�Z
"    float2 triangleUV = getCenterUV(vertexUVs, attr);\n"
"    float4 difTex = g_texDiffuse[instanceID].SampleLevel(g_samLinear, triangleUV, 0.0);\n"
//�q�b�g�����ʒu�̃e�N�X�`���̐F��payload.color�i�[����
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