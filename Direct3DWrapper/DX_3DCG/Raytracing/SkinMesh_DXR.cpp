//*****************************************************************************************//
//**                                                                                     **//
//**                   Å@Å@Å@      SkinMesh_DXR                                          **//
//**                                                                                     **//
//*****************************************************************************************//

#include "SkinMesh_DXR.h"

void SkinMesh_DXR::GetBuffer(float end_frame, bool singleMesh, bool deformer) {
	SkinMesh::GetBuffer(end_frame, singleMesh, deformer);
	ps = new DXR_Manager::ParameterSet[numMesh];
	for (int i = 0; i < numMesh; i++) {
		ps[i].create(mObj[i].dpara.NumMaterial, mObj[i].dxrPara.updateDXR[0].numVertex);
		ps[i].insNum = insnum;
	}
}

void SkinMesh_DXR::setMaterialType(MaterialType type, int meshIndex, int materialIndex) {
	if (materialIndex == -1) {
		for (int i = 0; i < numMesh; i++) {
			for (int i1 = 0; i1 < mObj[i].dpara.NumMaterial; i1++) {
				ps[i].type[i1] = type;
			}
		}
		return;
	}
	ps[meshIndex].type[materialIndex] = type;
}

bool SkinMesh_DXR::CreateFromFBX(bool disp, float divideBufferMagnification) {
	bool ret = SkinMesh::CreateFromFBX(disp, divideBufferMagnification);
	for (int i = 0; i < numMesh; i++) {
		DXR_Manager::setParameterDXR(&ps[i], mObj[i].getParameter());
	}
	return ret;
}

bool SkinMesh_DXR::CreateFromFBX() {
	bool ret = SkinMesh::CreateFromFBX();
	for (int i = 0; i < numMesh; i++) {
		DXR_Manager::setParameterDXR(&ps[i], mObj[i].getParameter());
	}
	return ret;
}

int SkinMesh_DXR::getEmissiveNo(int meshIndex, int materialIndex) {
	return ps[meshIndex].EMISSIVE_NO[materialIndex];
}

SkinMesh_DXR::~SkinMesh_DXR() {
	DXR_Manager::psetErase(numMesh, ps);
	ARR_DELETE(ps);
}