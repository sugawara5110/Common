//*****************************************************************************************//
//**                                                                                     **//
//**                   　　　          Waveクラス                                        **//
//**                                                                                     **//
//*****************************************************************************************//

#include "Dx_Wave.h"

Wave::Wave() {
	dx = Dx12Process::GetInstance();
	mCommandList = dx->dx_sub[0].mCommandList.Get();
	d3varray = nullptr;
	d3varrayI = nullptr;

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
	free(d3varray);
	d3varray = nullptr;
	free(d3varrayI);
	d3varrayI = nullptr;
	S_DELETE(mObjectCB);
	S_DELETE(mObjectCB1);
	S_DELETE(mObjectCB_WAVE);
}

void Wave::SetVertex(int i,
	float vx, float vy, float vz,
	float nx, float ny, float nz,
	float u, float v) {
	d3varrayI[i] = i;
	d3varray[i].Pos.as(vx, vy, vz);
	d3varray[i].normal.as(nx, ny, nz);
	d3varray[i].tex.as(u, v);
}

void Wave::GetVBarray(int v) {
	ver = verI = v;
	d3varray = (Vertex*)malloc(sizeof(Vertex) * ver);
	d3varrayI = (std::uint16_t*)malloc(sizeof(std::uint16_t) * verI);
	mObjectCB = new ConstantBuffer<CONSTANT_BUFFER>(1);
	mObjectCB1 = new ConstantBuffer<CONSTANT_BUFFER2>(1);
	mObjectCB_WAVE = new ConstantBuffer<CONSTANT_BUFFER_WAVE>(1);
	dpara.NumMaterial = 1;
	dpara.material = std::make_unique<MY_MATERIAL_S[]>(dpara.NumMaterial);
	dpara.Vview = std::make_unique<VertexView>();
	dpara.Iview = std::make_unique<IndexView[]>(dpara.NumMaterial);
}

