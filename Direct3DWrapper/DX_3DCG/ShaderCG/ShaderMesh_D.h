///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                           ShaderMESH_D.hlsl                                           //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

char *ShaderMesh_D =
//****************************************メッシュ頂点**************************************************************//
"VS_OUTPUT VSMesh(float3 Pos : POSITION, float3 Nor : NORMAL, float3 GNor : GEO_NORMAL, float2 Tex : TEXCOORD, uint instanceID : SV_InstanceID)\n"
"{\n"
"    VS_OUTPUT output = (VS_OUTPUT)0;\n"

"    output.Pos = float4(Pos, 1);\n"
"    output.Nor = Nor;\n"
"    output.GNor = GNor;\n"
"    output.Tex0 = Tex;\n"
"    output.Tex1 = Tex;\n"
"    output.instanceID = instanceID;\n"

"    return output;\n"
"}\n";
//****************************************メッシュ頂点**************************************************************//
