///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                           ShaderMESH.hlsl                                             //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

char *ShaderMesh =
//****************************************メッシュ頂点**************************************************************//
"GS_Mesh_INPUT VSMesh(float3 Pos : POSITION, float3 Nor : NORMAL, float3 Tan : TANGENT, \n"
"                     float2 Tex : TEXCOORD, uint instanceID : SV_InstanceID)\n"
"{\n"
"    GS_Mesh_INPUT output = (GS_Mesh_INPUT)0;\n"

"    output.Pos = float4(Pos, 1);\n"
"    output.Nor = Nor;\n"
"    output.Tan = Tan;\n"
"    output.Tex0 = Tex;\n"
"    output.Tex1 = Tex;\n"
"    output.instanceID = instanceID;\n"

"    return output;\n"
"}\n";
//****************************************メッシュ頂点**************************************************************//

