///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                           ShaderWaveDraw.hlsl                                         //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

char *ShaderWaveDraw =
"struct WaveData\n"
"{\n"
"	float sinWave;\n"
"   float theta;\n"
"};\n"
"StructuredBuffer<WaveData> gInput : register(t3);\n"

//wave
"cbuffer cbWave  : register(b2)\n"
"{\n"
//x:waveHeight, y:������
"    float4 g_wHei_divide;\n"
"    float g_speed;\n"
"};\n"

//**************************************�h���C���V�F�[�_�[*********************************************************************//
"[domain(\"tri\")]\n"
"GS_Mesh_INPUT DSWave(HS_CONSTANT_OUTPUT In, float3 UV : SV_DomaInLocation, const OutputPatch<HS_OUTPUT, 3> patch)\n"
"{\n"
"	GS_Mesh_INPUT output = (GS_Mesh_INPUT)0;\n"

//UV���W�v�Z
"   output.Tex0 = patch[0].Tex0 * UV.x + patch[1].Tex0 * UV.y + patch[2].Tex0 * UV.z;\n"
"   output.Tex1 = patch[0].Tex1 * UV.x + patch[1].Tex1 * UV.y + patch[2].Tex1 * UV.z;\n"

//�摜���獂�����Z�o
"   float4 texheight = g_texDiffuse.SampleLevel(g_samLinear, output.Tex0, 0);\n"
"   float4 height = texheight * g_DispAmount.x;\n"
"   float hei = (height.x + height.y + height.z) / 3;\n"

//�R���s���[�g�V�F�[�_�[�Ōv�Z����sin�g���o��
"   float wh = g_wHei_divide.y;\n"
"   float sinwave = gInput[wh * wh * output.Tex0.y + wh * output.Tex0.x].sinWave;\n"

//pos���W�v�Z
"   output.Pos = patch[0].Pos * UV.x + patch[1].Pos * UV.y + patch[2].Pos * UV.z;\n"

//���[�J���@���̕�����hei�����_�ړ�
"   output.Pos.xyz += hei * patch[0].Nor;\n"

//���[�J���@���̕�����sin�g�𐶐�
"   float3 sinwave3 = patch[0].Nor * sinwave;\n"

//���_��sin�g����
"   output.Pos.xyz += sinwave3;\n"
"   output.Nor = patch[0].Nor;\n"
"   output.instanceID = patch[0].instanceID;\n"

"	return output;\n"
"}\n";
//**************************************�h���C���V�F�[�_�[*********************************************************************//
