///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                           ShaderWaveDraw.hlsl                                         //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

char *ShaderWaveDraw =
"Texture2D<float> gInput : register(t3, space0);\n"

"float3 NormalRecalculationSmoothPreparationWave(float2 tex, float centerHei)\n"
"{\n"
"   float2 nTex[4];\n"
"   float4 nHei[4];\n"
"   getNearTexAndHeight(tex, nTex, nHei);\n"

"   float sinwave[4];\n"
"   sinwave[0] = gInput.SampleLevel(g_samLinear, nTex[0], 0).r;\n"
"   sinwave[1] = gInput.SampleLevel(g_samLinear, nTex[1], 0).r;\n"
"   sinwave[2] = gInput.SampleLevel(g_samLinear, nTex[2], 0).r;\n"
"   sinwave[3] = gInput.SampleLevel(g_samLinear, nTex[3], 0).r;\n"

"   float3 v = getSmoothPreparationVec(nTex, nHei, sinwave);\n"
"   if(centerHei <= 0.0f){v = float3(0.0f, 0.0f, 0.0f);}\n"
"   return v;\n"
"}\n"

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
"   float sinwave = gInput.SampleLevel(g_samLinear, output.Tex0, 0).r;\n"

//法線ベクトル
"   output.Nor = patch[0].Nor * UV.x + patch[1].Nor * UV.y + patch[2].Nor * UV.z;\n"

//接ベクトル
"   output.Tan = patch[0].Tan * UV.x + patch[1].Tan * UV.y + patch[2].Tan * UV.z;\n"

//pos座標計算
"   output.Pos = patch[0].Pos * UV.x + patch[1].Pos * UV.y + patch[2].Pos * UV.z;\n"

//ローカル法線の方向に頂点移動
"   output.Pos.xyz += sinwave * output.Nor + hei * output.Nor;\n"

//Smooth用
"   output.AddNor = NormalRecalculationSmoothPreparationWave(output.Tex0, sinwave + hei);\n"
"   output.AddNor = normalTexConvert(output.AddNor, output.Nor, output.Tan);\n"

"   output.instanceID = patch[0].instanceID;\n"

"	return output;\n"
"}\n";
//**************************************ドメインシェーダー*********************************************************************//
