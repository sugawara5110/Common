///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                ShaderCalculateLighting.hlsl                                           //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

char* ShaderCalculateLighting =
//////////////////////////////////////���C�g�v�Z/////////////////////////////////////////////////////////
"struct LightOut\n"
"{\n"
"    float3 Diffuse: COLOR;\n"
"    float3 Speculer: COLOR;\n"
"};\n"

"LightOut LightCom(float3 SpeculerCol, float3 Diffuse, float3 Ambient, float3 Nor, \n"
"                  float3 wPos, float3 lightCol, float3 eyePos, float3 lightVec, float distAtten, float shininess)\n"
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
"    float angleAttenSpe = pow(saturate(dot(eyeVec, reflectVec)), shininess);\n"

//�f�B�t�F�[�Y�o��
"    Out.Diffuse = distAtten * lightCol * angleAttenDif * Diffuse + Ambient;\n"
//�X�y�L�����o��
"    Out.Speculer = distAtten * lightCol * angleAttenSpe * SpeculerCol;\n"
"    return Out;\n"
"}\n"
//////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////�|�C���g���C�g�v�Z(���C�g�������[�v�����Ďg�p����)////////////////////////////////
"LightOut PointLightCom(float3 SpeculerCol, float3 Diffuse, float3 Ambient, float3 Nor, \n"
"                       float4 lightPos, float3 wPos, float4 lightSt, float3 lightCol, float3 eyePos, float shininess)\n"
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
"                      wPos, DCol, eyePos, -Dir, 1.0f, shininess);\n"

"    }\n"
"    return Out;\n"
"}\n";
////////////////////////////////////////////////////////////////////////////////////////////////////////