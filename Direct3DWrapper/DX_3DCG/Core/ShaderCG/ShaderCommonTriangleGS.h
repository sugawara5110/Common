///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                 ShaderCommonTriangleGS.hlsl                                           //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

char* ShaderCommonTriangleGS =
"PS_INPUT PsInput(GS_Mesh_INPUT input)\n"
"{\n"
"	PS_INPUT output = (PS_INPUT)0;\n"

"   output.wPos = mul(input.Pos, wvpCb[input.instanceID].world);\n"
"   output.Pos = mul(input.Pos, wvpCb[input.instanceID].wvp);\n"
"   output.Tex0 = input.Tex0;\n"
"   output.Tex1 = input.Tex1;\n"
"   output.instanceID = input.instanceID;\n"
"   return output;\n"
"}\n"

"void Stream(GS_Mesh_INPUT Input[3], inout TriangleStream <PS_INPUT> triStream, NormalTangent tan[3])\n"
"{\n"
"	PS_INPUT p = (PS_INPUT)0;\n"

"   p = PsInput(Input[0]);\n"
"   p.Nor = tan[0].normal;\n"
"   p.tangent = tan[0].tangent;\n"
"   triStream.Append(p);\n"

"   p = PsInput(Input[1]);\n"
"   p.Nor = tan[1].normal;\n"
"   p.tangent = tan[1].tangent;\n"
"	triStream.Append(p);\n"

"   p = PsInput(Input[2]);\n"
"   p.Nor = tan[2].normal;\n"
"   p.tangent = tan[2].tangent;\n"
"	triStream.Append(p);\n"

"	triStream.RestartStrip();\n"
"}\n"
//********************************ジオメトリシェーダー***************************************************************//
//****************************前がDS,ノーマルマップ有**********************************************//
//********Smooth*********//
"[maxvertexcount(3)]\n"
"void GS_Before_ds_Smooth(triangle GS_Mesh_INPUT Input[3], inout TriangleStream <PS_INPUT> triStream)\n"
"{\n"
"   NormalTangent tan[3];\n"
"   GS_Mesh_INPUT recal[3];\n"

//DS後法線再計算
"   recal[0] = NormalRecalculationSmooth(Input[0]);\n"
"   recal[1] = NormalRecalculationSmooth(Input[1]);\n"
"   recal[2] = NormalRecalculationSmooth(Input[2]);\n"

//接ベクトル計算
"   tan[0] = GetTangent(recal[0].Nor, (float3x3)wvpCb[recal[0].instanceID].world, recal[0].Tan);\n"
"   tan[1] = GetTangent(recal[1].Nor, (float3x3)wvpCb[recal[1].instanceID].world, recal[1].Tan);\n"
"   tan[2] = GetTangent(recal[2].Nor, (float3x3)wvpCb[recal[2].instanceID].world, recal[2].Tan);\n"

"   Stream(recal, triStream, tan);\n"
"}\n"
//********Smooth*********//
//********Edge***********//
"[maxvertexcount(3)]\n"
"void GS_Before_ds_Edge(triangle GS_Mesh_INPUT Input[3], inout TriangleStream <PS_INPUT> triStream)\n"
"{\n"
"   NormalTangent tan[3];\n"

//DS後法線再計算
"   NormalRecalculationEdge(Input);\n"

//接ベクトル計算
"   tan[0] = GetTangent(Input[0].Nor, (float3x3)wvpCb[Input[0].instanceID].world, Input[0].Tan);\n"
"   tan[1] = GetTangent(Input[1].Nor, (float3x3)wvpCb[Input[1].instanceID].world, Input[1].Tan);\n"
"   tan[2] = GetTangent(Input[2].Nor, (float3x3)wvpCb[Input[2].instanceID].world, Input[2].Tan);\n"

"   Stream(Input, triStream, tan);\n"
"}\n"
//********Edge***********//
//****************************前がDS,ノーマルマップ無**********************************************//
//********Smooth*********//
"[maxvertexcount(3)]\n"
"void GS_Before_ds_NoNormalMap_Smooth(triangle GS_Mesh_INPUT Input[3], inout TriangleStream <PS_INPUT> triStream)\n"
"{\n"
"   NormalTangent tan[3];\n"
"   GS_Mesh_INPUT recal[3];\n"

