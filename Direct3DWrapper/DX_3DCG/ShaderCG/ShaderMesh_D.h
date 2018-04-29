///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                           ShaderMESH_D.hlsl                                           //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

//ShaderFunction.h�ɘA�������Ďg��
char *ShaderMesh_D =
"struct VS_OUTPUT\n"
"{\n"
"    float4 Pos        : POSITION;\n"
"    float3 Nor        : NORMAL;\n"
"    float3 GNor       : GEO_NORMAL;\n"
"    float2 Tex        : TEXCOORD;\n"
"    uint   instanceID : SV_InstanceID;\n"
"};\n"

//****************************************���b�V�����_**************************************************************//
"VS_OUTPUT VSMesh(float3 Pos : POSITION, float3 Nor : NORMAL, float3 GNor : GEO_NORMAL, float2 Tex : TEXCOORD, uint instanceID : SV_InstanceID)\n"
"{\n"
"    VS_OUTPUT output = (VS_OUTPUT)0;\n"
"    output.Pos = float4(Pos, 1);\n"
"    output.Nor = Nor;\n"
"    output.GNor = GNor;\n"
"    output.Tex = Tex;\n"
"    output.instanceID = instanceID;\n"
"    return output;\n"
"}\n";
//****************************************���b�V�����_**************************************************************//
