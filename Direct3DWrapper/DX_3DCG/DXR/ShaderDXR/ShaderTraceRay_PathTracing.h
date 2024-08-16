///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                      ShaderTraceRay_PathTracing.hlsl                                  //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

char* ShaderTraceRay_PathTracing =

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
"    float distAtten = distance * distance * lightst[emIndex].w;\n"
"    return cosine1 * cosine2 / distAtten;\n"
"}\n"

///////////////////////NeeGetLight///////////////////////////////////////////////////////////////
"RayPayload NeeGetLight(in uint RecursionCnt, in float3 hitPosition, in float3 normal, inout uint emIndex)\n"
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

///////////////////////NextEventEstimation////////////////////////////////////////////////////
"float3 NextEventEstimation(in uint RecursionCnt, in float3 hitPosition, \n"
"                           in float3 difTexColor, in float3 speTexColor, in float3 normal)\n"
"{\n"
"    uint emIndex = 0;\n"
"    RayPayload neeP = NeeGetLight(RecursionCnt, hitPosition, normal, emIndex);\n"

"    float g = G(hitPosition, normal, neeP, emIndex);\n"

"    float3 brdf = sumBRDF(neeP.hitPosition, hitPosition, difTexColor, speTexColor, normal);\n"

"    float PDF = LightPDF(emIndex);\n"
"    return (brdf * g / PDF) * neeP.color;\n"
"}\n"

///////////////////////PathTracing////////////////////////////////////////////////////////////
"RayPayload PathTracing(in uint RecursionCnt, in float3 hitPosition, \n"
"                       in float3 difTexColor, in float3 speTexColor, in float3 normal, \n"
"                       in float3 throughput, in uint matNo)\n"
"{\n"
"    RayPayload payload;\n"
"    payload.hitPosition = hitPosition;\n"
"    payload.hit = false;\n"

"    RayDesc ray;\n"
"    float LightArea = LightArea_RandNum.x;\n"
"    ray.Direction = RandomVector(normal, LightArea);\n"

"    float PDF = CosinePDF(normal, ray.Direction);\n"
"    if(PDF <= 0)PDF = 1.0f;\n"

"    float3 brdf = sumBRDF(ray.Direction, hitPosition, difTexColor, speTexColor, normal);\n"
"    float cosine = abs(dot(normal, ray.Direction));\n"
"    payload.throughput = throughput * brdf * cosine / PDF;\n"

"    payload.hitPosition = hitPosition;\n"
"    payload.mNo = matNo;\n"//��������p

"    traceRay(RecursionCnt, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, 1, 1, ray, payload);\n"

"    return payload;\n"
"}\n"

///////////////////////PayloadCalculate_PathTracing///////////////////////////////////////////
"float3 PayloadCalculate_PathTracing(in uint RecursionCnt, in float3 hitPosition, \n"
"                                    in float3 difTexColor, in float3 speTexColor, in float3 normal, inout float3 throughput)\n"
"{\n"
"    float3 ret = difTexColor;\n"//�����������ꍇ�A�f�荞�݂����̂܂܏o��E�E�E���ƂŕύX����

"    uint materialID = getMaterialID();\n"
"    MaterialCB mcb = material[materialID];\n"
"    uint mNo = mcb.materialNo;\n"

"    bool mf = materialIdent(mNo, EMISSIVE);\n"
"    if(!mf) {\n"//emissive�ȊO

////////PathTracing
"       uint matNo = EMISSIVE | NEE_PATHTRACER;\n"
"       if(traceMode == 1)matNo = EMISSIVE;\n"

"       RayPayload pathPay = PathTracing(RecursionCnt, hitPosition, difTexColor, speTexColor, normal, throughput, matNo);\n"

////////NextEventEstimation
"       if(traceMode == 2){\n"
"          float3 neeCol = NextEventEstimation(RecursionCnt, hitPosition, difTexColor, speTexColor, normal);\n"
"          ret = pathPay.color + neeCol * throughput;\n"
"       }\n"
"       else{\n"
"          ret = pathPay.color * throughput;\n"
"       }\n"

"       throughput = pathPay.throughput;\n"//throughput�̍X�V
"    }\n"
"    return ret;\n"
"}\n";