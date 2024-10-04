///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                           Shader2D.hlsl                                               //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

Texture2D g_texColor : register(t0, space0);
SamplerState g_samLinear : register(s0, space0);

cbuffer global : register(b0, space0)
{
	float4 g_pos[256];
	float4 g_ObjCol[256];
	float4 g_sizeXY[256];
	float4 g_WidHei;
}

struct VS_OUTPUT
{
	float4 Pos : SV_POSITION;
	float4 Col : COLOR;
	float2 Tex : TEXCOORD;
};

//**************************基本色頂点**********************************//
VS_OUTPUT VSBaseColor(float4 Pos : POSITION, float4 Col : COLOR, uint instanceID : SV_InstanceID)
{
	VS_OUTPUT output = (VS_OUTPUT) 0;

	output.Pos = Pos;

	if (output.Pos.x > 0)
		output.Pos.x += g_sizeXY[instanceID].x;
	if (output.Pos.y > 0)
		output.Pos.y += g_sizeXY[instanceID].y;
	output.Pos.x = -1.0f + (output.Pos.x + g_pos[instanceID].x) * 2.0f / g_WidHei.x;
	output.Pos.y = 1.0f - (output.Pos.y + g_pos[instanceID].y) * 2.0f / g_WidHei.y;
	output.Pos.z = output.Pos.z + g_pos[instanceID].z;

	output.Col = Col + g_ObjCol[instanceID];

	return output;
}
//**************************基本色頂点**********************************//

//**************************基本色ピクセル******************************//
float4 PSBaseColor(VS_OUTPUT input) : SV_Target
{
	return input.Col;
}
//**************************基本色ピクセル******************************//

//**************************テクスチャ頂点******************************//
VS_OUTPUT VSTextureColor(float4 Pos : POSITION, float4 Col : COLOR, float2 Tex : TEXCOORD, uint instanceID : SV_InstanceID)
{
	VS_OUTPUT output = (VS_OUTPUT) 0;

	output.Pos = Pos;

	if (output.Pos.x > 0)
		output.Pos.x += g_sizeXY[instanceID].x;
	if (output.Pos.y > 0)
		output.Pos.y += g_sizeXY[instanceID].y;
	output.Pos.x = -1.0f + (output.Pos.x + g_pos[instanceID].x) * 2.0f / g_WidHei.x;
	output.Pos.y = 1.0f - (output.Pos.y + g_pos[instanceID].y) * 2.0f / g_WidHei.y;
	output.Pos.z = output.Pos.z + g_pos[instanceID].z;

	output.Col = Col + g_ObjCol[instanceID];
	output.Tex = Tex;

	return output;
}
//**************************テクスチャ頂点******************************//

//**************************テクスチャピクセル**************************//
float4 PSTextureColor(VS_OUTPUT input) : SV_Target
{
	return input.Col * g_texColor.Sample(g_samLinear, input.Tex);
}
//**************************テクスチャピクセル**************************//
