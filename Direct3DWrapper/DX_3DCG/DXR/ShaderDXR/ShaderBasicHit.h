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
"}\n";