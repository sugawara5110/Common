//*****************************************************************************************//
//**                                                                                     **//
//**                   　　　          Waveクラス                                        **//
//**                                                                                     **//
//*****************************************************************************************//

#include "Dx_Wave.h"

Wave::Wave() {
	sg.vDiffuse.x = 1.0f;
	sg.vDiffuse.y = 1.0f;
	sg.vDiffuse.z = 1.0f;
	sg.vDiffuse.w = 1.0f;

	sg.vSpeculer.x = 1.0f;
	sg.vSpeculer.y = 1.0f;
	sg.vSpeculer.z = 1.0f;
	sg.vSpeculer.w = 1.0f;

	sg.vAmbient.x = 0.0f;
	sg.vAmbient.y = 0.0f;
	sg.vAmbient.z = 0.0f;
	sg.vAmbient.w = 0.0f;
}

Wave::~Wave() {
	S_DELETE(mObjectCB_WAVE);
}

void Wave::SetVertex(int i,
	float vx, float vy, float vz,
	float nx, float ny, float nz,
	float u, float v) {
	if (!ver) {
		ver = new Vertex[numVer];
		index = new UINT * [1];
		index[0] = new UINT[numIndex];
		mObj.getVertexBuffer(sizeof(Vertex), numVer);
		const UINT ibByteSize = numIndex * sizeof(UINT);
		mObj.getIndexBuffer(0, ibByteSize, numIndex);
	}
	index[0][i] = i;
	ver[i].Pos.as(vx, vy, vz);
	ver[i].normal.as(nx, ny, nz);
	ver[i].tex.as(u, v);
}

void Wave::GetVBarray(int v) {
	numVer = numIndex = v;
	mObjectCB_WAVE = new ConstantBuffer<CONSTANT_BUFFER_WAVE>(1);
	mObj.getBuffer(1);
}

void Wave::GetShaderByteCode() {
	Dx12Process* dx = mObj.dx;
	cs = dx->pComputeShader_Wave.Get();
	vs = dx->pVertexShader_Wave.Get();
	ps = dx->pPixelShader_3D.Get();
	ps_NoMap = dx->pPixelShader_3D_NoNormalMap.Get();
	hs = dx->pHullShader_Wave.Get();
	ds = dx->pDomainShader_Wave.Get();
	gs = dx->pGeometryShader_Before_ds.Get();
	gs_NoMap = dx->pGeometryShader_Before_ds_NoNormalMap.Get();
}

