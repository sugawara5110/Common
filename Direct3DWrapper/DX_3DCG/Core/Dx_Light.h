//*****************************************************************************************//
//**                                                                                     **//
//**                   　　　          Dx_Light                                          **//
//**                                                                                     **//
//*****************************************************************************************//

#ifndef Class_Dx_Light_Header
#define Class_Dx_Light_Header

#include "Dx_Device.h"

//ポイントライト
struct PointLight {
	CoordTf::VECTOR4 LightPos[LIGHT_PCS];   //xyz:Pos, w:オンオフ
	CoordTf::VECTOR4 LightColor[LIGHT_PCS];//色
	CoordTf::VECTOR4 Lightst[LIGHT_PCS];  //レンジ, 減衰1, 減衰2, 減衰3
	int     LightPcs;     //ライト個数
};

//平行光源
struct DirectionLight {
	CoordTf::VECTOR4 Direction;  //方向
	CoordTf::VECTOR4 LightColor;//色
	float onoff;
};

//フォグ
struct Fog {
	CoordTf::VECTOR4  FogColor;//フォグの色
	float    Amount;  //フォグ量
	float    Density;//密度
	float    on_off;
};

class Dx_Light {

public:
	struct Update {
		PointLight plight = {};//ラスタライズ用
		int lightNum = 0;
		Fog fog = {};//ラスタライズ用
		DirectionLight dlight = {};//ラスタライズ, レイトレ共用
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
