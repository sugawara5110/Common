//*****************************************************************************************//
//**                                                                                     **//
//**                   　　　          PolygonDataクラス                                 **//
//**                                   GetVBarray関数                                    **//
//*****************************************************************************************//

#define _CRT_SECURE_NO_WARNINGS
#include "Dx12ProcessCore.h"

std::mutex PolygonData::mtx;

PolygonData::PolygonData() {
	dx = Dx12Process::GetInstance();
	mCommandList = dx->dx_sub[0].mCommandList.Get();
	d3varray = nullptr;
	d3varrayBC = nullptr;
	d3varrayI = nullptr;

	sg.vDiffuse.x = 1.0f;
	sg.vDiffuse.y = 1.0f;
	sg.vDiffuse.z = 1.0f;
	sg.vDiffuse.w = 1.0f;
	sg.vSpeculer.x = 1.0f;
	sg.vSpeculer.y = 1.0f;
	sg.vSpeculer.z = 1.0f;
	sg.vSpeculer.w = 1.0f;
}

PolygonData::~PolygonData() {
	free(d3varray);
	d3varray = nullptr;
	free(d3varrayBC);
	d3varrayBC = nullptr;
	free(d3varrayI);
	d3varrayI = nullptr;
	S_DELETE(mObjectCB);
	RELEASE(texture);
	RELEASE(textureUp);
}

ID3D12PipelineState *PolygonData::GetPipelineState() {
	return mPSO.Get();
}

void PolygonData::SetVertex(int I1, int I2, int i,
	float vx, float vy, float vz,
	float nx, float ny, float nz,
	float u, float v) {
	d3varrayI[I1] = i;
	d3varrayI[I2] = i;
	d3varray[i].Pos.as(vx, vy, vz);
	d3varray[i].normal.as(nx, ny, nz);
	d3varray[i].tex.as(u, v);
}

void PolygonData::SetVertex(int I1, int i,
	float vx, float vy, float vz,
	float nx, float ny, float nz,
	float u, float v) {
	d3varrayI[I1] = i;
	d3varray[i].Pos.as(vx, vy, vz);
	d3varray[i].normal.as(nx, ny, nz);
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
		primType_draw = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		ver = v * 4;//v==四角形の個数
		verI = v * 6;
	}
	if (type == POINt) {
		primType_draw = D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;
		ver = v;//v==点の個数
		verI = v;
	}
	if (type == LINE_L) {
		primType_draw = D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
		ver = v * 2;//v==線の個数
		verI = v * 2;
	}
	if (type == LINE_S) {
		primType_draw = D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP;
		ver = v * 2;//v==線の個数
		verI = v * 2;
	}
	if (type == CONTROL_POINT) {
		primType_draw = D3D11_PRIMITIVE_TOPOLOGY_4_CONTROL_POINT_PATCHLIST;
		ver = v * 4;//v==パッチの個数
		verI = v * 4;
	}

	d3varray = (Vertex*)malloc(sizeof(Vertex) * ver);
	d3varrayBC = (VertexBC*)malloc(sizeof(VertexBC) * ver);
	d3varrayI = (std::uint16_t*)malloc(sizeof(std::uint16_t) * verI);
	mObjectCB = new ConstantBuffer<CONSTANT_BUFFER>(1);
	mObjectCB1 = new ConstantBuffer<CONSTANT_BUFFER2>(1);
	Vview = std::make_unique<VertexView>();
	Iview = std::make_unique<IndexView[]>(1);
}

void PolygonData::GetShaderByteCode(bool light, int tNo, int nortNo) {
	material[0].tex_no = tNo;
	bool disp = false;
	if (primType_create == D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH)disp = true;
	if (tNo == -1 && m_on == false) {
		vs = dx->pVertexShader_BC.Get();
		ps = dx->pPixelShader_BC.Get();
		return;
	}
	if (!disp && light) {
		vs = dx->pVertexShader_TC.Get();
		ps = dx->pPixelShader_3D.Get();
		return;
	}
	if (!disp && !light) {
		vs = dx->pVertexShader_TC.Get();
		ps = dx->pPixelShader_Emissive.Get();
		return;
	}
	if (disp && light) {
		vs = dx->pVertexShader_DISP.Get();
		if (nortNo == -1)
			ps = dx->pPixelShader_3D.Get();
		else
			ps = dx->pPixelShader_Bump.Get();
		hs = dx->pHullShader_DISP.Get();
		ds = dx->pDomainShader_DISP.Get();
		return;
	}
	if (disp && !light) {
		vs = dx->pVertexShader_DISP.Get();
		ps = dx->pPixelShader_Emissive.Get();
		hs = dx->pHullShader_DISP.Get();
		ds = dx->pDomainShader_DISP.Get();
		return;
	}
}

void PolygonData::SetCol(float difR, float difG, float difB, float speR, float speG, float speB) {
	sg.vDiffuse.x = difR;
	sg.vDiffuse.y = difG;
	sg.vDiffuse.z = difB;
	sg.vSpeculer.x = speR;
	sg.vSpeculer.y = speG;
	sg.vSpeculer.z = speB;
}

bool PolygonData::Create(bool light, int tNo, bool blend, bool alpha) {
	return Create(light, tNo, -1, blend, alpha);
}

