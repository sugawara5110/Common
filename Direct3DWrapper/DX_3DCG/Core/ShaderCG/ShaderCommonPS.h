///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                       ShaderCommonPS.hlsl                                             //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

char *ShaderCommonPS =
//**************************************�s�N�Z���V�F�[�_�[********************************************************************//
/////////////////////////////////////////���C�g�L//////////////////////////////////////////////////////////////////////
"float4 PS_L_Common(PS_INPUT input, float4 Tdif, float4 Tspe, float3 Nor)\n"
"{\n"
"    float3 N = normalize(Nor);\n"

//�t�H�O�v�Z(�e�N�X�`���ɑ΂��Čv�Z)
"    Tdif = FogCom(g_FogColor, g_FogAmo_Density, g_C_Pos, input.wPos, Tdif);\n"

//���C�g�v�Z
"    LightOut Out = (LightOut)0;\n"
"    LightOut tmp;\n"
"    for (int i = 0; i < g_numLight.x; i++){\n"
"        tmp = PointLightCom(g_Speculer.xyz, g_Diffuse.xyz, g_Ambient.xyz, N, \n"
"                            g_LightPos[i], input.wPos.xyz, g_Lightst[i], g_LightColor[i].xyz, g_C_Pos.xyz, g_DispAmount.z);\n"
"        Out.Diffuse += tmp.Diffuse;\n"
"        Out.Speculer += tmp.Speculer;\n"
"    }\n"

//���s�����v�Z
"    tmp = DirectionalLightCom(g_Speculer.xyz, g_Diffuse.xyz, g_Ambient.xyz, N, \n"
"                              g_DLightst, g_DLightDirection.xyz, g_DLightColor.xyz, input.wPos.xyz, g_C_Pos.xyz, g_DispAmount.z);\n"
"    Out.Diffuse += tmp.Diffuse;\n"
"    Out.Speculer += tmp.Speculer;\n"
//�O���[�o���A���r�G���g�𑫂�
"    Out.Diffuse += g_GlobalAmbientLight.xyz;\n"

//�Ō�Ƀe�N�X�`���̐F���|�����킹��
"    float alpha = Tdif.w;\n"
"    float3 dif = Out.Diffuse * Tdif.xyz;\n"
"    float3 spe = Out.Speculer * Tspe.xyz;\n"
"    return float4(dif + spe, alpha) + g_ObjCol;\n"
"}\n"
/***************************************�m�[�}���}�b�v�L*********************************************/
"float4 PS_L(PS_INPUT input) : SV_Target\n"
"{\n"
//�e�N�X�`��
"    float4 Tdif = g_texDiffuse.Sample(g_samLinear, input.Tex0);\n"
"    float4 Tnor = g_texNormal.Sample(g_samLinear, input.Tex0);\n"
"    float4 Tspe = g_texSpecular.Sample(g_samLinear, input.Tex1);\n"

//�@���̍Čv�Z
"    float3 N = GetNormal(Tnor.xyz, input.Nor, input.tangent);\n"

"    return PS_L_Common(input, Tdif, Tspe, N);\n"
"}\n"
/***************************************�m�[�}���}�b�v�L*********************************************/
/***************************************�m�[�}���}�b�v��*********************************************/
"float4 PS_L_NoNormalMap(PS_INPUT input) : SV_Target\n"
"{\n"
//�e�N�X�`��
"    float4 Tdif = g_texDiffuse.Sample(g_samLinear, input.Tex0);\n"
"    float4 Tspe = g_texSpecular.Sample(g_samLinear, input.Tex1);\n"

"    return PS_L_Common(input, Tdif, Tspe, input.Nor);\n"
"}\n"
/***************************************�m�[�}���}�b�v��*********************************************/
/////////////////////////////////////////���C�g�L//////////////////////////////////////////////////////////////////////

/////////////////////////////////////////���C�g��/////////////////////////////////////////////////////////////////////
"float4 PS(PS_INPUT input) : SV_Target\n"
"{\n"
//�e�N�X�`��
"    float4 Tdif = g_texDiffuse.Sample(g_samLinear, input.Tex0);\n"

//�t�H�O�v�Z�e�N�X�`���ɑ΂��Čv�Z
"    Tdif = FogCom(g_FogColor, g_FogAmo_Density, g_C_Pos, input.wPos, Tdif);\n"

"    return Tdif + g_ObjCol;\n"
"}\n";
/////////////////////////////////////////���C�g��////////////////////////////////////////////////////////////////////
//**************************************�s�N�Z���V�F�[�_�[*******************************************************************//