///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                           Shader3D.hlsl                                               //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "ShaderCommonParameters.hlsl"

struct PS_INPUT_BC
{
	float4 Pos : SV_POSITION;
	float4 Col : COLOR;
};
//****************************************基本色頂点******************************************************************//
PS_INPUT_BC VSBaseColor(float4 Pos : POSITION, float4 Col : COLOR, uint instanceID : SV_InstanceID)
{
	PS_INPUT_BC output = (PS_INPUT_BC) 0;
	output.Pos = mul(Pos, wvpCb[instanceID].wvp);

	output.Col = Col + wvpCb[instanceID].ObjCol;

	return output;
}
//****************************************基本色頂点******************************************************************//

//****************************************基本色ピクセル**************************************************************//
float4 PSBaseColor(PS_INPUT_BC input) : SV_Target
{
	return input.Col;
}
//****************************************基本色ピクセル**************************************************************//
