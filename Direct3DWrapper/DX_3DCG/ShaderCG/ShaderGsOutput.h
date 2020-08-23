///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                     ShaderGsOutput.hlsl                                               //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

//ShaderFunction.h‚É˜AŒ‹‚³‚¹‚ÄŽg‚¤
char* ShaderGsOutput =

"struct DXR_INPUT\n"
"{\n"
"    float3 Pos      : POSITION;\n"
"    float3 Nor      : NORMAL;\n"
"    float2 Tex0     : TEXCOORD0;\n"
"    float2 Tex1     : TEXCOORD1;\n"
"};\n"
//**************************************‘O‚ªVS*************************************************//
"[maxvertexcount(3)]\n"
"void GS_Before_vs(triangle GS_Mesh_INPUT Input[3], inout TriangleStream <DXR_INPUT> triStream)\n"
"{\n"
"	DXR_INPUT p = (DXR_INPUT)0;\n"

"   p.Pos = Input[0].Pos.xyz;\n"
"   p.Nor = Input[0].Nor;\n"
"   p.Tex0 = Input[0].Tex0;\n"
"   p.Tex1 = Input[0].Tex1;\n"
"   triStream.Append(p);\n"
"   p.Pos = Input[1].Pos.xyz;\n"
"   p.Nor = Input[1].Nor;\n"
"   p.Tex0 = Input[1].Tex0;\n"
"   p.Tex1 = Input[1].Tex1;\n"
"   triStream.Append(p);\n"
"   p.Pos = Input[2].Pos.xyz;\n"
"   p.Nor = Input[2].Nor;\n"
"   p.Tex0 = Input[2].Tex0;\n"
"   p.Tex1 = Input[2].Tex1;\n"
"   triStream.Append(p);\n"
"	triStream.RestartStrip();\n"
"}\n";