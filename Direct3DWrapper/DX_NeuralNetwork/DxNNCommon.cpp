//*****************************************************************************************//
//**                                                                                     **//
//**                   　  　　　 DxNNCommon                                             **//
//**                                                                                     **//
//*****************************************************************************************//

#include "DxNNCommon.h"
#include "ShaderNN\ShaderNNCommon.h"

void DxNNCommon::CreareNNTexture(UINT width, UINT height, UINT num) {
	texWid = width;
	texHei = height * num;

	D3D12_DESCRIPTOR_HEAP_DESC uavHeapDesc = {};
	uavHeapDesc.NumDescriptors = 1;
	uavHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	uavHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	dx->md3dDevice->CreateDescriptorHeap(&uavHeapDesc, IID_PPV_ARGS(&mUavHeap2));

	D3D12_RESOURCE_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Alignment = 0;
	texDesc.Width = texWid;
	texDesc.Height = texHei;
	texDesc.DepthOrArraySize = 1;
	texDesc.MipLevels = 1;
	texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	//RWTexture2D用
	dx->md3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(&mTextureBuffer));

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	uavDesc.Texture2D.MipSlice = 0;

	CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(mUavHeap2->GetCPUDescriptorHandleForHeapStart());
	dx->md3dDevice->CreateUnorderedAccessView(mTextureBuffer.Get(), nullptr, &uavDesc, hDescriptor);

	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mTextureBuffer.Get(),
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));

	//ルートシグネチャ
	CD3DX12_DESCRIPTOR_RANGE uavTable;
	uavTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 1);

	CD3DX12_ROOT_PARAMETER slotRootParameter[3];
	slotRootParameter[0].InitAsUnorderedAccessView(0);//RWStructuredBuffer(u0)
	slotRootParameter[1].InitAsDescriptorTable(1, &uavTable);//RWTexture2D(u1)
	slotRootParameter[2].InitAsConstantBufferView(0);//mObjectCB2(b0)
	mRootSignatureCom2 = CreateRsCompute(3, slotRootParameter);

	pCS2 = CompileShader(ShaderNeuralNetworkTexture, strlen(ShaderNeuralNetworkTexture), "NNTexCopyCS", "cs_5_0");

	//PSO
	mPSOCom2 = CreatePsoCompute(pCS2.Get(), mRootSignatureCom2.Get());

	mObjectCB2 = new ConstantBuffer<NNCBTexture>(1);
	cb2.Wid_Hei.as(texWid, texHei, 0.0f, 0.0f);
	mObjectCB2->CopyData(0, cb2);
	created = true;
}

void DxNNCommon::TextureCopy(ID3D12Resource *texture, int comNo) {
	if (!created)return;
	dx->Bigin(comNo);
	mCommandList->SetPipelineState(mPSOCom2.Get());

	ID3D12DescriptorHeap* descriptorHeaps[] = { mUavHeap2.Get() };
	mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	mCommandList->SetComputeRootSignature(mRootSignatureCom2.Get());

	mCommandList->SetComputeRootUnorderedAccessView(0, texture->GetGPUVirtualAddress());
	CD3DX12_GPU_DESCRIPTOR_HANDLE uav(mUavHeap2->GetGPUDescriptorHandleForHeapStart());
	mCommandList->SetComputeRootDescriptorTable(1, uav);
	mCommandList->SetComputeRootConstantBufferView(2, mObjectCB2->Resource()->GetGPUVirtualAddress());
	mCommandList->Dispatch(texWid, texHei, 1);
	dx->End(comNo);
	dx->WaitFenceCurrent();
}

void DxNNCommon::CreateResourceDef(Microsoft::WRL::ComPtr<ID3D12Resource> &def, UINT64 size) {
	dx->md3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(size, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS),
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		nullptr,
		IID_PPV_ARGS(&def));
}

void DxNNCommon::CreateResourceUp(Microsoft::WRL::ComPtr<ID3D12Resource> &up, UINT64 size) {
	dx->md3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(size),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&up));
}

void DxNNCommon::CreateResourceRead(Microsoft::WRL::ComPtr<ID3D12Resource> &re, UINT64 size) {
	dx->md3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(size),
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&re));
}

void DxNNCommon::SubresourcesUp(void *pData, UINT num, Microsoft::WRL::ComPtr<ID3D12Resource> &def,
	Microsoft::WRL::ComPtr<ID3D12Resource> &up)
{
	D3D12_SUBRESOURCE_DATA subResourceData = {};
	subResourceData.pData = pData;
	subResourceData.RowPitch = num;
	subResourceData.SlicePitch = subResourceData.RowPitch;

	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(def.Get(),
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_DEST));
	UpdateSubresources(mCommandList, def.Get(), up.Get(), 0, 0, 1, &subResourceData);
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(def.Get(),
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
}

ID3D12Resource *DxNNCommon::GetNNTextureResource() {
	return mTextureBuffer.Get();
}

D3D12_RESOURCE_STATES DxNNCommon::GetNNTextureResourceStates() {
	return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
}

DxNNCommon::~DxNNCommon() {
	S_DELETE(mObjectCB2);
}