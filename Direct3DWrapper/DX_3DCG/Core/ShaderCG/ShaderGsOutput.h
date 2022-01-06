///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                     ShaderGsOutput.hlsl                                               //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

char* ShaderGsOutput =

"struct DXR_INPUT\n"
"{\n"
"    float3 Pos  : POSITION;\n"
"    float3 Nor  : NORMAL;\n"
"    float3 Tan  : TANGENT;\n"
"    float2 Tex0 : TEXCOORD0;\n"
"    float2 Tex1 : TEXCOORD1;\n"
"};\n"
//**************************************前がVS*************************************************//
"[maxvertexcount(3)]\n"
"void GS_Before_vs(triangle GS_Mesh_INPUT Input[3], inout TriangleStream <DXR_INPUT> triStream)\n"
"{\n"
"	DXR_INPUT p = (DXR_INPUT)0;\n"

"   p.Pos = Input[0].Pos.xyz;\n"
"   p.Nor = Input[0].Nor;\n"
"   p.Tan = Input[0].Tan;\n"
"   p.Tex0 = Input[0].Tex0;\n"
"   p.Tex1 = Input[0].Tex1;\n"
"   triStream.Append(p);\n"
"   p.Pos = Input[1].Pos.xyz;\n"
"   p.Nor = Input[1].Nor;\n"
"   p.Tan = Input[1].Tan;\n"
"   p.Tex0 = Input[1].Tex0;\n"
"   p.Tex1 = Input[1].Tex1;\n"
"   triStream.Append(p);\n"
"   p.Pos = Input[2].Pos.xyz;\n"
"   p.Nor = Input[2].Nor;\n"
"   p.Tan = Input[2].Tan;\n"
"   p.Tex0 = Input[2].Tex0;\n"
"   p.Tex1 = Input[2].Tex1;\n"
"   triStream.Append(p);\n"
"	triStream.RestartStrip();\n"
"}\n"
//**************************************前がVS*************************************************//

//**************************************前がDS*************************************************//
//********Smooth*********//
"[maxvertexcount(3)]\n"
"void GS_Before_ds_Smooth(triangle GS_Mesh_INPUT Input[3], inout TriangleStream <DXR_INPUT> triStream)\n"
"{\n"
"	DXR_INPUT p = (DXR_INPUT)0;\n"
"   GS_Mesh_INPUT recal[3];\n"

//DS後法線再計算
"   NormalRecalculationEdge(Input);\n"
"   recal[0] = NormalRecalculationSmooth(Input[0]);\n"
"   recal[1] = NormalRecalculationSmooth(Input[1]);\n"
"   recal[2] = NormalRecalculationSmooth(Input[2]);\n"

"   p.Pos = recal[0].Pos.xyz;\n"
"   p.Nor = recal[0].Nor;\n"
"   p.Tan = recal[0].Tan;\n"
"   p.Tex0 = recal[0].Tex0;\n"
"   p.Tex1 = recal[0].Tex1;\n"
"   triStream.Append(p);\n"
"   p.Pos = recal[1].Pos.xyz;\n"
"   p.Nor = recal[1].Nor;\n"
"   p.Tan = recal[1].Tan;\n"
"   p.Tex0 = recal[1].Tex0;\n"
"   p.Tex1 = recal[1].Tex1;\n"
"   triStream.Append(p);\n"
"   p.Pos = recal[2].Pos.xyz;\n"
"   p.Nor = recal[2].Nor;\n"
"   p.Tan = recal[2].Tan;\n"
"   p.Tex0 = recal[2].Tex0;\n"
"   p.Tex1 = recal[2].Tex1;\n"
"   triStream.Append(p);\n"
"	triStream.RestartStrip();\n"
"}\n"
//********Smooth*********//
//********Edge***********//
"[maxvertexcount(3)]\n"
"void GS_Before_ds_Edge(triangle GS_Mesh_INPUT Input[3], inout TriangleStream <DXR_INPUT> triStream)\n"
"{\n"
"	DXR_INPUT p = (DXR_INPUT)0;\n"

//DS後法線再計算
"   NormalRecalculationEdge(Input);\n"

"   p.Pos = Input[0].Pos.xyz;\n"
"   p.Nor = Input[0].Nor;\n"
"   p.Tan = Input[0].Tan;\n"
"   p.Tex0 = Input[0].Tex0;\n"
"   p.Tex1 = Input[0].Tex1;\n"
"   triStream.Append(p);\n"
"   p.Pos = Input[1].Pos.xyz;\n"
"   p.Nor = Input[1].Nor;\n"
"   p.Tan = Input[1].Tan;\n"
"   p.Tex0 = Input[1].Tex0;\n"
"   p.Tex1 = Input[1].Tex1;\n"
"   triStream.Append(p);\n"
"   p.Pos = Input[2].Pos.xyz;\n"
"   p.Nor = Input[2].Nor;\n"
"   p.Tan = Input[2].Tan;\n"
"   p.Tex0 = Input[2].Tex0;\n"
"   p.Tex1 = Input[2].Tex1;\n"
"   triStream.Append(p);\n"
"	triStream.RestartStrip();\n"
"}\n";
//********Edge***********//
//**************************************前がDS*************************************************//
