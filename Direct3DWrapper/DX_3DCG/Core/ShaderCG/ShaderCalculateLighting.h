///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                ShaderCalculateLighting.hlsl                                           //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

char* ShaderCalculateLighting =
//////////////////////////////////////���C�g�v�ZOutPut/////////////////////////////////////////////////////
"struct LightOut\n"
"{\n"
"    float3 Diffuse: COLOR;\n"
"    float3 Speculer: COLOR;\n"
"};\n"
/////////////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////�����o�[�g�g�U����/////////////////////////////////////////////////
"float3 Lambert(float distAtten, float3 lightCol, float3 Diffuse, float3 Ambient, float3 Nor, float3 LVec)\n"
"{\n"
//�p�x�������f�B�t�F�[�Y
//���ςŌv�Z����̂Ō����̃x�N�g�����t�ɂ���
"    float angleAttenDif = saturate(dot(-LVec, Nor));\n"

"    return distAtten * lightCol * angleAttenDif * Diffuse + Ambient;\n"
"}\n"
/////////////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////�t�H�����ʔ���/////////////////////////////////////////////////////
"float3 Phong(float distAtten, float3 lightCol, float3 SpeculerCol, float3 Nor, float3 LVec, \n"
"             float3 wPos, float3 eyePos, float shininess)\n"
"{\n"
//�����x�N�g��
"    float3 eyeVec = normalize(wPos - eyePos);\n"
//���˃x�N�g��
"    float3 reflectVec = normalize(reflect(LVec, Nor));\n"
//�p�x�������X�y�L����
"    float angleAttenSpe = pow(saturate(dot(reflectVec, -eyeVec)), shininess);\n"

"    return distAtten * lightCol * angleAttenSpe * SpeculerCol;\n"
"}\n"
/////////////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////���C�g�v�Z/////////////////////////////////////////////////////////
"LightOut LightCom(float3 SpeculerCol, float3 Diffuse, float3 Ambient, float3 Nor, \n"
"                  float3 wPos, float3 lightCol, float3 eyePos, float3 lightVec, float distAtten, float shininess)\n"
"{\n"
//�o�͗p
"    LightOut Out = (LightOut)0;\n"

//���C�g�x�N�g�����K��
"    float3 LVec = normalize(lightVec);\n"

//�f�B�t�F�[�Y�o��
"    Out.Diffuse = Lambert(distAtten, lightCol, Diffuse, Ambient, Nor, LVec);\n"
//�X�y�L�����o��
"    Out.Speculer = Phong(distAtten, lightCol, SpeculerCol, Nor, LVec, wPos, eyePos, shininess);\n"
"    return Out;\n"
"}\n"
//////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////�|�C���g���C�g�v�Z(���C�g�������[�v�����Ďg�p����)////////////////////////////////
"LightOut PointLightCom(float3 SpeculerCol, float3 Diffuse, float3 Ambient, float3 Nor, \n"
"                       float4 lightPos, float3 wPos, float4 lightSt, float3 lightCol, float3 eyePos, float shininess)\n"
"{\n"
//�o�͗p
"    LightOut Out = (LightOut)0;\n"

//���C�g�x�N�g�� (���_�̈ʒu - �_�����̈ʒu)
"    float3 lightVec = wPos - lightPos.xyz;\n"

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
"                      wPos, lightCol, eyePos, lightVec, distAtten, shininess);\n"

"    }\n"
"    return Out;\n"
"}\n"
//////////////////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////���s�����v�Z/////////////////////////////////////////////////////////
"LightOut DirectionalLightCom(float3 SpeculerCol, float3 Diffuse, float3 Ambient, float3 Nor, \n"
"                             float4 DlightSt, float3 Dir, float3 DCol, float3 wPos, float3 eyePos, float shininess)\n"
"{\n"
//�o�͗p
"    LightOut Out = (LightOut)0;\n"

"    if(DlightSt.x == 1.0f)\n"
"    {\n"

//�����v�Z
"       Out = LightCom(SpeculerCol, Diffuse, Ambient, Nor, \n"
"                      wPos, DCol, eyePos, Dir, 1.0f, shininess);\n"

"    }\n"
"    return Out;\n"
"}\n";
////////////////////////////////////////////////////////////////////////////////////////////////////////