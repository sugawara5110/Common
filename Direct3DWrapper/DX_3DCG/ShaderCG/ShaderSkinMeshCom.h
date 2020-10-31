///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                       ShaderSkinMeshCom.hlsl                                          //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

char* ShaderSkinMeshCom =
"cbuffer global_bones : register(b0)\n"//�{�[���̃|�[�Y�s�񂪓���
"{\n"
"   matrix g_mConstBoneWorld[256];\n"
"};\n"

"cbuffer global_uv : register(b1)\n"//uv�؂�ւ�
"{\n"
"   float4 g_uvSw;\n"//.x�̂�
"};\n"

//�X�L�j���O��̒��_�E�@��������
"struct Skin\n"
"{\n"
"   float4 Pos;\n"
"   float3 Nor;\n"
"};\n"
//�o�[�e�b�N�X�o�b�t�@�[�̓���
"struct VSSkinIn\n"
"{\n"
"   float3 Pos;\n"//���_   
"   float3 Nor;\n"//�@��
"   float3 gNor;\n"//�����邽�߂̃_�~�[
"   float2 Tex0;\n"//�e�N�X�`���[���W0
"   float2 Tex1;\n"//�e�N�X�`���[���W1
"   uint4  Bones;\n"//�{�[���̃C���f�b�N�X
"   float4 Weights;\n"//�{�[���̏d��
"};\n"
"struct DXR_INPUT\n"
"{\n"
"    float3 Pos;\n"
"    float3 Nor;\n"
"    float2 Tex0;\n"
"    float2 Tex1;\n"
"};\n"

"StructuredBuffer<VSSkinIn> VerticesSkin : register(t0);\n"
"RWStructuredBuffer<DXR_INPUT> VerticesDXR : register(u0);\n"

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
//�{�[��0
"   uint iBone = Input.Bones.x;\n"
"   float fWeight = Input.Weights.x;\n"
"   matrix m = FetchBoneMatrix(iBone);\n"
"   Output.Pos += fWeight * mul(Pos, m);\n"
"   Output.Nor += fWeight * mul(Nor, (float3x3)m);\n"
//�{�[��1
"   iBone = Input.Bones.y;\n"
"   fWeight = Input.Weights.y;\n"
"   m = FetchBoneMatrix(iBone);\n"
"   Output.Pos += fWeight * mul(Pos, m);\n"
"   Output.Nor += fWeight * mul(Nor, (float3x3)m);\n"
//�{�[��2
"   iBone = Input.Bones.z;\n"
"   fWeight = Input.Weights.z;\n"
"   m = FetchBoneMatrix(iBone);\n"
"   Output.Pos += fWeight * mul(Pos, m);\n"
"   Output.Nor += fWeight * mul(Nor, (float3x3)m);\n"
//�{�[��3
"   iBone = Input.Bones.w;\n"
"   fWeight = Input.Weights.w;\n"
"   m = FetchBoneMatrix(iBone);\n"
"   Output.Pos += fWeight * mul(Pos, m);\n"
"   Output.Nor += fWeight * mul(Nor, (float3x3)m);\n"

"   return Output;\n"
"}\n"

//****************************************���b�V�����_**************************************************************//
"[numthreads(1, 1, 1)]\n"
"void VSSkinCS(int2 id : SV_DispatchThreadID)\n"
"{\n"
"   int verID = id.x;\n"

"   DXR_INPUT output = (DXR_INPUT)0;\n"

"   VSSkinIn input = VerticesSkin[verID];\n"

"   Skin vSkinned = SkinVert(input);\n"

"   output.Pos = vSkinned.Pos.xyz;\n"
"   output.Nor = vSkinned.Nor;\n"

"   if(g_uvSw.x == 0.0f)\n"//�؂�ւ���
"   {\n"
"      output.Tex0 = input.Tex0;\n"
"      output.Tex1 = input.Tex1;\n"
"   }\n"
"   if(g_uvSw.x == 1.0f)\n"//�t�]
"   {\n"
"      output.Tex0 = input.Tex1;\n"
"      output.Tex1 = input.Tex0;\n"
"   }\n"
"   if(g_uvSw.x == 2.0f)\n"//�ǂ����uv0
"   {\n"
"      output.Tex0 = input.Tex0;\n"
"      output.Tex1 = input.Tex0;\n"
"   }\n"
"   if(g_uvSw.x == 3.0f)\n"//�ǂ����uv1
"   {\n"
"      output.Tex0 = input.Tex1;\n"
"      output.Tex1 = input.Tex1;\n"
"   }\n"

"   VerticesDXR[verID] = output;\n"
"}\n";
//****************************************���b�V�����_**************************************************************//

