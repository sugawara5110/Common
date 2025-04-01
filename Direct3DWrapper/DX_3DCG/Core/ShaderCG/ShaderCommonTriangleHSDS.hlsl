///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                 ShaderCommonTriangleHSDS.hlsl                                         //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "ShaderCommonParameters.hlsl"
#include "ShaderNormalTangent.hlsl"

//***************************************�n���V�F�[�_�[�R���X�^���g*************************************************//
HS_CONSTANT_OUTPUT HSConstant(InputPatch<VS_OUTPUT, 3> ip, uint pid : SV_PrimitiveID)
{
    HS_CONSTANT_OUTPUT output = (HS_CONSTANT_OUTPUT) 0;

    uint instanceID = ip[0].instanceID;

//���[���h�ϊ�
    float4 wPos = mul(ip[0].Pos, wvpCb[instanceID].world);
//���_���猻�ݒn�܂ł̋������v�Z
    float distance = length(g_C_Pos.xyz - wPos.xyz);

//�����Ń|���S��������
    float divide = 2.0f;
    for (int i = 0; i < g_DispAmount.y; i++)
    {
        if (distance < g_divide[i].x)
        {
            divide = g_divide[i].y;
        }
    }

    output.factor[0] = divide;
    output.factor[1] = divide;
    output.factor[2] = divide;
//u �c�̕������i���̃��C�������{�Ђ����j
    output.inner_factor = divide;
//divide��2  ��   3 *  6���_
//divide��4  ��   3 * 24
//divide��8  ��   3 * 96
    return output;
}
//***************************************�n���V�F�[�_�[�R���X�^���g*************************************************//

//***************************************�n���V�F�[�_�[*************************************************************//
[domain("tri")]
[partitioning("integer")]
[outputtopology("triangle_cw")]//���\cw, ccw
[outputcontrolpoints(3)]
[patchconstantfunc("HSConstant")]
HS_OUTPUT HS(InputPatch<VS_OUTPUT, 3> ip, uint cpid : SV_OutputControlPointID, uint pid : SV_PrimitiveID)
{
	HS_OUTPUT output = (HS_OUTPUT) 0;
	output.Pos = ip[cpid].Pos;
	output.Nor = ip[cpid].Nor;
	output.Tan = ip[cpid].Tan;
	output.GNor = ip[cpid].GNor;
	output.Tex0 = ip[cpid].Tex0;
	output.Tex1 = ip[cpid].Tex1;
	output.instanceID = ip[cpid].instanceID;
	return output;
}
//***************************************�n���V�F�[�_�[*************************************************************//

//**************************************�h���C���V�F�[�_�[**********************************************************//
//�O�p�`�͏d�S���W�n  (UV.x + UV.y + UV.z) == 1.0f �����藧��
[domain("tri")]
GS_Mesh_INPUT DS(HS_CONSTANT_OUTPUT In, float3 UV : SV_DomaInLocation, const OutputPatch<HS_OUTPUT, 3> patch)
{
	GS_Mesh_INPUT output = (GS_Mesh_INPUT) 0;

//UV���W�v�Z
	output.Tex0 = patch[0].Tex0 * UV.x + patch[1].Tex0 * UV.y + patch[2].Tex0 * UV.z;
	output.Tex1 = patch[0].Tex1 * UV.x + patch[1].Tex1 * UV.y + patch[2].Tex1 * UV.z;

//�摜���獂�����Z�o
	float4 height = g_texDiffuse.SampleLevel(g_samLinear, output.Tex0, 0);
	float hei = (height.x + height.y + height.z) / 3 * g_DispAmount.x;

//�@���x�N�g��
	output.Nor = patch[0].Nor * UV.x + patch[1].Nor * UV.y + patch[2].Nor * UV.z;

//�ڃx�N�g��
	output.Tan = patch[0].Tan * UV.x + patch[1].Tan * UV.y + patch[2].Tan * UV.z;

//pos���W�v�Z
	output.Pos = patch[0].Pos * UV.x + patch[1].Pos * UV.y + patch[2].Pos * UV.z;

//���[�J���@���̕�����hei�����_�ړ�(�R���g���[���|�C���g�ʒu�ŏ����𕪂���)
	if (UV.x == 0.0f || UV.y == 0.0f || UV.z == 0.0f)//�ǂꂩ�̗v�f��0.0f�̏ꍇ�[�ɗL����
	{
		float3 geoDir = patch[0].GNor * UV.x + patch[1].GNor * UV.y + patch[2].GNor * UV.z;
		output.Pos.xyz += hei * geoDir; //�[�̓W�I���g���@���g�p(�N���b�L���O�΍�)
	}
	else
	{
		output.Pos.xyz += hei * output.Nor;
	}

//Smooth�p
	output.AddNor = NormalRecalculationSmoothPreparation(output.Tex0);
	output.AddNor = normalTexConvert(output.AddNor, output.Nor, output.Tan);

	output.instanceID = patch[0].instanceID;

	return output;
}
//**************************************�h���C���V�F�[�_�[**********************************************************//