//DS後法線再計算
"   recal[0] = NormalRecalculationSmooth(Input[0]);\n"
"   recal[1] = NormalRecalculationSmooth(Input[1]);\n"
"   recal[2] = NormalRecalculationSmooth(Input[2]);\n"

//接ベクトル計算無し
"   tan[0].normal = mul(recal[0].Nor, (float3x3)wvpCb[recal[0].instanceID].world);\n"
"   tan[1].normal = mul(recal[1].Nor, (float3x3)wvpCb[recal[1].instanceID].world);\n"
"   tan[2].normal = mul(recal[2].Nor, (float3x3)wvpCb[recal[2].instanceID].world);\n"
"   tan[0].tangent = float3(0.0f, 0.0f, 0.0f);\n"
"   tan[1].tangent = float3(0.0f, 0.0f, 0.0f);\n"
"   tan[2].tangent = float3(0.0f, 0.0f, 0.0f);\n"

"   Stream(recal, triStream, tan);\n"
"}\n"
//********Smooth*********//
//********Edge***********//
"[maxvertexcount(3)]\n"
"void GS_Before_ds_NoNormalMap_Edge(triangle GS_Mesh_INPUT Input[3], inout TriangleStream <PS_INPUT> triStream)\n"
"{\n"
"   NormalTangent tan[3];\n"

//DS後法線再計算
"   NormalRecalculationEdge(Input);\n"

//接ベクトル計算無し
"   tan[0].normal = mul(Input[0].Nor, (float3x3)wvpCb[Input[0].instanceID].world);\n"
"   tan[1].normal = mul(Input[1].Nor, (float3x3)wvpCb[Input[1].instanceID].world);\n"
"   tan[2].normal = mul(Input[2].Nor, (float3x3)wvpCb[Input[2].instanceID].world);\n"
"   tan[0].tangent = float3(0.0f, 0.0f, 0.0f);\n"
"   tan[1].tangent = float3(0.0f, 0.0f, 0.0f);\n"
"   tan[2].tangent = float3(0.0f, 0.0f, 0.0f);\n"

"   Stream(Input, triStream, tan);\n"
"}\n"
//********Edge***********//
//****************************前がVS,ノーマルマップ有**********************************************//
"[maxvertexcount(3)]\n"
"void GS_Before_vs(triangle GS_Mesh_INPUT Input[3], inout TriangleStream <PS_INPUT> triStream)\n"
"{\n"
"   NormalTangent tan[3];\n"

//接ベクトル計算
"   tan[0] = GetTangent(Input[0].Nor, (float3x3)wvpCb[Input[0].instanceID].world, Input[0].Tan);\n"
"   tan[1] = GetTangent(Input[1].Nor, (float3x3)wvpCb[Input[1].instanceID].world, Input[1].Tan);\n"
"   tan[2] = GetTangent(Input[2].Nor, (float3x3)wvpCb[Input[2].instanceID].world, Input[2].Tan);\n"

"   Stream(Input, triStream, tan);\n"
"}\n"
//****************************前がVS,ノーマルマップ無**********************************************//
"[maxvertexcount(3)]\n"
"void GS_Before_vs_NoNormalMap(triangle GS_Mesh_INPUT Input[3], inout TriangleStream <PS_INPUT> triStream)\n"
"{\n"
"   NormalTangent tan[3];\n"

//接ベクトル計算無し
"   tan[0].normal = mul(Input[0].Nor, (float3x3)wvpCb[Input[0].instanceID].world);\n"
"   tan[1].normal = mul(Input[1].Nor, (float3x3)wvpCb[Input[1].instanceID].world);\n"
"   tan[2].normal = mul(Input[2].Nor, (float3x3)wvpCb[Input[2].instanceID].world);\n"
"   tan[0].tangent = float3(0.0f, 0.0f, 0.0f);\n"
"   tan[1].tangent = float3(0.0f, 0.0f, 0.0f);\n"
"   tan[2].tangent = float3(0.0f, 0.0f, 0.0f);\n"

"   Stream(Input, triStream, tan);\n"
"}\n";
//**************************************ジオメトリシェーダー********************************************************//