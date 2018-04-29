//*****************************************************************************************//
//**                                                                                     **//
//**                   �@�@�@          Wave�N���X                                        **//
//**                                                                                     **//
//*****************************************************************************************//

#include "Dx_Wave.h"

std::mutex Wave::mtx;

Wave::Wave() {
	dx = Dx12Process::GetInstance();
	mCommandList = dx->dx_sub[0].mCommandList.Get();
	d3varray = NULL;
	d3varrayI = NULL;
	texNum = 1;

	sg.vDiffuse.x = 1.0f;
	sg.vDiffuse.y = 1.0f;
	sg.vDiffuse.z = 1.0f;
	sg.vDiffuse.w = 1.0f;
	sg.vSpeculer.x = 1.0f;
	sg.vSpeculer.y = 1.0f;
	sg.vSpeculer.z = 1.0f;
	sg.vSpeculer.w = 1.0f;
}

void Wave::SetCommandList(int no) {
	com_no = no;
	mCommandList = dx->dx_sub[com_no].mCommandList.Get();
}

Wave::~Wave() {
	free(d3varray);
	d3varray = NULL;
	free(d3varrayI);
	d3varrayI = NULL;
	S_DELETE(mObjectCB);
	S_DELETE(mObjectCB_WAVE);
	RELEASE(texture);
	RELEASE(textureUp);
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
	mObjectCB = new UploadBuffer<CONSTANT_BUFFER>(dx->md3dDevice.Get(), 1, true);
	mObjectCB1 = new UploadBuffer<CONSTANT_BUFFER2>(dx->md3dDevice.Get(), 1, true);
	mObjectCB_WAVE = new UploadBuffer<CONSTANT_BUFFER_WAVE>(dx->md3dDevice.Get(), 1, true);
	Vview = std::make_unique<VertexView>();
	Iview = std::make_unique<IndexView>();
}

void Wave::GetShaderByteCode(int texNum) {
	cs = dx->pComputeShader_Wave.Get();
	vs = dx->pVertexShader_Wave.Get();
	if (texNum <= 1)
		ps = dx->pPixelShader_3D.Get();
	else
		ps = dx->pPixelShader_Bump.Get();
	hs = dx->pHullShader_Wave.Get();
	ds = dx->pDomainShader_Wave.Get();
}

void Wave::ComCreate() {

	//CS����DS�ւ̎󂯓n���p
	int divide = (int)(cbw.wHei_divide.y * cbw.wHei_divide.y);
	div = (int)(cbw.wHei_divide.y / 32.0f);//32��CS���X���b�h��
	std::vector<WaveData> wdata(divide);
	for (int i = 0; i < divide; ++i)
	{
		wdata[i].sinWave = 0;
		wdata[i].theta = (float)(i % 360);
	}

	UINT64 byteSize = wdata.size() * sizeof(WaveData);
	//RWStructuredBuffer�p
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
	//UpLoad�p
	dx->md3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(byteSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&mInputUploadBuffer));

	D3D12_SUBRESOURCE_DATA subResourceData = {};
	subResourceData.pData = wdata.data();
	subResourceData.RowPitch = wdata.size();
	subResourceData.SlicePitch = subResourceData.RowPitch;
	//wdata,UpLoad
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mInputBuffer.Get(),
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST));
	UpdateSubresources(mCommandList, mInputBuffer.Get(), mInputUploadBuffer.Get(), 0, 0, 1, &subResourceData);
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mInputBuffer.Get(),
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));

	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mOutputBuffer.Get(),
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));

	//���[�g�V�O�l�`��
	CD3DX12_ROOT_PARAMETER slotRootParameter[3];
	slotRootParameter[0].InitAsUnorderedAccessView(0);//RWStructuredBuffer(u0)
	slotRootParameter[1].InitAsUnorderedAccessView(1);//RWStructuredBuffer(u1)
	slotRootParameter[2].InitAsConstantBufferView(0);//mObjectCB_WAVE

	mRootSignatureCom = CreateRsCompute(3, slotRootParameter);

	//PSO
	mPSOCom = CreatePsoCompute(cs, mRootSignatureCom.Get());
}

