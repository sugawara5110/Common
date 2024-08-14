///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                      ShaderTraceRay_NEE.hlsl                                          //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

char* ShaderTraceRay_NEE =

///////////////////////G��/////////////////////////////////////////////////////////////////////
"float G(in float3 hitPosition, in float3 normal, in RayPayload payload, uint emIndex)\n"
"{\n"
"    float3 lightVec = payload.hitPosition - hitPosition;\n"
"    float3 light_normal = payload.normal;\n"
"    float3 hitnormal = normal;\n"
"    float3 Lvec = normalize(lightVec);\n"
"    float cosine1 = dot(-Lvec, light_normal);\n"
"    float cosine2 = dot(Lvec, hitnormal);\n"
"    float distance = length(lightVec);\n"

"    float distAtten = lightst[emIndex].y + \n"
"                      lightst[emIndex].z * distance + \n"
"                      lightst[emIndex].w * distance * distance;\n"

//"    float distAtten = (distance * distance);\n"
"    return cosine1 * cosine2 / distAtten;\n"
"}\n"

///////////////////////NEE/////////////////////////////////////////////////////////////////////
"RayPayload Nee(in uint RecursionCnt, in float3 hitPosition, in float3 normal, inout uint emIndex)\n"
"{\n"
"    uint NumEmissive = numEmissive.x;\n"
/////�����T�C�Y���v
"    float sumSize = 0.0f;\n"
"    for(uint i = 0; i < NumEmissive; i++){\n"
"       sumSize += emissiveNo[i].y;\n"
"    }\n"

/////�����𐶐�
"    uint rnd = Rand_integer() % 101;\n"

/////�������̃T�C�Y����S�����̊������v�Z,��������C���f�b�N�X��I��
"    uint sum_min = 0;\n"
"    uint sum_max = 0;\n"
"    for(uint i = 0; i < NumEmissive; i++){\n"
"       sum_min = sum_max;\n"
"       sum_max += (uint)(emissiveNo[i].y / sumSize * 100.0f);\n"//�T�C�Y�̊�����ݐ�
"       if(sum_min <= rnd && rnd < sum_max){emIndex = i; break;}\n"//�������ݐϒl�͈̔͂ɓ������炻�̃C���f�b�N�X�l��I��
"    }\n"

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
"                                    in float3 difTexColor, in float3 speTexColor, in float3 normal, inout float3 throughput)\n"
"{\n"
"    float3 ret = difTexColor;\n"//�����������ꍇ�A�f�荞�݂����̂܂܏o��E�E�E���ƂŕύX����

"    uint materialID = getMaterialID();\n"
"    MaterialCB mcb = material[materialID];\n"
"    uint mNo = mcb.materialNo;\n"

"    bool mf = materialIdent(mNo, EMISSIVE);\n"
"    if(!mf) {\n"//emissive�ȊO

////////Nee
"       uint emIndex = 0;\n"
"       RayPayload neeP = Nee(RecursionCnt, hitPosition, normal, emIndex);\n"

"       float g = G(hitPosition, normal, neeP, emIndex);\n"

"       float3 brdf_nee = sumBRDF(neeP.hitPosition, hitPosition, difTexColor, speTexColor, normal);\n"

"       float neePDF = LightPDF(emIndex) + radiusPDF();\n"
"       float3 neeCol = (brdf_nee * g / neePDF) * neeP.color;\n"

////////PathTracing
"       RayPayload payload;\n"
"       payload.hitPosition = hitPosition;\n"
"       payload.hit = false;\n"

"       RayDesc ray;\n"
"       float LightArea = LightArea_RandNum.x;\n"
"       ray.Direction = RandomVector(normal, LightArea);\n"

"       float paPDF = CosinePDF(normal, ray.Direction);\n"
"       if(paPDF <= 0)paPDF = 1.0f;\n"

"       float3 brdf_pa = sumBRDF(ray.Direction, hitPosition, difTexColor, speTexColor, normal);\n"
"       float cosine = abs(dot(normal, ray.Direction));\n"
"       payload.throughput = throughput * brdf_pa * cosine / paPDF;\n"

"       payload.hitPosition = hitPosition;\n"
"       payload.EmissiveIndex = 0;\n"
"       payload.mNo = EMISSIVE | NEE_PATHTRACER;\n"//��������p

"       traceRay(RecursionCnt, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, 1, 1, ray, payload);\n"

"       throughput = payload.throughput;\n"

"       ret = neeCol * throughput;\n"
"    }\n"
"    return ret;\n"
"}\n";