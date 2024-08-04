///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                      ShaderTraceRay_NEE.hlsl                                          //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

char* ShaderTraceRay_NEE =

///////////////////////NEE/////////////////////////////////////////////////////////////////////
"LightOut Nee(in uint RecursionCnt, in float3 hitPosition, in float3 normal)\n"
"{\n"
"    uint NumEmissive = numEmissive.x;\n"
"    uint emIndex = Rand_integer() % NumEmissive;\n"
"    float3 ePos = emissivePosition[emIndex].xyz;\n"

"    RayDesc ray;\n"
"    ray.Direction = RandomVector(float3(1.0f, 0.0f, 0.0f), 2.0f);\n"//�S����

"    LightOut col = (LightOut)0;\n"

"    RayPayload payload;\n"
"    payload.hitPosition = ePos;\n"
"    payload.mNo = EMISSIVE | NEE;\n"//��������p

/////��������_�������_���Ŏ擾
"    traceRay(RecursionCnt, RAY_FLAG_CULL_FRONT_FACING_TRIANGLES, 1, 1, ray, payload);\n"

"    if(payload.hit){\n"
"       float3 lightVec = payload.hitPosition - hitPosition;\n"
"       ray.Direction = normalize(lightVec);\n"
"       payload.hitPosition = hitPosition;\n"
"       payload.mNo = EMISSIVE | NEE;\n"//��������p
////////���̈ʒu����擾���������ʒu�֔�΂�
"       traceRay(RecursionCnt, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, 1, 1, ray, payload);\n"

"       lightVec = payload.hitPosition - hitPosition;\n"
"       float3 light_normal = payload.normal;\n"
"       float3 hitnormal = normal;\n"
"       float3 Lvec = normalize(lightVec);\n"
"       float cosine1 = saturate(dot(-Lvec, light_normal));\n"
"       float cosine2 = saturate(dot(Lvec, hitnormal));\n"
"       float distance = length(lightVec);\n"
"       float distAtten = 1.0f / (distance * distance);\n"
"       float G = cosine1 * cosine2 * distAtten;\n"

"       uint materialID = getMaterialID();\n"
"       MaterialCB mcb = material[materialID];\n"

"       float3 SpeculerCol = mcb.Speculer.xyz;\n"
"       float3 Diffuse = mcb.Diffuse.xyz;\n"
"       float3 Ambient = mcb.Ambient.xyz + GlobalAmbientColor.xyz;\n"
"       float shininess = mcb.shininess;\n"

"       float3 difBRDF = DiffuseBRDF(Diffuse);\n"
"       float3 eyeVec = normalize(cameraPosition.xyz - hitPosition);\n"
"       float3 speBRDF = SpecularPhongBRDF(SpeculerCol, normal, eyeVec, Lvec, shininess);\n"

"       col.Diffuse = (difBRDF * G * payload.color + Ambient) * 1400;\n"
"       col.Speculer = speBRDF * G * payload.color * 1400;\n"
"    }\n"
"    return col;\n"
"}\n"

///////////////////////�����֌������΂�, �q�b�g�����ꍇ���邳�����Z//////////////////////////
"float3 EmissivePayloadCalculate_NEE(in uint RecursionCnt, in float3 hitPosition, \n"
"                                    in float3 difTexColor, in float3 speTexColor, in float3 normal)\n"
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
"       RayDesc ray;\n"
"       payload.hitPosition = hitPosition;\n"
"       RecursionCnt++;\n"
"       payload.RecursionCnt = RecursionCnt;\n"

"       if(RecursionCnt <= maxRecursion) {\n"
//�_�����v�Z
"          float LightArea = LightArea_RandNum.x;\n"
"          uint RandNum = LightArea_RandNum.y;\n"

"          float3 dif = float3(0.0f, 0.0f, 0.0f);\n"
"          float3 spe = float3(0.0f, 0.0f, 0.0f);\n"

"          for(uint k = 0; k < RandNum; k++){\n"

"             ray.Direction = RandomVector(normal, LightArea);\n"
"             payload.mNo = EMISSIVE | NEE_PATHTRACER;\n"//��������p

"             bool loop = true;\n"
"             payload.hitPosition = hitPosition;\n"
"             payload.EmissiveIndex = 0;\n"

"             traceRay(RecursionCnt, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, 1, 1, ray, payload);\n"

"             LightOut col = Nee(RecursionCnt, hitPosition, normal);\n"

"             if(payload.hit){\n"
"                col.Diffuse *= payload.color;\n"
"                col.Speculer *= payload.color;\n"
"             }\n"

"             dif += col.Diffuse;\n"
"             spe += col.Speculer;\n"
"          }\n"
"          emissiveColor.Diffuse += (dif / (float)RandNum);\n"
"          emissiveColor.Speculer += (spe / (float)RandNum);\n"
"          float PI2 = (2 * PI);\n"//��ōl����
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