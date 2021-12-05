///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                ShaderCommonParameters.hlsl                                            //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

char* ShaderCommonParameters =
//////////////////////////////////���ʃp�����[�^�[////////////////////////////////////////////////////////
"Texture2D g_texDiffuse : register(t0, space0);\n"
"Texture2D g_texNormal : register(t1, space0);\n"
"Texture2D g_texSpecular : register(t2, space0);\n"
"SamplerState g_samLinear : register(s0, space0);\n"

"struct WVPCB {\n"
"    matrix wvp;\n"
"    matrix world;\n"
"};\n"
"ConstantBuffer<WVPCB> wvpCb[] : register(b0, space1);\n"

"cbuffer global : register(b0, space0)\n"
"{\n"
//���_
"    float4 g_C_Pos;\n"
//�I�u�W�F�N�g�ǉ��J���[
"    float4 g_ObjCol;\n"
//�O���[�o���A���r�G���g
"    float4 g_GlobalAmbientLight;\n"
//xyz:�����ʒu, w:�I���I�t
"    float4 g_LightPos[256];\n"
//���C�g�F
"    float4 g_LightColor[256];\n"
//�����W, ����1, ����2, ����3
"    float4 g_Lightst[256];\n"
//x:���C�g��
"    float4 g_numLight;\n"
//���s��������
"    float4 g_DLightDirection;\n"
//���s�����F
"    float4 g_DLightColor;\n"
//x:���s�����I���I�t
"    float4 g_DLightst;\n"
//�t�H�O��x, ���xy, onoffz
"    float4 g_FogAmo_Density;\n"
//�t�H�O�F
"    float4 g_FogColor;\n"
//x:�f�B�X�v�N����, y:divide�z��, z:shininess
"    float4 g_DispAmount;\n"
//divide�z�� x:distance, y:divide
"    float4 g_divide[16];\n"
//UV���W�ړ��p
"    float4 g_pXpYmXmY;\n"
"};\n"

//�}�e���A�����̐F
"cbuffer global_1 : register(b1, space0)\n"
"{\n"
"    float4 g_Diffuse;\n"
"    float4 g_Speculer; \n"
"    float4 g_Ambient;\n"
"};\n"

"cbuffer global_2 : register(b3, space0)\n"
"{\n"
//DXR�p
"    float4 g_instanceID;\n"//x:ID, y:1.0f on 0.0f off
"};\n"

"struct VS_OUTPUT\n"
"{\n"
"    float4 Pos    : POSITION;\n"
"    float3 Nor    : NORMAL;\n"
"    float3 Tan    : TANGENT;\n"
"    float3 GNor   : GEO_NORMAL;\n"
"    float2 Tex0   : TEXCOORD0;\n"
"    float2 Tex1   : TEXCOORD1;\n"
"    uint   instanceID : SV_InstanceID;\n"
"};\n"

"struct GS_Mesh_INPUT\n"
"{\n"
"    float4 Pos   : POSITION;\n"
"    float3 Nor   : NORMAL;\n"
"    float3 Tan   : TANGENT;\n"
"    float2 Tex0  : TEXCOORD0;\n"
"    float2 Tex1  : TEXCOORD1;\n"
"    float3 AddPos: POSITION1;\n"
"    uint   instanceID : SV_InstanceID;\n"
"};\n"

"struct PS_INPUT\n"
"{\n"
"    float4 Pos      : SV_POSITION;\n"
"    float4 wPos     : POSITION;\n"
"    float3 Nor      : NORMAL;\n"
"    float2 Tex0     : TEXCOORD0;\n"
"    float2 Tex1     : TEXCOORD1;\n"
"    float3 tangent  : TANGENT;\n"
"};\n"

"struct HS_CONSTANT_OUTPUT\n"
"{\n"
"	 float factor[3]    : SV_TessFactor;\n"
"	 float inner_factor : SV_InsideTessFactor;\n"
"};\n"

