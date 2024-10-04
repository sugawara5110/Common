///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                       ShaderCommonPS.hlsl                                             //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

//マテリアル毎の色
cbuffer global_2 : register(b2, space0)
{
	float4 g_Diffuse;
	float4 g_Speculer;
	float4 g_Ambient;
};

////////////////////////////////フォグ計算(テクスチャに対して計算)////////////////////////////////////////
float4 FogCom(float4 FogCol, float4 Fog, float4 CPos, float4 wPos, float4 Tex)
{
	float fd; //距離
	float ff; //フォグファクター
	if (Fog.z == 1.0f)
	{
		fd = length(CPos.xyz - wPos.xyz) * 0.01f; //距離計算, 0.01は補正値
		ff = pow(2.71828, -fd * Fog.y); //フォグファクター計算(変化量)
		ff *= Fog.x; //フォグ全体の量(小さい方が多くなる)
		ff = saturate(ff);
		if (Tex.w > 0.3f)
		{
			Tex = ff * Tex + (1.0f - ff) * FogCol;
		}
	}
	return Tex;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////

//**************************************ピクセルシェーダー********************************************************************//
/////////////////////////////////////////ライト有//////////////////////////////////////////////////////////////////////
float4 PS_L_Common(PS_INPUT input, float4 Tdif, float4 Tspe, float3 Nor)
{
	float3 N = normalize(Nor);

//フォグ計算(テクスチャに対して計算)
	Tdif = FogCom(g_FogColor, g_FogAmo_Density, g_C_Pos, input.wPos, Tdif);

//アンビエント加算
	float3 Ambient = g_Ambient.xyz + g_GlobalAmbientLight.xyz;

//ライト計算
	LightOut Out = (LightOut) 0;
	LightOut tmp;
	for (int i = 0; i < g_numLight.x; i++)
	{
		tmp = PointLightCom(g_Speculer.xyz, g_Diffuse.xyz, Ambient, N,
                            g_LightPos[i], input.wPos.xyz, g_Lightst[i], g_LightColor[i].xyz, g_C_Pos.xyz, g_DispAmount.z);
		Out.Diffuse += tmp.Diffuse;
		Out.Speculer += tmp.Speculer;
	}

//平行光源計算
	tmp = DirectionalLightCom(g_Speculer.xyz, g_Diffuse.xyz, Ambient, N,
                              g_DLightst, g_DLightDirection.xyz, g_DLightColor.xyz, input.wPos.xyz, g_C_Pos.xyz, g_DispAmount.z);
	Out.Diffuse += tmp.Diffuse;
	Out.Speculer += tmp.Speculer;

//最後にテクスチャの色を掛け合わせる
	float alpha = Tdif.w;
	float3 dif = Out.Diffuse * Tdif.xyz;
	float3 spe = Out.Speculer * Tspe.xyz;
	return float4(dif + spe, alpha);
}
/***************************************ノーマルマップ有*********************************************/
float4 PS_L(PS_INPUT input) : SV_Target
{
//テクスチャ
	float4 Tdif = g_texDiffuse.Sample(g_samLinear, input.Tex0);
	float4 Tnor = g_texNormal.Sample(g_samLinear, input.Tex0);
	float4 Tspe = g_texSpecular.Sample(g_samLinear, input.Tex1);

//法線の再計算
	float3 N = GetNormal(Tnor.xyz, input.Nor, input.tangent);

	return PS_L_Common(input, Tdif, Tspe, N) + wvpCb[input.instanceID].ObjCol;
}

//法線テスト
float4 PS_L_NorTest(PS_INPUT input) : SV_Target
{
//テクスチャ
	float4 Tnor = g_texNormal.Sample(g_samLinear, input.Tex0);

//法線の再計算
	float3 N = GetNormal(Tnor.xyz, input.Nor, input.tangent);

	return float4(N, 1);
}
/***************************************ノーマルマップ有*********************************************/
/***************************************ノーマルマップ無*********************************************/
float4 PS_L_NoNormalMap(PS_INPUT input) : SV_Target
{
//テクスチャ
	float4 Tdif = g_texDiffuse.Sample(g_samLinear, input.Tex0);
	float4 Tspe = g_texSpecular.Sample(g_samLinear, input.Tex1);

	return PS_L_Common(input, Tdif, Tspe, input.Nor) + wvpCb[input.instanceID].ObjCol;
}

//法線テスト
float4 PS_L_NoNormalMap_NorTest(PS_INPUT input) : SV_Target
{
	return float4(input.Nor, 1);
}
/***************************************ノーマルマップ無*********************************************/
/////////////////////////////////////////ライト有//////////////////////////////////////////////////////////////////////

/////////////////////////////////////////ライト無/////////////////////////////////////////////////////////////////////
float4 PS(PS_INPUT input) : SV_Target
{
//テクスチャ
	float4 Tdif = g_texDiffuse.Sample(g_samLinear, input.Tex0);

//フォグ計算テクスチャに対して計算
	Tdif = FogCom(g_FogColor, g_FogAmo_Density, g_C_Pos, input.wPos, Tdif);

	return Tdif + wvpCb[input.instanceID].ObjCol;
}
/////////////////////////////////////////ライト無////////////////////////////////////////////////////////////////////
//**************************************ピクセルシェーダー*******************************************************************//