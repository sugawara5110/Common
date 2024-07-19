///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                           ShaderEmissiveHit.hlsl                                      //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

char* ShaderEmissiveHit =

"uint getEmissiveIndex(){\n"
"   uint ret = 0;\n"
"   for(uint i = 0; i < 256; i++){\n"
"      if(emissiveNo[i].x == InstanceID()){\n"
"         ret = i;\n"
"         break;\n"
"      }\n"
"   }\n"
"   return ret;\n"
"}\n"

"[shader(\"closesthit\")]\n"
"void emissiveHit(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attr)\n"
"{\n"
"    uint materialID = getMaterialID();\n"
"    uint mNo = material[materialID].materialNo;\n"
"    Vertex3 v3 = getVertex();\n"
"    float4 difTex = getDifPixel(attr, v3);\n"
"    float3 normalMap = getNorPixel(attr, v3);\n"
"    float3 speTex = getSpePixel(attr, v3);\n"
"    payload.hitPosition = HitWorldPosition();\n"
"    payload.hit = false;\n"
"    payload.reTry = false;\n"
//�q�b�g�����ʒu�̃e�N�X�`���̐F��payload.color�i�[����
//////�_����
"    bool pay_mNoF = materialIdent(payload.mNo, EMISSIVE);\n"
"    bool mNoF     = materialIdent(mNo, EMISSIVE);\n"
"    if(pay_mNoF && mNoF) {\n"
"       payload.color = difTex.xyz;\n"
"       payload.EmissiveIndex = getEmissiveIndex();\n"
"       if(difTex.w <= 0.0f) {\n"
"          payload.reTry = true;\n"//�����̏ꍇ�f�ʂ�
"       }\n"
"       else{\n"
"          payload.hit = true;\n"
"       }\n"
"       return;\n"
"    }\n"
//////���s����
"    pay_mNoF = materialIdent(payload.mNo, DIRECTIONLIGHT);\n"
"    if(pay_mNoF) {\n"
"       if(materialIdent(mNo, DIRECTIONLIGHT)) {\n"//���s���������}�e���A����?
"          payload.color = dLightColor.xyz;\n"
"       }\n"
"       if(materialIdent(mNo, EMISSIVE)) {\n"//�_�����̏ꍇ�f�ʂ�
"          payload.reTry = true;\n"
"       }\n"
"    }\n"
//////�����ӊO
"    if(difTex.w >= 1.0f) {\n"
"       uint RandNum = LightArea_RandNum.y;\n"
"       if(RandNum > 1){\n"
//�����ւ̌���
"          payload.color = EmissivePayloadCalculate(payload.RecursionCnt, payload.hitPosition, \n"
"                                                   difTex.xyz, speTex, normalMap);\n"
"       }\n"
"       else{\n"
"          payload.color = float3(0.0f, 0.0f, 0.0f);\n"
"       }\n"
"    }\n"
"    else {\n"
"       payload.reTry = true;\n"
"    }\n"
"}\n";