"struct HS_OUTPUT\n"
"{\n"
"    float4 Pos   : POSITION;\n"
"    float3 Nor   : NORMAL;\n"
"    float3 Tan   : TANGENT;\n"
"    float3 GNor  : GEO_NORMAL;\n"
"    float2 Tex0  : TEXCOORD0;\n"
"    float2 Tex1  : TEXCOORD1;\n"
"    uint   instanceID : SV_InstanceID;\n"
"};\n"
//////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////DS��@���Čv�Z/////////////////////////////////////////////////////////
"GS_Mesh_INPUT NormalRecalculationSmooth(GS_Mesh_INPUT input)\n"
"{\n"
"    GS_Mesh_INPUT output;\n"

"    float3 add = input.AddPos;\n"

"    float3 pos = input.Pos.xyz + add;\n"
"    float3 tangent = input.Tan;\n"
"    float3 binormal = normalize(cross(input.Nor, tangent));\n"

"    float3 posT = (input.Pos.xyz + tangent * 0.05f) + add;\n"
"    float3 posB = (input.Pos.xyz + binormal * 0.05f) + add;\n"

"    float3 modifiedTangent = posT - pos;\n"
"    float3 modifiedBinormal = posB - pos;\n"
"    output.Nor = normalize(cross(modifiedTangent, modifiedBinormal));\n"

"    float3 differenceN = output.Nor - input.Nor;\n"
"    output.Tan = input.Tan + differenceN;\n"
"    output.Pos.xyz = pos;\n"
"    output.Pos.w = input.Pos.w;\n"
"    output.Tex0 = input.Tex0;\n"
"    output.Tex1 = input.Tex1;\n"
"    output.instanceID = input.instanceID;\n"

"    return output;\n"
"}\n"

"void NormalRecalculationEdge(inout GS_Mesh_INPUT Input[3])\n"
"{\n"
"   Input[0].Pos.xyz += Input[0].AddPos;\n"
"   Input[1].Pos.xyz += Input[1].AddPos;\n"
"   Input[2].Pos.xyz += Input[2].AddPos;\n"

"   float3 vecX = Input[0].Pos.xyz - Input[1].Pos.xyz;\n"
"   float3 vecY = Input[0].Pos.xyz - Input[2].Pos.xyz;\n"
"   float3 vecNor = cross(vecX, vecY);\n"

"    float3 differenceN[3];\n"
"    differenceN[0] = vecNor - Input[0].Nor;\n"
"    differenceN[1] = vecNor - Input[1].Nor;\n"
"    differenceN[2] = vecNor - Input[2].Nor;\n"

"    Input[0].Nor = vecNor;\n"
"    Input[1].Nor = vecNor;\n"
"    Input[2].Nor = vecNor;\n"
"    Input[0].Tan += differenceN[0];\n"
"    Input[1].Tan += differenceN[1];\n"
"    Input[2].Tan += differenceN[2];\n"
"}\n"
//////////////////////////////////////////////////////////////////////////////////////////////////////////
 
////////////////////////////////�t�H�O�v�Z(�e�N�X�`���ɑ΂��Čv�Z)////////////////////////////////////////
"float4 FogCom(float4 FogCol, float4 Fog, float4 CPos, float4 wPos, float4 Tex)\n"
"{\n"
"    float fd;\n"//����
"    float ff;\n"//�t�H�O�t�@�N�^�[
"    if(Fog.z == 1.0f){\n"
"       fd = length(CPos.xyz - wPos.xyz) * 0.01f;\n"//�����v�Z, 0.01�͕␳�l
"       ff = pow(2.71828, -fd * Fog.y);\n"//�t�H�O�t�@�N�^�[�v�Z(�ω���)
"       ff *= Fog.x;\n"//�t�H�O�S�̗̂�(���������������Ȃ�)
"       ff = saturate(ff);\n"
"       if(Tex.w > 0.3f){\n"
"         Tex = ff * Tex + (1.0f - ff) * FogCol;\n"
"       }\n"
"    }\n"
"   return Tex;\n"
"}\n";
//////////////////////////////////////////////////////////////////////////////////////////////////////////