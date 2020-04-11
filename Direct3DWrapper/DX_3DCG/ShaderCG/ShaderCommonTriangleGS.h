///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                 ShaderCommonTriangleGS.hlsl                                           //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

//ShaderFunction.h�ɘA�������Ďg��
char* ShaderCommonTriangleGS =
"PS_INPUT PsInput(GS_Mesh_INPUT input)\n"
"{\n"
"	PS_INPUT output = (PS_INPUT)0;\n"

"   output.wPos = mul(input.Pos, g_World[input.instanceID]);\n"
"   output.Pos = mul(input.Pos, g_WVP[input.instanceID]);\n"
"   output.Tex = input.Tex;\n"

"   return output;\n"
"}\n"
//**************************************�W�I���g���V�F�[�_�[********************************************************//
//***********************�O��DS****************************************************************//
"[maxvertexcount(3)]\n"
"void GS_Before_ds(triangle GS_Mesh_INPUT Input[3], inout TriangleStream <PS_INPUT> triStream)\n"
"{\n"
"	PS_INPUT p = (PS_INPUT)0;\n"
"   NormalTangent tan0 = (NormalTangent)0;\n"
"   NormalTangent tan1 = (NormalTangent)0;\n"
"   NormalTangent tan2 = (NormalTangent)0;\n"

//DS�Œ��_�ړ���̖@���Čv�Z
"   float3 vecX = Input[0].Pos.xyz - Input[1].Pos.xyz;\n"
"   float3 vecY = Input[0].Pos.xyz - Input[2].Pos.xyz;\n"
"   float3 vecNor = cross(vecX, vecY);\n"
"   float3 nor0 = Input[0].Nor * vecNor;\n"
"   float3 nor1 = Input[1].Nor * vecNor;\n"
"   float3 nor2 = Input[2].Nor * vecNor;\n"

//�ڃx�N�g���v�Z
"   tan0 = GetTangent(nor0, Input[0].instanceID);\n"
"   tan1 = GetTangent(nor1, Input[1].instanceID);\n"
"   tan2 = GetTangent(nor2, Input[2].instanceID);\n"

"   p = PsInput(Input[0]);\n"
"   p.Nor = tan0.normal;\n"
"   p.tangent = tan0.tangent;\n"
"	triStream.Append(p);\n"
"   p = PsInput(Input[1]);\n"
"   p.Nor = tan1.normal;\n"
"   p.tangent = tan1.tangent;\n"
"	triStream.Append(p);\n"
"   p = PsInput(Input[2]);\n"
"   p.Nor = tan2.normal;\n"
"   p.tangent = tan2.tangent;\n"
"	triStream.Append(p);\n"

"	triStream.RestartStrip();\n"
"}\n"

//***********************�O��VS****************************************************************//
"[maxvertexcount(3)]\n"
"void GS_Before_vs(triangle GS_Mesh_INPUT Input[3], inout TriangleStream <PS_INPUT> triStream)\n"
"{\n"
"	PS_INPUT p = (PS_INPUT)0;\n"
"   NormalTangent tan0 = (NormalTangent)0;\n"
"   NormalTangent tan1 = (NormalTangent)0;\n"
"   NormalTangent tan2 = (NormalTangent)0;\n"

//�ڃx�N�g���v�Z
"   tan0 = GetTangent(Input[0].Nor, Input[0].instanceID);\n"
"   tan1 = GetTangent(Input[1].Nor, Input[1].instanceID);\n"
"   tan2 = GetTangent(Input[2].Nor, Input[2].instanceID);\n"

"   p = PsInput(Input[0]);\n"
"   p.Nor = tan0.normal;\n"
"   p.tangent = tan0.tangent;\n"
"	triStream.Append(p);\n"
"   p = PsInput(Input[1]);\n"
"   p.Nor = tan1.normal;\n"
"   p.tangent = tan1.tangent;\n"
"	triStream.Append(p);\n"
"   p = PsInput(Input[2]);\n"
"   p.Nor = tan2.normal;\n"
"   p.tangent = tan2.tangent;\n"
"	triStream.Append(p);\n"

"	triStream.RestartStrip();\n"
"}\n";
//**************************************�W�I���g���V�F�[�_�[********************************************************//