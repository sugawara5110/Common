//*****************************************************************************************//
//**                                                                                     **//
//**                   　　　          Dx_Light                                          **//
//**                                                                                     **//
//*****************************************************************************************//

#include "Dx_Light.h"

bool Dx_Light::initialized = false;
Dx_Light::Update Dx_Light::upd[2] = {};//cBuffSwap
CoordTf::VECTOR4 Dx_Light::GlobalAmbientLight = { 0.1f,0.1f,0.1f,0.0f };

void Dx_Light::initCheck() {
	if (!initialized) {
		Dx_Util::ErrorMessage("Dx_Light: not initialized !!");
	}
}

void Dx_Light::ResetPointLight() {
	for (int i = 0; i < LIGHT_PCS; i++) {
		upd[0].plight.LightPos[i].as(0.0f, 0.0f, 0.0f, 0.0f);
		upd[0].plight.LightColor[i].as(0.0f, 0.0f, 0.0f, 0.0f);
		upd[0].plight.Lightst[i].as(0.0f, 1.0f, 0.001f, 0.001f);
		upd[1].plight.LightPos[i].as(0.0f, 0.0f, 0.0f, 0.0f);
		upd[1].plight.LightColor[i].as(0.0f, 0.0f, 0.0f, 0.0f);
		upd[1].plight.Lightst[i].as(0.0f, 1.0f, 0.001f, 0.001f);
	}
	upd[0].lightNum = 0;
	upd[1].lightNum = 0;
}

bool Dx_Light::PointLightPosSet(int Idx, CoordTf::VECTOR3 pos, CoordTf::VECTOR4 Color, bool on_off,
	float range, CoordTf::VECTOR3 atten) {

	initCheck();

	if (Idx > LIGHT_PCS - 1 || Idx < 0) {
		Dx_Util::ErrorMessage("lightNumの値が範囲外です");
		return false;
	}

	Dx_Device* dev = Dx_Device::GetInstance();
	Update& u = upd[dev->cBuffSwapUpdateIndex()];

	if (Idx + 1 > u.lightNum && on_off)u.lightNum = Idx + 1;

	float onoff;
	if (on_off)onoff = 1.0f; else onoff = 0.0f;
	u.plight.LightPos[Idx].as(pos.x, pos.y, pos.z, onoff);
	u.plight.LightColor[Idx].as(Color.x, Color.y, Color.z, Color.w);
	u.plight.Lightst[Idx].as(range, atten.x, atten.y, atten.z);
	u.plight.LightPcs = u.lightNum;

	return true;
}

void Dx_Light::DirectionLight(float x, float y, float z, float r, float g, float b) {

	initCheck();
	Dx_Device* dev = Dx_Device::GetInstance();
	Update& u = upd[dev->cBuffSwapUpdateIndex()];
	u.dlight.Direction.as(x, y, z, 0.0f);
	u.dlight.LightColor.as(r, g, b, 0.0f);
}

void Dx_Light::SetDirectionLight(bool onoff) {

	initCheck();
	float f = 0.0f;
	if (onoff)f = 1.0f;
	upd[0].dlight.onoff = f;
	upd[1].dlight.onoff = f;
}

void Dx_Light::Fog(float r, float g, float b, float amount, float density, bool onoff) {

	initCheck();
	Dx_Device* dev = Dx_Device::GetInstance();
	Update& u = upd[dev->cBuffSwapUpdateIndex()];

	if (!onoff) {
		u.fog.on_off = 0.0f;
		return;
	}
	u.fog.on_off = 1.0f;
	u.fog.FogColor.as(r, g, b, 1.0f);
	u.fog.Amount = amount;
	u.fog.Density = density;
}

void Dx_Light::setGlobalAmbientLight(float r, float g, float b) {
	GlobalAmbientLight.as(r, g, b, 0.0f);
}

CoordTf::VECTOR4 Dx_Light::getGlobalAmbientLight() {
	return GlobalAmbientLight;
}

Dx_Light::Update Dx_Light::getUpdate(int index) {
	initCheck();
	return upd[index];
}

void Dx_Light::Initialize() {
	//ポイントライト構造体初期化
	ResetPointLight();

	//平行光源初期化
	upd[0].dlight.Direction.as(0.0f, 0.0f, 0.0f, 0.0f);
	upd[0].dlight.LightColor.as(1.0f, 1.0f, 1.0f, 1.0f);
	upd[0].dlight.onoff = 0.0f;
	upd[1].dlight.Direction.as(0.0f, 0.0f, 0.0f, 0.0f);
	upd[1].dlight.LightColor.as(1.0f, 1.0f, 1.0f, 1.0f);
	upd[1].dlight.onoff = 0.0f;

	//フォグ初期化
	for (int i = 0; i < 2; i++) {
		upd[i].fog.FogColor.as(1.0f, 1.0f, 1.0f, 1.0f);
		upd[i].fog.Amount = 0.0f;
		upd[i].fog.Density = 0.0f;
		upd[i].fog.on_off = 0.0f;
	}

	initialized = true;
}
