///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                          ShaderSkinMesh_D.hlsl                                        //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

cbuffer global_bones : register(b1, space0) //�{�[���̃|�[�Y�s�񂪓���
{
	matrix g_mConstBoneWorld[256];
};

//�X�L�j���O��̒��_�E�@��������
struct Skin
{
	float4 Pos : POSITION;
	float3 Nor : NORMAL;
	float3 Tan : TANGENT;
	float3 GNor : GEO_NORMAL;
};
//�o�[�e�b�N�X�o�b�t�@�[�̓���
struct VSSkinIn
{
	float3 Pos : POSITION; //���_   
	float3 Nor : NORMAL; //�@��
	float3 Tan : TANGENT; //�ڃx�N�g��
	float3 GNor : GEO_NORMAL; //�W�I���g���@��
	float2 Tex0 : TEXCOORD0; //�e�N�X�`���[���W0
	float2 Tex1 : TEXCOORD1; //�e�N�X�`���[���W1
	uint4 Bones : BONE_INDEX; //�{�[���̃C���f�b�N�X
	float4 Weights : BONE_WEIGHT; //�{�[���̏d��
};

//�w�肵���ԍ��̃{�[���̃|�[�Y�s���Ԃ��@�T�u�֐��i�o�[�e�b�N�X�V�F�[�_�[�Ŏg�p�j
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
	float3 GNor = Input.GNor;
//�{�[��0
	uint iBone = Input.Bones.x;
	float fWeight = Input.Weights.x;
	matrix m = FetchBoneMatrix(iBone);
	Output.Pos += fWeight * mul(Pos, m);
	Output.Nor += fWeight * mul(Nor, (float3x3) m);
	Output.Tan += fWeight * mul(Tan, (float3x3) m);
	Output.GNor += fWeight * mul(GNor, (float3x3) m);
//�{�[��1
	iBone = Input.Bones.y;
	fWeight = Input.Weights.y;
	m = FetchBoneMatrix(iBone);
	Output.Pos += fWeight * mul(Pos, m);
	Output.Nor += fWeight * mul(Nor, (float3x3) m);
	Output.Tan += fWeight * mul(Tan, (float3x3) m);
	Output.GNor += fWeight * mul(GNor, (float3x3) m);
//�{�[��2
	iBone = Input.Bones.z;
	fWeight = Input.Weights.z;
	m = FetchBoneMatrix(iBone);
	Output.Pos += fWeight * mul(Pos, m);
	Output.Nor += fWeight * mul(Nor, (float3x3) m);
	Output.Tan += fWeight * mul(Tan, (float3x3) m);
	Output.GNor += fWeight * mul(GNor, (float3x3) m);
//�{�[��3
	iBone = Input.Bones.w;
	fWeight = Input.Weights.w;
	m = FetchBoneMatrix(iBone);
	Output.Pos += fWeight * mul(Pos, m);
	Output.Nor += fWeight * mul(Nor, (float3x3) m);
	Output.Tan += fWeight * mul(Tan, (float3x3) m);
	Output.GNor += fWeight * mul(GNor, (float3x3) m);

	return Output;
}

//****************************************�o�[�e�b�N�X�V�F�[�_�[****************************************************//
VS_OUTPUT VS(VSSkinIn input, uint instanceID : SV_InstanceID)
{
	VS_OUTPUT output = (VS_OUTPUT) 0;

	Skin vSkinned = SkinVert(input);

	output.Pos = vSkinned.Pos;
	output.Nor = vSkinned.Nor;
	output.Tan = vSkinned.Tan;
	output.GNor = vSkinned.GNor;
	output.instanceID = instanceID;
	output.Tex0 = input.Tex0;
	output.Tex1 = input.Tex1;

	return output;
}
//****************************************�o�[�e�b�N�X�V�F�[�_�[****************************************************//
