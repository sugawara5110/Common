///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                           ShaderWaveDraw.hlsl                                         //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

//ShaderFunction.hに連結させて使う
char *ShaderWaveDraw =
"struct WaveData\n"
"{\n"
"	float sinWave;\n"
"   float theta;\n"
"};\n"
"StructuredBuffer<WaveData> gInput : register(t2);\n"

//wave
"cbuffer cbWave  : register(b2)\n"
"{\n"
//x:waveHeight, y:分割数
"    float4 g_wHei_divide;\n"
"};\n"

"struct VS_OUTPUT1\n"
"{\n"
"    float3 Pos        : POSITION;\n"
"    float3 Nor        : NORMAL;\n"
"    float2 Tex        : TEXCOORD;\n"
"    uint   instanceID : SV_InstanceID;\n"
"};\n"

"struct HS_CONSTANT_OUTPUT\n"
"{\n"
"	 float factor[4]       : SV_TessFactor;\n"
"	 float inner_factor[2] : SV_InsideTessFactor;\n"
"};\n"

"struct HS_OUTPUT\n"
"{\n"
"    float3 Pos        : POSITION;\n"
"    float3 Nor        : NORMAL;\n"
"    float2 Tex        : TEXCOORD;\n"
"    uint   instanceID : SV_InstanceID;\n"
"};\n"

//*********************************************頂点シェーダー*******************************************************************//
"VS_OUTPUT1 VSWave(float3 Pos : POSITION, float3 Nor : NORMAL, float2 Tex : TEXCOORD, uint instanceID : SV_InstanceID)\n"
"{\n"
"    VS_OUTPUT1 output = (VS_OUTPUT1)0;\n"
"    output.Pos = Pos;\n"
"    output.Nor = Nor;\n"
"    output.Tex = Tex;\n"
"    output.instanceID = instanceID;\n"
"    return output;\n"
"}\n"
//*********************************************頂点シェーダー*******************************************************************//

//***************************************ハルシェーダーコンスタント*************************************************************//
"HS_CONSTANT_OUTPUT HSConstant(InputPatch<VS_OUTPUT1, 4> ip, uint pid : SV_PrimitiveID)\n"
"{\n"
"	HS_CONSTANT_OUTPUT output = (HS_CONSTANT_OUTPUT)0;\n"

//ワールド変換
"   float3 wPos = mul(ip[0].Pos, (float3x3)g_World[ip[0].instanceID]);\n"
//頂点から現在地までの距離を計算
"   float distance = length(g_C_Pos.xyz - wPos);\n"

//ポリゴン数決定
"   float divide = g_wHei_divide.y;\n"

"	output.factor[0] = divide;\n"
"	output.factor[1] = divide;\n"
"	output.factor[2] = divide;\n"
"	output.factor[3] = divide;\n"
//u 縦の分割数（横のラインを何本ひくか）
"	output.inner_factor[0] = divide;\n"
//v
"	output.inner_factor[1] = divide;\n"

"	return output;\n"
"}\n"
//***************************************ハルシェーダーコンスタント*************************************************************//

//***************************************ハルシェーダー*************************************************************************//
"[domain(\"quad\")]\n"
"[partitioning(\"integer\")]\n"
"[outputtopology(\"triangle_ccw\")]\n"
"[outputcontrolpoints(4)]\n"
"[patchconstantfunc(\"HSConstant\")]\n"
"HS_OUTPUT HSWave(InputPatch<VS_OUTPUT1, 4> ip, uint cpid : SV_OutputControlPointID, uint pid : SV_PrimitiveID)\n"
"{\n"
"	HS_OUTPUT output = (HS_OUTPUT)0;\n"
"	output.Pos = ip[cpid].Pos;\n"
"	output.Nor = ip[cpid].Nor;\n"
"	output.Tex = ip[cpid].Tex;\n"
"   output.instanceID = ip[cpid].instanceID;\n"
"	return output;\n"
"}\n"
//***************************************ハルシェーダー*************************************************************************//

//**************************************ドメインシェーダー*********************************************************************//
"[domain(\"quad\")]\n"
"GS_Mesh_INPUT DSWave(HS_CONSTANT_OUTPUT In, float2 UV : SV_DomaInLocation, const OutputPatch<HS_OUTPUT, 4> patch)\n"
"{\n"
"	GS_Mesh_INPUT output = (GS_Mesh_INPUT)0;\n"

//UV座標計算
"   float2 top_uv = lerp(patch[0].Tex, patch[1].Tex, UV.x);\n"
"   float2 bottom_uv = lerp(patch[3].Tex, patch[2].Tex, UV.x);\n"
"   float2 uv = float2(lerp(top_uv, bottom_uv, UV.y));\n"
"   output.Tex = uv;\n"

//画像から高さを算出
"   float4 texheight = g_texDiffuse.SampleLevel(g_samLinear, uv, 0);\n"
"   float4 height = texheight * g_DispAmount.x;\n"
"   float hei = (height.x + height.y + height.z) / 3;\n"

//コンピュートシェーダーで計算したsin波取り出し
"   float uvy = g_wHei_divide.y * (g_wHei_divide.y - 1.0f);\n"
"   float uvx = g_wHei_divide.y;\n"
"   float sinwave = gInput[uvy * uv.y + uvx * uv.x].sinWave;\n"

//pos座標計算
"   float3 top_pos = lerp(patch[0].Pos, patch[1].Pos, UV.x);\n"
"   float3 bottom_pos = lerp(patch[3].Pos, patch[2].Pos, UV.x);\n"
"   output.Pos = float4(lerp(top_pos, bottom_pos, UV.y), 1);\n"

//ローカル法線の方向にhei分頂点移動
"   output.Pos.xyz += hei * patch[0].Nor;\n"

//ローカル法線の方向にsin波を生成
"   float3 sinwave3 = patch[0].Nor * sinwave;\n"

//頂点にsin波合成
"   output.Pos.xyz += sinwave3;\n"

"   output.Nor = patch[0].Nor;\n"
"   output.instanceID = patch[0].instanceID;\n"

"	return output;\n"
"}\n";
//**************************************ドメインシェーダー*********************************************************************//
