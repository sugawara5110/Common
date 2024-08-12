///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                      ShaderTraceRay_NEE.hlsl                                          //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

char* ShaderTraceRay_NEE =

///////////////////////G��/////////////////////////////////////////////////////////////////////
"float G(in float3 hitPosition, in float3 normal, in RayPayload payload)\n"
"{\n"
"    float3 lightVec = payload.hitPosition - hitPosition;\n"
"    float3 light_normal = payload.normal;\n"
"    float3 hitnormal = normal;\n"
"    float3 Lvec = normalize(lightVec);\n"
"    float cosine1 = saturate(dot(-Lvec, light_normal));\n"
"    float cosine2 = saturate(dot(Lvec, hitnormal));\n"
"    float distance = length(lightVec);\n"

"    float distAtten = (distance * distance);\n"
"    return cosine1 * cosine2 / distAtten;\n"
"}\n"

///////////////////////NEE/////////////////////////////////////////////////////////////////////
"RayPayload Nee(in uint RecursionCnt, in float3 hitPosition, in float3 normal)\n"
"{\n"
"    uint NumEmissive = numEmissive.x;\n"
"    uint emIndex = Rand_integer() % NumEmissive;\n"
"    float3 ePos = emissivePosition[emIndex].xyz;\n"

"    RayDesc ray;\n"
"    ray.Direction = RandomVector(float3(1.0f, 0.0f, 0.0f), 2.0f);\n"//�S����

"    RayPayload payload;\n"
"    payload.color = float3(0.0f, 0.0f, 0.0f);\n"
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
"    }\n"
"    return payload;\n"
"}\n"

///////////////////////�����֌������΂�, �q�b�g�����ꍇ���邳�����Z//////////////////////////
"float3 EmissivePayloadCalculate_NEE(in uint RecursionCnt, in float3 hitPosition, \n"
"                                    in float3 difTexColor, in float3 speTexColor, in float3 normal, in float3 throughput)\n"
"{\n"
"    uint materialID = getMaterialID();\n"
"    MaterialCB mcb = material[materialID];\n"
"    float3 Diffuse = mcb.Diffuse.xyz * difTexColor;\n"
"    float3 Speculer = mcb.Speculer.xyz * speTexColor;\n"
//"    float3 Ambient = mcb.Ambient.xyz + GlobalAmbientColor.xyz;\n"
"    float shininess = mcb.shininess;\n"

"    uint mNo = mcb.materialNo;\n"
"    float3 ret = difTexColor;\n"//�����������ꍇ�A�f�荞�݂����̂܂܏o��E�E�E���ƂŕύX����

"    bool mf = materialIdent(mNo, EMISSIVE);\n"
"    if(!mf) {\n"//emissive�ȊO

"       RayPayload payload;\n"
"       payload.hitPosition = hitPosition;\n"
"       payload.hit = false;\n"

"       RayDesc ray;\n"
"       float LightArea = LightArea_RandNum.x;\n"
"       ray.Direction = RandomVector(normal, LightArea);\n"

////////Nee
"       RayPayload neeP = Nee(RecursionCnt, hitPosition, normal);\n"

"       float g = G(hitPosition, normal, neeP);\n"

"       float3 Lvec = normalize(neeP.hitPosition - hitPosition);\n"

"       float3 difBRDF = DiffuseBRDF(Diffuse);\n"
"       float3 eyeVec = normalize(cameraPosition.xyz - hitPosition);\n"
"       float3 speBRDF = SpecularPhongBRDF(Speculer, normal, eyeVec, Lvec, shininess);\n"

"       float PDF = CosinePDF(normal, ray.Direction) * 0.05;\n"
"       if(PDF <= 0)PDF = 1.0f;\n"
"       ret = throughput * ((difBRDF + speBRDF) * g / PDF) * neeP.color;\n"

"       float cosine = abs(dot(normal, ray.Direction));\n"
"       payload.throughput = throughput * (difBRDF + speBRDF) * cosine / PDF;\n"

////////PathTracing
"       payload.mNo = EMISSIVE | NEE_PATHTRACER;\n"//��������p

"       payload.hitPosition = hitPosition;\n"
"       payload.EmissiveIndex = 0;\n"

"       traceRay(RecursionCnt, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, 1, 1, ray, payload);\n"

"       ret += payload.color;\n"
"    }\n"
"    return ret;\n"
"}\n";