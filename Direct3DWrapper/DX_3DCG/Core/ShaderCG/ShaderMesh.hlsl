///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                           ShaderMESH.hlsl                                             //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "ShaderCommonParameters.hlsl"

//****************************************メッシュ頂点**************************************************************//
GS_Mesh_INPUT VSMesh(float3 Pos : POSITION, float3 Nor : NORMAL, float3 Tan : TANGENT, 
                     float2 Tex : TEXCOORD, uint instanceID : SV_InstanceID)
{
	GS_Mesh_INPUT output = (GS_Mesh_INPUT) 0;

	output.Pos = float4(Pos, 1);
	output.Nor = Nor;
	output.Tan = Tan;
	output.Tex0.x = Tex.x * g_pXpYmXmY.x + g_pXpYmXmY.x * g_pXpYmXmY.z;
	output.Tex0.y = Tex.y * g_pXpYmXmY.y + g_pXpYmXmY.y * g_pXpYmXmY.w;
	output.Tex1 = output.Tex0;
	output.instanceID = instanceID;

	return output;
}
//****************************************メッシュ頂点**************************************************************//

