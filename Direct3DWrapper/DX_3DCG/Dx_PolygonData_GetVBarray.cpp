//*****************************************************************************************//
//**                                                                                     **//
//**                   　　　          PolygonDataクラス                                 **//
//**                                   GetVBarray関数                                    **//
//*****************************************************************************************//

#define _CRT_SECURE_NO_WARNINGS
#include "Dx12ProcessCore.h"

PolygonData::PolygonData() {
	dx = Dx12Process::GetInstance();
	mCommandList = dx->dx_sub[0].mCommandList.Get();

	sg.vDiffuse.x = 0.7f;
	sg.vDiffuse.y = 0.7f;
	sg.vDiffuse.z = 0.7f;
	sg.vDiffuse.w = 0.0f;

	sg.vSpeculer.x = 0.3f;
	sg.vSpeculer.y = 0.3f;
	sg.vSpeculer.z = 0.3f;
	sg.vSpeculer.w = 0.0f;

	sg.vAmbient.x = 0.0f;
	sg.vAmbient.y = 0.0f;
	sg.vAmbient.z = 0.0f;
	sg.vAmbient.w = 0.0f;

	divArr[0].distance = 1000.0f;
	divArr[0].divide = 2;
	divArr[1].distance = 500.0f;
	divArr[1].divide = 50;
	divArr[2].distance = 300.0f;
	divArr[2].divide = 100;
}

PolygonData::~PolygonData() {
	S_DELETE(mObjectCB);
	S_DELETE(mObjectCB1);
}

ID3D12PipelineState* PolygonData::GetPipelineState() {
	return dpara.PSO[0].Get();
}

void PolygonData::SetVertex(int I1, int I2, int i,
	float vx, float vy, float vz,
	float nx, float ny, float nz,
	float u, float v) {
	if (!ver) {
		ver = new VertexM[numVer];
		bcOn = false;
		getVertexBuffer(1);
		index = new UINT * [dpara.NumMaterial];
		index[0] = new UINT[numInd];
	}
	VertexM* vm = (VertexM*)ver;
	index[0][I1] = i;
	index[0][I2] = i;
	vm[i].Pos.as(vx, vy, vz);
	vm[i].normal.as(nx, ny, nz);
	vm[i].geoNormal.as(nx, ny, nz);
	vm[i].tex.as(u, v);
}

void PolygonData::SetVertex(int I1, int i,
	float vx, float vy, float vz,
	float nx, float ny, float nz,
	float u, float v) {
	if (!ver) {
		ver = new VertexM[numVer];
		bcOn = false;
		getVertexBuffer(1);
		index = new UINT * [dpara.NumMaterial];
		index[0] = new UINT[numInd];
	}
	VertexM* vm = (VertexM*)ver;
	index[0][I1] = i;
	vm[i].Pos.as(vx, vy, vz);
	vm[i].normal.as(nx, ny, nz);
	vm[i].geoNormal.as(nx, ny, nz);
	vm[i].tex.as(u, v);
}

void PolygonData::SetVertexBC(int I1, int I2, int i,
	float vx, float vy, float vz,
	float r, float g, float b, float a) {
	if (!ver) {
		ver = new VertexBC[numVer];
		bcOn = true;
		getVertexBuffer(1);
		index = new UINT * [dpara.NumMaterial];
		index[0] = new UINT[numInd];
	}
	VertexBC* v = (VertexBC*)ver;
	index[0][I1] = i;
	index[0][I2] = i;
	v[i].Pos.as(vx, vy, vz);
	v[i].color.as(r, g, b, a);
}

void PolygonData::SetVertexBC(int I1, int i,
	float vx, float vy, float vz,
	float r, float g, float b, float a) {
	if (!ver) {
		ver = new VertexBC[numVer];
		bcOn = true;
		getVertexBuffer(1);
		index = new UINT * [dpara.NumMaterial];
		index[0] = new UINT[numInd];
	}
	VertexBC* v = (VertexBC*)ver;
	index[0][I1] = i;
	v[i].Pos.as(vx, vy, vz);
	v[i].color.as(r, g, b, a);
}

void PolygonData::getBuffer() {
	mObjectCB = new ConstantBuffer<CONSTANT_BUFFER>(1);
}

void PolygonData::getVertexBuffer(int numMaterial) {
	dpara.NumMaterial = numMaterial;
	mObjectCB1 = new ConstantBuffer<CONSTANT_BUFFER2>(dpara.NumMaterial);
	dpara.material = std::make_unique<MY_MATERIAL_S[]>(dpara.NumMaterial);
	dpara.PSO = std::make_unique<ComPtr<ID3D12PipelineState>[]>(dpara.NumMaterial);
	dpara.Vview = std::make_unique<VertexView>();
	dpara.Iview = std::make_unique<IndexView[]>(dpara.NumMaterial);
}

