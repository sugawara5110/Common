///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                       ShaderFunction.hlsl                                             //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

char *ShaderFunction =
//////////////////////////////////���ʃp�����[�^�[////////////////////////////////////////////////////////
"Texture2D g_texDiffuse : register(t0);\n"
"Texture2D g_texNormal : register(t1);\n"
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
//x:�f�B�X�v�N����, y:divide�z��
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
"};\n"

"struct VS_OUTPUT\n"
"{\n"
"    float4 Pos    : POSITION;\n"
"    float3 Nor    : NORMAL;\n"
"    float3 GNor   : GEO_NORMAL;\n"
"    float2 Tex        : TEXCOORD;\n"
"    uint   instanceID : SV_InstanceID;\n"
"};\n"

"struct GS_Mesh_INPUT\n"
"{\n"
"    float4 Pos   : POSITION;\n"
"    float3 Nor   : NORMAL;\n"
"    float2 Tex        : TEXCOORD;\n"
"    uint   instanceID : SV_InstanceID;\n"
"};\n"

"struct PS_INPUT\n"
"{\n"
"    float4 Pos      : SV_POSITION;\n"
"    float4 wPos     : POSITION;\n"
"    float3 Nor      : NORMAL;\n"
"    float2 Tex      : TEXCOORD0;\n"
"    float3 tangent  : TEXCOORD1;\n"
"    float3 binormal : TEXCOORD2;\n"
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
"}\n"
//////////////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////���C�g�v�Z/////////////////////////////////////////////////////////
"struct LightOut\n"
"{\n"
"    float3 Diffuse: COLOR;\n"
"    float3 Speculer: COLOR;\n"
"};\n"

"LightOut LightCom(float3 SpeculerCol, float3 Diffuse, float3 Ambient, float3 Nor, \n"
"                  float3 wPos, float3 lightCol, float3 eyePos, float3 lightVec, float distAtten)\n"
"{\n"
//�o�͗p
"    LightOut Out = (LightOut)0;\n"

//���C�g�x�N�g�����K��
"    float3 LVec = normalize(lightVec);\n"
//�p�x�������f�B�t�F�[�Y
"    float angleAttenDif = saturate(dot(LVec, Nor));"

//�����x�N�g��
"    float3 eyeVec = normalize(eyePos - wPos);\n"
//���˃x�N�g��
"    float3 reflectVec = reflect(-LVec, Nor);\n"
//�p�x�������X�y�L����
"    float angleAttenSpe = pow(saturate(dot(eyeVec, reflectVec)), 4);\n"//4:shininess

//�f�B�t�F�[�Y�o��
"    Out.Diffuse = distAtten * lightCol * (angleAttenDif * Diffuse + Ambient);\n"
//�X�y�L�����o��
"    Out.Speculer = distAtten * lightCol * angleAttenSpe * SpeculerCol;\n"
"    return Out;\n"
"}\n"
//////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////�|�C���g���C�g�v�Z(���C�g�������[�v�����Ďg�p����)////////////////////////////////
"LightOut PointLightCom(float3 SpeculerCol, float3 Diffuse, float3 Ambient, float3 Nor, \n"
"                       float4 lightPos, float3 wPos, float4 lightSt, float3 lightCol, float3 eyePos)\n"
"{\n"
//�o�͗p
"    LightOut Out = (LightOut)0;\n"

//���C�g�x�N�g��
"    float3 lightVec = lightPos.xyz - wPos;\n"

//���_��������܂ł̋������v�Z
"    float distance = length(lightVec);\n"

//���C�g�I�t, �����W���O�͔�΂�
"    if (lightPos.w == 1.0f && distance < lightSt.x){\n"

//����������         
"       float distAtten = 1.0f / \n"
"                        (lightSt.y + \n"
"                         lightSt.z * distance + \n"
"                         lightSt.w * distance * distance);\n"

//�����v�Z
"       Out = LightCom(SpeculerCol, Diffuse, Ambient, Nor, \n"
"                      wPos, lightCol, eyePos, lightVec, distAtten);\n"

"    }\n"
"    return Out;\n"
"}\n"
//////////////////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////���s�����v�Z/////////////////////////////////////////////////////////
"LightOut DirectionalLightCom(float3 SpeculerCol, float3 Diffuse, float3 Ambient, float3 Nor, \n"
"                             float4 DlightSt, float3 Dir, float3 DCol, float3 wPos, float3 eyePos)\n"
"{\n"
//�o�͗p
"    LightOut Out = (LightOut)0;\n"

"    if(DlightSt.x == 1.0f)\n"
"    {\n"

//�����v�Z
"       Out = LightCom(SpeculerCol, Diffuse, Ambient, Nor, \n"
"                      wPos, DCol, eyePos, -Dir, 1.0f);\n"

"    }\n"
"    return Out;\n"
"}\n"
//////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////Tangent/Binormal//////////////////////////////////////////////////////
"struct TangentBinormal\n"
"{\n"
"    float3 normal  : NORMAL;\n"
"    float3 tangent : TEXCOORD;\n"
"    float3 binormal: TEXCOORD;\n"
"};\n"

"TangentBinormal GetTangentBinormal(float3 normal, int instanceID)\n"
"{\n"
"	TangentBinormal Out = (TangentBinormal)0;\n"

"   Out.normal = mul(normal, (float3x3)g_World[instanceID]);\n"
"   Out.normal = normalize(Out.normal);\n"
"   Out.tangent = cross(Out.normal, g_viewUp.xyz);\n"
"   Out.binormal = cross(Out.normal, Out.tangent);\n"

"   return Out;\n"
"}\n"

"float3 GetNormal(float3 norTex, float3 normal, float3 tangent, float3 binormal)\n"
"{\n"
"   float3 norT = norTex * 2.0f - 1.0f;\n"
"   return tangent * norT.x + binormal * (-1.0f * norT.y) + normal * norT.z;\n"
"}\n";
//////////////////////////////////////////////////////////////////////////////////////////////////////////