void Wave::SetCol(float difR, float difG, float difB, float speR, float speG, float speB) {
	sg.vDiffuse.x = difR;
	sg.vDiffuse.y = difG;
	sg.vDiffuse.z = difB;
	sg.vSpeculer.x = speR;
	sg.vSpeculer.y = speG;
	sg.vSpeculer.z = speB;
}

void Wave::DrawCreate(int texNo, int nortNo, bool blend, bool alpha) {

	mObjectCB1->CopyData(0, sg);

	CD3DX12_DESCRIPTOR_RANGE texTable, nortexTable;
	texTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);//����DescriptorRange�̓V�F�[�_�[���\�[�X�r���[,Descriptor 1��, �J�nIndex 0��
	nortexTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1);

	CD3DX12_ROOT_PARAMETER slotRootParameter[6];
	slotRootParameter[0].InitAsDescriptorTable(1, &texTable, D3D12_SHADER_VISIBILITY_ALL);//(t0)DescriptorRange�̐���1��, DescriptorRange�̐擪�A�h���X
	slotRootParameter[1].InitAsDescriptorTable(1, &nortexTable, D3D12_SHADER_VISIBILITY_ALL);//(t1)
	slotRootParameter[2].InitAsConstantBufferView(0);//(b0)
	slotRootParameter[3].InitAsConstantBufferView(1);//(b1)
	slotRootParameter[4].InitAsConstantBufferView(2);//mObjectCB_WAVE(b2)
	slotRootParameter[5].InitAsShaderResourceView(2);//StructuredBuffer(t2)

	mRootSignatureDraw = CreateRs(6, slotRootParameter);

	TextureNo te;
	te.diffuse = texNo;
	te.normal = nortNo;
	te.movie = m_on;
	mSrvHeap = CreateSrvHeap(1, texNum, &te);

	const UINT vbByteSize = ver * sizeof(Vertex);
	const UINT ibByteSize = verI * sizeof(std::uint16_t);

	D3DCreateBlob(vbByteSize, &Vview->VertexBufferCPU);
	CopyMemory(Vview->VertexBufferCPU->GetBufferPointer(), d3varray, vbByteSize);

	D3DCreateBlob(ibByteSize, &Iview->IndexBufferCPU);
	CopyMemory(Iview->IndexBufferCPU->GetBufferPointer(), d3varrayI, ibByteSize);

	Vview->VertexBufferGPU = dx->CreateDefaultBuffer(dx->md3dDevice.Get(),
		mCommandList, d3varray, vbByteSize, Vview->VertexBufferUploader);

	Iview->IndexBufferGPU = dx->CreateDefaultBuffer(dx->md3dDevice.Get(),
		mCommandList, d3varrayI, ibByteSize, Iview->IndexBufferUploader);

	Vview->VertexByteStride = sizeof(Vertex);
	Vview->VertexBufferByteSize = vbByteSize;
	Iview->IndexFormat = DXGI_FORMAT_R16_UINT;
	Iview->IndexBufferByteSize = ibByteSize;
	Iview->IndexCount = verI;

	//�p�C�v���C���X�e�[�g�I�u�W�F�N�g����
	mPSODraw = CreatePsoVsHsDsPs(vs, hs, ds, ps, mRootSignatureDraw.Get(), dx->pVertexLayout_3D, alpha, blend);
}

void Wave::Create(int texNo, bool blend, bool alpha, float waveHeight, float divide) {
	Create(texNo, -1, blend, alpha, waveHeight, divide);
}

void Wave::Create(int texNo, int nortNo, bool blend, bool alpha, float waveHeight, float divide) {
	cbw.wHei_divide.as(waveHeight, divide, 0.0f, 0.0f);
	mObjectCB_WAVE->CopyData(0, cbw);
	t_no = texNo;
	if (nortNo != -1)texNum = 2;
	GetShaderByteCode(texNum);
	ComCreate();
	DrawCreate(texNo, nortNo, blend, alpha);
}

void Wave::InstancedMap(float x, float y, float z, float theta) {
	dx->InstancedMap(&cb[sw], x, y, z, theta, 0, 0, 1.0f);
}

void Wave::InstancedMap(float x, float y, float z, float theta, float size) {
	dx->InstancedMap(&cb[sw], x, y, z, theta, 0, 0, size);
}

