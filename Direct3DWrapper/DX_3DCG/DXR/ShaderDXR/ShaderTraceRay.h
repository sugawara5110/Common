///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                      ShaderTraceRay.hlsl                                              //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

char* ShaderTraceRay =

///////////////////////�����֌������΂�, �q�b�g�����ꍇ���邳�����Z//////////////////////////
"float3 EmissivePayloadCalculate(in uint RecursionCnt, in float3 hitPosition, \n"
"                                in float3 difTexColor, in float3 speTexColor, in float3 normal)\n"
"{\n"
"    uint materialID = getMaterialID();\n"
"    MaterialCB mcb = material[materialID];\n"
"    uint mNo = mcb.materialNo;\n"
"    float3 ret = difTexColor;\n"//�����������ꍇ�A�f�荞�݂����̂܂܏o��E�E�E���ƂŕύX����

"    bool mf = materialIdent(mNo, EMISSIVE);\n"
"    if(!mf) {\n"//emissive�ȊO

"       RayPayload payload;\n"
"       payload.hit = false;\n"
"       LightOut emissiveColor = (LightOut)0;\n"
"       LightOut Out;\n"
"       RayDesc ray;\n"
"       payload.hitPosition = hitPosition;\n"

"       float3 SpeculerCol = mcb.Speculer.xyz;\n"
"       float3 Diffuse = mcb.Diffuse.xyz;\n"
"       float3 Ambient = mcb.Ambient.xyz + GlobalAmbientColor.xyz;\n"
"       float shininess = mcb.shininess;\n"

//�_�����v�Z
"       uint NumEmissive = numEmissive.x;\n"
"       for(uint i = 0; i < NumEmissive; i++) {\n"
"           if(emissivePosition[i].w == 1.0f) {\n"
"              float3 lightVec = normalize(emissivePosition[i].xyz - hitPosition);\n"

"              ray.Direction = lightVec;\n"
"              payload.mNo = EMISSIVE;\n"//��������p

"              payload.hitPosition = hitPosition;\n"

"              traceRay(RecursionCnt, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, 1, 1, ray, payload);\n"

"              float4 emissiveHitPos = emissivePosition[i];\n"
"              emissiveHitPos.xyz = payload.hitPosition;\n"

"              Out = PointLightCom(SpeculerCol, Diffuse, Ambient, normal, emissiveHitPos, \n"//ShaderCG���֐�
"                                  hitPosition, lightst[i], payload.color, cameraPosition.xyz, shininess);\n"

"              emissiveColor.Diffuse += Out.Diffuse;\n"
"              emissiveColor.Speculer += Out.Speculer;\n"
"           }\n"
"       }\n"
//���s�����v�Z
"       if(dLightst.x == 1.0f){\n"
"          payload.hitPosition = hitPosition;\n"
"          ray.Direction = -dDirection.xyz;\n"
"          payload.mNo = DIRECTIONLIGHT;\n"//��������p

"          traceRay(RecursionCnt, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, 1, 1, ray, payload);\n"

"          Out = DirectionalLightCom(SpeculerCol, Diffuse, Ambient, normal, dLightst, dDirection.xyz, \n"//ShaderCG���֐�
"                                    payload.color, hitPosition, cameraPosition.xyz, shininess);\n"

"          emissiveColor.Diffuse += Out.Diffuse;\n"
"          emissiveColor.Speculer += Out.Speculer;\n"
"       }\n"
//�Ō�Ƀe�N�X�`���̐F�Ɋ|�����킹
"       difTexColor *= emissiveColor.Diffuse;\n"
"       speTexColor *= emissiveColor.Speculer;\n"
"       ret = difTexColor + speTexColor;\n"
"    }\n"
"    return ret;\n"
"}\n"

///////////////////////���˕����֌������΂�, �q�b�g�����ꍇ�s�N�Z���l��Z///////////////////////
"float3 MetallicPayloadCalculate(in uint RecursionCnt, in float3 hitPosition, \n"
"                                in float3 difTexColor, in float3 normal, inout int hitInstanceId, in float fresnel)\n"
"{\n"
"    uint materialID = getMaterialID();\n"
"    uint mNo = material[materialID].materialNo;\n"
"    float3 ret = difTexColor;\n"

"    hitInstanceId = (int)getInstancingID(); \n"//���g��ID��������

"    if(materialIdent(mNo, METALLIC)) {\n"//METALLIC

"       RayPayload payload;\n"
"       payload.hit = false;\n"
"       RayDesc ray;\n"
//�����x�N�g�� 
"       float3 eyeVec = WorldRayDirection();\n"
//���˃x�N�g��
"       float3 reflectVec = reflect(eyeVec, normalize(normal));\n"
"       ray.Direction = reflectVec;\n"//���˕�����Ray���΂�
"       payload.hitPosition = hitPosition;\n"

"       traceRay(RecursionCnt, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, 0, 0, ray, payload);\n"

"       float3 outCol = float3(0.0f, 0.0f, 0.0f);\n"
"       if (payload.hit) {\n"
"           float3 refCol = payload.color * (1.0f - fresnel);\n"

"           outCol = difTexColor * refCol;\n"//�q�b�g�����ꍇ�f�荞�݂Ƃ��ď�Z
"           hitInstanceId = payload.hitInstanceId;\n"//�q�b�g����ID��������
"           int hitmNo = payload.mNo;\n"
"           if(materialIdent(hitmNo, EMISSIVE)){\n"
"              outCol = refCol;\n"
"           }\n"
"       }\n"
"       else {\n"
"           outCol = difTexColor;\n"//�q�b�g���Ȃ������ꍇ�f�荞�ݖ����Ō��̃s�N�Z����������
"       }\n"
"       ret = outCol;\n"
"    }\n"
"    return ret;\n"
"}\n"

////////////////////////////////////////������//////////////////////////////////////////
"float3 Translucent(in uint RecursionCnt, in float3 hitPosition, in float4 difTexColor, in float3 normal, in float fresnel)\n"
"{\n"
"    uint materialID = getMaterialID();\n"
"    MaterialCB mcb = material[materialID];\n"
"    uint mNo = mcb.materialNo;\n"
"    float3 ret = difTexColor.xyz;\n"
"    float Alpha = difTexColor.w;\n"

"    if(materialIdent(mNo, TRANSLUCENCE) && Alpha < 1.0f) {\n"

"       float Alpha = difTexColor.w;\n"
"       RayPayload payload;\n"
"       RayDesc ray; \n"
//�����x�N�g�� 
"       float3 eyeVec = WorldRayDirection();\n"
"       ray.Direction = refract(eyeVec, normalize(normal), mcb.RefractiveIndex);\n"
"       payload.hitPosition = hitPosition;\n"

"       traceRay(RecursionCnt, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, 0, 0, ray, payload);\n"

//�A���t�@�l�̔䗦�Ō��̐F�ƌ����Փː�̐F��z��
"       ret = payload.color * fresnel * (1.0f - Alpha) + difTexColor * Alpha;\n"
"    }\n"
"    return ret;\n"
"}\n";