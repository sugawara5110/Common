///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                           Shader2D.hlsl                                               //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

char *Shader2D =
"Texture2D g_texColor : register(t0);\n"
"SamplerState g_samLinear : register(s0);\n"

"cbuffer global  : register(b0)\n"
"{\n"
"    float4  g_pos[80];\n"
"    float4  g_ObjCol[80];\n"
"    float4  g_sizeXY[80];\n"
"    float4  g_WidHei;\n"
"}\n"

"struct VS_OUTPUT\n"
"{\n"
"	float4 Pos : SV_POSITION;\n"
"	float4 Col : COLOR;\n"
"	float2 Tex : TEXCOORD;\n"
"};\n"

//**************************基本色頂点**********************************//
"VS_OUTPUT VSBaseColor(float4 Pos : POSITION, float4 Col : COLOR, uint instanceID : SV_InstanceID)\n"
"{\n"
"	VS_OUTPUT output = (VS_OUTPUT)0;\n"

"   output.Pos = Pos;\n"

"   if(output.Pos.x > 0)output.Pos.x += g_sizeXY[instanceID].x;\n"
"   if(output.Pos.y > 0)output.Pos.y += g_sizeXY[instanceID].y;\n"
"	output.Pos.x = -1.0f + (output.Pos.x + g_pos[instanceID].x) * 2.0f / g_WidHei.x;\n"
"	output.Pos.y = 1.0f - (output.Pos.y + g_pos[instanceID].y) * 2.0f / g_WidHei.y;\n"
"   output.Pos.z = output.Pos.z + g_pos[instanceID].z;\n"

"	output.Col = Col + g_ObjCol[instanceID];\n"

"	return output;\n"
"}\n"
//**************************基本色頂点**********************************//

//**************************基本色ピクセル******************************//
"float4 PSBaseColor(VS_OUTPUT input) : SV_Target\n"
"{\n"
"	return input.Col;\n"
"}\n"
//**************************基本色ピクセル******************************//

//**************************テクスチャ頂点******************************//
"VS_OUTPUT VSTextureColor(float4 Pos : POSITION, float4 Col : COLOR, float2 Tex : TEXCOORD, uint instanceID : SV_InstanceID)\n"
"{\n"
"	VS_OUTPUT output = (VS_OUTPUT)0;\n"

"   output.Pos = Pos;\n"

"   if(output.Pos.x > 0)output.Pos.x += g_sizeXY[instanceID].x;\n"
"   if(output.Pos.y > 0)output.Pos.y += g_sizeXY[instanceID].y;\n"
"	output.Pos.x = -1.0f + (output.Pos.x + g_pos[instanceID].x) * 2.0f / g_WidHei.x;\n"
"	output.Pos.y = 1.0f - (output.Pos.y + g_pos[instanceID].y) * 2.0f / g_WidHei.y;\n"
"   output.Pos.z = output.Pos.z + g_pos[instanceID].z;\n"

"	output.Col = Col + g_ObjCol[instanceID];\n"
"	output.Tex = Tex;\n"

"	return output;\n"
"}\n"
//**************************テクスチャ頂点******************************//

//**************************テクスチャピクセル**************************//
"float4 PSTextureColor(VS_OUTPUT input) : SV_Target\n"
"{\n"
"	return input.Col * g_texColor.Sample(g_samLinear, input.Tex);\n"
"}\n";
//**************************テクスチャピクセル**************************//
