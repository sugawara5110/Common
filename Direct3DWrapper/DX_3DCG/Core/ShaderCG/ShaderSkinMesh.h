///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                          ShaderSkinMesh.hlsl                                          //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

char *ShaderSkinMesh =
"cbuffer global_bones : register(b1, space0)\n"//�{�[���̃|�[�Y�s�񂪓���
"{\n"
"   matrix g_mConstBoneWorld[256];\n"
"};\n"

//�X�L�j���O��̒��_�E�@��������
"struct Skin\n"
"{\n"
"   float4 Pos : POSITION;\n"
"   float3 Nor : NORMAL;\n"
"   float3 Tan : TANGENT;\n"
"};\n"
//�o�[�e�b�N�X�o�b�t�@�[�̓���
"struct VSSkinIn\n"
"{\n"
"   float3 Pos : POSITION;\n"//���_   
"   float3 Nor : NORMAL;\n"//�@��
"   float3 Tan : TANGENT;\n"//�ڃx�N�g��
"   float2 Tex0 : TEXCOORD0;\n"//�e�N�X�`���[���W0
"   float2 Tex1 : TEXCOORD1;\n"//�e�N�X�`���[���W1
"   uint4  Bones : BONE_INDEX;\n"//�{�[���̃C���f�b�N�X
"   float4 Weights : BONE_WEIGHT;\n"//�{�[���̏d��
"};\n"

//�w�肵���ԍ��̃{�[���̃|�[�Y�s���Ԃ�
"matrix FetchBoneMatrix(uint iBone)\n"
"{\n"
"   return g_mConstBoneWorld[iBone];\n"
"}\n"

//���_���X�L�j���O
"Skin SkinVert(VSSkinIn Input)\n"
"{\n"
"   Skin Output = (Skin)0;\n"

"   float4 Pos = float4(Input.Pos, 1);\n"
"   float3 Nor = Input.Nor;\n"
"   float3 Tan = Input.Tan;\n"
//�{�[��0
"   uint iBone = Input.Bones.x;\n"
"   float fWeight = Input.Weights.x;\n"
"   matrix m = FetchBoneMatrix(iBone);\n"
"   Output.Pos += fWeight * mul(Pos, m);\n"
"   Output.Nor += fWeight * mul(Nor, (float3x3)m);\n"
"   Output.Tan += fWeight * mul(Tan, (float3x3)m);\n"
//�{�[��1
"   iBone = Input.Bones.y;\n"
"   fWeight = Input.Weights.y;\n"
"   m = FetchBoneMatrix(iBone);\n"
"   Output.Pos += fWeight * mul(Pos, m);\n"
"   Output.Nor += fWeight * mul(Nor, (float3x3)m);\n"
"   Output.Tan += fWeight * mul(Tan, (float3x3)m);\n"
//�{�[��2
"   iBone = Input.Bones.z;\n"
"   fWeight = Input.Weights.z;\n"
"   m = FetchBoneMatrix(iBone);\n"
"   Output.Pos += fWeight * mul(Pos, m);\n"
"   Output.Nor += fWeight * mul(Nor, (float3x3)m);\n"
"   Output.Tan += fWeight * mul(Tan, (float3x3)m);\n"
//�{�[��3
"   iBone = Input.Bones.w;\n"
"   fWeight = Input.Weights.w;\n"
"   m = FetchBoneMatrix(iBone);\n"
"   Output.Pos += fWeight * mul(Pos, m);\n"
"   Output.Nor += fWeight * mul(Nor, (float3x3)m);\n"
"   Output.Tan += fWeight * mul(Tan, (float3x3)m);\n"

"   return Output;\n"
"}\n"

//****************************************���b�V�����_**************************************************************//
"GS_Mesh_INPUT VSSkin(VSSkinIn input, uint instanceID : SV_InstanceID)\n"
"{\n"
"    GS_Mesh_INPUT output = (GS_Mesh_INPUT)0;\n"

"    Skin vSkinned = SkinVert(input);\n"

"    output.Pos = vSkinned.Pos;\n"
"    output.Nor = vSkinned.Nor;\n"
"    output.Tan = vSkinned.Tan;\n"
"    output.instanceID = instanceID;\n"
"    output.Tex0 = input.Tex0;\n"
"    output.Tex1 = input.Tex1;\n"

"    return output;\n"
"}\n";
//****************************************���b�V�����_**************************************************************//
