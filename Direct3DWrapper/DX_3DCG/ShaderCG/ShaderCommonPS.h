///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                       ShaderCommonPS.hlsl                                             //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

//ShaderFunction.hに連結させて使う
char *ShaderCommonPS =
//**************************************ピクセルシェーダー********************************************************************//
////////////////////////////////////////////ライト有/////////////////////////////////////////////////////////////////
"float4 PS_L(PS_INPUT input) : SV_Target\n"
"{\n"
//法線正規化
"    float3 N = normalize(input.Nor);\n"
//テクスチャ
"    float4 T1 = g_texColor.Sample(g_samLinear, input.Tex);\n"

//フォグ計算(テクスチャに対して計算)
"    float4 T = FogCom(g_FogColor, g_FogAmo_Density, g_C_Pos, input.wPos, T1);\n"

//ライト計算
"    float3 Col = { 0.0f, 0.0f, 0.0f };\n"
"    for (int i = 0; i < g_ShadowLow_Lpcs.y; i++){\n"
"        Col = Col + PointLightCom(g_Speculer, g_Diffuse, N, g_ShadowLow_Lpcs, g_LightPos[i], input.wPos, g_Lightst[i], g_LightColor[i], g_C_Pos);\n"
"    }\n"

//平行光源計算
"    Col = Col + DirectionalLightCom(g_Speculer, g_Diffuse, N, g_DLightst, g_DLightDirection, g_DLightColor, input.wPos, g_C_Pos);\n"
//アンビエント加算
"    Col = Col + g_Ambient.xyz;\n"

//最後に基本色にテクスチャの色を掛け合わせる
"    return float4(Col, 1.0f) * T + g_ObjCol;\n"
"}\n"
////////////////////////////////////////////ライト有/////////////////////////////////////////////////////////////////

/////////////////////////////////////ライト有バンプマップ////////////////////////////////////////////////////////////
"float4 PS_LBump(PS_INPUT input) : SV_Target\n"
"{\n"
//テクスチャ
"    float4 T1 = g_texColor.Sample(g_samLinear, input.Tex);\n"
"    float4 T2 = g_texNormal.Sample(g_samLinear, input.Tex);\n"
//NormalMapと法線を掛け合わせて正規化
"    float3 N = normalize(input.Nor * T2.xyz);\n"

//フォグ計算(テクスチャに対して計算)
"    float4 T = FogCom(g_FogColor, g_FogAmo_Density, g_C_Pos, input.wPos, T1);\n"

//ライト計算
"    float3 Col = { 0.0f, 0.0f, 0.0f };\n"
"    for (int i = 0; i < g_ShadowLow_Lpcs.y; i++){\n"
"        Col = Col + PointLightCom(g_Speculer, g_Diffuse, N, g_ShadowLow_Lpcs, g_LightPos[i], input.wPos, g_Lightst[i], g_LightColor[i], g_C_Pos);\n"
"    }\n"

//平行光源計算
"    Col = Col + DirectionalLightCom(g_Speculer, g_Diffuse, N, g_DLightst, g_DLightDirection, g_DLightColor, input.wPos, g_C_Pos);\n"
//アンビエント加算
"    Col = Col + g_Ambient.xyz;\n"

//最後に基本色にテクスチャの色を掛け合わせる
"    return float4(Col, 1.0f) * T + g_ObjCol;\n"
"}\n"
/////////////////////////////////////ライト有バンプマップ////////////////////////////////////////////////////////////

/////////////////////////////////////////ライト無し/////////////////////////////////////////////////////////////////
"float4 PS(PS_INPUT input) : SV_Target\n"
"{\n"
//テクスチャ
"    float4 T1 = g_texColor.Sample(g_samLinear, input.Tex);\n"

//フォグ計算テクスチャに対して計算
"    float4 T = FogCom(g_FogColor, g_FogAmo_Density, g_C_Pos, input.wPos, T1);\n"

"    float4 col = T;\n"
"    return col + g_ObjCol;\n"
"}\n";
/////////////////////////////////////////ライト無し/////////////////////////////////////////////////////////////////
//**************************************ピクセルシェーダー*******************************************************************//