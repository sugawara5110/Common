//*****************************************************************************************//
//**                                                                                     **//
//**                               PolygonData                                           **//
//**                                                                                     **//
//*****************************************************************************************//

#include "Dx_PolygonData.h"

PolygonData::PolygonData() {
}

PolygonData::~PolygonData() {
}

void PolygonData::GetVBarray(PrimitiveType type, int numMaxInstance) {

	primType_create = type;
	
	getBuffer(1, numMaxInstance);
}

void PolygonData::SetCol(CoordTf::VECTOR3 dif, CoordTf::VECTOR3 spe, CoordTf::VECTOR3 am) {
	sg.vDiffuse.x = dif.x;
	sg.vDiffuse.y = dif.y;
	sg.vDiffuse.z = dif.z;

	sg.vSpeculer.x = spe.x;
	sg.vSpeculer.y = spe.y;
	sg.vSpeculer.z = spe.z;

	sg.vAmbient.x = am.x;
	sg.vAmbient.y = am.y;
	sg.vAmbient.z = am.z;
}

bool PolygonData::Create(int comIndex, bool light, int tNo, bool blend, bool alpha) {
	return Create(comIndex, light, tNo, -1, -1, blend, alpha);
}

void PolygonData::setMaterialType(MaterialType type) {
	dxrPara.mType[0] = type;
}

void PolygonData::setPointLight(int InstanceIndex, bool on_off,
	float range, CoordTf::VECTOR3 atten) {

	Dx_Device* dev = Dx_Device::GetInstance();
	dxrPara.setPointLight(dev->dxrBuffSwapIndex(), 0, 0, InstanceIndex, on_off, range, atten);
}

void PolygonData::setPointLightAll(bool on_off,
	float range, CoordTf::VECTOR3 atten) {

	Dx_Device* dev = Dx_Device::GetInstance();
	dxrPara.setPointLightAll(dev->dxrBuffSwapIndex(), on_off, range, atten);
}

bool PolygonData::Create(int comIndex, bool light, int tNo, int nortNo, int spetNo, bool blend, bool alpha,
	bool smooth,
	float divideBufferMagnification) {

	dpara.material[0].diftex_no = tNo;
	dpara.material[0].nortex_no = nortNo;
	dpara.material[0].spetex_no = spetNo;
	GetShaderByteCode(primType_create, light, smooth, (tNo == -1 && (!movOn || !movOn[0].m_on)), nullptr, nullptr);
	mObjectCB2->CopyData(0, sg);

	UINT* indexCntArr = NEW UINT[dpara.NumMaterial];
	for (int m = 0; m < dpara.NumMaterial; m++) {
		indexCntArr[m] = dpara.Iview[m].IndexCount;
	}
	if (tNo >= 0 || (movOn && movOn[0].m_on)) {
		Dx_Util::createTangent(dpara.NumMaterial, indexCntArr,
			ver, index, sizeof(VertexM), 0, 3 * 4, 12 * 4, 6 * 4);
	}
	ARR_DELETE(indexCntArr);
	createDefaultBuffer(comIndex, ver, index);
	ARR_DELETE(index[0]);
	ARR_DELETE(index);

	if (!createTexResource(comIndex))return false;

	const int numSrvTex = 3;
	const int numCbv = 1;
	int numUav = 0;
	if (tNo == -1 && (!movOn || !movOn[0].m_on)) {
		VertexBC* v = (VertexBC*)ver;
		ARR_DELETE(v);
		if (!createPSO(Dx_ShaderHolder::pVertexLayout_3DBC, numSrvTex, numCbv, numUav, blend, alpha))return false;
	}
	else {
		VertexM* vm = (VertexM*)ver;
		ARR_DELETE(vm);
		if (!createPSO(Dx_ShaderHolder::pVertexLayout_MESH, numSrvTex, numCbv, numUav, blend, alpha))return false;

		if (Dx_Device::GetInstance()->getDxrCreateResourceState()) {
			setParameterDXR(alpha);
			if (!createStreamOutputResource(comIndex, divideBufferMagnification))return false;
			setColorDXR(0, sg);
			if (!createPSO_DXR(Dx_ShaderHolder::pVertexLayout_MESH, numSrvTex, numCbv, numUav, smooth))return false;
			setTextureDXR();
		}
	}

	return setDescHeap(numSrvTex, 0, nullptr, nullptr, numCbv, 0, 0);
}
