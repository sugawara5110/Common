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
"   output.Nor = mul(input.Nor, (float3x3)g_World[input.instanceID]);\n"
"   output.Nor = normalize(output.Nor);\n"
"   output.Tex = input.Tex;\n"

"   return output;\n"
"}\n"
//**************************************�W�I���g���V�F�[�_�[********************************************************//
//***********************�O��DS****************************************************************//
"[maxvertexcount(3)]\n"
"void GS_Before_ds(triangle GS_Mesh_INPUT Input[3], inout TriangleStream <PS_INPUT> triStream)\n"
"{\n"
"	PS_INPUT p = (PS_INPUT)0;\n"
"   TangentBinormal tan = (TangentBinormal)0;\n"

//DS�Œ��_�ړ���̖@���Čv�Z
"   float3 vecX = Input[0].Pos.xyz - Input[1].Pos.xyz;\n"
"   float3 vecY = Input[0].Pos.xyz - Input[2].Pos.xyz;\n"
"   float3 vecNor = cross(vecX, vecY);\n"
"   Input[0].Nor = vecNor;\n"
"   Input[1].Nor = vecNor;\n"
"   Input[2].Nor = vecNor;\n"

//�ڃx�N�g���v�Z
"   tan = GetTangentBinormal(Input[0].Tex, Input[1].Tex, Input[2].Tex, \n"
"                            Input[0].Pos.xyz,  Input[1].Pos.xyz,  Input[2].Pos.xyz, \n"
"                            Input[0].instanceID);\n"

"   p = PsInput(Input[0]);\n"
"   p.tangent = tan.tangent;\n"
"   p.binormal = tan.binormal;\n"
"	triStream.Append(p);\n"
"   p = PsInput(Input[1]);\n"
"   p.tangent = tan.tangent;\n"
"   p.binormal = tan.binormal;\n"
"	triStream.Append(p);\n"
"   p = PsInput(Input[2]);\n"
"   p.tangent = tan.tangent;\n"
"   p.binormal = tan.binormal;\n"
"	triStream.Append(p);\n"

"	triStream.RestartStrip();\n"
"}\n"

//***********************�O��VS****************************************************************//
"[maxvertexcount(3)]\n"
"void GS_Before_vs(triangle GS_Mesh_INPUT Input[3], inout TriangleStream <PS_INPUT> triStream)\n"
"{\n"
"	PS_INPUT p = (PS_INPUT)0;\n"
"   TangentBinormal tan = (TangentBinormal)0;\n"

//�ڃx�N�g���v�Z
"   tan = GetTangentBinormal(Input[0].Tex, Input[1].Tex, Input[2].Tex, \n"
"                            Input[0].Pos.xyz,  Input[1].Pos.xyz,  Input[2].Pos.xyz, \n"
"                            Input[0].instanceID);\n"

"   p = PsInput(Input[0]);\n"
"   p.tangent = tan.tangent;\n"
"   p.binormal = tan.binormal;\n"
"	triStream.Append(p);\n"
"   p = PsInput(Input[1]);\n"
"   p.tangent = tan.tangent;\n"
"   p.binormal = tan.binormal;\n"
"	triStream.Append(p);\n"
"   p = PsInput(Input[2]);\n"
"   p.tangent = tan.tangent;\n"
"   p.binormal = tan.binormal;\n"
"	triStream.Append(p);\n"

"	triStream.RestartStrip();\n"
"}\n";
//**************************************�W�I���g���V�F�[�_�[********************************************************//