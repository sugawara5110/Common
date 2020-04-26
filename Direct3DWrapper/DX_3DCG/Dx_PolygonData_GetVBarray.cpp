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
	d3varray = nullptr;
	d3varrayBC = nullptr;
	d3varrayI = nullptr;

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
	free(d3varray);
	d3varray = nullptr;
	free(d3varrayBC);
	d3varrayBC = nullptr;
	free(d3varrayI);
	d3varrayI = nullptr;
	S_DELETE(mObjectCB);
	S_DELETE(mObjectCB1);
}

ID3D12PipelineState* PolygonData::GetPipelineState() {
	return dpara.PSO.Get();
}

void PolygonData::SetVertex(int I1, int I2, int i,
	float vx, float vy, float vz,
	float nx, float ny, float nz,
	float u, float v) {
	d3varrayI[I1] = i;
	d3varrayI[I2] = i;
	d3varray[i].Pos.as(vx, vy, vz);
	d3varray[i].normal.as(nx, ny, nz);
	d3varray[i].geoNormal.as(nx, ny, nz);
	d3varray[i].tex.as(u, v);
}

void PolygonData::SetVertex(int I1, int i,
	float vx, float vy, float vz,
	float nx, float ny, float nz,
	float u, float v) {
	d3varrayI[I1] = i;
	d3varray[i].Pos.as(vx, vy, vz);
	d3varray[i].normal.as(nx, ny, nz);
	d3varray[i].geoNormal.as(nx, ny, nz);
	d3varray[i].tex.as(u, v);
}

void PolygonData::SetVertexBC(int I1, int I2, int i,
	float vx, float vy, float vz,
	float r, float g, float b, float a) {
	d3varrayI[I1] = i;
	d3varrayI[I2] = i;
	d3varrayBC[i].Pos.as(vx, vy, vz);
	d3varrayBC[i].color.as(r, g, b, a);
}

void PolygonData::SetVertexBC(int I1, int i,
	float vx, float vy, float vz,
	float r, float g, float b, float a) {
	d3varrayI[I1] = i;
	d3varrayBC[i].Pos.as(vx, vy, vz);
	d3varrayBC[i].color.as(r, g, b, a);
}

void PolygonData::GetVBarray(PrimitiveType type, int v) {

	primType_create = type;
	if (type == SQUARE) {
		dpara.TOPOLOGY = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		ver = v * 4;//v==四角形の個数
		verI = v * 6;
	}
	if (type == POINt) {
		dpara.TOPOLOGY = D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;
		ver = v;//v==点の個数
		verI = v;
	}
	if (type == LINE_L) {
		dpara.TOPOLOGY = D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
		ver = v * 2;//v==線の個数
		verI = v * 2;
	}
	if (type == LINE_S) {
		dpara.TOPOLOGY = D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP;
		ver = v * 2;//v==線の個数
		verI = v * 2;
	}
	if (type == CONTROL_POINT) {
		dpara.TOPOLOGY = D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST;
		ver = v * 4;//v==パッチの個数
		verI = v * 6;
	}

	d3varray = (VertexM*)malloc(sizeof(VertexM) * ver);
	d3varrayBC = (VertexBC*)malloc(sizeof(VertexBC) * ver);
	d3varrayI = (std::uint16_t*)malloc(sizeof(std::uint16_t) * verI);
	mObjectCB = new ConstantBuffer<CONSTANT_BUFFER>(1);
	mObjectCB1 = new ConstantBuffer<CONSTANT_BUFFER2>(1);
	dpara.NumMaterial = 1;
	dpara.material = std::make_unique<MY_MATERIAL_S[]>(dpara.NumMaterial);
	dpara.Vview = std::make_unique<VertexView>();
	dpara.Iview = std::make_unique<IndexView[]>(dpara.NumMaterial);
}