bool PolygonData::Create(bool light, int tNo, int nortNo, bool blend, bool alpha) {

	GetShaderByteCode(light, tNo, nortNo);

	mObjectCB1->CopyData(0, sg);

	//BuildRootSignature
	CD3DX12_DESCRIPTOR_RANGE texTable, nortexTable;
	texTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);//このDescriptorRangeはシェーダーリソースビュー,Descriptor 1個, 開始Index 0番
	nortexTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1);

	//BuildRootSignatureParameter
	CD3DX12_ROOT_PARAMETER slotRootParameter[4];
	slotRootParameter[0].InitAsDescriptorTable(1, &texTable, D3D12_SHADER_VISIBILITY_ALL);// DescriptorRangeの数は1つ, DescriptorRangeの先頭アドレス
	slotRootParameter[1].InitAsDescriptorTable(1, &nortexTable, D3D12_SHADER_VISIBILITY_ALL);
	slotRootParameter[2].InitAsConstantBufferView(0);
	slotRootParameter[3].InitAsConstantBufferView(1);

	mRootSignature = CreateRs(4, slotRootParameter);
	if (mRootSignature == nullptr)return false;

	//SRVのデスクリプターヒープ生成
	texNum = 1;
	material[0].tex_no = tNo;
	material[0].nortex_no = nortNo;
	material[0].dwNumFace = 1;
	if (nortNo != -1)texNum = 2;//normalMap有りの場合2個生成
	TextureNo te;
	te.diffuse = tNo;
	te.normal = nortNo;
	te.movie = m_on;
	mSrvHeap = CreateSrvHeap(1, texNum, &te, texture);
	if (mSrvHeap == nullptr)return false;

	UINT VertexSize;
	if (tNo == -1 && !m_on)
		VertexSize = sizeof(VertexBC);
	else
		VertexSize = sizeof(Vertex);

	const UINT vbByteSize = VertexSize * ver;
	const UINT ibByteSize = verI * sizeof(std::uint16_t);

	if (tNo == -1 && !m_on)
		Vview->VertexBufferGPU = dx->CreateDefaultBuffer(mCommandList, d3varrayBC, vbByteSize, Vview->VertexBufferUploader);
	else
		Vview->VertexBufferGPU = dx->CreateDefaultBuffer(mCommandList, d3varray, vbByteSize, Vview->VertexBufferUploader);

	Iview[0].IndexBufferGPU = dx->CreateDefaultBuffer(mCommandList, d3varrayI, ibByteSize, Iview[0].IndexBufferUploader);

	Vview->VertexByteStride = VertexSize;
	Vview->VertexBufferByteSize = vbByteSize;
	Iview[0].IndexFormat = DXGI_FORMAT_R16_UINT;
	Iview[0].IndexBufferByteSize = ibByteSize;
	Iview[0].IndexCount = verI;

	//パイプラインステートオブジェクト生成
	if (tNo == -1 && !m_on)
		mPSO = CreatePsoVsHsDsPs(vs, hs, ds, ps, mRootSignature.Get(), dx->pVertexLayout_3DBC, alpha, blend, primType_create);
	else
		mPSO = CreatePsoVsHsDsPs(vs, hs, ds, ps, mRootSignature.Get(), dx->pVertexLayout_3D, alpha, blend, primType_create);

	if (mPSO == nullptr)return false;

	return true;
}

void PolygonData::InstancedMap(float x, float y, float z, float theta, float sizeX, float sizeY, float sizeZ) {
	dx->InstancedMap(&cb[sw], x, y, z, theta, 0, 0, sizeX, sizeY, sizeZ);
}

void PolygonData::CbSwap() {
	Lock();
	if (!UpOn) {
		upCount++;
		if (upCount > 1)UpOn = true;//cb,2要素初回更新終了
	}
	sw = 1 - sw;//cbスワップ
	insNum = dx->ins_no;
	dx->ins_no = 0;
	Unlock();
	DrawOn = true;
}

void PolygonData::InstanceUpdate(float r, float g, float b, float a, float disp, float px, float py, float mx, float my) {
	dx->MatrixMap(&cb[sw], r, g, b, a, disp, px, py, mx, my);
	CbSwap();
}

void PolygonData::Update(float x, float y, float z, float r, float g, float b, float a, float theta, float disp, float size, float px, float py, float mx, float my) {
	dx->InstancedMap(&cb[sw], x, y, z, theta, 0, 0, size);
	dx->MatrixMap(&cb[sw], r, g, b, a, disp, px, py, mx, my);
	CbSwap();
}

void PolygonData::DrawOff() {
	DrawOn = false;
}

void PolygonData::Draw() {

	if (!UpOn | !DrawOn)return;

	Lock();
	mObjectCB->CopyData(0, cb[1 - sw]);
	Unlock();

	drawPara para;
	para.NumMaterial = 1;
	para.srv = mSrvHeap.Get();
	para.rootSignature = mRootSignature.Get();
	para.Vview = Vview.get();
	para.Iview = Iview.get();
	para.material = material;
	para.haveNortexTOPOLOGY = primType_draw;
	para.notHaveNortexTOPOLOGY = primType_draw;
	para.haveNortexPSO = mPSO.Get();
	para.notHaveNortexPSO = mPSO.Get();
	para.cbRes0 = mObjectCB->Resource();
	para.cbRes1 = mObjectCB1->Resource();
	para.cbRes2 = nullptr;
	para.sRes0 = nullptr;
	para.sRes1 = nullptr;
	para.insNum = insNum;
	drawsub(para);
}