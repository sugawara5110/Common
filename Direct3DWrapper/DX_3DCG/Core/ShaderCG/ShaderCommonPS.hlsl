///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                       ShaderCommonPS.hlsl                                             //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

//�}�e���A�����̐F
cbuffer global_2 : register(b2, space0)
{
	float4 g_Diffuse;
	float4 g_Speculer;
	float4 g_Ambient;
};

////////////////////////////////�t�H�O�v�Z(�e�N�X�`���ɑ΂��Čv�Z)////////////////////////////////////////
float4 FogCom(float4 FogCol, float4 Fog, float4 CPos, float4 wPos, float4 Tex)
{
	float fd; //����
	float ff; //�t�H�O�t�@�N�^�[
	if (Fog.z == 1.0f)
	{
		fd = length(CPos.xyz - wPos.xyz) * 0.01f; //�����v�Z, 0.01�͕␳�l
		ff = pow(2.71828, -fd * Fog.y); //�t�H�O�t�@�N�^�[�v�Z(�ω���)
		ff *= Fog.x; //�t�H�O�S�̗̂�(���������������Ȃ�)
		ff = saturate(ff);
		if (Tex.w > 0.3f)
		{
			Tex = ff * Tex + (1.0f - ff) * FogCol;
		}
	}
	return Tex;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////

//**************************************�s�N�Z���V�F�[�_�[********************************************************************//
/////////////////////////////////////////���C�g�L//////////////////////////////////////////////////////////////////////
float4 PS_L_Common(PS_INPUT input, float4 Tdif, float4 Tspe, float3 Nor)
{
	float3 N = normalize(Nor);

//�t�H�O�v�Z(�e�N�X�`���ɑ΂��Čv�Z)
	Tdif = FogCom(g_FogColor, g_FogAmo_Density, g_C_Pos, input.wPos, Tdif);

//�A���r�G���g���Z
	float3 Ambient = g_Ambient.xyz + g_GlobalAmbientLight.xyz;

//���C�g�v�Z
	LightOut Out = (LightOut) 0;
	LightOut tmp;
	for (int i = 0; i < g_numLight.x; i++)
	{
		tmp = PointLightCom(g_Speculer.xyz, g_Diffuse.xyz, Ambient, N,
                            g_LightPos[i], input.wPos.xyz, g_Lightst[i], g_LightColor[i].xyz, g_C_Pos.xyz, g_DispAmount.z);
		Out.Diffuse += tmp.Diffuse;
		Out.Speculer += tmp.Speculer;
	}

//���s�����v�Z
	tmp = DirectionalLightCom(g_Speculer.xyz, g_Diffuse.xyz, Ambient, N,
                              g_DLightst, g_DLightDirection.xyz, g_DLightColor.xyz, input.wPos.xyz, g_C_Pos.xyz, g_DispAmount.z);
	Out.Diffuse += tmp.Diffuse;
	Out.Speculer += tmp.Speculer;

//�Ō�Ƀe�N�X�`���̐F���|�����킹��
	float alpha = Tdif.w;
	float3 dif = Out.Diffuse * Tdif.xyz;
	float3 spe = Out.Speculer * Tspe.xyz;
	return float4(dif + spe, alpha);
}
/***************************************�m�[�}���}�b�v�L*********************************************/
float4 PS_L(PS_INPUT input) : SV_Target
{
//�e�N�X�`��
	float4 Tdif = g_texDiffuse.Sample(g_samLinear, input.Tex0);
	float4 Tnor = g_texNormal.Sample(g_samLinear, input.Tex0);
	float4 Tspe = g_texSpecular.Sample(g_samLinear, input.Tex1);

//�@���̍Čv�Z
	float3 N = GetNormal(Tnor.xyz, input.Nor, input.tangent);

	return PS_L_Common(input, Tdif, Tspe, N) + wvpCb[input.instanceID].ObjCol;
}

//�@���e�X�g
float4 PS_L_NorTest(PS_INPUT input) : SV_Target
{
//�e�N�X�`��
	float4 Tnor = g_texNormal.Sample(g_samLinear, input.Tex0);

//�@���̍Čv�Z
	float3 N = GetNormal(Tnor.xyz, input.Nor, input.tangent);

	return float4(N, 1);
}
/***************************************�m�[�}���}�b�v�L*********************************************/
/***************************************�m�[�}���}�b�v��*********************************************/
float4 PS_L_NoNormalMap(PS_INPUT input) : SV_Target
{
//�e�N�X�`��
	float4 Tdif = g_texDiffuse.Sample(g_samLinear, input.Tex0);
	float4 Tspe = g_texSpecular.Sample(g_samLinear, input.Tex1);

	return PS_L_Common(input, Tdif, Tspe, input.Nor) + wvpCb[input.instanceID].ObjCol;
}

//�@���e�X�g
float4 PS_L_NoNormalMap_NorTest(PS_INPUT input) : SV_Target
{
	return float4(input.Nor, 1);
}
/***************************************�m�[�}���}�b�v��*********************************************/
/////////////////////////////////////////���C�g�L//////////////////////////////////////////////////////////////////////

/////////////////////////////////////////���C�g��/////////////////////////////////////////////////////////////////////
float4 PS(PS_INPUT input) : SV_Target
{
//�e�N�X�`��
	float4 Tdif = g_texDiffuse.Sample(g_samLinear, input.Tex0);

//�t�H�O�v�Z�e�N�X�`���ɑ΂��Čv�Z
	Tdif = FogCom(g_FogColor, g_FogAmo_Density, g_C_Pos, input.wPos, Tdif);

	return Tdif + wvpCb[input.instanceID].ObjCol;
}
/////////////////////////////////////////���C�g��////////////////////////////////////////////////////////////////////
//**************************************�s�N�Z���V�F�[�_�[*******************************************************************//