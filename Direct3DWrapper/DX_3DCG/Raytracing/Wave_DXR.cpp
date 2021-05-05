//*****************************************************************************************//
//**                                                                                     **//
//**                   Å@Å@Å@         Wave_DXR                                           **//
//**                                                                                     **//
//*****************************************************************************************//

#include "Wave_DXR.h"

void Wave_DXR::GetVBarray(int numMaxInstance) {
	Wave::GetVBarray(numMaxInstance);
	ps = new DXR_Manager::ParameterSet[1];
	ps[0].create(mObj.dpara.NumMaterial, mObj.dxrPara.updateDXR[0].numVertex);
	ps[0].insNum = mObj.insNum;
}

void Wave_DXR::setMaterialType(MaterialType type) {
	ps[0].type[0] = type;
}

bool Wave_DXR::Create(int texNo, bool blend, bool alpha, float waveHeight, float divide) {
	bool ret = Wave::Create(texNo, blend, alpha, waveHeight, divide);
	DXR_Manager::setParameterDXR(&ps[0], getParameter());
	return ret;
}

bool Wave_DXR::Create(int texNo, int nortNo, bool blend, bool alpha, float waveHeight, float divide) {
	bool ret = Wave::Create(texNo, nortNo, blend, alpha, waveHeight, divide);
	DXR_Manager::setParameterDXR(&ps[0], getParameter());
	return ret;
}

int Wave_DXR::getEmissiveNo() {
	return ps[0].EMISSIVE_NO[0];
}

Wave_DXR::~Wave_DXR() {
	DXR_Manager::psetErase(1, ps);
	ARR_DELETE(ps);
}