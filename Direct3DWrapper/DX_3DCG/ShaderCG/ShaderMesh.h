///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                           ShaderMESH.hlsl                                             //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

//ShaderFunction.h�ɘA�������Ďg��
char *ShaderMesh =
//****************************************���b�V�����_**************************************************************//
"PS_INPUT VSMesh(float3 Pos : POSITION, float3 Nor : NORMAL, float2 Tex : TEXCOORD, uint instanceID : SV_InstanceID)\n"
"{\n"
"    PS_INPUT output = (PS_INPUT)0;\n"
"    float4 pos4 = float4(Pos, 1);\n"
"    output.Pos = mul(pos4, g_WVP[instanceID]);\n"
"    output.wPos = mul(pos4, g_World[instanceID]);\n"
"    output.Nor = mul(Nor, (float3x3)g_World[instanceID]);\n"
"    output.Tex = Tex;\n"

"    return output;\n"
"}\n";
//****************************************���b�V�����_**************************************************************//

