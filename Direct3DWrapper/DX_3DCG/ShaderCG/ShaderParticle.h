///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                           ShaderParticle.hlsl                                         //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

char *ShaderParticle =
"Texture2D g_texColor : register(t0);\n"
"SamplerState g_samLinear : register(s0);\n"

"cbuffer global  : register(b0)\n"
"{\n"
"	matrix g_WV;\n"
"	matrix g_invRot;\n"
"	matrix g_Proj;\n"
"   float4 g_size;\n"//x:サイズ, y:初期化フラグ, z:スピード
"   float4 g_ObjCol;\n"
"};\n"

"struct GS_INPUT\n"
"{\n"
"	float3 Pos    : POSITION0;\n"
"   float3 Pos_S  : POSITION1;\n"
"   float3 Pos_E  : POSITION2;\n"
"   float3 Nor    : NORMAL;\n"
"};\n"

"struct PS_INPUT\n"
"{\n"
"	float4 Pos   : SV_POSITION;\n"
"   float3 Nor   : NORMAL;\n"
"	float2 UV    : TEXCOORD;\n"
"};\n"

//*****************************************************************//
//****************************計算処理*****************************//
//*****************************************************************//

//****************************頂点SO*******************************//
"GS_INPUT VS_SO(GS_INPUT Out)\n"
"{\n"
"	return Out;\n"
"}\n"
//****************************頂点SO*******************************//

//**************************ジオメトリSO***************************//
"[maxvertexcount(1)]\n"
"void GS_Point_SO(point GS_INPUT Input[1], inout PointStream <GS_INPUT> ParticleStream)\n"
"{\n"
"	GS_INPUT p = (GS_INPUT)0;\n"

"   p.Pos = Input[0].Pos;\n"
"	p.Pos_S = Input[0].Pos_S;\n"
"	p.Pos_E = Input[0].Pos_E;\n"
//初期化フラグ有の場合初期化
"   if(g_size.y == 1.0f) p.Pos = p.Pos_S;\n"

//移動計算
"   if(p.Pos.x > p.Pos_E.x) p.Pos.x -= g_size.z;\n"
"   if(p.Pos.x < p.Pos_E.x) p.Pos.x += g_size.z;\n"
"   if(p.Pos.y > p.Pos_E.y) p.Pos.y -= g_size.z;\n"
"   if(p.Pos.y < p.Pos_E.y) p.Pos.y += g_size.z;\n"
"   if(p.Pos.z > p.Pos_E.z) p.Pos.z -= g_size.z;\n"
"   if(p.Pos.z < p.Pos_E.z) p.Pos.z += g_size.z;\n"

"	ParticleStream.Append(p);\n"
"}\n"
//***************************ジオメトリSO****************************//


//*****************************************************************//
//****************************描画処理*****************************//
//*****************************************************************//

//****************************頂点*********************************//
"GS_INPUT VS(GS_INPUT Out)\n"
"{\n"
"	return Out;\n"
"}\n"
//****************************頂点*********************************//

//**************************ジオメトリ*****************************//
"[maxvertexcount(4)]\n"
"void GS_Point(point GS_INPUT Input[1], inout TriangleStream <PS_INPUT> ParticleStream)\n"
"{\n"
"   float4 pos = float4(Input[0].Pos, 1.0f);\n"
"   pos = mul(pos, g_WV);\n"