void Wave::InstancedMapSize3(float x, float y, float z, float theta, float sizeX, float sizeY, float sizeZ) {
	dx->InstancedMapSize3(&cb[sw], x, y, z, theta, 0, 0, sizeX, sizeY, sizeZ);
}

void Wave::InstanceUpdate(float r, float g, float b, float a, float disp) {
	InstanceUpdate(r, g, b, a, disp, 1.0f, 1.0f, 1.0f, 1.0f);
}

void Wave::CbSwap() {
	Lock();
	if (!UpOn) {
		upCount++;
		if (upCount > 1)UpOn = TRUE;//cb,2�v�f����X�V�I��
	}
	sw = 1 - sw;//cb�X���b�v
	insNum = dx->ins_no;
	dx->ins_no = 0;
	Unlock();
	DrawOn = TRUE;
}

void Wave::InstanceUpdate(float r, float g, float b, float a, float disp, float px, float py, float mx, float my) {
	dx->MatrixMap2(&cb[sw], r, g, b, a, disp, px, py, mx, my);
	CbSwap();
}

void Wave::Update(float x, float y, float z, float r, float g, float b, float a, float theta, float disp) {
	Update(x, y, z, r, g, b, a, theta, disp, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);
}

void Wave::Update(float x, float y, float z, float r, float g, float b, float a, float theta, float disp, float size) {
	Update(x, y, z, r, g, b, a, theta, disp, size, 1.0f, 1.0f, 1.0f, 1.0f);
}

void Wave::Update(float x, float y, float z, float r, float g, float b, float a, float theta, float disp, float size, float px, float py, float mx, float my) {
	dx->MatrixMap(&cb[sw], x, y, z, r, g, b, a, theta, 0, 0, size, disp, px, py, mx, my);
	CbSwap();
}

void Wave::DrawOff() {
	DrawOn = FALSE;
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

	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mInputBuffer.Get(),
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_GENERIC_READ));
}

void Wave::DrawSub() {

	mCommandList->SetPipelineState(mPSODraw.Get());

	//mSwapChainBuffer PRESENT��RENDER_TARGET
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(dx->mSwapChainBuffer[dx->mCurrBackBuffer].Get(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	ID3D12DescriptorHeap* descriptorHeaps[] = { mSrvHeap.Get() };
	mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);//�e�N�X�`�������̏ꍇ���̂܂܂ŗǂ��̂��E�E�G���[�͖���

	mCommandList->SetGraphicsRootSignature(mRootSignatureDraw.Get());

	mCommandList->IASetVertexBuffers(0, 1, &Vview->VertexBufferView());
	mCommandList->IASetIndexBuffer(&Iview->IndexBufferView());
	mCommandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_4_CONTROL_POINT_PATCHLIST);

	CD3DX12_GPU_DESCRIPTOR_HANDLE tex(mSrvHeap->GetGPUDescriptorHandleForHeapStart());
	mCommandList->SetGraphicsRootDescriptorTable(0, tex);
	if (texNum == 2) {
		tex.Offset(1, dx->mCbvSrvUavDescriptorSize);
		mCommandList->SetGraphicsRootDescriptorTable(1, tex);
	}
	mCommandList->SetGraphicsRootConstantBufferView(2, mObjectCB->Resource()->GetGPUVirtualAddress());
	mCommandList->SetGraphicsRootConstantBufferView(3, mObjectCB1->Resource()->GetGPUVirtualAddress());
	mCommandList->SetGraphicsRootConstantBufferView(4, mObjectCB_WAVE->Resource()->GetGPUVirtualAddress());
	mCommandList->SetGraphicsRootShaderResourceView(5, mOutputBuffer->GetGPUVirtualAddress());

	mCommandList->DrawIndexedInstanced(Iview->IndexCount, insNum, 0, 0, 0);

	//mSwapChainBuffer RENDER_TARGET��PRESENT
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(dx->mSwapChainBuffer[dx->mCurrBackBuffer].Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
}

void Wave::Draw() {

	if (!UpOn | !DrawOn)return;

	Lock();
	mObjectCB->CopyData(0, cb[1 - sw]);
	Unlock();

	Compute();
	DrawSub();
}



