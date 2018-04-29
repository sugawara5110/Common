///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                           ShaderDisp.hlsl                                             //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

//ShaderFunction.h�ɘA�������Ďg��
char *ShaderDisp =
"struct VS_OUTPUT_TC\n"
"{\n"
"    float3 Pos        : POSITION;\n"
"    float3 Nor        : NORMAL;\n"
"    float2 Tex        : TEXCOORD;\n"
"    uint   instanceID : SV_InstanceID;\n"
"};\n"

"struct HS_CONSTANT_OUTPUT\n"
"{\n"
"	 float factor[4]       : SV_TessFactor;\n"
"	 float inner_factor[2] : SV_InsideTessFactor;\n"
"};\n"

"struct HS_OUTPUT_TC\n"
"{\n"
"	 float3 Pos        : POSITION;\n"
"    float3 Nor        : NORMAL;\n"
"    float2 Tex        : TEXCOORD;\n"
"    uint   instanceID : SV_InstanceID;\n"
"};\n"

//*********************************************���_�V�F�[�_�[*******************************************************************//
"VS_OUTPUT_TC VSDisp(float3 Pos : POSITION, float3 Nor : NORMAL, float2 Tex : TEXCOORD, uint instanceID : SV_InstanceID)\n"
"{\n"
"    VS_OUTPUT_TC output = (VS_OUTPUT_TC)0;\n"
"    output.Pos = Pos;\n"
"    output.Nor = Nor;\n"
"    output.Tex = Tex;\n"
"    output.instanceID = instanceID;\n"
"    return output;\n"
"}\n"
//*********************************************���_�V�F�[�_�[*******************************************************************//

//***************************************�n���V�F�[�_�[�R���X�^���g*************************************************************//
"HS_CONSTANT_OUTPUT HSConstant(InputPatch<VS_OUTPUT_TC, 4> ip, uint pid : SV_PrimitiveID)\n"
"{\n"
"	HS_CONSTANT_OUTPUT output = (HS_CONSTANT_OUTPUT)0;\n"

//���[���h�ϊ�
"   float3 wPos = mul(ip[0].Pos, (float3x3)g_World[ip[0].instanceID]);\n"
//���_���猻�ݒn�܂ł̋������v�Z
"   float distance = length(g_C_Pos.xyz - wPos);\n"

//�����Ń|���S��������
"   float divide = 1;\n"
"   if(distance < 1000){divide = 2;}\n"
"   if(distance < 500){divide = 50;}\n"
"   if(distance < 300){divide = 80;}\n"

"	output.factor[0] = divide;\n"
"	output.factor[1] = divide;\n"
"	output.factor[2] = divide;\n"
"	output.factor[3] = divide;\n"
//u �c�̕������i���̃��C�������{�Ђ����j
"	output.inner_factor[0] = divide;\n"
//v
"	output.inner_factor[1] = divide;\n"

"	return output;\n"
"}\n"
//***************************************�n���V�F�[�_�[�R���X�^���g*************************************************************//

//***************************************�n���V�F�[�_�[*************************************************************************//
"[domain(\"quad\")]\n"
"[partitioning(\"integer\")]\n"
"[outputtopology(\"triangle_ccw\")]\n"
"[outputcontrolpoints(4)]\n"
"[patchconstantfunc(\"HSConstant\")]\n"
"HS_OUTPUT_TC HSDisp(InputPatch<VS_OUTPUT_TC, 4> ip, uint cpid : SV_OutputControlPointID, uint pid : SV_PrimitiveID)\n"
"{\n"
"	HS_OUTPUT_TC output;\n"
"	output.Pos = ip[cpid].Pos;\n"
"	output.Nor = ip[cpid].Nor;\n"
"	output.Tex = ip[cpid].Tex;\n"
"   output.instanceID = ip[cpid].instanceID;\n"
"	return output;\n"
"}\n"
//***************************************�n���V�F�[�_�[*************************************************************************//

//**************************************�h���C���V�F�[�_�[*********************************************************************//
"[domain(\"quad\")]\n"
"PS_INPUT DSDisp(HS_CONSTANT_OUTPUT In, float2 UV : SV_DomaInLocation, const OutputPatch<HS_OUTPUT_TC, 4> patch)\n"
"{\n"
"	PS_INPUT output = (PS_INPUT)0;\n"

//UV���W�v�Z
"   float2 top_uv = lerp(patch[0].Tex, patch[1].Tex, UV.x);\n"
"   float2 bottom_uv = lerp(patch[3].Tex, patch[2].Tex, UV.x);\n"
"   float2 uv = float2(lerp(top_uv, bottom_uv, UV.y));\n"
"   output.Tex = uv;\n"
//�摜���獂�����Z�o
"   float4 texheight = g_texColor.SampleLevel(g_samLinear, uv, 0);\n"
"   float4 height = texheight * g_DispAmount.x;\n"
"   float hei = (height.x + height.y + height.z) / 3;\n"
//pos���W�v�Z
"   float3 top_pos = lerp(patch[0].Pos, patch[1].Pos, UV.x);\n"
"   float3 bottom_pos = lerp(patch[3].Pos, patch[2].Pos, UV.x);\n"
"   output.Pos = float4(lerp(top_pos, bottom_pos, UV.y), 1);\n"
//�@���v�Z
"   float3 top_nor = lerp(patch[0].Nor, patch[1].Nor, UV.x);\n"
"   float3 bottom_nor = lerp(patch[3].Nor, patch[2].Nor, UV.x);\n"
"   float3 Normal = lerp(top_nor, bottom_nor, UV.y);\n"
//���[�J���@���̕�����hei�����_�ړ�
"   output.Pos.xyz += hei * Normal;\n"
//�摜���琶�������x�N�g���Ƀ��[�J���@���𑫂��@���x�N�g���Ƃ���
"   float3 nor1 = height.xyz + Normal;\n"
//���W�ϊ�
"   output.wPos = mul(output.Pos, g_World[patch[0].instanceID]);\n"
"   output.Pos = mul(output.Pos, g_WVP[patch[0].instanceID]);\n"
//�o�͂���@���̍쐬
"   output.Nor = mul(nor1, (float3x3)g_World[patch[0].instanceID]);\n"

"	return output;\n"
"}\n";
//**************************************�h���C���V�F�[�_�[*********************************************************************//