"	PS_INPUT p = (PS_INPUT)0;\n"
//左上
"	p.Pos = pos + float4(-g_size.x, g_size.x, 0, 0);\n"
"	p.Pos = mul(p.Pos, g_Proj);\n"
"   p.Nor = Input[0].Nor;\n"
"	p.UV = float2(0, 0);\n"
"	ParticleStream.Append(p);\n"
//右上
"	p.Pos = pos + float4(g_size.x, g_size.x, 0, 0);\n"
"	p.Pos = mul(p.Pos, g_Proj);\n"
"   p.Nor = Input[0].Nor;\n"
"	p.UV = float2(1, 0);\n"
"	ParticleStream.Append(p);\n"
//左下
"	p.Pos = pos + float4(-g_size.x, -g_size.x, 0, 0);\n"
"	p.Pos = mul(p.Pos, g_Proj);\n"
"   p.Nor = Input[0].Nor;\n"
"	p.UV = float2(0, 1);\n"
"	ParticleStream.Append(p);\n"
//右下
"	p.Pos = pos + float4(g_size.x, -g_size.x, 0, 0);\n"
"	p.Pos = mul(p.Pos, g_Proj);\n"
"   p.Nor = Input[0].Nor;\n"
"	p.UV = float2(1, 1);\n"
"	ParticleStream.Append(p);\n"

"	ParticleStream.RestartStrip();\n"
"}\n"
//***************************ジオメトリ******************************//

//***************************ピクセル********************************//
"float4 PS(PS_INPUT Input) : SV_Target\n"
"{\n"
"	return g_texColor.Sample(g_samLinear, Input.UV) + g_ObjCol;\n"
"}\n"
//***************************ピクセル********************************//

//*****************************DXR***********************************//
"struct DXR_INPUT\n"
"{\n"
"    float3 Pos      : POSITION;\n"
"    float3 Nor      : NORMAL;\n"
"    float2 Tex0     : TEXCOORD0;\n"
"    float2 Tex1     : TEXCOORD1;\n"
"};\n"

"[maxvertexcount(6)]\n"
"void GS_PointDxr(point GS_INPUT Input[1], inout TriangleStream <DXR_INPUT> ParticleStream)\n"
"{\n"
"	DXR_INPUT p = (DXR_INPUT)0;\n"
"   float3 pos = Input[0].Pos;\n"

"   float3 pos4[4];\n"
"   pos4[0] = float3(-g_size.x, 0, g_size.x);\n"//左上
"   pos4[1] = float3(g_size.x, 0, g_size.x);\n"//右上
"   pos4[2] = float3(-g_size.x, 0, -g_size.x);\n"//左下
"   pos4[3] = float3(g_size.x, 0, -g_size.x);\n"//右下

"   pos4[0] = mul(pos4[0], (float3x3)g_invRot);\n"
"   pos4[1] = mul(pos4[1], (float3x3)g_invRot);\n"
"   pos4[2] = mul(pos4[2], (float3x3)g_invRot);\n"
"   pos4[3] = mul(pos4[3], (float3x3)g_invRot);\n"

//左上
"	p.Pos = pos4[0] + pos;\n"
"   p.Nor = Input[0].Nor;\n"
"	p.Tex0 = p.Tex1 = float2(0, 0);\n"
"	ParticleStream.Append(p);\n"
//右下
"	p.Pos = pos4[3] + pos;\n"
"   p.Nor = Input[0].Nor;\n"
"	p.Tex0 = p.Tex1 = float2(1, 1);\n"
"	ParticleStream.Append(p);\n"
//右上
"	p.Pos = pos4[1] + pos;\n"
"   p.Nor = Input[0].Nor;\n"
"	p.Tex0 = p.Tex1 = float2(1, 0);\n"
"	ParticleStream.Append(p);\n"

"	ParticleStream.RestartStrip();\n"

//左下
"	p.Pos = pos4[2] + pos;\n"
"   p.Nor = Input[0].Nor;\n"
"	p.Tex0 = p.Tex1 = float2(0, 1);\n"
"	ParticleStream.Append(p);\n"
//右下
"	p.Pos = pos4[3] + pos;\n"
"   p.Nor = Input[0].Nor;\n"
"	p.Tex0 = p.Tex1 = float2(1, 1);\n"
"	ParticleStream.Append(p);\n"
//左上
"	p.Pos = pos4[0] + pos;\n"
"   p.Nor = Input[0].Nor;\n"
"	p.Tex0 = p.Tex1 = float2(0, 0);\n"
"	ParticleStream.Append(p);\n"

"	ParticleStream.RestartStrip();\n"
"}\n";