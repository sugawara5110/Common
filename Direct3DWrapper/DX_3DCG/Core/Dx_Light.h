//*****************************************************************************************//
//**                                                                                     **//
//**                   �@�@�@          Dx_Light                                          **//
//**                                                                                     **//
//*****************************************************************************************//

#ifndef Class_Dx_Light_Header
#define Class_Dx_Light_Header

#include "Dx_Device.h"

//�|�C���g���C�g
struct PointLight {
	CoordTf::VECTOR4 LightPos[LIGHT_PCS];   //xyz:Pos, w:�I���I�t
	CoordTf::VECTOR4 LightColor[LIGHT_PCS];//�F
	CoordTf::VECTOR4 Lightst[LIGHT_PCS];  //�����W, ����1, ����2, ����3
	int     LightPcs;     //���C�g��
};

//���s����
struct DirectionLight {
	CoordTf::VECTOR4 Direction;  //����
	CoordTf::VECTOR4 LightColor;//�F
	float onoff;
};

//�t�H�O
struct Fog {
	CoordTf::VECTOR4  FogColor;//�t�H�O�̐F
	float    Amount;  //�t�H�O��
	float    Density;//���x
	float    on_off;
};

class Dx_Light {

public:
	struct Update {
		PointLight plight = {};//���X�^���C�Y�p
		int lightNum = 0;
		Fog fog = {};//���X�^���C�Y�p
		DirectionLight dlight = {};//���X�^���C�Y, ���C�g�����p
	};

	static void Initialize();

	static void setGlobalAmbientLight(float r, float g, float b);

	static CoordTf::VECTOR4 getGlobalAmbientLight();

	static Update getUpdate(int index);

	static void ResetPointLight();

	static bool PointLightPosSet(int Idx, CoordTf::VECTOR3 pos, CoordTf::VECTOR4 Color, bool on_off,
		float range, CoordTf::VECTOR3 atten = { 0.01f, 0.001f, 0.001f });

	static void DirectionLight(float x, float y, float z, float r, float g, float b);
	static void SetDirectionLight(bool onoff);
	static void Fog(float r, float g, float b, float amount, float density, bool onoff);

private:
	static bool initialized;
	static Update upd[2];//cBuffSwap
	static CoordTf::VECTOR4 GlobalAmbientLight;

	Dx_Light() {};
	~Dx_Light() {};
	static void initCheck();
};

#endif
