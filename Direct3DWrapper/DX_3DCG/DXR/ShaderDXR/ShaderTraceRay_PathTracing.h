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
"    float cosine1 = saturate(dot(-Lvec, light_normal));\n"
"    float cosine2 = saturate(dot(Lvec, hitnormal));\n"
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
"    ray.Direction = RandomVector(float3(1.0f, 0.0f, 0.0f), 2.0f);\n"//2.0f�S����

"    RayPayload payload;\n"
"    payload.color = float3(0.0f, 0.0f, 0.0f);\n"
"    payload.hitPosition = ePos;\n"
"    payload.mNo = NEE;\n"//��������p

/////��������_�������_���Ŏ擾
"    traceRay(RecursionCnt, RAY_FLAG_CULL_FRONT_FACING_TRIANGLES, 1, 1, ray, payload);\n"

"    if(payload.hit){\n"
"       float3 lightVec = payload.hitPosition - hitPosition;\n"
"       ray.Direction = normalize(lightVec);\n"
"       payload.hitPosition = hitPosition;\n"
"       payload.mNo = NEE;\n"//��������p
////////���̈ʒu����擾���������ʒu�֔�΂�
"       traceRay(RecursionCnt, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, 1, 1, ray, payload);\n"
"    }\n"
"    return payload;\n"
"}\n"

///////////////////////NextEventEstimation////////////////////////////////////////////////////
"float3 NextEventEstimation(in float3 outDir, in uint RecursionCnt, in float3 hitPosition, \n"
"                           in float3 difTexColor, in float3 speTexColor, in float3 normal)\n"
"{\n"
"    uint emIndex = 0;\n"
"    RayPayload neeP = NeeGetLight(RecursionCnt, hitPosition, normal, emIndex);\n"

"    float g = G(hitPosition, normal, neeP, emIndex);\n"

"    float3 inDir = normalize(neeP.hitPosition - hitPosition);\n"

"    float3 local_inDir = worldToLocal(normal, inDir);\n"
"    float3 local_outDir = worldToLocal(normal, outDir);\n"

"    const float3 local_normal = float3(0.0f, 0.0f, 1.0f);\n"

"    float pdf;\n"
"    float3 bsdf = DiffSpeBSDF(local_inDir, local_outDir, difTexColor, speTexColor, local_normal, pdf);\n"

"    float PDF = LightPDF(emIndex);\n"
"    return (bsdf * g / PDF) * neeP.color;\n"
"}\n"

///////////////////////PathTracing////////////////////////////////////////////////////////////
"RayPayload PathTracing(in float3 outDir, in uint RecursionCnt, in float3 hitPosition, \n"
"                       in float4 difTexColor, in float3 speTexColor, in float3 normal, \n"
"                       in float3 throughput, in uint matNo)\n"
"{\n"
"    RayPayload payload;\n"
"    payload.hitPosition = hitPosition;\n"
"    payload.hit = false;\n"

"    float rouPDF = min(max(max(throughput.x, throughput.y), throughput.z), 1.0f);\n"
/////�m���I�ɏ�����ł��؂� ������Ȃ��Ɣ����ۂ��Ȃ�
"    uint rnd = Rand_integer() % 101;\n"
"    if(rnd > (uint)(rouPDF * 100.0f)){\n"
"       payload.throughput = float3(0.0f, 0.0f, 0.0f);\n"
"       payload.color = float3(0.0f, 0.0f, 0.0f);\n"
"       return payload;\n"
"    }\n"

"    uint materialID = getMaterialID();\n"
"    MaterialCB mcb = material[materialID];\n"
"    float3 Diffuse = mcb.Diffuse.xyz;\n"
"    float3 Speculer = mcb.Speculer.xyz;\n"
"    uint mNo = mcb.materialNo;\n"
"    float roughness = mcb.roughness;\n"

"    float sum_diff = Diffuse.x + Diffuse.y + Diffuse.z;\n"
"    float sum_spe = Speculer.x + Speculer.y + Speculer.z;\n"
"    float sum = sum_diff + sum_spe;\n"
"    uint diff_threshold = (uint)(sum_diff / sum * 100.0f);\n"

"    float Alpha = difTexColor.w;\n"

"    float3 rDir = float3(0.0f, 0.0f, 0.0f);\n"

"    bool bsdf_f = true;\n"
"    rnd = Rand_integer() % 101;\n"
"    if((uint)(Alpha * 100.0f) < rnd && materialIdent(mNo, TRANSLUCENCE)){\n"//����

