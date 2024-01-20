///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                           ShaderRayGen.hlsl                                           //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

char* ShaderRayGen =

//����
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

"    float3 col = payload.color;\n"
"    float3 colsatu;\n"
"    colsatu.x = saturate(col.x);\n"
"    colsatu.y = saturate(col.y);\n"
"    colsatu.z = saturate(col.z);\n"
"    gOutput[index] = float4(colsatu, 1.0f);\n"
"}\n"

//�ʏ�
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
"}\n";
