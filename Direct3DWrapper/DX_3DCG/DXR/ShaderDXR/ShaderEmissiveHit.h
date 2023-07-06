///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                           ShaderEmissiveHit.hlsl                                      //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

char* ShaderEmissiveHit =

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
"       payload.color = difTex.xyz;\n"
"       if(InstanceID() != payload.instanceID || difTex.w <= 0.0f) {\n"
"          payload.reTry = true;\n"//�ڕW�̓_�����ȊO�̏ꍇ�f�ʂ�
"       }\n"
"    }\n"
//////���s����
"    pay_mNoF = materialIdent(payload.mNo, DIRECTIONLIGHT | METALLIC);\n"
"    if(pay_mNoF) {\n"
"       if(materialIdent(mNo, DIRECTIONLIGHT)) {\n"//���s���������}�e���A����?
"          payload.color = dLightColor.xyz;\n"
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
"          payload.color = float3(0.0f, 0.0f, 0.0f);\n"
"       }\n"
"       else {\n"
"          payload.reTry = true;\n"
"       }\n"
"    }\n"
"}\n";
