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
	d3varray = NULL;
	d3varrayBC = NULL;
	d3varrayI = NULL;

	sg.vDiffuse.x = 1.0f;
	sg.vDiffuse.y = 1.0f;
	sg.vDiffuse.z = 1.0f;
	sg.vDiffuse.w = 1.0f;
	sg.vSpeculer.x = 1.0f;
	sg.vSpeculer.y = 1.0f;
	sg.vSpeculer.z = 1.0f;
	sg.vSpeculer.w = 1.0f;
}

void PolygonData::SetCommandList(int no) {
	com_no = no;
	mCommandList = dx->dx_sub[com_no].mCommandList.Get();
}

PolygonData::~PolygonData() {
	free(d3varray);
	d3varray = NULL;
	free(d3varrayBC);
	d3varrayBC = NULL;
	free(d3varrayI);
	d3varrayI = NULL;
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
	Iview = std::make_unique<IndexView>();
}

void PolygonData::GetShaderByteCode(bool light, int tNo, int nortNo) {
	t_no = tNo;
	bool disp = FALSE;
	if (primType_create == D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH)disp = TRUE;
	if (tNo == -1 && m_on == FALSE) {
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

void PolygonData::Create(bool light, int tNo, bool blend, bool alpha) {
	Create(light, tNo, -1, blend, alpha);
}

void PolygonData::Create(bool light, int tNo, int nortNo, bool blend, bool alpha) {

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

	//SRVのデスクリプターヒープ生成
	texNum = 1;
	if (nortNo != -1)texNum = 2;//normalMap有りの場合2個生成
	TextureNo te;
	te.diffuse = tNo;
	te.normal = nortNo;
	te.movie = m_on;
	mSrvHeap = CreateSrvHeap(1, texNum, &te, texture);

	UINT VertexSize;
	if (tNo == -1 && !m_on)
		VertexSize = sizeof(VertexBC);
	else
		VertexSize = sizeof(Vertex);

	const UINT vbByteSize = VertexSize * ver;
	const UINT ibByteSize = verI * sizeof(std::uint16_t);

	if (tNo == -1 && !m_on)
		Vview->VertexBufferGPU = dx->CreateDefaultBuffer(dx->md3dDevice.Get(),
			mCommandList, d3varrayBC, vbByteSize, Vview->VertexBufferUploader);
	else
		Vview->VertexBufferGPU = dx->CreateDefaultBuffer(dx->md3dDevice.Get(),
			mCommandList, d3varray, vbByteSize, Vview->VertexBufferUploader);

	Iview->IndexBufferGPU = dx->CreateDefaultBuffer(dx->md3dDevice.Get(),
		mCommandList, d3varrayI, ibByteSize, Iview->IndexBufferUploader);

	Vview->VertexByteStride = VertexSize;
	Vview->VertexBufferByteSize = vbByteSize;
	Iview->IndexFormat = DXGI_FORMAT_R16_UINT;
	Iview->IndexBufferByteSize = ibByteSize;
	Iview->IndexCount = verI;

	//パイプラインステートオブジェクト生成
	if (tNo == -1 && !m_on)
		mPSO = CreatePsoVsHsDsPs(vs, hs, ds, ps, mRootSignature.Get(), dx->pVertexLayout_3DBC, alpha, blend, primType_create);
	else
		mPSO = CreatePsoVsHsDsPs(vs, hs, ds, ps, mRootSignature.Get(), dx->pVertexLayout_3D, alpha, blend, primType_create);
}

void PolygonData::InstancedMap(float x, float y, float z, float theta) {
	dx->InstancedMap(&cb[sw], x, y, z, theta, 0, 0, 1.0f);
}

void PolygonData::InstancedMap(float x, float y, float z, float theta, float size) {
	dx->InstancedMap(&cb[sw], x, y, z, theta, 0, 0, size);
}

void PolygonData::InstancedMapSize3(float x, float y, float z, float theta, float sizeX, float sizeY, float sizeZ) {
	dx->InstancedMapSize3(&cb[sw], x, y, z, theta, 0, 0, sizeX, sizeY, sizeZ);
}

void PolygonData::InstanceUpdate(float r, float g, float b, float a, float disp) {
	InstanceUpdate(r, g, b, a, disp, 1.0f, 1.0f, 1.0f, 1.0f);
}

void PolygonData::CbSwap() {
	Lock();
	if (!UpOn) {
		upCount++;
		if (upCount > 1)UpOn = TRUE;//cb,2要素初回更新終了
	}
	sw = 1 - sw;//cbスワップ
	insNum = dx->ins_no;
	dx->ins_no = 0;
	Unlock();
	DrawOn = TRUE;
}

void PolygonData::InstanceUpdate(float r, float g, float b, float a, float disp, float px, float py, float mx, float my) {
	dx->MatrixMap2(&cb[sw], r, g, b, a, disp, px, py, mx, my);
	CbSwap();
}

void PolygonData::Update(float x, float y, float z, float r, float g, float b, float a, float theta, float disp) {
	Update(x, y, z, r, g, b, a, theta, disp, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);
}

void  PolygonData::Update(float x, float y, float z, float r, float g, float b, float a, float theta, float disp, float size) {
	Update(x, y, z, r, g, b, a, theta, disp, size, 1.0f, 1.0f, 1.0f, 1.0f);
}

void PolygonData::Update(float x, float y, float z, float r, float g, float b, float a, float theta, float disp, float size, float px, float py, float mx, float my) {
	dx->MatrixMap(&cb[sw], x, y, z, r, g, b, a, theta, 0, 0, size, disp, px, py, mx, my);
	CbSwap();
}

void PolygonData::DrawOff() {
	DrawOn = FALSE;
}

void PolygonData::Draw() {

	if (!UpOn | !DrawOn)return;

	Lock();
	mObjectCB->CopyData(0, cb[1 - sw]);
	Unlock();

	mCommandList->SetPipelineState(mPSO.Get());

	//mSwapChainBuffer PRESENT→RENDER_TARGET
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(dx->mSwapChainBuffer[dx->mCurrBackBuffer].Get(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	ID3D12DescriptorHeap* descriptorHeaps[] = { mSrvHeap.Get() };
	mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);//テクスチャ無しの場合このままで良いのやら・・エラーは無し

	mCommandList->SetGraphicsRootSignature(mRootSignature.Get());

	mCommandList->IASetVertexBuffers(0, 1, &Vview->VertexBufferView());
	mCommandList->IASetIndexBuffer(&Iview->IndexBufferView());
	mCommandList->IASetPrimitiveTopology(primType_draw);

	CD3DX12_GPU_DESCRIPTOR_HANDLE tex(mSrvHeap->GetGPUDescriptorHandleForHeapStart());
	mCommandList->SetGraphicsRootDescriptorTable(0, tex);
	if (texNum == 2) {
		tex.Offset(1, dx->mCbvSrvUavDescriptorSize);
		mCommandList->SetGraphicsRootDescriptorTable(1, tex);
	}
	mCommandList->SetGraphicsRootConstantBufferView(2, mObjectCB->Resource()->GetGPUVirtualAddress());
	mCommandList->SetGraphicsRootConstantBufferView(3, mObjectCB1->Resource()->GetGPUVirtualAddress());

	mCommandList->DrawIndexedInstanced(Iview->IndexCount, insNum, 0, 0, 0);

	//mSwapChainBuffer RENDER_TARGET→PRESENT
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(dx->mSwapChainBuffer[dx->mCurrBackBuffer].Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
}