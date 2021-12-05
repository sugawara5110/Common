///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                           ShaderWaveDraw.hlsl                                         //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

char *ShaderWaveDraw =
"struct WaveData\n"
"{\n"
"	float sinWave;\n"
"   float theta;\n"
"};\n"
"StructuredBuffer<WaveData> gInput : register(t3, space0);\n"

//wave
"cbuffer cbWave  : register(b2, space0)\n"
"{\n"
//x:waveHeight, y:分割数
"    float4 g_wHei_divide;\n"
"    float g_speed;\n"
"};\n"

//**************************************ドメインシェーダー*********************************************************************//
"[domain(\"tri\")]\n"
"GS_Mesh_INPUT DSWave(HS_CONSTANT_OUTPUT In, float3 UV : SV_DomaInLocation, const OutputPatch<HS_OUTPUT, 3> patch)\n"
"{\n"
"	GS_Mesh_INPUT output = (GS_Mesh_INPUT)0;\n"

//UV座標計算
"   output.Tex0 = patch[0].Tex0 * UV.x + patch[1].Tex0 * UV.y + patch[2].Tex0 * UV.z;\n"
"   output.Tex1 = patch[0].Tex1 * UV.x + patch[1].Tex1 * UV.y + patch[2].Tex1 * UV.z;\n"

//画像から高さを算出
"   float4 texheight = g_texDiffuse.SampleLevel(g_samLinear, output.Tex0, 0);\n"
"   float4 height = texheight * g_DispAmount.x;\n"
"   float hei = (height.x + height.y + height.z) / 3;\n"

//コンピュートシェーダーで計算したsin波取り出し
"   float wh = g_wHei_divide.y;\n"
"   float sinwave = gInput[wh * wh * output.Tex0.y + wh * output.Tex0.x].sinWave;\n"

//法線ベクトル
"   output.Nor = patch[0].Nor * UV.x + patch[1].Nor * UV.y + patch[2].Nor * UV.z;\n"

//接ベクトル
"   output.Tan = patch[0].Tan * UV.x + patch[1].Tan * UV.y + patch[2].Tan * UV.z;\n"

//pos座標計算
"   output.Pos = patch[0].Pos * UV.x + patch[1].Pos * UV.y + patch[2].Pos * UV.z;\n"

//ローカル法線の方向に頂点移動
"   output.AddPos = sinwave * output.Nor + hei * output.Nor;\n"
"   output.instanceID = patch[0].instanceID;\n"

"	return output;\n"
"}\n";
//**************************************ドメインシェーダー*********************************************************************//
