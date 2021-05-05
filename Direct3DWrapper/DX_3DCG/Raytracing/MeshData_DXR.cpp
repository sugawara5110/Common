//*****************************************************************************************//
//**                                                                                     **//
//**                   Å@Å@Å@      MeshData_DXR                                          **//
//**                                                                                     **//
//*****************************************************************************************//

#include "MeshData_DXR.h"

bool MeshData_DXR::GetBuffer(char* FileName, int numMaxInstance) {
	bool ret = MeshData::GetBuffer(FileName, numMaxInstance);
	ps = new DXR_Manager::ParameterSet[1];
	ps[0].create(mObj.dpara.NumMaterial, mObj.dxrPara.updateDXR[0].numVertex);
	ps[0].insNum = mObj.insNum;
	return ret;
}

void MeshData_DXR::setMaterialType(MaterialType type, int materialIndex) {
	if (materialIndex == -1) {
		for (int i = 0; i < mObj.dpara.NumMaterial; i++) {
			ps[0].type[i] = type;
		}
		return;
	}
	ps[0].type[materialIndex] = type;
}

bool MeshData_DXR::CreateMesh(float divideBufferMagnification) {
	bool ret = MeshData::CreateMesh(divideBufferMagnification);
	DXR_Manager::setParameterDXR(&ps[0], getParameter());
	return ret;
}

int MeshData_DXR::getEmissiveNo(int materialIndex) {
	return ps[0].EMISSIVE_NO[materialIndex];
}

MeshData_DXR::~MeshData_DXR() {
	DXR_Manager::psetErase(1, ps);
	ARR_DELETE(ps);
}