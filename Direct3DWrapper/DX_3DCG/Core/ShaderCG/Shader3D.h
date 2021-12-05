///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                           Shader3D.hlsl                                               //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

char *Shader3D =
"struct PS_INPUT_BC\n"
"{\n"
"    float4 Pos        : SV_POSITION;\n"
"    float4 Col        : COLOR;\n"
"};\n"

//****************************************テクスチャ頂点**************************************************************//
"GS_Mesh_INPUT VSTextureColor(float3 Pos : POSITION, float3 Nor : NORMAL, float3 Tan : TANGENT, \n"
"                             float2 Tex : TEXCOORD, uint instanceID : SV_InstanceID)\n"
"{\n"
"    GS_Mesh_INPUT output = (GS_Mesh_INPUT)0;\n"

"    output.Pos = float4(Pos, 1);\n"
"    output.Nor = Nor;\n"
"    output.Tan = Tan;\n"
"    output.Tex0.x = Tex.x * g_pXpYmXmY.x + g_pXpYmXmY.x * g_pXpYmXmY.z;\n"
"    output.Tex0.y = Tex.y * g_pXpYmXmY.y + g_pXpYmXmY.y * g_pXpYmXmY.w;\n"
"    output.Tex1 = output.Tex0;\n"
"    output.instanceID = instanceID;\n"

"    return output;\n"
"}\n"
//****************************************テクスチャ頂点**************************************************************//

//****************************************基本色頂点******************************************************************//
"PS_INPUT_BC VSBaseColor(float4 Pos : POSITION, float4 Col : COLOR, uint instanceID : SV_InstanceID)\n"
"{\n"
"    PS_INPUT_BC output = (PS_INPUT_BC)0;\n"
"    output.Pos = mul(Pos, wvpCb[instanceID].wvp);\n"

"    output.Col = Col;\n"

"    return output;\n"
"}\n"
//****************************************基本色頂点******************************************************************//

//****************************************基本色ピクセル**************************************************************//
"float4 PSBaseColor(PS_INPUT_BC input) : SV_Target\n"
"{\n"
"   return input.Col + g_ObjCol;\n"
"}\n";
//****************************************基本色ピクセル**************************************************************//
