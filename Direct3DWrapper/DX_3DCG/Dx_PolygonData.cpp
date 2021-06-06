//*****************************************************************************************//
//**                                                                                     **//
//**                   　　　      PolygonDataクラス                                     **//
//**                                                                                     **//
//*****************************************************************************************//

#include "Dx_PolygonData.h"

PolygonData::PolygonData() {
}

PolygonData::~PolygonData() {
}

ID3D12PipelineState* PolygonData::GetPipelineState() {
	return dpara.PSO[0].Get();
}

void PolygonData::GetVBarray(PrimitiveType type, int numMaxInstance) {

	primType_create = type;
	if (type == SQUARE) {
		dpara.TOPOLOGY = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	}
	if (type == POINt) {
		dpara.TOPOLOGY = D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;
	}
	if (type == LINE_L) {
		dpara.TOPOLOGY = D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
	}
	if (type == LINE_S) {
		dpara.TOPOLOGY = D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP;
	}
	if (type == CONTROL_POINT) {
		dpara.TOPOLOGY = D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST;
	}

	getBuffer(1, numMaxInstance);
}

void PolygonData::GetShaderByteCode(bool light, int tNo, bool smooth) {
	bool disp = false;
	Dx_ShaderHolder* sh = dx->shaderH.get();
	if (primType_create == CONTROL_POINT)disp = true;
	if (tNo == -1 && (!movOn || !movOn[0].m_on)) {
		vs = sh->pVertexShader_BC.Get();
		ps = sh->pPixelShader_BC.Get();
		ps_NoMap = sh->pPixelShader_BC.Get();
		return;
	}
	if (!disp && light) {
		vs = sh->pVertexShader_TC.Get();
		gs = sh->pGeometryShader_Before_vs.Get();
		gs_NoMap = sh->pGeometryShader_Before_vs_NoNormalMap.Get();
		ps = sh->pPixelShader_3D.Get();
		ps_NoMap = sh->pPixelShader_3D_NoNormalMap.Get();
		return;
	}
	if (!disp && !light) {
		vs = sh->pVertexShader_TC.Get();
		gs = sh->pGeometryShader_Before_vs.Get();
		gs_NoMap = sh->pGeometryShader_Before_vs_NoNormalMap.Get();
		ps = sh->pPixelShader_Emissive.Get();
		ps_NoMap = sh->pPixelShader_Emissive.Get();
		return;
	}
	if (disp && light) {
		vs = sh->pVertexShader_MESH_D.Get();
		ps = sh->pPixelShader_3D.Get();
		ps_NoMap = sh->pPixelShader_3D_NoNormalMap.Get();
		hs = sh->pHullShaderTriangle.Get();
		ds = sh->pDomainShaderTriangle.Get();
		if (smooth) {
			gs = sh->pGeometryShader_Before_ds_Smooth.Get();
			gs_NoMap = sh->pGeometryShader_Before_ds_NoNormalMap_Smooth.Get();
		}
		else {
			gs = sh->pGeometryShader_Before_ds_Edge.Get();
			gs_NoMap = sh->pGeometryShader_Before_ds_NoNormalMap_Edge.Get();
		}
		return;
	}
	if (disp && !light) {
		vs = sh->pVertexShader_MESH_D.Get();
		ps = sh->pPixelShader_Emissive.Get();
		ps_NoMap = sh->pPixelShader_Emissive.Get();
		hs = sh->pHullShaderTriangle.Get();
		ds = sh->pDomainShaderTriangle.Get();
		if (smooth) {
			gs = sh->pGeometryShader_Before_ds_Smooth.Get();
			gs_NoMap = sh->pGeometryShader_Before_ds_NoNormalMap_Smooth.Get();
		}
		else {
			gs = sh->pGeometryShader_Before_ds_Edge.Get();
			gs_NoMap = sh->pGeometryShader_Before_ds_NoNormalMap_Edge.Get();
		}
		return;
	}
}

void PolygonData::SetCol(float difR, float difG, float difB, float speR, float speG, float speB,
	float amR, float amG, float amB) {
	sg.vDiffuse.x = difR;
	sg.vDiffuse.y = difG;
	sg.vDiffuse.z = difB;

	sg.vSpeculer.x = speR;
	sg.vSpeculer.y = speG;
	sg.vSpeculer.z = speB;

	sg.vAmbient.x = amR;
	sg.vAmbient.y = amG;
	sg.vAmbient.z = amB;
}

bool PolygonData::Create(bool light, int tNo, bool blend, bool alpha) {
	return Create(light, tNo, -1, -1, blend, alpha);
}

void PolygonData::setMaterialType(MaterialType type) {
	dxrPara.mType[0] = type;
}

bool PolygonData::Create(bool light, int tNo, int nortNo, int spetNo, bool blend, bool alpha,
	bool smooth,
	float divideBufferMagnification) {

	dpara.material[0].diftex_no = tNo;
	dpara.material[0].nortex_no = nortNo;
	dpara.material[0].spetex_no = spetNo;
	GetShaderByteCode(light, tNo, smooth);
	mObjectCB1->CopyData(0, sg);

	UINT* indexCntArr = new UINT[dpara.NumMaterial];
	for (int m = 0; m < dpara.NumMaterial; m++) {
		indexCntArr[m] = dpara.Iview[m].IndexCount;
	}
	if (tNo >= 0 || (movOn && movOn[0].m_on)) {
		Dx_Util::createTangent(dpara.NumMaterial, indexCntArr,
			ver, index, sizeof(VertexM), 0, 12 * 4, 6 * 4);
	}
	ARR_DELETE(indexCntArr);
	createDefaultBuffer(ver, index);
	ARR_DELETE(index);
	createParameterDXR(alpha, blend, divideBufferMagnification);
	setColorDXR(0, sg);

	const int numSrvTex = 3;
	const int numCbv = 2;
	int numUav = 0;
	Dx_ShaderHolder* sh = dx->shaderH.get();
	if (tNo == -1 && (!movOn || !movOn[0].m_on)) {
		VertexBC* v = (VertexBC*)ver;
		ARR_DELETE(v);
		if (!createPSO(sh->pVertexLayout_3DBC, numSrvTex, numCbv, numUav, blend, alpha))return false;
	}
	else {
		VertexM* vm = (VertexM*)ver;
		ARR_DELETE(vm);
		if (!createPSO(sh->pVertexLayout_MESH, numSrvTex, numCbv, numUav, blend, alpha))return false;
		if (!createPSO_DXR(sh->pVertexLayout_MESH, numSrvTex, numCbv, numUav, smooth))return false;
	}

	return setDescHeap(numSrvTex, 0, nullptr, nullptr, numCbv, 0, 0);
}
