///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                       ShaderCommonPS.hlsl                                             //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

//ShaderFunction.h�ɘA�������Ďg��
char *ShaderCommonPS =
//**************************************�s�N�Z���V�F�[�_�[********************************************************************//
////////////////////////////////////////////���C�g�L/////////////////////////////////////////////////////////////////
"float4 PS_L(PS_INPUT input) : SV_Target\n"
"{\n"
//�@�����K��
"    float3 N = normalize(input.Nor);\n"
//�e�N�X�`��
"    float4 T1 = g_texColor.Sample(g_samLinear, input.Tex);\n"

//�t�H�O�v�Z(�e�N�X�`���ɑ΂��Čv�Z)
"    float4 T = FogCom(g_FogColor, g_FogAmo_Density, g_C_Pos, input.wPos, T1);\n"

//���C�g�v�Z
"    float3 Col = { 0.0f, 0.0f, 0.0f };\n"
"    for (int i = 0; i < g_ShadowLow_Lpcs.y; i++){\n"
"        Col = Col + PointLightCom(g_Speculer, g_Diffuse, N, g_ShadowLow_Lpcs, g_LightPos[i], input.wPos, g_Lightst[i], g_LightColor[i], g_C_Pos);\n"
"    }\n"

//���s�����v�Z
"    Col = Col + DirectionalLightCom(g_Speculer, g_Diffuse, N, g_DLightst, g_DLightDirection, g_DLightColor, input.wPos, g_C_Pos);\n"

//�Ō�Ɋ�{�F�Ƀe�N�X�`���̐F���|�����킹��
"    return float4(Col, 1.0f) * T + g_ObjCol;\n"
"}\n"
////////////////////////////////////////////���C�g�L/////////////////////////////////////////////////////////////////

/////////////////////////////////////���C�g�L�o���v�}�b�v////////////////////////////////////////////////////////////
"float4 PS_LBump(PS_INPUT input) : SV_Target\n"
"{\n"
//�e�N�X�`��
"    float4 T1 = g_texColor.Sample(g_samLinear, input.Tex);\n"
"    float4 T2 = g_texNormal.Sample(g_samLinear, input.Tex);\n"
//NormalMap�Ɩ@�����|�����킹�Đ��K��
"    float3 N = normalize(input.Nor * T2.xyz);\n"

//�t�H�O�v�Z(�e�N�X�`���ɑ΂��Čv�Z)
"    float4 T = FogCom(g_FogColor, g_FogAmo_Density, g_C_Pos, input.wPos, T1);\n"

//���C�g�v�Z
"    float3 Col = { 0.0f, 0.0f, 0.0f };\n"
"    for (int i = 0; i < g_ShadowLow_Lpcs.y; i++){\n"
"        Col = Col + PointLightCom(g_Speculer, g_Diffuse, N, g_ShadowLow_Lpcs, g_LightPos[i], input.wPos, g_Lightst[i], g_LightColor[i], g_C_Pos);\n"
"    }\n"

//���s�����v�Z
"    Col = Col + DirectionalLightCom(g_Speculer, g_Diffuse, N, g_DLightst, g_DLightDirection, g_DLightColor, input.wPos, g_C_Pos);\n"

//�Ō�Ɋ�{�F�Ƀe�N�X�`���̐F���|�����킹��
"    return float4(Col, 1.0f) * T + g_ObjCol;\n"
"}\n"
/////////////////////////////////////���C�g�L�o���v�}�b�v////////////////////////////////////////////////////////////

/////////////////////////////////////////���C�g����/////////////////////////////////////////////////////////////////
"float4 PS(PS_INPUT input) : SV_Target\n"
"{\n"
//�e�N�X�`��
"    float4 T1 = g_texColor.Sample(g_samLinear, input.Tex);\n"

//�t�H�O�v�Z�e�N�X�`���ɑ΂��Čv�Z
"    float4 T = FogCom(g_FogColor, g_FogAmo_Density, g_C_Pos, input.wPos, T1);\n"

"    float4 col = T;\n"
"    return col + g_ObjCol;\n"
"}\n";
/////////////////////////////////////////���C�g����/////////////////////////////////////////////////////////////////
//**************************************�s�N�Z���V�F�[�_�[*******************************************************************//