void Wave::GetShaderByteCode() {
	cs = dx->pComputeShader_Wave.Get();
	vs = dx->pVertexShader_Wave.Get();
	ps = dx->pPixelShader_3D.Get();
	hs = dx->pHullShader_Wave.Get();
	ds = dx->pDomainShader_Wave.Get();
	gs = dx->pGeometryShader_Before_ds.Get();
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

	UINT64 byteSize = wdata.size() * sizeof(WaveData);
	//RWStructuredBuffer用
	dx->md3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(byteSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS),
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(&mOutputBuffer));

	dx->md3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(byteSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS),
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(&mInputBuffer));
	//UpLoad用
	dx->md3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(byteSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&mInputUploadBuffer));

	D3D12_SUBRESOURCE_DATA subResourceData = {};
	subResourceData.pData = wdata.data();
	subResourceData.RowPitch = byteSize;
	subResourceData.SlicePitch = subResourceData.RowPitch;
	//wdata,UpLoad
	dx->dx_sub[com_no].ResourceBarrier(mInputBuffer.Get(),
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);

	dx->CopyResourcesToGPU(com_no, mInputUploadBuffer.Get(), mInputBuffer.Get(), subResourceData.pData, subResourceData.RowPitch);

	dx->dx_sub[com_no].ResourceBarrier(mInputBuffer.Get(),
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	dx->dx_sub[com_no].ResourceBarrier(mOutputBuffer.Get(),
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	//ルートシグネチャ
	CD3DX12_ROOT_PARAMETER slotRootParameter[3];
	slotRootParameter[0].InitAsUnorderedAccessView(0);//RWStructuredBuffer(u0)
	slotRootParameter[1].InitAsUnorderedAccessView(1);//RWStructuredBuffer(u1)
	slotRootParameter[2].InitAsConstantBufferView(0);//mObjectCB_WAVE

	mRootSignatureCom = CreateRsCompute(3, slotRootParameter);
	if (mRootSignatureCom == nullptr)return false;

	//PSO
	mPSOCom = CreatePsoCompute(cs, mRootSignatureCom.Get());
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

	mObjectCB1->CopyData(0, sg);

	dpara.rootSignature = CreateRootSignature("Wave");
	if (dpara.rootSignature == nullptr)return false;

	dpara.material[0].diftex_no = texNo;
	dpara.material[0].nortex_no = nortNo;
	dpara.material[0].spetex_no = dx->GetTexNumber("dummyDifSpe.");

	TextureNo te;
	if (texNo < 0)te.diffuse = dx->GetTexNumber("dummyDifSpe.");
	else
		te.diffuse = texNo;

	if (nortNo < 0)te.normal = dx->GetTexNumber("dummyNor.");
	else
		te.normal = nortNo;

	te.specular = dx->GetTexNumber("dummyDifSpe.");

	createTextureResource(0, 1, &te);
	dpara.descHeap = CreateDescHeap(4);
	if (dpara.descHeap == nullptr)return false;
	CreateSrvTexture(dpara.descHeap.Get(), 0, texture->GetAddressOf(), 3);
	CreateSrvBuffer(dpara.descHeap.Get(), 3, mOutputBuffer.GetAddressOf(), 1, sizeof(WaveData));
	dpara.numDesc = 4;

	const UINT vbByteSize = ver * sizeof(Vertex);
	const UINT ibByteSize = verI * sizeof(std::uint16_t);

	dpara.Vview->VertexBufferGPU = dx->CreateDefaultBuffer(com_no, d3varray, vbByteSize, dpara.Vview->VertexBufferUploader);

	dpara.Iview[0].IndexBufferGPU = dx->CreateDefaultBuffer(com_no, d3varrayI, ibByteSize, dpara.Iview[0].IndexBufferUploader);

	dpara.Vview->VertexByteStride = sizeof(Vertex);
	dpara.Vview->VertexBufferByteSize = vbByteSize;
	dpara.Iview[0].IndexFormat = DXGI_FORMAT_R16_UINT;
	dpara.Iview[0].IndexBufferByteSize = ibByteSize;
	dpara.Iview[0].IndexCount = verI;

	//パイプラインステートオブジェクト生成
	dpara.PSO = CreatePsoVsHsDsPs(vs, hs, ds, ps, gs, dpara.rootSignature.Get(), dx->pVertexLayout_3D, alpha, blend, CONTROL_POINT);
	if (dpara.PSO == nullptr)return false;

	return true;
}

bool Wave::Create(int texNo, bool blend, bool alpha, float waveHeight, float divide) {
	return Create(texNo, -1, blend, alpha, waveHeight, divide);
}

bool Wave::Create(int texNo, int nortNo, bool blend, bool alpha, float waveHeight, float divide) {
	cbw.wHei_divide.as(waveHeight, divide, 0.0f, 0.0f);
	mObjectCB_WAVE->CopyData(0, cbw);
	dpara.material[0].diftex_no = texNo;
	dpara.material[0].nortex_no = nortNo;
	GetShaderByteCode();
	if (!ComCreate())return false;
	return DrawCreate(texNo, nortNo, blend, alpha);
}

void Wave::InstancedMap(float x, float y, float z, float theta, float sizeX, float sizeY, float sizeZ) {
	dx->InstancedMap(ins_no, &cb[dx->cBuffSwap[0]], x, y, z, theta, 0, 0, sizeX, sizeY, sizeZ);
}

void Wave::CbSwap() {
	if (!UpOn) {
		upCount++;
		if (upCount > 1)UpOn = true;//cb,2要素初回更新終了
	}
	insNum[dx->cBuffSwap[0]] = ins_no;
	ins_no = 0;
	DrawOn = true;
}

void Wave::InstanceUpdate(float r, float g, float b, float a, float disp, float shininess,
	float px, float py, float mx, float my) {

	dx->MatrixMap(&cb[dx->cBuffSwap[0]], r, g, b, a, disp, px, py, mx, my, nullptr, 0, shininess);
	CbSwap();
}

void Wave::Update(float x, float y, float z, float r, float g, float b, float a, float theta, float disp, float shininess,
	float size, float px, float py, float mx, float my) {

	dx->InstancedMap(ins_no, &cb[dx->cBuffSwap[0]], x, y, z, theta, 0, 0, size);
	dx->MatrixMap(&cb[dx->cBuffSwap[0]], r, g, b, a, disp, px, py, mx, my, nullptr, 0, shininess);
	CbSwap();
}

void Wave::DrawOff() {
	DrawOn = false;
}

void Wave::Compute() {

	mCommandList->SetPipelineState(mPSOCom.Get());

	mCommandList->SetComputeRootSignature(mRootSignatureCom.Get());

	mCommandList->SetComputeRootUnorderedAccessView(0, mInputBuffer->GetGPUVirtualAddress());
	mCommandList->SetComputeRootUnorderedAccessView(1, mOutputBuffer->GetGPUVirtualAddress());
	mCommandList->SetComputeRootConstantBufferView(2, mObjectCB_WAVE->Resource()->GetGPUVirtualAddress());

	mCommandList->Dispatch(div, div, 1);

	auto tmp = mInputBuffer;
	mInputBuffer = mOutputBuffer;
	mOutputBuffer = tmp;

	dx->dx_sub[com_no].ResourceBarrier(mInputBuffer.Get(),
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_GENERIC_READ);
}

void Wave::DrawSub() {

	dpara.TOPOLOGY = D3D11_PRIMITIVE_TOPOLOGY_4_CONTROL_POINT_PATCHLIST;
	dpara.cbRes0 = mObjectCB->Resource();
	dpara.cbRes1 = mObjectCB1->Resource();
	dpara.cbRes2 = mObjectCB_WAVE->Resource();
	dpara.insNum = insNum[dx->cBuffSwap[1]];
	drawsub(dpara);
}

void Wave::Draw() {

	if (!UpOn | !DrawOn)return;

	mObjectCB->CopyData(0, cb[dx->cBuffSwap[1]]);

	Compute();
	DrawSub();
}



