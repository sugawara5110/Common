///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                           Shader3D.hlsl                                               //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

//ShaderFunction.h�ɘA�������Ďg��
char *Shader3D =
"struct PS_INPUT_BC\n"
"{\n"
"    float4 Pos        : SV_POSITION;\n"
"    float4 Col        : COLOR;\n"
"};\n"

//****************************************�e�N�X�`�����_**************************************************************//
"PS_INPUT VSTextureColor(float3 Pos : POSITION, float3 Nor : NORMAL, float2 Tex : TEXCOORD, uint instanceID : SV_InstanceID)\n"
"{\n"
"    PS_INPUT output = (PS_INPUT)0;\n"
"    float4 pos4 = float4(Pos, 1);\n"
"    output.Pos = mul(pos4, g_WVP[instanceID]);\n"
"    output.wPos = mul(pos4, g_World[instanceID]);\n"
"    output.Nor = mul(Nor, (float3x3)g_World[instanceID]);\n"
"    output.Tex.x = Tex.x * g_pXpYmXmY.x + g_pXpYmXmY.x * g_pXpYmXmY.z;\n"
"    output.Tex.y = Tex.y * g_pXpYmXmY.y + g_pXpYmXmY.y * g_pXpYmXmY.w;\n"

"    return output;\n"
"}\n"
//****************************************�e�N�X�`�����_**************************************************************//

//****************************************��{�F���_******************************************************************//
"PS_INPUT_BC VSBaseColor(float4 Pos : POSITION, float4 Col : COLOR, uint instanceID : SV_InstanceID)\n"
"{\n"
"    PS_INPUT_BC output = (PS_INPUT_BC)0;\n"
"    output.Pos = mul(Pos, g_WVP[instanceID]);\n"

"    output.Col = Col;\n"

"    return output;\n"
"}\n"
//****************************************��{�F���_******************************************************************//

//****************************************��{�F�s�N�Z��**************************************************************//
"float4 PSBaseColor(PS_INPUT_BC input) : SV_Target\n"
"{\n"
"   return input.Col + g_ObjCol;\n"
"}\n";
//****************************************��{�F�s�N�Z��**************************************************************//
