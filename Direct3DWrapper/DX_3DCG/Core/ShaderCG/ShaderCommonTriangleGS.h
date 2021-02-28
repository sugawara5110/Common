///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                 ShaderCommonTriangleGS.hlsl                                           //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

char* ShaderCommonTriangleGS =
"PS_INPUT PsInput(GS_Mesh_INPUT input)\n"
"{\n"
"	PS_INPUT output = (PS_INPUT)0;\n"

"   output.wPos = mul(input.Pos, g_World[input.instanceID]);\n"
"   output.Pos = mul(input.Pos, g_WVP[input.instanceID]);\n"
"   output.Tex0 = input.Tex0;\n"
"   output.Tex1 = input.Tex1;\n"
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

"struct Normal3\n"
"{\n"
"    NormalTangent nor0;\n"
"    NormalTangent nor1;\n"
"    NormalTangent nor2;\n"
"};\n"
"Normal3 NormalRecalculation(GS_Mesh_INPUT Input[3])\n"
"{\n"
    //DSで頂点移動後の法線再計算
"   float3 vecX = Input[0].Pos.xyz - Input[1].Pos.xyz;\n"
"   float3 vecY = Input[0].Pos.xyz - Input[2].Pos.xyz;\n"
"   float3 vecNor = cross(vecX, vecY);\n"
"   Normal3 nor3;\n"
"   nor3.nor0.normal = Input[0].Nor * vecNor;\n"
"   nor3.nor1.normal = Input[1].Nor * vecNor;\n"
"   nor3.nor2.normal = Input[2].Nor * vecNor;\n"
"   nor3.nor0.tangent = Input[0].Tan * vecNor;\n"
"   nor3.nor1.tangent = Input[1].Tan * vecNor;\n"
"   nor3.nor2.tangent = Input[2].Tan * vecNor;\n"
"   return nor3;\n"
"}\n"
//********************************ジオメトリシェーダー***************************************************************//
//****************************前がDS,ノーマルマップ有**********************************************//
"[maxvertexcount(3)]\n"
"void GS_Before_ds(triangle GS_Mesh_INPUT Input[3], inout TriangleStream <PS_INPUT> triStream)\n"
"{\n"
"   NormalTangent tan[3];\n"

"   Normal3 n3 = NormalRecalculation(Input);\n"

//接ベクトル計算
"   tan[0] = GetTangent(n3.nor0.normal, (float3x3)g_World[Input[0].instanceID], n3.nor0.tangent);\n"
"   tan[1] = GetTangent(n3.nor1.normal, (float3x3)g_World[Input[1].instanceID], n3.nor1.tangent);\n"
"   tan[2] = GetTangent(n3.nor2.normal, (float3x3)g_World[Input[2].instanceID], n3.nor2.tangent);\n"

"   Stream(Input, triStream, tan);\n"
"}\n"
//****************************前がDS,ノーマルマップ無**********************************************//
"[maxvertexcount(3)]\n"
"void GS_Before_ds_NoNormalMap(triangle GS_Mesh_INPUT Input[3], inout TriangleStream <PS_INPUT> triStream)\n"
"{\n"
"   NormalTangent tan[3];\n"

"   Normal3 n3 = NormalRecalculation(Input);\n"

//接ベクトル計算無し
"   tan[0].normal = mul(n3.nor0.normal, (float3x3)g_World[Input[0].instanceID]);\n"
"   tan[1].normal = mul(n3.nor1.normal, (float3x3)g_World[Input[1].instanceID]);\n"
"   tan[2].normal = mul(n3.nor2.normal, (float3x3)g_World[Input[2].instanceID]);\n"
"   tan[0].tangent = float3(0.0f, 0.0f, 0.0f);\n"
"   tan[1].tangent = float3(0.0f, 0.0f, 0.0f);\n"
"   tan[2].tangent = float3(0.0f, 0.0f, 0.0f);\n"

"   Stream(Input, triStream, tan);\n"
"}\n"
//****************************前がVS,ノーマルマップ有**********************************************//
"[maxvertexcount(3)]\n"
"void GS_Before_vs(triangle GS_Mesh_INPUT Input[3], inout TriangleStream <PS_INPUT> triStream)\n"
"{\n"
"   NormalTangent tan[3];\n"

//接ベクトル計算
"   tan[0] = GetTangent(Input[0].Nor, (float3x3)g_World[Input[0].instanceID], Input[0].Tan);\n"
"   tan[1] = GetTangent(Input[1].Nor, (float3x3)g_World[Input[1].instanceID], Input[1].Tan);\n"
"   tan[2] = GetTangent(Input[2].Nor, (float3x3)g_World[Input[2].instanceID], Input[2].Tan);\n"

"   Stream(Input, triStream, tan);\n"
"}\n"
//****************************前がVS,ノーマルマップ無**********************************************//
"[maxvertexcount(3)]\n"
"void GS_Before_vs_NoNormalMap(triangle GS_Mesh_INPUT Input[3], inout TriangleStream <PS_INPUT> triStream)\n"
"{\n"
"   NormalTangent tan[3];\n"

//接ベクトル計算無し
"   tan[0].normal = mul(Input[0].Nor, (float3x3)g_World[Input[0].instanceID]);\n"
"   tan[1].normal = mul(Input[1].Nor, (float3x3)g_World[Input[1].instanceID]);\n"
"   tan[2].normal = mul(Input[2].Nor, (float3x3)g_World[Input[2].instanceID]);\n"
"   tan[0].tangent = float3(0.0f, 0.0f, 0.0f);\n"
"   tan[1].tangent = float3(0.0f, 0.0f, 0.0f);\n"
"   tan[2].tangent = float3(0.0f, 0.0f, 0.0f);\n"

"   Stream(Input, triStream, tan);\n"
"}\n";
//**************************************ジオメトリシェーダー********************************************************//