//*****************************************************************************************//
//**                                                                                     **//
//**                   Å@Å@Å@      PolygonData_DXR                                       **//
//**                                                                                     **//
//*****************************************************************************************//

#include "PolygonData_DXR.h"

void PolygonData_DXR::GetVBarray(PrimitiveType type, int numMaxInstance) {
	PolygonData::GetVBarray(type, numMaxInstance);
	ps = new DXR_Manager::ParameterSet[1];
	ps[0].create(dpara.NumMaterial, dxrPara.updateDXR[0].numVertex);
	ps[0].insNum = insNum;
}

void PolygonData_DXR::setMaterialType(MaterialType type) {
	ps[0].type[0] = type;
}

bool PolygonData_DXR::Create(bool light, int tNo, bool blend, bool alpha) {
	bool ret = PolygonData::Create(light, tNo, blend, alpha);
	DXR_Manager::setParameterDXR(&ps[0], getParameter());
	return ret;
}

bool PolygonData_DXR::Create(bool light, int tNo, int nortNo, int spetNo, bool blend, bool alpha,
	float divideBufferMagnification) {

	bool ret = PolygonData::Create(light, tNo, nortNo, spetNo, blend, alpha, divideBufferMagnification);
	DXR_Manager::setParameterDXR(&ps[0], getParameter());
	return ret;
}

int PolygonData_DXR::getEmissiveNo() {
	return ps[0].EMISSIVE_NO[0];
}

PolygonData_DXR::~PolygonData_DXR() {
	DXR_Manager::psetErase(1, ps);
	ARR_DELETE(ps);
}