bool Wave::ComCreate() {

	//CSからDSへの受け渡し用
	int divide = (int)(cbw.wHei_divide.y * cbw.wHei_divide.y);
	div = (int)(cbw.wHei_divide.y / 32.0f);//32はCS内スレッド数
	std::vector<WaveData> wdata(divide);
	for (int i = 0; i < divide; ++i)
	{
		wdata[i].sinWave = 0.0f;
		wdata[i].theta = (float)(i % 360);
	}
	Dx12Process* dx = mObj.dx;
	UINT64 byteSize = wdata.size() * sizeof(WaveData);

	if (FAILED(dx->createDefaultResourceBuffer_UNORDERED_ACCESS(&mOutputBuffer, byteSize))) {
		ErrorMessage("Wave::ComCreate Error!!"); return false;
	}
	if (FAILED(dx->createDefaultResourceBuffer_UNORDERED_ACCESS(&mInputBuffer, byteSize))) {
		ErrorMessage("Wave::ComCreate Error!!"); return false;
	}
	if (FAILED(dx->createUploadResource(&mInputUploadBuffer, byteSize))) {
		ErrorMessage("Wave::ComCreate Error!!"); return false;
	}

	D3D12_SUBRESOURCE_DATA subResourceData = {};
	subResourceData.pData = wdata.data();
	subResourceData.RowPitch = byteSize;
	subResourceData.SlicePitch = subResourceData.RowPitch;
	//wdata,UpLoad
	dx->dx_sub[mObj.com_no].ResourceBarrier(mInputBuffer.Get(),
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);

	if (FAILED(dx->CopyResourcesToGPU(mObj.com_no, mInputUploadBuffer.Get(), mInputBuffer.Get(),
		subResourceData.pData, subResourceData.RowPitch))) {
		ErrorMessage("Wave::ComCreate Error!!"); return false;
	}

	dx->dx_sub[mObj.com_no].ResourceBarrier(mInputBuffer.Get(),
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	dx->dx_sub[mObj.com_no].ResourceBarrier(mOutputBuffer.Get(),
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	//ルートシグネチャ
	CD3DX12_ROOT_PARAMETER slotRootParameter[3];
	slotRootParameter[0].InitAsUnorderedAccessView(0);//RWStructuredBuffer(u0)
	slotRootParameter[1].InitAsUnorderedAccessView(1);//RWStructuredBuffer(u1)
	slotRootParameter[2].InitAsConstantBufferView(0);//mObjectCB_WAVE

	mRootSignatureCom = mObj.CreateRsCompute(3, slotRootParameter);
	if (mRootSignatureCom == nullptr)return false;

	//PSO
	mPSOCom = mObj.CreatePsoCompute(cs, mRootSignatureCom.Get());
	if (mPSOCom == nullptr)return false;

	return true;
}

void Wave::SetCol(float difR, float difG, float difB, float speR, float speG, float speB,
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

bool Wave::DrawCreate(int texNo, int nortNo, bool blend, bool alpha) {
	mObj.dpara.material[0].diftex_no = texNo;
	mObj.dpara.material[0].nortex_no = nortNo;
	mObj.dpara.material[0].spetex_no = mObj.dx->GetTexNumber("dummyDifSpe.");
	mObj.dpara.TOPOLOGY = D3D11_PRIMITIVE_TOPOLOGY_4_CONTROL_POINT_PATCHLIST;
	mObj.mObjectCB1->CopyData(0, sg);
	const int numSrvTex = 3;
	const int numSrvBuf = 1;
	const int numCbv = 3;
	mObj.primType_create = CONTROL_POINT;
	mObj.vs = vs;
	mObj.hs = hs;
	mObj.ds = ds;
	mObj.ps = ps;
	mObj.ps_NoMap = ps_NoMap;
	mObj.gs = gs;
	mObj.gs_NoMap = gs_NoMap;
	mObj.createDefaultBuffer(ver, index, true);
	if (!mObj.createPSO(mObj.dx->pVertexLayout_3D, numSrvTex + numSrvBuf, numCbv, blend, alpha, 0))return false;
	UINT cbSize = mObjectCB_WAVE->getSizeInBytes();
	D3D12_GPU_VIRTUAL_ADDRESS ad = mObjectCB_WAVE->Resource()->GetGPUVirtualAddress();
	ID3D12Resource* res[1] = {};
	res[0] = mOutputBuffer.Get();
	UINT buSize[1] = {};
	buSize[0] = sizeof(WaveData);
	if (!mObj.setDescHeap(numSrvTex, numSrvBuf, res, buSize, numCbv, ad, cbSize))return false;
	return true;
}

bool Wave::Create(int texNo, bool blend, bool alpha, float waveHeight, float divide) {
	return Create(texNo, -1, blend, alpha, waveHeight, divide);
}

bool Wave::Create(int texNo, int nortNo, bool blend, bool alpha, float waveHeight, float divide) {
	cbw.wHei_divide.as(waveHeight, divide, 0.0f, 0.0f);
	mObjectCB_WAVE->CopyData(0, cbw);
	GetShaderByteCode();
	if (!ComCreate())return false;
	return DrawCreate(texNo, nortNo, blend, alpha);
}

void Wave::InstancedMap(float x, float y, float z, float theta, float sizeX, float sizeY, float sizeZ) {
	mObj.InstancedMap(x, y, z, theta, 0, 0, sizeX, sizeY, sizeZ);
}

void Wave::InstanceUpdate(float r, float g, float b, float a, float disp, float shininess,
	float px, float py, float mx, float my) {

	mObj.InstanceUpdate(r, g, b, a, disp, shininess, px, py, mx, my);
}

void Wave::Update(float x, float y, float z, float r, float g, float b, float a, float theta, float disp, float shininess,
	float size, float px, float py, float mx, float my) {

	mObj.Update(x, y, z, r, g, b, a, theta, disp, shininess, size, px, py, mx, my);
}

void Wave::DrawOff() {
	mObj.DrawOff();
}

void Wave::Compute() {

	mObj.mCommandList->SetPipelineState(mPSOCom.Get());

	mObj.mCommandList->SetComputeRootSignature(mRootSignatureCom.Get());

	mObj.mCommandList->SetComputeRootUnorderedAccessView(0, mInputBuffer->GetGPUVirtualAddress());
	mObj.mCommandList->SetComputeRootUnorderedAccessView(1, mOutputBuffer->GetGPUVirtualAddress());
	mObj.mCommandList->SetComputeRootConstantBufferView(2, mObjectCB_WAVE->Resource()->GetGPUVirtualAddress());

	mObj.mCommandList->Dispatch(div, div, 1);

	auto tmp = mInputBuffer;
	mInputBuffer = mOutputBuffer;
	mOutputBuffer = tmp;

	mObj.dx->dx_sub[mObj.com_no].ResourceBarrier(mInputBuffer.Get(),
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_GENERIC_READ);
}

void Wave::DrawSub() {
	mObj.Draw();
}

void Wave::Draw() {
	if (!mObj.UpOn | !mObj.DrawOn)return;
	Compute();
	DrawSub();
}

void Wave::SetCommandList(int no) {
	mObj.SetCommandList(no);
}

void Wave::CopyResource(ID3D12Resource* texture, D3D12_RESOURCE_STATES res, int index) {
	mObj.CopyResource(texture, res, index);
}

void Wave::TextureInit(int width, int height, int index) {
	mObj.TextureInit(width, height, index);
}

HRESULT Wave::SetTextureMPixel(BYTE* frame, int index) {
	return mObj.SetTextureMPixel(frame, index);
}