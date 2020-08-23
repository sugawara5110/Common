///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                           ShaderBasicDXR.hlsl                                         //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

char* ShaderBasicDXR =

//**************************************rayGen_Shader*******************************************************************//
//�ŏ��Ɏ��s�����V�F�[�_�[, �s�N�Z���̐������N��?
"[shader(\"raygeneration\")]\n"
"void rayGen()\n"
"{\n"
"    uint2 index = DispatchRaysIndex();\n"//���̊֐������s���Ă���s�N�Z�����W���擾
"    uint2 dim = DispatchRaysDimensions();\n"//��ʑS�̂̕��ƍ������擾
"    float2 screenPos = (index + 0.5f) / dim * 2.0f - 1.0f;\n"

"    RayDesc ray;\n"
     //�����̌��_, �����������X�^�[�g, ��������n�܂�
"    ray.Origin = cameraPosition.xyz;\n"
     //�����̕���
"    float4 world = mul(float4(screenPos.x, -screenPos.y, 0, 1), projectionToWorld);\n"
"    world.xyz /= world.w;\n"
"    ray.Direction = normalize(world.xyz - ray.Origin);\n"
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

//**************************************camMiss_Shader*******************************************************************//
//�������q�b�g���Ȃ������ꍇ�N�������
"[shader(\"miss\")]\n"
"void camMiss(inout RayPayload payload)\n"
"{\n"
"    payload.color = float3(0.0, 0.0, 0.0);\n"
"}\n"
//**************************************camMiss_Shader*******************************************************************//

//**************************************camHit_Shader*******************************************************************//
//�������q�b�g�����ꍇ�N�������
"[shader(\"closesthit\")]\n"
"void camHit(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attr)\n"
"{\n"
"    float3 hitPosition = HitWorldPosition();\n"
"    uint materialID = getMaterialID();\n"
"    uint mNo = material[materialID].materialNo;\n"
//UV�v�Z
"    float2 triangleUV0 = getUV(attr, 0);\n"
"    float2 triangleUV1 = getUV(attr, 1);\n"
//�@���̌v�Z
"    float3 triangleNormal = getNormal(attr);\n"
//�m�[�}���}�b�v����̖@���o��
"    float3 normalMap = getNormalMap(triangleNormal, triangleUV0);\n"
//�e�N�X�`��
"    float4 difTex = g_texDiffuse[materialID].SampleLevel(g_samLinear, triangleUV0, 0.0);\n"
"    float4 speTex = g_texSpecular[materialID].SampleLevel(g_samLinear, triangleUV1, 0.0);\n"

"    if(mNo == 0) {\n"//�}�e���A�������^���b�N�̏ꍇ
//�����ւ̌���
"       difTex.xyz = EmissivePayloadCalculate(hitPosition, difTex.xyz, speTex.xyz, normalMap);\n"
//���˕����ւ̌���
"       difTex.xyz = MetallicPayloadCalculate(hitPosition, difTex.xyz, normalMap);\n"
"    }\n"
//�A���t�@�e�X�g
"    if(material[materialID].alphaTest == 1.0f) {\n"
"       difTex.xyz = AlphaTest(hitPosition, difTex.xyz, difTex.w, 0);\n"
"    }\n"
"    payload.color = difTex.xyz;\n"
"}\n"
//**************************************camHit_Shader*******************************************************************//

//***********************************metallicHit_Shader*****************************************************************//
//camHit�����΂��ꂽ�������q�b�g�����ꍇ�N�������
"[shader(\"closesthit\")]\n"
"void metallicHit(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attr)\n"
"{\n"
"    uint materialID = getMaterialID();\n"
"    float3 hitPosition = HitWorldPosition();\n"
//UV�v�Z
"    float2 triangleUV0 = getUV(attr, 0);\n"
"    float2 triangleUV1 = getUV(attr, 1);\n"
//�@���̌v�Z
"    float3 triangleNormal = getNormal(attr);\n"
//�m�[�}���}�b�v����̖@���o��
"    float3 normalMap = getNormalMap(triangleNormal, triangleUV0);\n"
//�q�b�g�����ʒu�̃e�N�X�`���̐F��payload.color�i�[����
"    float4 difTex = g_texDiffuse[materialID].SampleLevel(g_samLinear, triangleUV0, 0.0);\n"
"    float4 speTex = g_texSpecular[materialID].SampleLevel(g_samLinear, triangleUV1, 0.0);\n"
//�����ւ̌���
"    difTex.xyz = EmissivePayloadCalculate(hitPosition, difTex.xyz, speTex.xyz, normalMap);\n"
//�A���t�@�e�X�g
"    if(material[materialID].alphaTest == 1.0f) {\n"
"       difTex.xyz = AlphaTest(hitPosition, difTex.xyz, difTex.w, 1);\n"
"    }\n"
"    payload.color = difTex.xyz;\n"
"    payload.hit = true;\n"
"    payload.Alpha = difTex.w;\n"
"}\n"
//***********************************metallicHit_Shader*****************************************************************//

//***********************************metallicMiss_Shader****************************************************************//
//camHit�����΂��ꂽ�������q�b�g���Ȃ������ꍇ�N�������
"[shader(\"miss\")]\n"
"void metallicMiss(inout RayPayload payload)\n"
"{\n"
"    payload.hit = false;\n"
"}\n"
//***********************************metallicMiss_Shader****************************************************************//

//***********************************emissiveHit_Shader*****************************************************************//
//camHit�����΂��ꂽ�����ւ̌������q�b�g�����ꍇ�N�������
"[shader(\"closesthit\")]\n"
"void emissiveHit(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attr)\n"
"{\n"
"    uint materialID = getMaterialID();\n"
"    uint mNo = material[materialID].materialNo;\n"

"    if(mNo == 1) {\n"//�}�e���A�����G�~�b�V�u�̏ꍇ�̂ݏ���
//UV�v�Z
"       float2 triangleUV0 = getUV(attr, 0);\n"
"       float4 difTex = g_texDiffuse[materialID].SampleLevel(g_samLinear, triangleUV0, 0.0);\n"
//�q�b�g�����ʒu�̃e�N�X�`���̐F��payload.color�i�[����
"       payload.color = difTex.xyz;\n"
"       payload.hit = true;\n"
"    }\n"
"    else {\n"//�G�~�b�V�u�ł͂Ȃ������ꍇ�e�ɂȂ�
"       payload.color = GlobalAmbientColor.xyz;\n"
"       payload.hit = false;\n"
"    }\n"
"}\n"
//***********************************emissiveHit_Shader*****************************************************************//

//***********************************emissiveMiss_Shader****************************************************************//
//camHit�����΂��ꂽ�����ւ̌������q�b�g���Ȃ������ꍇ�N�������
"[shader(\"miss\")]\n"
"void emissiveMiss(inout RayPayload payload)\n"
"{\n"
"    payload.color = GlobalAmbientColor.xyz;\n"
"    payload.hit = false;\n"
"}\n";
//***********************************emissiveMiss_Shader****************************************************************//