"       float norDir = dot(outDir, normal);\n"
"       if(norDir < 0.0f)normal *= -1.0f;\n"

"       float3 eyeVec = -outDir;\n"
"       float3 refractVec = refract(eyeVec, normal, mcb.RefractiveIndex);\n"
"       float Area = roughness * roughness;\n"
"       rDir = RandomVector(refractVec, Area);\n"
"    }\n"
"    else{\n"
"       bsdf_f = false;\n"
"       rnd = Rand_integer() % 101;\n"
"       if(diff_threshold < rnd && materialIdent(mNo, METALLIC)){\n"//Speculer
"          float3 eyeVec = -outDir;\n"
"          float3 reflectVec = reflect(eyeVec, normal);\n"
"          float Area = roughness * roughness;\n"
"          rDir = RandomVector(reflectVec, Area);\n"
"       }\n"
"       else{\n"//Diffuse
"          rDir = RandomVector(normal, 1.0f);\n"//1.0f����
"       }\n"
"    }\n"

"    RayDesc ray;\n"
"    ray.Direction = rDir;\n"

"    float3 local_inDir = worldToLocal(normal, ray.Direction);\n"
"    float3 local_outDir = worldToLocal(normal, outDir);\n"

"    const float3 local_normal = float3(0.0f, 0.0f, 1.0f);\n"

"    float PDF = 0.0f;\n"
"    float3 bsdf;\n"
"    float cosine = abs(dot(local_normal, local_inDir));\n"

"    if(bsdf_f){\n"
"       const float3 H = normalize(local_inDir + local_outDir);\n"
"       bsdf = RefSpeBSDF(local_inDir, local_outDir, difTexColor, local_normal, H, PDF);\n"
"    }\n"
"    else{\n"
"       bsdf = DiffSpeBSDF(local_inDir, local_outDir, difTexColor.xyz, speTexColor, local_normal, PDF);\n"
"    }\n"

"    throughput *= (bsdf * cosine / PDF);\n"

"    payload.throughput = throughput / rouPDF;\n"

"    payload.hitPosition = hitPosition;\n"
"    payload.mNo = matNo;\n"//��������p

"    traceRay(RecursionCnt, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, 1, 1, ray, payload);\n"
 
/////NEE�̓p�X�g���ł̌����͊�^���Ȃ����A���˃}�e���A����������ւ�Ray�͌����\���ׁ̈A�[���ɂ��Ȃ�
"    if(payload.hit && matNo == NEE_PATHTRACER && !materialIdent(mNo, METALLIC))payload.color = float3(0.0f, 0.0f, 0.0f);\n"

"    return payload;\n"
"}\n"

///////////////////////PayloadCalculate_PathTracing///////////////////////////////////////////
"float3 PayloadCalculate_PathTracing(in uint RecursionCnt, in float3 hitPosition, \n"
"                                    in float4 difTexColor, in float3 speTexColor, in float3 normal, \n"
"                                    inout float3 throughput, inout int hitInstanceId)\n"
"{\n"
"    float3 ret = difTexColor.xyz;\n"

"    hitInstanceId = (int)getInstancingID(); \n"

"    uint materialID = getMaterialID();\n"
"    MaterialCB mcb = material[materialID];\n"
"    uint mNo = mcb.materialNo;\n"

"    float3 outDir = -WorldRayDirection();\n"

/////PathTracing
"    uint matNo = NEE_PATHTRACER;\n"
"    if(traceMode == 1)matNo = EMISSIVE;\n"

"    RayPayload pathPay = PathTracing(outDir, RecursionCnt, hitPosition, difTexColor, speTexColor, normal, throughput, matNo);\n"

/////NextEventEstimation
"    if(traceMode == 2){\n"
"       float3 neeCol = NextEventEstimation(outDir, RecursionCnt, hitPosition, difTexColor.xyz, speTexColor, normal);\n"
"       ret = pathPay.color + neeCol * throughput;\n"
"    }\n"
"    else{\n"
"       if(pathPay.hit){\n"
"          ret = pathPay.color * pathPay.throughput;\n"//�����q�b�g���̂�throughput����Z���l��Ԃ�
"       }\n"
"       else{\n"
"          ret = pathPay.color;\n"//�����q�b�g���Ȃ��ꍇ�͂��̂܂ܒl��Ԃ�
"       }\n"
"    }\n"
"    throughput = pathPay.throughput;\n"//throughput�̍X�V
"    hitInstanceId = pathPay.hitInstanceId;\n"
"    return ret;\n"
"}\n";