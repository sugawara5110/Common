///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                      ShaderTraceRay.hlsl                                              //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "ShaderTraceRay_PathTracing.hlsl"
#include "../../Core/ShaderCG/ShaderCalculateLighting.hlsl"

///////////////////////�����֌������΂�, �q�b�g�����ꍇ���邳�����Z//////////////////////////
float3 EmissivePayloadCalculate(in uint RecursionCnt, in float3 hitPosition, 
                                in float3 difTexColor, in float3 speTexColor, in float3 normal)
{
	uint materialID = getMaterialID();
	MaterialCB mcb = material[materialID];
	uint mNo = mcb.materialNo;
	float3 ret = difTexColor; //�����������ꍇ�A�f�荞�݂����̂܂܏o��E�E�E���ƂŕύX����

	bool mf = materialIdent(mNo, EMISSIVE);
	if (!mf)
	{ //emissive�ȊO

		RayPayload payload;
		payload.hit = false;
		LightOut emissiveColor = (LightOut) 0;
		LightOut Out;
		RayDesc ray;

		float3 SpeculerCol = mcb.Speculer.xyz;
		float3 Diffuse = mcb.Diffuse.xyz;
		float3 Ambient = mcb.Ambient.xyz + GlobalAmbientColor.xyz;
		float shininess = mcb.shininess;

////////�����v�Z
		uint NumEmissive = numEmissive.x;
		for (uint i = 0; i < NumEmissive; i++)
		{
			if (emissivePosition[i].w == 1.0f)
			{
				float3 lightVec = normalize(emissivePosition[i].xyz - hitPosition);

				ray.Direction = lightVec;
				payload.mNo = EMISSIVE; //��������p

				payload.hitPosition = hitPosition;

				traceRay(RecursionCnt, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, 0, 1, ray, payload);

				float4 emissiveHitPos = emissivePosition[i];
				emissiveHitPos.xyz = payload.hitPosition;

				Out = PointLightCom(SpeculerCol, Diffuse, Ambient, normal, emissiveHitPos, //ShaderCG���֐�
                                  hitPosition, lightst[i], payload.color, cameraPosition.xyz, shininess);

				emissiveColor.Diffuse += Out.Diffuse;
				emissiveColor.Speculer += Out.Speculer;
			}
		}
////////�Ō�Ƀe�N�X�`���̐F�Ɋ|�����킹
		difTexColor *= emissiveColor.Diffuse;
		speTexColor *= emissiveColor.Speculer;
		ret = difTexColor + speTexColor;
	}
	return ret;
}

///////////////////////���˕����֌������΂�, �q�b�g�����ꍇ�s�N�Z���l��Z///////////////////////
float3 MetallicPayloadCalculate(in uint RecursionCnt, in float3 hitPosition, 
                                in float3 difTexColor, in float3 normal, inout int hitInstanceId, in float fresnel)
{
    uint materialID = getMaterialID();
    uint mNo = material[materialID].materialNo;
    float3 ret = difTexColor;

    hitInstanceId = (int) getInstancingID(); //���g��ID��������

    if (materialIdent(mNo, METALLIC))
    { //METALLIC

        RayPayload payload;
        RayDesc ray;
//�����x�N�g�� 
        float3 eyeVec = WorldRayDirection();
//���˃x�N�g��
        float3 reflectVec = reflect(eyeVec, normalize(normal));
        ray.Direction = reflectVec; //���˕�����Ray���΂�
        payload.hitPosition = hitPosition;

        traceRay(RecursionCnt, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, 0, 1, ray, payload);

        float3 outCol = float3(0.0f, 0.0f, 0.0f);
        if (payload.hit)
        {
            float3 refCol = payload.color * (1.0f - fresnel);

            outCol = difTexColor * refCol; //�q�b�g�����ꍇ�f�荞�݂Ƃ��ď�Z
            hitInstanceId = payload.hitInstanceId; //�q�b�g����ID��������
            int hitmNo = payload.mNo;
            if (materialIdent(hitmNo, EMISSIVE))
            {
                outCol = refCol;
            }
        }
        else
        {
            outCol = difTexColor; //�q�b�g���Ȃ������ꍇ�f�荞�ݖ����Ō��̃s�N�Z����������
        }
        ret = outCol;
    }
    return ret;
}

////////////////////////////////////////������//////////////////////////////////////////
float3 Translucent(in uint RecursionCnt, in float3 hitPosition, in float4 difTexColor, in float3 normal, in float fresnel)
{
    uint materialID = getMaterialID();
    MaterialCB mcb = material[materialID];
    uint mNo = mcb.materialNo;
    float3 ret = difTexColor.xyz;
    float Alpha = difTexColor.w;

    if (materialIdent(mNo, TRANSLUCENCE) && Alpha < 1.0f)
    {

        float Alpha = difTexColor.w;
        RayPayload payload;
        RayDesc ray;
//�����x�N�g�� 
        float3 eyeVec = WorldRayDirection();
        float eta = 1.0f / mcb.RefractiveIndex;
        ray.Direction = refract(eyeVec, normalize(normal), eta);
        payload.hitPosition = hitPosition;

        traceRay(RecursionCnt, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, 0, 1, ray, payload);

//�A���t�@�l�̔䗦�Ō��̐F�ƌ����Փː�̐F��z��
        ret = payload.color * fresnel * (1.0f - Alpha) + difTexColor * Alpha;
    }
    return ret;
}

////////////////////////////////////////ONE_RAY//////////////////////////////////////////
float3 PayloadCalculate_OneRay(in uint RecursionCnt, in float3 hitPosition,
                               in float4 difTex, in float3 speTex, in float3 normalMap,
                               inout int hitInstanceId)
{
////////�����ւ̌���
    difTex.xyz = EmissivePayloadCalculate(RecursionCnt, hitPosition, difTex.xyz, speTex, normalMap);

////////�@���؂�ւ�
    float3 r_eyeVec = -WorldRayDirection(); //�����ւ̃x�N�g��
    float norDir = dot(r_eyeVec, normalMap);
    if (norDir < 0.0f)
        normalMap *= -1.0f;

////////�t���l���v�Z
    float fresnel = saturate(dot(r_eyeVec, normalMap));

////////���˕����ւ̌���
    difTex.xyz = MetallicPayloadCalculate(RecursionCnt, hitPosition, difTex.xyz, normalMap, hitInstanceId, fresnel);

////////������
    difTex.xyz = Translucent(RecursionCnt, hitPosition, difTex, normalMap, fresnel);

    return difTex.xyz;
}