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
"    bool loop = true;\n"
"    payload.hitPosition = ray.Origin;\n"
"    while(loop){\n"
"       ray.Origin = payload.hitPosition;\n"
"       TraceRay(gRtScene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, 0xFF, 0, 0, 0, ray, payload);\n"
"       loop = payload.reTry;\n"
"    }\n"
"    float3 col = linearToSrgb(payload.color);\n"
"    gOutput[index] = float4(col, 1);\n"
"}\n"
//**************************************rayGen_Shader*******************************************************************//

//**************************************basicMiss_Shader*******************************************************************//
"[shader(\"miss\")]\n"
"void basicMiss(inout RayPayload payload)\n"
"{\n"
"    payload.color = float3(0.0, 0.0, 0.0);\n"
"    payload.hit = false;\n"
"    payload.reTry = false;\n"
"}\n"
//**************************************basicMiss_Shader*******************************************************************//

//**************************************basicHit_Shader*******************************************************************//
"[shader(\"closesthit\")]\n"
"void basicHit(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attr)\n"
"{\n"
"    payload.hitPosition = HitWorldPosition();\n"
//�e�N�X�`���擾
"    float4 difTex = getDifPixel(attr);\n"
"    float3 normalMap = getNorPixel(attr);\n"
"    float3 speTex = getSpePixel(attr);\n"

"    payload.reTry = true;\n"
"    if(AlphaTestSw(difTex.w)) {\n"
"       payload.reTry = false;\n"
//�����ւ̌���
"       difTex.xyz = EmissivePayloadCalculate(payload.RecursionCnt, payload.hitPosition, difTex.xyz, speTex, normalMap);\n"
//���˕����ւ̌���
"       difTex.xyz = MetallicPayloadCalculate(payload.RecursionCnt, payload.hitPosition, difTex.xyz, normalMap);\n"
//�A���t�@�e�X�g
"       difTex.xyz = AlphaTest(payload.RecursionCnt, payload.hitPosition, difTex);\n"
"    }\n"
"    payload.color = difTex.xyz;\n"
"    payload.hit = true;\n"
"    payload.Alpha = difTex.w;\n"
"}\n"
//**************************************basicHit_Shader*******************************************************************//

//***********************************emissiveHit_Shader*****************************************************************//
"[shader(\"closesthit\")]\n"
"void emissiveHit(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attr)\n"
"{\n"
"    uint materialID = getMaterialID();\n"
"    uint mNo = material[materialID].materialNo;\n"
"    float4 difTex = getDifPixel(attr);\n"
"    payload.hitPosition = HitWorldPosition();\n"
"    payload.reTry = false;\n"
//�q�b�g�����ʒu�̃e�N�X�`���̐F��payload.color�i�[����
//////�_����
"    if(payload.mNo == 2 && mNo == 2) {\n"
"       payload.color = difTex.xyz;\n"
"       if(InstanceID() != payload.instanceID) {\n"//�ڕW�̓_������?
"          payload.reTry = true;\n"//�ڕW�̓_�����ȊO�̏ꍇ�f�ʂ�
"       }\n"
"    }\n"
//////���s����
"    if(payload.mNo == 3) {\n"
"       if(mNo > 2) {\n"//���s���������}�e���A����?
"          payload.color = dLightColor.xyz;\n"
"       }\n"
"       if(mNo == 2) {\n"//�_�����̏ꍇ�f�ʂ�
"          payload.reTry = true;\n"
"       }\n"
"    }\n"
//////�e
"    if(mNo != 2 && payload.mNo == 2 || mNo < 2 && payload.mNo == 3) {\n"
"       if(difTex.w >= 1.0f) {\n"
"          payload.color = GlobalAmbientColor.xyz;\n"
"       }\n"
"       else {\n"
"          payload.reTry = true;\n"
"       }\n"
"    }\n"
"}\n"
//***********************************emissiveHit_Shader*****************************************************************//

//***********************************emissiveMiss_Shader****************************************************************//
"[shader(\"miss\")]\n"
"void emissiveMiss(inout RayPayload payload)\n"
"{\n"
"    payload.color = GlobalAmbientColor.xyz;\n"
"    payload.reTry = false;\n"
"}\n";
//***********************************emissiveMiss_Shader****************************************************************//