void PolygonData::GetVBarray(PrimitiveType type, int v) {

	primType_create = type;
	if (type == SQUARE) {
		dpara.TOPOLOGY = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		numVer = v * 4;//v==四角形の個数
		numInd = v * 6;
	}
	if (type == POINt) {
		dpara.TOPOLOGY = D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;
		numVer = v;//v==点の個数
		numInd = v;
	}
	if (type == LINE_L) {
		dpara.TOPOLOGY = D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
		numVer = v * 2;//v==線の個数
		numInd = v * 2;
	}
	if (type == LINE_S) {
		dpara.TOPOLOGY = D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP;
		numVer = v * 2;//v==線の個数
		numInd = v * 2;
	}
	if (type == CONTROL_POINT) {
		dpara.TOPOLOGY = D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST;
		numVer = v * 4;//v==パッチの個数
		numInd = v * 6;
	}

	getBuffer();
}

void PolygonData::GetShaderByteCode(bool light, int tNo) {
	bool disp = false;
	if (primType_create == CONTROL_POINT)disp = true;
	if (tNo == -1 && movOn[0].m_on == false) {
		vs = dx->pVertexShader_BC.Get();
		ps = dx->pPixelShader_BC.Get();
		ps_NoMap = dx->pPixelShader_BC.Get();
		return;
	}
	if (!disp && light) {
		vs = dx->pVertexShader_TC.Get();
		gs = dx->pGeometryShader_Before_vs.Get();
		gs_NoMap = dx->pGeometryShader_Before_vs_NoNormalMap.Get();
		ps = dx->pPixelShader_3D.Get();
		ps_NoMap = dx->pPixelShader_3D_NoNormalMap.Get();
		return;
	}
	if (!disp && !light) {
		vs = dx->pVertexShader_TC.Get();
		gs = dx->pGeometryShader_Before_vs.Get();
		gs_NoMap = dx->pGeometryShader_Before_vs_NoNormalMap.Get();
		ps = dx->pPixelShader_Emissive.Get();
		ps_NoMap = dx->pPixelShader_Emissive.Get();
		return;
	}
	if (disp && light) {
		vs = dx->pVertexShader_MESH_D.Get();
		ps = dx->pPixelShader_3D.Get();
		ps_NoMap = dx->pPixelShader_3D_NoNormalMap.Get();
		hs = dx->pHullShaderTriangle.Get();
		ds = dx->dx->pDomainShaderTriangle.Get();
		gs = dx->pGeometryShader_Before_ds.Get();
		gs_NoMap = dx->pGeometryShader_Before_ds_NoNormalMap.Get();
		return;
	}
	if (disp && !light) {
		vs = dx->pVertexShader_MESH_D.Get();
		ps = dx->pPixelShader_Emissive.Get();
		ps_NoMap = dx->pPixelShader_Emissive.Get();
		hs = dx->pHullShaderTriangle.Get();
		ds = dx->dx->pDomainShaderTriangle.Get();
		gs = dx->pGeometryShader_Before_ds.Get();
		gs_NoMap = dx->pGeometryShader_Before_ds_NoNormalMap.Get();
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

bool PolygonData::createPSO(std::vector<D3D12_INPUT_ELEMENT_DESC>& vertexLayout,
	const int numSrv, const int numCbv, bool blend, bool alpha) {

	dpara.rootSignature = CreateRootSignature(numSrv, numCbv);
	if (dpara.rootSignature == nullptr)return false;

	//パイプラインステートオブジェクト生成
	for (int i = 0; i < dpara.NumMaterial; i++) {
		if (dpara.material[i].nortex_no < 0)
			dpara.PSO[i] = CreatePsoVsHsDsPs(vs, hs, ds, ps_NoMap, gs_NoMap, dpara.rootSignature.Get(),
				vertexLayout, alpha, blend, primType_create);
		else
			dpara.PSO[i] = CreatePsoVsHsDsPs(vs, hs, ds, ps, gs, dpara.rootSignature.Get(),
				vertexLayout, alpha, blend, primType_create);

		if (dpara.PSO[i] == nullptr)return false;
	}
	return true;
}

bool PolygonData::setDescHeap(const int numSrv, const int numCbv, D3D12_GPU_VIRTUAL_ADDRESS ad3, UINT ad3Size) {
	TextureNo* te = new TextureNo[dpara.NumMaterial];
	int tCnt = 0;
	for (int i = 0; i < dpara.NumMaterial; i++) {
		if (dpara.material[i].diftex_no < 0)te[tCnt].diffuse = dx->GetTexNumber("dummyDifSpe.");
		else
			te[tCnt].diffuse = dpara.material[i].diftex_no;

		if (dpara.material[i].nortex_no < 0)te[tCnt].normal = dx->GetTexNumber("dummyNor.");
		else
			te[tCnt].normal = dpara.material[i].nortex_no;

		if (dpara.material[i].spetex_no < 0)te[tCnt].specular = dx->GetTexNumber("dummyDifSpe.");
		else
			te[tCnt].specular = dpara.material[i].spetex_no;
		tCnt++;
	}
	createTextureResource(0, tCnt, te);
	dpara.numDesc = numSrv + numCbv;
	int numHeap = dpara.NumMaterial * dpara.numDesc;
	dpara.descHeap = CreateDescHeap(numHeap);
	ARR_DELETE(te);
	if (dpara.descHeap == nullptr)return false;
	const int numMaxCB = 3;
	UINT cbSize[numMaxCB] = {};
	cbSize[0] = mObjectCB->getSizeInBytes();
	cbSize[1] = mObjectCB1->getSizeInBytes();
	cbSize[2] = ad3Size;
	for (int i = 0; i < dpara.NumMaterial; i++) {
		CreateSrvTexture(dpara.descHeap.Get(), dpara.numDesc * i, texture[numSrv * i].GetAddressOf(), numSrv);
		D3D12_GPU_VIRTUAL_ADDRESS ad[numMaxCB];
		ad[0] = mObjectCB->Resource()->GetGPUVirtualAddress();
		ad[1] = mObjectCB1->Resource()->GetGPUVirtualAddress() + cbSize[1] * i;
		ad[2] = ad3;
		CreateCbv(dpara.descHeap.Get(), dpara.numDesc * i + numSrv, ad, cbSize, numCbv);
	}
	return true;
}

bool PolygonData::Create(bool light, int tNo, int nortNo, int spetNo, bool blend, bool alpha) {
	dpara.material[0].diftex_no = tNo;
	dpara.material[0].nortex_no = nortNo;
	dpara.material[0].spetex_no = spetNo;
	GetShaderByteCode(light, tNo);
	mObjectCB1->CopyData(0, sg);
	UINT VertexSize;
	if (tNo == -1 && !movOn[0].m_on)
		VertexSize = sizeof(VertexBC);
	else
		VertexSize = sizeof(VertexM);
	const UINT vbByteSize = VertexSize * numVer;
	const UINT ibByteSize = numInd * sizeof(UINT);
	dpara.Vview->VertexByteStride = VertexSize;
	dpara.Vview->VertexBufferByteSize = vbByteSize;
	dpara.Iview[0].IndexFormat = DXGI_FORMAT_R32_UINT;
	dpara.Iview[0].IndexBufferByteSize = ibByteSize;
	dpara.Iview[0].IndexCount = numInd;

	createDefaultBuffer(ver, index, true);
	const int numSrv = 3;
	const int numCbv = 2;

	if (tNo == -1 && !movOn[0].m_on) {
		if (!createPSO(dx->pVertexLayout_3DBC, numSrv, numCbv, blend, alpha))return false;
	}
	else {
		if (!createPSO(dx->pVertexLayout_MESH, numSrv, numCbv, blend, alpha))return false;
	}

	return setDescHeap(numSrv, numCbv, 0, 0);
}

void PolygonData::InstancedMap(float x, float y, float z, float theta, float sizeX, float sizeY, float sizeZ) {
	dx->InstancedMap(ins_no, &cb[dx->cBuffSwap[0]], x, y, z, theta, 0, 0, sizeX, sizeY, sizeZ);
}

void PolygonData::CbSwap() {
	if (!UpOn) {
		upCount++;
		if (upCount > 1)UpOn = true;//cb,2要素初回更新終了
	}
	insNum[dx->cBuffSwap[0]] = ins_no;
	ins_no = 0;
	DrawOn = true;
}

void PolygonData::InstanceUpdate(float r, float g, float b, float a, float disp, float shininess, float px, float py, float mx, float my) {
	dx->MatrixMap(&cb[dx->cBuffSwap[0]], r, g, b, a, disp, px, py, mx, my, divArr, numDiv, shininess);
	CbSwap();
}

void PolygonData::Update(float x, float y, float z, float r, float g, float b, float a, float theta, float disp, float shininess, float size, float px, float py, float mx, float my) {
	dx->InstancedMap(ins_no, &cb[dx->cBuffSwap[0]], x, y, z, theta, 0, 0, size);
	dx->MatrixMap(&cb[dx->cBuffSwap[0]], r, g, b, a, disp, px, py, mx, my, divArr, numDiv, shininess);
	CbSwap();
}

void PolygonData::update(int ind, float time, float x, float y, float z, float r, float g, float b, float a,
	float thetaZ, float thetaY, float thetaX, float size,
	DivideArr* divArr, int numDiv,
	float disp, float shininess) {
	dx->InstancedMap(ins_no, &cb[dx->cBuffSwap[0]], x, y, z, thetaZ, thetaY, thetaX, size);
	dx->MatrixMap(&cb[dx->cBuffSwap[0]], r, g, b, a, disp, 1.0f, 1.0f, 1.0f, 1.0f, divArr, numDiv, shininess);
	CbSwap();
}

void PolygonData::DrawOff() {
	DrawOn = false;
}

void PolygonData::Draw() {

	if (!UpOn | !DrawOn)return;

	mObjectCB->CopyData(0, cb[dx->cBuffSwap[1]]);
	dpara.insNum = insNum[dx->cBuffSwap[1]];
	drawsub(dpara);
}