//*****************************************************************************************//
//**                                                                                     **//
//**                   Å@Å@Å@         DXR_Manager                                        **//
//**                                                                                     **//
//*****************************************************************************************//

#include "DXR_Manager.h"

DXR_Basic* DXR_Manager::dxr = nullptr;
int DXR_Manager::num_pdx = 0;
int DXR_Manager::num_type = 0;
std::vector<DXR_Manager::ParameterSet*> DXR_Manager::pset;
std::unique_ptr<ParameterDXR* []> DXR_Manager::Pdx = nullptr;
std::unique_ptr<MaterialType[]> DXR_Manager::Type = nullptr;

void DXR_Manager::setParameterDXR(ParameterSet* ps, ParameterDXR* pdx) {
	ps->pdx = pdx;
	pset.push_back(ps);
}

void DXR_Manager::psetErase(int numMesh, ParameterSet* psArr) {
	for (int i = 0; i < numMesh; i++) {
		for (int i1 = 0; i1 < pset.size(); i1++) {
			if (pset[i1] == &psArr[i]) {
				pset.erase(pset.begin() + i1);
				break;
			}
		}
	}
}

void DXR_Manager::EmissiveNoUpdate() {
	int EMISSIVE_Cnt = 0;
	for (int i = 0; i < (int)pset.size(); i++) {
		for (int i1 = 0; i1 < pset[i]->NumMaterial; i1++) {
			if (pset[i]->type[i1] == EMISSIVE) {
				pset[i]->EMISSIVE_NO[i1] = EMISSIVE_Cnt;
				EMISSIVE_Cnt += pset[i]->NumEmissive * pset[i]->insNum[Dx12Process::GetInstance()->cBuffSwap[0]];
			}
			else {
				pset[i]->EMISSIVE_NO[i1] = -1;
			}
		}
	}
}

void DXR_Manager::createDxrParameter(UINT maxRecursion) {
	Pdx.reset();
	Type.reset();
	Pdx = std::make_unique<ParameterDXR* []>(num_pdx);
	Type = std::make_unique<MaterialType[]>(num_type);
	S_DELETE(dxr);
	int pdxCnt = 0;
	int typeCnt = 0;
	int EMISSIVE_Cnt = 0;
	for (int i = 0; i < (int)pset.size(); i++) {
		pset[i]->pdx->resetCreateAS();
		Pdx[pdxCnt] = pset[i]->pdx;
		memcpy(&Type[typeCnt], pset[i]->type.get(), sizeof(MaterialType) * pset[i]->NumMaterial);
		pdxCnt++;
		typeCnt += pset[i]->NumMaterial;
	}
	DXR_Manager::ParameterSet** tmp = new DXR_Manager::ParameterSet * [pset.size()];
	for (int i = 0; i < (int)pset.size(); i++)tmp[i] = pset[i];

	int pSize = (int)pset.size();
	std::vector<DXR_Manager::ParameterSet*>().swap(DXR_Manager::pset);//âï˙

	for (int i = 0; i < pSize; i++)pset.push_back(tmp[i]);
	ARR_DELETE(tmp);

	dxr = new DXR_Basic();
	dxr->initDXR(num_pdx, Pdx.get(), Type.get(), maxRecursion);
}

bool DXR_Manager::PointLightPosSet(int Idx, bool on_off, float range, CoordTf::VECTOR3 atten) {
	EmissiveNoUpdate();
	return Dx12Process::GetInstance()->PointLightPosSet(Idx, { 0.0f,0.0f,0.0f }, { 0.0f,0.0f,0.0f,0.0f }, on_off, range, atten);
}

void DXR_Manager::DeleteInstance() {
	std::vector<DXR_Manager::ParameterSet*>().swap(DXR_Manager::pset);//âï˙
	S_DELETE(dxr);
}