///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                 ShaderCommonTriangleHSDS.hlsl                                         //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

char *ShaderCommonTriangleHSDS =
//***************************************�n���V�F�[�_�[�R���X�^���g*************************************************//
"HS_CONSTANT_OUTPUT HSConstant(InputPatch<VS_OUTPUT, 3> ip, uint pid : SV_PrimitiveID)\n"
"{\n"
"	HS_CONSTANT_OUTPUT output = (HS_CONSTANT_OUTPUT)0;\n"

//instanceID�؂�ւ�, DXR �e�Z���[�V�������̂�
"   uint instanceID = ip[0].instanceID;\n"
"   if(g_instanceID.y == 1.0f) instanceID = g_instanceID.x;\n"
//���[���h�ϊ�
"   float4 wPos = mul(ip[0].Pos, wvpCb[instanceID].world);\n"
//���_���猻�ݒn�܂ł̋������v�Z
"   float distance = length(g_C_Pos.xyz - wPos.xyz);\n"

//�����Ń|���S��������
"   float divide = 2.0f;\n"
"   for(int i = 0;i < g_DispAmount.y;i++){\n"
"      if(distance < g_divide[i].x){divide = g_divide[i].y;}\n"
"   }\n"

"	output.factor[0] = divide;\n"
"	output.factor[1] = divide;\n"
"	output.factor[2] = divide;\n"
//u �c�̕������i���̃��C�������{�Ђ����j
"	output.inner_factor = divide;\n"
//divide��2  ��   3 *  6���_
//divide��4  ��   3 * 24
//divide��8  ��   3 * 96
"	return output;\n"
"}\n"
//***************************************�n���V�F�[�_�[�R���X�^���g*************************************************//

//***************************************�n���V�F�[�_�[*************************************************************//
"[domain(\"tri\")]\n"
"[partitioning(\"integer\")]\n"
"[outputtopology(\"triangle_cw\")]\n"//���\cw, ccw
"[outputcontrolpoints(3)]\n"
"[patchconstantfunc(\"HSConstant\")]\n"
"HS_OUTPUT HS(InputPatch<VS_OUTPUT, 3> ip, uint cpid : SV_OutputControlPointID, uint pid : SV_PrimitiveID)\n"
"{\n"
"	HS_OUTPUT output = (HS_OUTPUT)0;\n"
"	output.Pos = ip[cpid].Pos;\n"
"	output.Nor = ip[cpid].Nor;\n"
"   output.Tan = ip[cpid].Tan;\n"
"	output.GNor = ip[cpid].GNor;\n"
"	output.Tex0 = ip[cpid].Tex0;\n"
"	output.Tex1 = ip[cpid].Tex1;\n"
"   output.instanceID = ip[cpid].instanceID;\n"
"	return output;\n"
"}\n"
//***************************************�n���V�F�[�_�[*************************************************************//

//**************************************�h���C���V�F�[�_�[**********************************************************//
//�O�p�`�͏d�S���W�n  (UV.x + UV.y + UV.z) == 1.0f �����藧��
"[domain(\"tri\")]\n"
"GS_Mesh_INPUT DS(HS_CONSTANT_OUTPUT In, float3 UV : SV_DomaInLocation, const OutputPatch<HS_OUTPUT, 3> patch)\n"
"{\n"
"	GS_Mesh_INPUT output = (GS_Mesh_INPUT)0;\n"

//UV���W�v�Z
"   output.Tex0 = patch[0].Tex0 * UV.x + patch[1].Tex0 * UV.y + patch[2].Tex0 * UV.z;\n"
"   output.Tex1 = patch[0].Tex1 * UV.x + patch[1].Tex1 * UV.y + patch[2].Tex1 * UV.z;\n"

//�摜���獂�����Z�o
"   float4 height = g_texDiffuse.SampleLevel(g_samLinear, output.Tex0, 0);\n"
"   float hei = (height.x + height.y + height.z) / 3 * g_DispAmount.x;\n"

//�@���x�N�g��
"   output.Nor = patch[0].Nor * UV.x + patch[1].Nor * UV.y + patch[2].Nor * UV.z;\n"

//�ڃx�N�g��
"   output.Tan = patch[0].Tan * UV.x + patch[1].Tan * UV.y + patch[2].Tan * UV.z;\n"

//pos���W�v�Z
"   output.Pos = patch[0].Pos * UV.x + patch[1].Pos * UV.y + patch[2].Pos * UV.z;\n"

//���[�J���@���̕�����hei�����_�ړ�(�R���g���[���|�C���g�ʒu�ŏ����𕪂���)
"   if(UV.x == 0.0f || UV.y == 0.0f || UV.z == 0.0f)\n"//�ǂꂩ�̗v�f��0.0f�̏ꍇ�[�ɗL����
"   {\n"
"      float3 geoDir = patch[0].GNor * UV.x + patch[1].GNor * UV.y + patch[2].GNor * UV.z;\n"
"      output.Pos.xyz += hei * geoDir;\n"//�[�̓W�I���g���@���g�p(�N���b�L���O�΍�)
"   }\n"
"   else\n"
"   {\n"
"      output.Pos.xyz += hei * output.Nor;\n"
"   }\n"

//Smooth�p
"   output.AddNor = NormalRecalculationSmoothPreparation(output.Tex0);\n"
"   output.AddNor = normalTexConvert(output.AddNor, output.Nor, output.Tan);\n"

"   output.instanceID = patch[0].instanceID;\n"

"	return output;\n"
"}\n";
//**************************************�h���C���V�F�[�_�[**********************************************************//
