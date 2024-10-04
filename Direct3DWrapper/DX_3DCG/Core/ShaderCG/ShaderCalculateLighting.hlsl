///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                ShaderCalculateLighting.hlsl                                           //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////���C�g�v�ZOutPut/////////////////////////////////////////////////////
struct LightOut
{
	float3 Diffuse : COLOR;
	float3 Speculer : COLOR;
};
/////////////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////�����o�[�g�g�U����/////////////////////////////////////////////////
float3 Lambert(float distAtten, float3 lightCol, float3 Diffuse, float3 Ambient, float3 Nor, float3 LVec)
{
//�p�x�������f�B�t�F�[�Y
//���ςŌv�Z����̂Ō����̃x�N�g�����t�ɂ���
	float angleAttenDif = saturate(dot(-LVec, Nor));

	return distAtten * lightCol * angleAttenDif * Diffuse + Ambient;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////�t�H�����ʔ���/////////////////////////////////////////////////////
float3 Phong(float distAtten, float3 lightCol, float3 SpeculerCol, float3 Nor, float3 LVec,
             float3 wPos, float3 eyePos, float shininess)
{
//�����x�N�g��
	float3 eyeVec = normalize(wPos - eyePos);
//���˃x�N�g��
	float3 reflectVec = normalize(reflect(LVec, Nor));
//�p�x�������X�y�L����
	float angleAttenSpe = pow(saturate(dot(reflectVec, -eyeVec)), shininess);

	return distAtten * lightCol * angleAttenSpe * SpeculerCol;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////���C�g�v�Z/////////////////////////////////////////////////////////
LightOut LightCom(float3 SpeculerCol, float3 Diffuse, float3 Ambient, float3 Nor,
                  float3 wPos, float3 lightCol, float3 eyePos, float3 lightVec, float distAtten, float shininess)
{
//�o�͗p
	LightOut Out = (LightOut) 0;

//���C�g�x�N�g�����K��
	float3 LVec = normalize(lightVec);

//�f�B�t�F�[�Y�o��
	Out.Diffuse = Lambert(distAtten, lightCol, Diffuse, Ambient, Nor, LVec);
//�X�y�L�����o��
	Out.Speculer = Phong(distAtten, lightCol, SpeculerCol, Nor, LVec, wPos, eyePos, shininess);
	return Out;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////�|�C���g���C�g�v�Z(���C�g�������[�v�����Ďg�p����)////////////////////////////////
LightOut PointLightCom(float3 SpeculerCol, float3 Diffuse, float3 Ambient, float3 Nor,
                       float4 lightPos, float3 wPos, float4 lightSt, float3 lightCol, float3 eyePos, float shininess)
{
//�o�͗p
	LightOut Out = (LightOut) 0;

//���C�g�x�N�g�� (���_�̈ʒu - �_�����̈ʒu)
	float3 lightVec = wPos - lightPos.xyz;

//���_��������܂ł̋������v�Z
	float distance = length(lightVec);

//���C�g�I�t, �����W���O�͔�΂�
	if (lightPos.w == 1.0f && distance < lightSt.x)
	{

//����������         
		float distAtten = 1.0f /
                        (lightSt.y +
                         lightSt.z * distance +
                         lightSt.w * distance * distance);

//�����v�Z
		Out = LightCom(SpeculerCol, Diffuse, Ambient, Nor,
                      wPos, lightCol, eyePos, lightVec, distAtten, shininess);

	}
	return Out;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////�|�C���g���C�g�v�Z, ������������///////////////////////////////////////////////////
LightOut PointLightComNoDistance(float3 SpeculerCol, float3 Diffuse, float3 Ambient, float3 Nor, 
                                 float4 lightPos, float3 wPos, float3 lightCol, float3 eyePos, float shininess)
{
//���C�g�x�N�g�� (���_�̈ʒu - �_�����̈ʒu)
	float3 lightVec = wPos - lightPos.xyz;

//����������         
	float distAtten = 1.0f;

//�����v�Z
	return LightCom(SpeculerCol, Diffuse, Ambient, Nor,
                    wPos, lightCol, eyePos, lightVec, distAtten, shininess);
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////���s�����v�Z/////////////////////////////////////////////////////////
LightOut DirectionalLightCom(float3 SpeculerCol, float3 Diffuse, float3 Ambient, float3 Nor, 
                             float4 DlightSt, float3 Dir, float3 DCol, float3 wPos, float3 eyePos, float shininess)
{
//�o�͗p
	LightOut Out = (LightOut) 0;

	if (DlightSt.x == 1.0f)
	{

//�����v�Z
		Out = LightCom(SpeculerCol, Diffuse, Ambient, Nor,
                      wPos, DCol, eyePos, Dir, 1.0f, shininess);

	}
	return Out;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////