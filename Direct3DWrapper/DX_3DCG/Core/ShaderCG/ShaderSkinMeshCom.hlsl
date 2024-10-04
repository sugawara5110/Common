///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                       ShaderSkinMeshCom.hlsl                                          //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

cbuffer global_bones : register(b0, space0) //�{�[���̃|�[�Y�s�񂪓���
{
	matrix g_mConstBoneWorld[256];
};

//�X�L�j���O��̒��_�E�@��������
struct Skin
{
	float4 Pos;
	float3 Nor;
	float3 Tan;
};
//�o�[�e�b�N�X�o�b�t�@�[�̓���
struct VSSkinIn
{
	float3 Pos; //���_   
	float3 Nor; //�@��
	float3 Tan; //�ڃx�N�g��
	float3 gNor; //�t�H�[�}�b�g���킹�邽��
	float2 Tex0; //�e�N�X�`���[���W0
	float2 Tex1; //�e�N�X�`���[���W1
	uint4 Bones; //�{�[���̃C���f�b�N�X
	float4 Weights; //�{�[���̏d��
};
struct DXR_INPUT
{
	float3 Pos;
	float3 Nor;
	float3 Tan;
	float2 Tex0;
	float2 Tex1;
};

StructuredBuffer<VSSkinIn> VerticesSkin : register(t0, space0);
RWStructuredBuffer<DXR_INPUT> VerticesDXR : register(u0, space0);

//�w�肵���ԍ��̃{�[���̃|�[�Y�s���Ԃ�
matrix FetchBoneMatrix(uint iBone)
{
	return g_mConstBoneWorld[iBone];
}

//���_���X�L�j���O
Skin SkinVert(VSSkinIn Input)
{
	Skin Output = (Skin) 0;

	float4 Pos = float4(Input.Pos, 1);
	float3 Nor = Input.Nor;
	float3 Tan = Input.Tan;
//�{�[��0
	uint iBone = Input.Bones.x;
	float fWeight = Input.Weights.x;
	matrix m = FetchBoneMatrix(iBone);
	Output.Pos += fWeight * mul(Pos, m);
	Output.Nor += fWeight * mul(Nor, (float3x3) m);
	Output.Tan += fWeight * mul(Tan, (float3x3) m);
//�{�[��1
	iBone = Input.Bones.y;
	fWeight = Input.Weights.y;
	m = FetchBoneMatrix(iBone);
	Output.Pos += fWeight * mul(Pos, m);
	Output.Nor += fWeight * mul(Nor, (float3x3) m);
	Output.Tan += fWeight * mul(Tan, (float3x3) m);
//�{�[��2
	iBone = Input.Bones.z;
	fWeight = Input.Weights.z;
	m = FetchBoneMatrix(iBone);
	Output.Pos += fWeight * mul(Pos, m);
	Output.Nor += fWeight * mul(Nor, (float3x3) m);
	Output.Tan += fWeight * mul(Tan, (float3x3) m);
//�{�[��3
	iBone = Input.Bones.w;
	fWeight = Input.Weights.w;
	m = FetchBoneMatrix(iBone);
	Output.Pos += fWeight * mul(Pos, m);
	Output.Nor += fWeight * mul(Nor, (float3x3) m);
	Output.Tan += fWeight * mul(Tan, (float3x3) m);

	return Output;
}

//****************************************���b�V�����_**************************************************************//
[numthreads(1, 1, 1)]
void VSSkinCS(int2 id : SV_DispatchThreadID)
{
	int verID = id.x;

	DXR_INPUT output = (DXR_INPUT) 0;

	VSSkinIn input = VerticesSkin[verID];

	Skin vSkinned = SkinVert(input);

	output.Pos = vSkinned.Pos.xyz;
	output.Nor = vSkinned.Nor;
	output.Tan = vSkinned.Tan;
	output.Tex0 = input.Tex0;
	output.Tex1 = input.Tex1;

	VerticesDXR[verID] = output;
}
//****************************************���b�V�����_**************************************************************//

