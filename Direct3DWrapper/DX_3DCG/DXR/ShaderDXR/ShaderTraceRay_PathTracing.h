///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                      ShaderTraceRay_PathTracing.hlsl                                  //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

char* ShaderTraceRay_PathTracing =

///////////////////////�����֌������΂�, �q�b�g�����ꍇ���邳�����Z//////////////////////////
"float3 EmissivePayloadCalculate_PathTracing(in uint RecursionCnt, in float3 hitPosition, \n"
"                                            in float3 difTexColor, in float3 speTexColor, in float3 normal)\n"
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
"       ray.TMin = TMin_TMax.x;\n"
"       ray.TMax = TMin_TMax.y;\n"
"       RecursionCnt++;\n"
"       payload.RecursionCnt = RecursionCnt;\n"

"       float3 SpeculerCol = mcb.Speculer.xyz;\n"
"       float3 Diffuse = mcb.Diffuse.xyz;\n"
"       float3 Ambient = mcb.Ambient.xyz + GlobalAmbientColor.xyz;\n"
"       float shininess = mcb.shininess;\n"

"       if(RecursionCnt <= maxRecursion) {\n"
//�_�����v�Z
"          float LightArea = LightArea_RandNum.x;\n"
"          uint RandNum = LightArea_RandNum.y;\n"

"          float3 dif = float3(0.0f, 0.0f, 0.0f);\n"
"          float3 spe = float3(0.0f, 0.0f, 0.0f);\n"

"          for(uint k = 0; k < RandNum; k++){\n"

"             ray.Direction = RandomVector(normal, LightArea);\n"
"             payload.mNo = EMISSIVE;\n"//��������p

"             bool loop = true;\n"
"             payload.hitPosition = hitPosition;\n"
"             payload.EmissiveIndex = 0;\n"
"             while(loop){\n"
"                ray.Origin = payload.hitPosition;\n"
"                TraceRay(gRtScene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, \n"
"                         0xFF, 1, 0, 1, ray, payload);\n"
"                loop = payload.reTry;\n"
"             }\n"

"             uint emInd = payload.EmissiveIndex;\n"
"             float4 emissiveHitPos = emissivePosition[emInd];\n"
"             emissiveHitPos.xyz = payload.hitPosition;\n"

"             Out = PointLightComNoDistance(SpeculerCol, Diffuse, Ambient, normal, emissiveHitPos, \n"//ShaderCG���֐�
"                                           hitPosition, payload.color, cameraPosition.xyz, shininess);\n"

"             dif += Out.Diffuse;\n"
"             spe += Out.Speculer;\n"
"          }\n"
"          emissiveColor.Diffuse += (dif / (float)RandNum);\n"
"          emissiveColor.Speculer += (spe / (float)RandNum);\n"
"          float PI2 = (2 * PI) * 0.4f;\n"//��ōl����
"          emissiveColor.Diffuse *= PI2;\n"
"          emissiveColor.Speculer *= PI2;\n"
"       }\n"
//�Ō�Ƀe�N�X�`���̐F�Ɋ|�����킹
"       difTexColor *= emissiveColor.Diffuse;\n"
"       speTexColor *= emissiveColor.Speculer;\n"
"       ret = difTexColor + speTexColor;\n"
"    }\n"
"    return ret;\n"
"}\n";