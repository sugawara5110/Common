///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                           ShaderBasicDXR.hlsl                                         //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

char* ShaderBasicDXR =

//**************************************rayGen_Shader*******************************************************************//
"void rayGenIn()\n"
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
"    ray.TMin = TMin_TMax.x;\n"
//�����̍ő�͈�
"    ray.TMax = TMin_TMax.y;\n"

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
"    gDepthOut[index] = 1.0f;\n"
"    payload.depth = -1.0f;\n"
"    payload.reTry = false;\n"
"    payload.hitInstanceId = -1;\n"

"    while(loop){\n"
"       ray.Origin = payload.hitPosition;\n"
"       TraceRay(gRtScene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, 0xFF, 0, 0, 0, ray, payload);\n"
"       loop = payload.reTry;\n"
"    }\n"

"    if(payload.depth != -1.0f)"
"       gDepthOut[index] = payload.depth;\n"

"    gInstanceIdMap[index] = payload.hitInstanceId;\n"

"    float3 col = linearToSrgb(payload.color);\n"
"    float3 colsatu;\n"
"    colsatu.x = saturate(col.x);\n"
"    colsatu.y = saturate(col.y);\n"
"    colsatu.z = saturate(col.z);\n"
"    gOutput[index] = float4(colsatu, 1.0f);\n"
"}\n"
//�ŏ��Ɏ��s�����V�F�[�_�[, �s�N�Z���̐������N��
"[shader(\"raygeneration\")]\n"
"void rayGen()\n"
"{\n"
"    rayGenIn();\n"
"}\n"
//InstanceIdMap�e�X�g�p
"[shader(\"raygeneration\")]\n"
"void rayGen_InstanceIdMapTest()\n"
"{\n"
"    rayGenIn();\n"
"    uint2 index = DispatchRaysIndex(); \n"
"    int id = gInstanceIdMap[index];\n"
"    int numInstance = numEmissive.y;\n"
"    float3 ret = float3(0.0f, 0.0f, 0.0f);\n"

"    int step = 4096 / numInstance;\n"
"    int color3 = step;\n"

"    for(int i = 0; i < numInstance; i++){\n"
"       int r = color3 >> 8;\n"
"       int g = (color3 >> 4) & 0x00f;\n"
"       int b = color3 & 0x00f;\n"
"       float rf = float(r) / 15.0f;\n"
"       float gf = float(g) / 15.0f;\n"
"       float bf = float(b) / 15.0f;\n"
"       if(id == i){\n"
"         ret = float3(rf, gf, bf);\n"
"         break;\n"
"       }\n"
"       color3 += step;\n"
"    }\n"

"    gOutput[index] = float4(ret, 1.0f);\n"
"}\n"
//**************************************rayGen_Shader*******************************************************************//

//*************************************anyBasicHit_Shader***************************************************************//
"[shader(\"anyhit\")]\n"
"void anyBasicHit(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attr)\n"
"{\n"
//�e�N�X�`���擾
"    Vertex3 v3 = getVertex();\n"
"    float4 difTex = getDifPixel(attr, v3);\n"
"    float Alpha = difTex.w;\n"

"    if (Alpha <= 0.0f)\n"
"    {\n"
"        IgnoreHit();\n"
"    }\n"
"}\n"
//*************************************anyBasicHit_Shader***************************************************************//

//**************************************basicMiss_Shader****************************************************************//
"[shader(\"miss\")]\n"
"void basicMiss(inout RayPayload payload)\n"
"{\n"
"    payload.color = float3(0.0, 0.0, 0.0);\n"
"    payload.hit = false;\n"
"    payload.reTry = false;\n"
"}\n"
//**************************************basicMiss_Shader****************************************************************//

//**************************************basicHit_Shader*****************************************************************//
//�ʏ�
"[shader(\"closesthit\")]\n"
"void basicHit(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attr)\n"
"{\n"
"    payload.hitPosition = HitWorldPosition();\n"
"    Vertex3 v3 = getVertex();\n"
//�e�N�X�`���擾
"    float4 difTex = getDifPixel(attr, v3);\n"
"    float3 normalMap = getNorPixel(attr, v3);\n"
"    float3 speTex = getSpePixel(attr, v3);\n"

