//*****************************************************************************************//
//**                                                                                     **//
//**                   　　　       PostEffectクラス                                     **//
//**                                                                                     **//
//*****************************************************************************************//

#include "Dx_PostEffect.h"

PostEffect::PostEffect() {
	dx = Dx12Process::GetInstance();
	mCommandList = dx->dx_sub[0].mCommandList.Get();
	mObjectCB = new UploadBuffer<CONSTANT_BUFFER_PostMosaic>(dx->md3dDevice.Get(), 1, true);
}

PostEffect::~PostEffect() {
	S_DELETE(mObjectCB);
}

void PostEffect::SetCommandList(int no) {
	com_no = no;
	mCommandList = dx->dx_sub[com_no].mCommandList.Get();
}

void PostEffect::ComCreateMosaic() {
	ComCreate(0);
}

void PostEffect::ComCreateBlur() {
	ComCreate(1);
}

void PostEffect::ComCreate(int no) {

	D3D12_DESCRIPTOR_HEAP_DESC uavHeapDesc = {};
	uavHeapDesc.NumDescriptors = 2;
	uavHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	uavHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	dx->md3dDevice->CreateDescriptorHeap(&uavHeapDesc, IID_PPV_ARGS(&mUavHeap));

	D3D12_RESOURCE_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Alignment = 0;
	texDesc.Width = dx->mClientWidth;
	texDesc.Height = dx->mClientHeight;
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
		IID_PPV_ARGS(&mOutputBuffer));

	dx->md3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(&mInputBuffer));

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	uavDesc.Texture2D.MipSlice = 0;

	CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(mUavHeap->GetCPUDescriptorHandleForHeapStart());
	dx->md3dDevice->CreateUnorderedAccessView(mInputBuffer.Get(), nullptr, &uavDesc, hDescriptor);
	hDescriptor.Offset(1, dx->mCbvSrvUavDescriptorSize);
	dx->md3dDevice->CreateUnorderedAccessView(mOutputBuffer.Get(), nullptr, &uavDesc, hDescriptor);

	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mInputBuffer.Get(),
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));

	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mOutputBuffer.Get(),
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));

	//ルートシグネチャ
	CD3DX12_DESCRIPTOR_RANGE uavTable1, uavTable2;
	uavTable1.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);
	uavTable2.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 1);

	CD3DX12_ROOT_PARAMETER slotRootParameter[3];
	slotRootParameter[0].InitAsDescriptorTable(1, &uavTable1);//RWTexture2D(u0)
	slotRootParameter[1].InitAsDescriptorTable(1, &uavTable2);//RWTexture2D(u1)
	slotRootParameter[2].InitAsConstantBufferView(0);//mObjectCB(b0)

	mRootSignatureCom = CreateRsCompute(3, slotRootParameter);

	cs = dx->pComputeShader_Post[no].Get();

	//PSO
	mPSOCom = CreatePsoCompute(cs, mRootSignatureCom.Get());
}

void PostEffect::ComputeMosaic(bool On, int size) {
	Compute(On, size, 0.0f, 0.0f, 0.0f);
}

void PostEffect::ComputeBlur(bool On, float blurX, float blurY, float blurLevel) {
	Compute(On, 0, blurX, blurY, blurLevel);
}

void PostEffect::Compute(bool On, int size, float blurX, float blurY, float blurLevel) {

	if (!On)return;

	CONSTANT_BUFFER_PostMosaic cb;
	cb.mosaicSize.x = (float)size;
	cb.blur.x = blurX;
	cb.blur.y = blurY;
	cb.blur.z = blurLevel;
	mObjectCB->CopyData(0, cb);

	mCommandList->SetPipelineState(mPSOCom.Get());

	ID3D12DescriptorHeap* descriptorHeaps[] = { mUavHeap.Get() };
	mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	mCommandList->SetComputeRootSignature(mRootSignatureCom.Get());

	//バックバッファをコピー元にする
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(dx->mSwapChainBuffer[dx->mCurrBackBuffer].Get(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_COPY_SOURCE));
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mInputBuffer.Get(),
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_DEST));
	//現在のバックバッファをインプット用バッファにコピーする
	mCommandList->CopyResource(mInputBuffer.Get(), dx->mSwapChainBuffer[dx->mCurrBackBuffer].Get());

	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mInputBuffer.Get(),
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));

	CD3DX12_GPU_DESCRIPTOR_HANDLE uav(mUavHeap->GetGPUDescriptorHandleForHeapStart());
	mCommandList->SetComputeRootDescriptorTable(0, uav);
	uav.Offset(1, dx->mCbvSrvUavDescriptorSize);
	mCommandList->SetComputeRootDescriptorTable(1, uav);
	mCommandList->SetComputeRootConstantBufferView(2, mObjectCB->Resource()->GetGPUVirtualAddress());

	UINT disX = dx->mClientWidth / 32;
	UINT disY = dx->mClientHeight / 8;
	mCommandList->Dispatch(disX, disY, 1);//CS内32, 8, 1

	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(dx->mSwapChainBuffer[dx->mCurrBackBuffer].Get(),
		D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_COPY_DEST));

	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mOutputBuffer.Get(),
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_GENERIC_READ));

	//計算後バックバッファにコピー
	mCommandList->CopyResource(dx->mSwapChainBuffer[dx->mCurrBackBuffer].Get(), mOutputBuffer.Get());

	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mOutputBuffer.Get(),
		D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));

	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(dx->mSwapChainBuffer[dx->mCurrBackBuffer].Get(),
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PRESENT));
}
