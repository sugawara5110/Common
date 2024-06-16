///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                       ShaderCommonPS.hlsl                                             //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

char *ShaderCommonPS =
//マテリアル毎の色
"cbuffer global_2 : register(b2, space0)\n"
"{\n"
"    float4 g_Diffuse;\n"
"    float4 g_Speculer; \n"
"    float4 g_Ambient;\n"
"};\n"

////////////////////////////////フォグ計算(テクスチャに対して計算)////////////////////////////////////////
"float4 FogCom(float4 FogCol, float4 Fog, float4 CPos, float4 wPos, float4 Tex)\n"
"{\n"
"    float fd;\n"//距離
"    float ff;\n"//フォグファクター
"    if(Fog.z == 1.0f){\n"
"       fd = length(CPos.xyz - wPos.xyz) * 0.01f;\n"//距離計算, 0.01は補正値
"       ff = pow(2.71828, -fd * Fog.y);\n"//フォグファクター計算(変化量)
"       ff *= Fog.x;\n"//フォグ全体の量(小さい方が多くなる)
"       ff = saturate(ff);\n"
"       if(Tex.w > 0.3f){\n"
"         Tex = ff * Tex + (1.0f - ff) * FogCol;\n"
"       }\n"
"    }\n"
"   return Tex;\n"
"}\n"
//////////////////////////////////////////////////////////////////////////////////////////////////////////

//**************************************ピクセルシェーダー********************************************************************//
/////////////////////////////////////////ライト有//////////////////////////////////////////////////////////////////////
"float4 PS_L_Common(PS_INPUT input, float4 Tdif, float4 Tspe, float3 Nor)\n"
"{\n"
"    float3 N = normalize(Nor);\n"

//フォグ計算(テクスチャに対して計算)
"    Tdif = FogCom(g_FogColor, g_FogAmo_Density, g_C_Pos, input.wPos, Tdif);\n"

//アンビエント加算
"    float3 Ambient = g_Ambient.xyz + g_GlobalAmbientLight.xyz;"

//ライト計算
"    LightOut Out = (LightOut)0;\n"
"    LightOut tmp;\n"
"    for (int i = 0; i < g_numLight.x; i++){\n"
"        tmp = PointLightCom(g_Speculer.xyz, g_Diffuse.xyz, Ambient, N, \n"
"                            g_LightPos[i], input.wPos.xyz, g_Lightst[i], g_LightColor[i].xyz, g_C_Pos.xyz, g_DispAmount.z);\n"
"        Out.Diffuse += tmp.Diffuse;\n"
"        Out.Speculer += tmp.Speculer;\n"
"    }\n"

//平行光源計算
"    tmp = DirectionalLightCom(g_Speculer.xyz, g_Diffuse.xyz, Ambient, N, \n"
"                              g_DLightst, g_DLightDirection.xyz, g_DLightColor.xyz, input.wPos.xyz, g_C_Pos.xyz, g_DispAmount.z);\n"
"    Out.Diffuse += tmp.Diffuse;\n"
"    Out.Speculer += tmp.Speculer;\n"

//最後にテクスチャの色を掛け合わせる
"    float alpha = Tdif.w;\n"
"    float3 dif = Out.Diffuse * Tdif.xyz;\n"
"    float3 spe = Out.Speculer * Tspe.xyz;\n"
"    return float4(dif + spe, alpha);\n"
"}\n"
/***************************************ノーマルマップ有*********************************************/
"float4 PS_L(PS_INPUT input) : SV_Target\n"
"{\n"
//テクスチャ
"    float4 Tdif = g_texDiffuse.Sample(g_samLinear, input.Tex0);\n"
"    float4 Tnor = g_texNormal.Sample(g_samLinear, input.Tex0);\n"
"    float4 Tspe = g_texSpecular.Sample(g_samLinear, input.Tex1);\n"

//法線の再計算
"    float3 N = GetNormal(Tnor.xyz, input.Nor, input.tangent);\n"

"    return PS_L_Common(input, Tdif, Tspe, N) + wvpCb[input.instanceID].ObjCol;\n"
"}\n"

//法線テスト
"float4 PS_L_NorTest(PS_INPUT input) : SV_Target\n"
"{\n"
//テクスチャ
"    float4 Tnor = g_texNormal.Sample(g_samLinear, input.Tex0);\n"

//法線の再計算
"    float3 N = GetNormal(Tnor.xyz, input.Nor, input.tangent);\n"

"    return float4(N, 1);\n"
"}\n"
/***************************************ノーマルマップ有*********************************************/
/***************************************ノーマルマップ無*********************************************/
"float4 PS_L_NoNormalMap(PS_INPUT input) : SV_Target\n"
"{\n"
//テクスチャ
"    float4 Tdif = g_texDiffuse.Sample(g_samLinear, input.Tex0);\n"
"    float4 Tspe = g_texSpecular.Sample(g_samLinear, input.Tex1);\n"

"    return PS_L_Common(input, Tdif, Tspe, input.Nor) + wvpCb[input.instanceID].ObjCol;\n"
"}\n"

//法線テスト
"float4 PS_L_NoNormalMap_NorTest(PS_INPUT input) : SV_Target\n"
"{\n"
"    return float4(input.Nor, 1);\n"
"}\n"
/***************************************ノーマルマップ無*********************************************/
/////////////////////////////////////////ライト有//////////////////////////////////////////////////////////////////////

/////////////////////////////////////////ライト無/////////////////////////////////////////////////////////////////////
"float4 PS(PS_INPUT input) : SV_Target\n"
"{\n"
//テクスチャ
"    float4 Tdif = g_texDiffuse.Sample(g_samLinear, input.Tex0);\n"

//フォグ計算テクスチャに対して計算
"    Tdif = FogCom(g_FogColor, g_FogAmo_Density, g_C_Pos, input.wPos, Tdif);\n"

"    return Tdif + wvpCb[input.instanceID].ObjCol;\n"
"}\n";
/////////////////////////////////////////ライト無////////////////////////////////////////////////////////////////////
//**************************************ピクセルシェーダー*******************************************************************//