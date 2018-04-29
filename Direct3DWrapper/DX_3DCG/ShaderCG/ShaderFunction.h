///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                       ShaderFunction.hlsl                                             //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

char *ShaderFunction =
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
"}\n"
//////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////�|�C���g���C�g�v�Z(���C�g�������[�v�����Ďg�p����)////////////////////////////////
"float3 PointLightCom(float4 SpeculerCol, float4 Diffuse, float3 Nor, float4 shadow, float4 lightPos, float4 wPos, float4 lightSt, float4 lightCol, float4 CPos)\n"
"{\n"
//�o�͗pCol
"    float3 Col = { 0.0f, 0.0f, 0.0f };\n"
//���_��������܂ł̋������v�Z
"    float distance = length(lightPos.xyz - wPos.xyz);\n"

//���C�g�I�t, �����W�~3���O�͔�΂�
"    if (lightSt.w == 1.0f && distance < lightSt.x * 3){\n"

//���C�g�x�N�g��
"       float3 L = normalize(lightPos.xyz - wPos.xyz);\n"
"       float NL = saturate(dot(Nor, L));"

//�����x�N�g��
"       float3 EyeVec = normalize(CPos.xyz - wPos.xyz);\n"
//�X�؃L����
"       float3 Reflect = normalize(2 * NL * Nor - L);\n"
"       float3 specular = pow(saturate(dot(Reflect, EyeVec)), 4);\n"

//�f�t�H���g������
"       float attenuation = 2.0f;\n"
//�����W�O�����������K�p
"       if (distance > lightSt.x){ attenuation = lightSt.z; }\n"
//�����v�Z           
"       float r = lightSt.y / (pow(distance, attenuation) * 0.001f);\n"

//�@��,���C�g��������A�e�쐬
"       Col = (max(NL, shadow.x) * Diffuse.xyz * lightCol.xyz + specular * SpeculerCol.xyz) * r;\n"
"    }\n"
"    return Col;\n"
"}\n"
//////////////////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////���s�����v�Z/////////////////////////////////////////////////////////
"float3 DirectionalLightCom(float4 SpeculerCol, float4 Diffuse, float3 Nor, float4 DlightSt, float4 Dir, float4 DCol, float4 wPos, float4 CPos)\n"
"{\n"
//�o�͗pCol
"    float3 Col = { 0.0f, 0.0f, 0.0f };\n"

"    if(DlightSt.y == 1.0f)\n"
"    {\n"

//���C�g�x�N�g��
"       float3 L = normalize(Dir.xyz);\n"
"       float NL = max(saturate(dot(Nor, L)), DlightSt.z);\n"

//�����x�N�g��
"       float3 EyeVec = normalize(CPos.xyz - wPos.xyz);\n"
//�X�؃L����
"       float3 Reflect = normalize(2 * NL * Nor - L);\n"
"       float3 specular = pow(saturate(dot(Reflect, EyeVec)), 4);\n"

"       Col = DCol.xyz * DlightSt.x * Diffuse.xyz * NL + specular * SpeculerCol.xyz;\n"
"    }\n"
"    return Col;\n"
"}\n"
//////////////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////���ʃp�����[�^�[////////////////////////////////////////////////////////
"Texture2D g_texColor : register(t0);\n"
"Texture2D g_texNormal : register(t1);\n"
"SamplerState g_samLinear : register(s0);\n"

"cbuffer global : register(b0)\n"
"{\n"
"    matrix g_World[150]; \n"
"    matrix g_WVP[150];\n"
//���_
"    float4 g_C_Pos;\n"
//�I�u�W�F�N�g�ǉ��J���[
"    float4 g_ObjCol;\n"
//�����ʒu
"    float4 g_LightPos[150];\n"
//���C�g�F
"    float4 g_LightColor[150];\n"
//�����W, ���邳, �����̑傫��, �I���I�t
"    float4 g_Lightst[150];\n"
//�e�̉����lx, ���C�g��y
"    float4 g_ShadowLow_Lpcs;\n"
//���s��������
"    float4 g_DLightDirection;\n"
//���s�����F
"    float4 g_DLightColor;\n"
//���s�������邳x,�I���I�ty, �e�̉����lz
"    float4 g_DLightst;\n"
//�t�H�O��x, ���xy, onoffz
"    float4 g_FogAmo_Density;\n"
//�t�H�O�F
"    float4 g_FogColor;\n"
//�f�B�X�v�N����x
"    float4 g_DispAmount;\n"
//UV���W�ړ��p
"    float4 g_pXpYmXmY;\n"
"};\n"

//�}�e���A�����̐F
"cbuffer global_1 : register(b1)\n"
"{\n"
"    float4 g_Diffuse;\n"
"    float4 g_Speculer; \n"
"};\n"

//DS, PS�Ŏg�p
"struct PS_INPUT\n"
"{\n"
"    float4 Pos        : SV_POSITION;\n"
"    float4 wPos       : POSITION;\n"
"    float3 Nor        : NORMAL;\n"
"    float2 Tex        : TEXCOORD;\n"
"};\n";
/////////////////////////////////////////////////////////////////////////////////////////////////////////