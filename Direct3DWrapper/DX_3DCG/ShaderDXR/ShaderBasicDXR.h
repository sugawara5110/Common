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
"    payload.RecursionCnt = 1;\n"
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
//�e�N�X�`���擾
"    float4 difTex = getDifPixel(attr);\n"
"    float3 normalMap = getNorPixel(attr);\n"
"    float3 speTex = getSpePixel(attr);\n"

"    float4 add = material[materialID].AddObjColor;\n"
"    difTex.x = saturate(difTex.x + add.x);\n"
"    difTex.y = saturate(difTex.y + add.y);\n"
"    difTex.z = saturate(difTex.z + add.z);\n"
"    difTex.w = saturate(difTex.w + add.w);\n"

//�����ւ̌���(emissive�ȊO)
"    if(mNo != 1)difTex.xyz = EmissivePayloadCalculate(payload.RecursionCnt, hitPosition, difTex.xyz, speTex, normalMap);\n"
//���˕����ւ̌���(metallic�̂�)
"    if(mNo == 0)difTex.xyz = MetallicPayloadCalculate(payload.RecursionCnt, hitPosition, difTex.xyz, normalMap);\n"

//�A���t�@�e�X�g
"    if(material[materialID].alphaTest == 1.0f) {\n"
"       difTex.xyz = AlphaTest(payload.RecursionCnt, hitPosition, difTex.xyz, difTex.w, 0);\n"
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
"    uint mNo = material[materialID].materialNo;\n"
//�e�N�X�`���擾
"    float4 difTex = getDifPixel(attr);\n"
"    float3 normalMap = getNorPixel(attr);\n"
"    float3 speTex = getSpePixel(attr);\n"

//�����ւ̌���(emissive�ȊO)
"    if (mNo != 1)difTex.xyz = EmissivePayloadCalculate(payload.RecursionCnt, hitPosition, difTex.xyz, speTex.xyz, normalMap);\n"
//�A���t�@�e�X�g
"    if(material[materialID].alphaTest == 1.0f) {\n"
"       difTex.xyz = AlphaTest(payload.RecursionCnt, hitPosition, difTex.xyz, difTex.w, 1);\n"
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
"void emissiveHit(inout RayPayloadEmissive payload, in BuiltInTriangleIntersectionAttributes attr)\n"
"{\n"
"    uint materialID = getMaterialID();\n"
"    uint mNo = material[materialID].materialNo;\n"

"    payload.reTry = false;\n"
"    if(mNo == 1) {\n"//(emissive�̂�)
"       float4 difTex = getDifPixel(attr);\n"
//�q�b�g�����ʒu�̃e�N�X�`���̐F��payload.color�i�[����
"       payload.hitPosition = HitWorldPosition();\n"
"       payload.color = difTex.xyz;\n"
"       if(InstanceID() != payload.instanceID) {\n"
"          payload.reTry = true;\n"
"       }\n"
"    }\n"
"    else {\n"//�G�~�b�V�u�ł͂Ȃ������ꍇ�e�ɂȂ�
"       payload.color = GlobalAmbientColor.xyz;\n"
"    }\n"
"}\n"
//***********************************emissiveHit_Shader*****************************************************************//

//***********************************emissiveMiss_Shader****************************************************************//
//camHit�����΂��ꂽ�����ւ̌������q�b�g���Ȃ������ꍇ�N�������
"[shader(\"miss\")]\n"
"void emissiveMiss(inout RayPayloadEmissive payload)\n"
"{\n"
"    payload.color = GlobalAmbientColor.xyz;\n"
"    payload.reTry = false;\n"
"}\n";
//***********************************emissiveMiss_Shader****************************************************************//