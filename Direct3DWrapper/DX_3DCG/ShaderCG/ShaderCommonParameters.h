///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                ShaderCommonParameters.hlsl                                            //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

char* ShaderCommonParameters =
//////////////////////////////////���ʃp�����[�^�[////////////////////////////////////////////////////////
"Texture2D g_texDiffuse : register(t0);\n"
"Texture2D g_texNormal : register(t1);\n"
"Texture2D g_texSpecular : register(t2);\n"
"SamplerState g_samLinear : register(s0);\n"

"cbuffer global : register(b0)\n"
"{\n"
"    matrix g_World[256]; \n"
"    matrix g_WVP[256];\n"
//���_
"    float4 g_C_Pos;\n"
//���_�����
"    float4 g_viewUp;"
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
"cbuffer global_1 : register(b1)\n"
"{\n"
"    float4 g_Diffuse;\n"
"    float4 g_Speculer; \n"
"    float4 g_Ambient;\n"
"    float4 g_uvSw;\n"
"};\n"

"cbuffer global_2 : register(b3)\n"
"{\n"
//DXR�p
"    float4 g_instanceID;\n"//x:ID, y:1.0f on 0.0f off
"};\n"

"struct VS_OUTPUT\n"
"{\n"
"    float4 Pos    : POSITION;\n"
"    float3 Nor    : NORMAL;\n"
"    float3 GNor   : GEO_NORMAL;\n"
"    float2 Tex0   : TEXCOORD0;\n"
"    float2 Tex1   : TEXCOORD1;\n"
"    uint   instanceID : SV_InstanceID;\n"
"};\n"

"struct GS_Mesh_INPUT\n"
"{\n"
"    float4 Pos   : POSITION;\n"
"    float3 Nor   : NORMAL;\n"
"    float2 Tex0  : TEXCOORD0;\n"
"    float2 Tex1  : TEXCOORD1;\n"
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
"    float3 GNor  : GEO_NORMAL;\n"
"    float2 Tex0  : TEXCOORD0;\n"
"    float2 Tex1  : TEXCOORD1;\n"
"    uint   instanceID : SV_InstanceID;\n"
"};\n"

/////////////////////////////////////////////////////////////////////////////////////////////////////////

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