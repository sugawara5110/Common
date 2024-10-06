///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                           ShaderMESH_D.hlsl                                           //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "ShaderCommonParameters.hlsl"

//****************************************メッシュ頂点**************************************************************//
VS_OUTPUT VSMesh(float3 Pos : POSITION, float3 Nor : NORMAL, float3 Tan : TANGENT, 
                 float3 GNor : GEO_NORMAL, float2 Tex : TEXCOORD, uint instanceID : SV_InstanceID)
{
	VS_OUTPUT output = (VS_OUTPUT) 0;

	output.Pos = float4(Pos, 1);
	output.Nor = Nor;
	output.Tan = Tan;
	output.GNor = GNor;
	output.Tex0 = Tex;
	output.Tex1 = Tex;
	output.instanceID = instanceID;

	return output;
}
//****************************************メッシュ頂点**************************************************************//