"    payload.reTry = false;\n"
//�[�x�擾
"    if(payload.depth == -1.0f) {\n"
"       payload.depth = getDepth(attr, v3);\n"
"    }\n"
//�����ւ̌���
"    difTex.xyz = EmissivePayloadCalculate(payload.RecursionCnt, payload.hitPosition, difTex.xyz, speTex, normalMap);\n"
//���˕����ւ̌���
"    difTex.xyz = MetallicPayloadCalculate(payload.RecursionCnt, payload.hitPosition, difTex.xyz, normalMap, payload.hitInstanceId);\n"
//������
"    difTex.xyz = Translucent(payload.RecursionCnt, payload.hitPosition, difTex, normalMap);\n"
//�A���t�@�u�����h
"    difTex.xyz = AlphaBlend(payload.RecursionCnt, payload.hitPosition, difTex);\n"

"    payload.color = difTex.xyz;\n"
"    payload.hit = true;\n"
"    payload.Alpha = difTex.w;\n"
"}\n"
//�@���}�b�v�e�X�g�p
"[shader(\"closesthit\")]\n"
"void basicHit_normalMapTest(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attr)\n"
"{\n"
"    payload.hitPosition = HitWorldPosition();\n"
"    Vertex3 v3 = getVertex();\n"
//�e�N�X�`���擾
"    float4 difTex = getDifPixel(attr, v3);\n"
"    float3 normalMap = getNorPixel(attr, v3);\n"

"    payload.reTry = false;\n"
//�[�x�擾
"    if(payload.depth == -1.0f) {\n"
"       payload.depth = getDepth(attr, v3);\n"
"    }\n"

"    payload.color = normalMap.xyz;\n"
"    payload.hit = true;\n"
"    payload.Alpha = difTex.w;\n"
"}\n"
//**************************************basicHit_Shader*****************************************************************//

//***********************************emissiveHit_Shader*****************************************************************//
"[shader(\"closesthit\")]\n"
"void emissiveHit(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attr)\n"
"{\n"
"    uint materialID = getMaterialID();\n"
"    uint mNo = material[materialID].materialNo;\n"
"    Vertex3 v3 = getVertex();\n"
"    float4 difTex = getDifPixel(attr, v3);\n"
"    payload.hitPosition = HitWorldPosition();\n"
"    payload.reTry = false;\n"
//�q�b�g�����ʒu�̃e�N�X�`���̐F��payload.color�i�[����
//////�_����
"    bool pay_mNoF = materialIdent(payload.mNo, EMISSIVE);\n"
"    bool mNoF     = materialIdent(mNo, EMISSIVE);\n"
"    if(pay_mNoF && mNoF) {\n"
"       payload.color = difTex.xyz + GlobalAmbientColor.xyz;\n"
"       if(InstanceID() != payload.instanceID || difTex.w <= 0.0f) {\n"
"          payload.reTry = true;\n"//�ڕW�̓_�����ȊO�̏ꍇ�f�ʂ�
"       }\n"
"    }\n"
//////���s����
"    pay_mNoF = materialIdent(payload.mNo, DIRECTIONLIGHT | METALLIC);\n"
"    if(pay_mNoF) {\n"
"       if(materialIdent(mNo, DIRECTIONLIGHT)) {\n"//���s���������}�e���A����?
"          payload.color = dLightColor.xyz + GlobalAmbientColor.xyz;\n"
"       }\n"
"       if(materialIdent(mNo, EMISSIVE)) {\n"//�_�����̏ꍇ�f�ʂ�
"          payload.reTry = true;\n"
"       }\n"
"    }\n"
//////�e
"    if( \n"
"       !materialIdent(mNo, EMISSIVE) && \n" 
"       materialIdent(payload.mNo, EMISSIVE) || \n" 
"       (mNo == METALLIC || mNo == NONREFLECTION) && \n"
"       materialIdent(payload.mNo, DIRECTIONLIGHT | METALLIC) \n"
"       ) {\n"
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