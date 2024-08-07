///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                           ShaderBasicHit.hlsl                                         //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

char* ShaderBasicHit =

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
"    payload.hit = false;\n"

//�����ւ̌���
"    if(traceMode == 0){\n"
"       difTex.xyz = EmissivePayloadCalculate(payload.RecursionCnt, payload.hitPosition, \n"
"                                             difTex.xyz, speTex, normalMap);\n"
"    }\n"
"    else{\n"
"       uint loop = 1;\n"
"       uint RandNum = LightArea_RandNum.y;\n"
"       if(payload.RecursionCnt == 1)loop = RandNum;\n"
"       float3 col = float3(0.0f, 0.0f, 0.0f);\n"
"       for(uint i = 0; i < loop; i++){\n"
"          if(traceMode == 1){\n"
"             col += EmissivePayloadCalculate_PathTracing(payload.RecursionCnt, payload.hitPosition, \n"
"                                                         difTex.xyz, speTex, normalMap);\n"
"          }else if(traceMode == 2){\n"
"             col += EmissivePayloadCalculate_NEE(payload.RecursionCnt, payload.hitPosition, \n"
"                                                 difTex.xyz, speTex, normalMap);\n"
"          }\n"
"       }\n"
"       difTex.xyz = col / (float)loop;\n"
"    }\n"

//�@���؂�ւ�
"    float3 r_eyeVec = -WorldRayDirection();\n"//�����ւ̃x�N�g��
"    float norDir = dot(r_eyeVec, normalMap);\n"
"    if(norDir < 0.0f)normalMap *= -1.0f;\n"

//�t���l���v�Z
"    float fresnel = saturate(dot(r_eyeVec, normalMap));\n"

//���˕����ւ̌���
"    difTex.xyz = MetallicPayloadCalculate(payload.RecursionCnt, payload.hitPosition, \n"
"                                          difTex.xyz, normalMap, payload.hitInstanceId, fresnel);\n"

//������
"    difTex.xyz = Translucent(payload.RecursionCnt, payload.hitPosition, \n"
"                             difTex, normalMap, fresnel);\n"

//�[�x�擾
"    payload.depth = getDepth(attr, v3);\n"
//�@���擾
"    payload.normal = normalMap;\n"

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
"    payload.depth = getDepth(attr, v3);\n"
//�@���擾
"    payload.normal = normalMap;\n"

"    payload.color = normalMap;\n"
"    payload.hit = true;\n"
"    payload.Alpha = difTex.w;\n"
"}\n";