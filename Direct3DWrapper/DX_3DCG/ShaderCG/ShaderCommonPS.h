///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                       ShaderCommonPS.hlsl                                             //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

//ShaderFunction.hに連結させて使う
char *ShaderCommonPS =
//**************************************ピクセルシェーダー********************************************************************//
/////////////////////////////////////////ライト有////////////////////////////////////////////////////////////
"float4 PS_L(PS_INPUT input) : SV_Target\n"
"{\n"
//テクスチャ
"    float4 Tdif = g_texDiffuse.Sample(g_samLinear, input.Tex0);\n"
"    float4 Tnor = g_texNormal.Sample(g_samLinear, input.Tex0);\n"
"    float4 Tspe = g_texSpecular.Sample(g_samLinear, input.Tex1);\n"

//法線の再計算
"    float3 N = GetNormal(Tnor.xyz, input.Nor, input.tangent);\n"
"    N = normalize(N);\n"

//フォグ計算(テクスチャに対して計算)
"    Tdif = FogCom(g_FogColor, g_FogAmo_Density, g_C_Pos, input.wPos, Tdif);\n"

//ライト計算
"    LightOut Out = (LightOut)0;\n"
"    LightOut tmp;\n"
"    for (int i = 0; i < g_numLight.x; i++){\n"
"        tmp = PointLightCom(g_Speculer.xyz, g_Diffuse.xyz, g_Ambient.xyz, N, \n"
"                            g_LightPos[i], input.wPos.xyz, g_Lightst[i], g_LightColor[i].xyz, g_C_Pos.xyz);\n"
"        Out.Diffuse += tmp.Diffuse;\n"
"        Out.Speculer += tmp.Speculer;\n"
"    }\n"

//平行光源計算
"    tmp = DirectionalLightCom(g_Speculer.xyz, g_Diffuse.xyz, g_Ambient.xyz, N, \n"
"                                   g_DLightst, g_DLightDirection.xyz, g_DLightColor.xyz, input.wPos.xyz, g_C_Pos.xyz);\n"
"    Out.Diffuse += tmp.Diffuse;\n"
"    Out.Speculer += tmp.Speculer;\n"
//グローバルアンビエントを足す
"    Out.Diffuse += g_GlobalAmbientLight.xyz;\n"

//最後にテクスチャの色を掛け合わせる
"    float alpha = Tdif.w;\n"
"    float3 dif = Out.Diffuse * Tdif.xyz;\n"
"    float3 spe = Out.Speculer * Tspe.xyz;\n"
"    return float4(dif + spe, alpha) + g_ObjCol;\n"
"}\n"
/////////////////////////////////////////ライト有////////////////////////////////////////////////////////////

/////////////////////////////////////////ライト無////////////////////////////////////////////////////////////
"float4 PS(PS_INPUT input) : SV_Target\n"
"{\n"
//テクスチャ
"    float4 Tdif = g_texDiffuse.Sample(g_samLinear, input.Tex0);\n"

//フォグ計算テクスチャに対して計算
"    Tdif = FogCom(g_FogColor, g_FogAmo_Density, g_C_Pos, input.wPos, Tdif);\n"

"    return Tdif + g_ObjCol;\n"
"}\n";
/////////////////////////////////////////ライト無///////////////////////////////////////////////////////////
//**************************************ピクセルシェーダー*******************************************************************//