void PolygonData::GetShaderByteCode(bool light, int tNo) {
	dpara.material[0].diftex_no = tNo;
	bool disp = false;
	if (primType_create == CONTROL_POINT)disp = true;
	if (tNo == -1 && movOn[0].m_on == false) {
		vs = dx->pVertexShader_BC.Get();
		ps = dx->pPixelShader_BC.Get();
		return;
	}
	if (!disp && light) {
		vs = dx->pVertexShader_TC.Get();
		gs = dx->pGeometryShader_Before_vs.Get();
		ps = dx->pPixelShader_3D.Get();
		return;
	}
	if (!disp && !light) {
		vs = dx->pVertexShader_TC.Get();
		gs = dx->pGeometryShader_Before_vs.Get();
		ps = dx->pPixelShader_Emissive.Get();
		return;
	}
	if (disp && light) {
		vs = dx->pVertexShader_MESH_D.Get();
		ps = dx->pPixelShader_3D.Get();
		hs = dx->pHullShaderTriangle.Get();
		ds = dx->dx->pDomainShaderTriangle.Get();
		gs = dx->pGeometryShader_Before_ds.Get();
		return;
	}
	if (disp && !light) {
		vs = dx->pVertexShader_MESH_D.Get();
		ps = dx->pPixelShader_Emissive.Get();
		hs = dx->pHullShaderTriangle.Get();
		ds = dx->dx->pDomainShaderTriangle.Get();
		gs = dx->pGeometryShader_Before_ds.Get();
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

bool PolygonData::Create(bool light, int tNo, int nortNo, int spetNo, bool blend, bool alpha) {

	GetShaderByteCode(light, tNo);

	mObjectCB1->CopyData(0, sg);
	const int numTex = 3;
	const int numCB = 2;
	dpara.rootSignature = CreateRootSignature(numTex, numCB);
	if (dpara.rootSignature == nullptr)return false;

	dpara.material[0].diftex_no = tNo;
	dpara.material[0].nortex_no = nortNo;
	dpara.material[0].spetex_no = spetNo;

	TextureNo te;
	if (tNo < 0)te.diffuse = dx->GetTexNumber("dummyDifSpe.");
	else
		te.diffuse = tNo;

	if (nortNo < 0)te.normal = dx->GetTexNumber("dummyNor.");
	else
		te.normal = nortNo;

	if (spetNo < 0)te.specular = dx->GetTexNumber("dummyDifSpe.");
	else
		te.specular = spetNo;

	createTextureResource(0, 1, &te);
	dpara.numDesc = numTex + numCB;
	int numHeap = dpara.NumMaterial * dpara.numDesc;
	dpara.descHeap = CreateDescHeap(numHeap);
	if (dpara.descHeap == nullptr)return false;

	UINT cbSize[numCB] = {};
	cbSize[0] = mObjectCB->getSizeInBytes();
	cbSize[1] = mObjectCB1->getSizeInBytes();
	CreateSrvTexture(dpara.descHeap.Get(), 0, texture->GetAddressOf(), numTex);
	D3D12_GPU_VIRTUAL_ADDRESS ad[numCB];
	ad[0] = mObjectCB->Resource()->GetGPUVirtualAddress();
	ad[1] = mObjectCB1->Resource()->GetGPUVirtualAddress();
	CreateCbv(dpara.descHeap.Get(), numTex, ad, cbSize, numCB);

	UINT VertexSize;
	if (tNo == -1 && !movOn[0].m_on)
		VertexSize = sizeof(VertexBC);
	else
		VertexSize = sizeof(VertexM);

	const UINT vbByteSize = VertexSize * ver;
	const UINT ibByteSize = verI * sizeof(std::uint16_t);

	if (tNo == -1 && !movOn[0].m_on)
		dpara.Vview->VertexBufferGPU = dx->CreateDefaultBuffer(com_no, d3varrayBC, vbByteSize, dpara.Vview->VertexBufferUploader);
	else
		dpara.Vview->VertexBufferGPU = dx->CreateDefaultBuffer(com_no, d3varray, vbByteSize, dpara.Vview->VertexBufferUploader);

	dpara.Iview[0].IndexBufferGPU = dx->CreateDefaultBuffer(com_no, d3varrayI, ibByteSize, dpara.Iview[0].IndexBufferUploader);

	dpara.Vview->VertexByteStride = VertexSize;
	dpara.Vview->VertexBufferByteSize = vbByteSize;
	dpara.Iview[0].IndexFormat = DXGI_FORMAT_R16_UINT;
	dpara.Iview[0].IndexBufferByteSize = ibByteSize;
	dpara.Iview[0].IndexCount = verI;

	//パイプラインステートオブジェクト生成
	if (tNo == -1 && !movOn[0].m_on)
		dpara.PSO = CreatePsoVsHsDsPs(vs, hs, ds, ps, gs, dpara.rootSignature.Get(), dx->pVertexLayout_3DBC, alpha, blend, primType_create);
	else
		dpara.PSO = CreatePsoVsHsDsPs(vs, hs, ds, ps, gs, dpara.rootSignature.Get(), dx->pVertexLayout_MESH, alpha, blend, primType_create);

	if (dpara.PSO == nullptr)return false;

	return true;
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

void PolygonData::DrawOff() {
	DrawOn = false;
}

void PolygonData::Draw() {

	if (!UpOn | !DrawOn)return;

	mObjectCB->CopyData(0, cb[dx->cBuffSwap[1]]);
	dpara.insNum = insNum[dx->cBuffSwap[1]];
	drawsub(dpara);
}