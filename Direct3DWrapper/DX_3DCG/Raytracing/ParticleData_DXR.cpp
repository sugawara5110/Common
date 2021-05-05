//*****************************************************************************************//
//**                                                                                     **//
//**                   Å@Å@Å@    ParticleData_DXR                                        **//
//**                                                                                     **//
//*****************************************************************************************//

#include "ParticleData_DXR.h"

void ParticleData_DXR::GetBufferParticle(int texture_no, float size, float density) {
	ParticleData::GetBufferParticle(texture_no, size, density);
	ps = new DXR_Manager::ParameterSet[1];
	ps[0].create(1, dxrPara.updateDXR[0].numVertex);
	ps[0].insNum = insnum;
}

void ParticleData_DXR::GetBufferBill(int v) {
	ParticleData::GetBufferBill(v);
	ps = new DXR_Manager::ParameterSet[1];
	ps[0].create(1, dxrPara.updateDXR[0].numVertex);
	ps[0].insNum = insnum;
}

void ParticleData_DXR::setMaterialType(MaterialType type) {
	ps[0].type[0] = type;
}

bool ParticleData_DXR::CreateParticle(int texNo, bool alpha, bool blend) {
	bool ret = ParticleData::CreateParticle(texNo, alpha, blend);
	DXR_Manager::setParameterDXR(&ps[0], getParameter());
	return ret;
}

bool ParticleData_DXR::CreateBillboard(bool alpha, bool blend) {
	bool ret = ParticleData::CreateBillboard(alpha, blend);
	DXR_Manager::setParameterDXR(&ps[0], getParameter());
	return ret;
}

int ParticleData_DXR::getEmissiveNo() {
	return ps[0].EMISSIVE_NO[0];
}

ParticleData_DXR::~ParticleData_DXR() {
	DXR_Manager::psetErase(1, ps);
	ARR_DELETE(ps);
}