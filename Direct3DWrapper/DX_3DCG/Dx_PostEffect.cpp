//*****************************************************************************************//
//**                                                                                     **//
//**                   　　　       PostEffectクラス                                     **//
//**                                                                                     **//
//*****************************************************************************************//

#include "Dx_PostEffect.h"

PostEffect::PostEffect() {
	dx = Dx12Process::GetInstance();
	mCommandList = dx->dx_sub[0].mCommandList.Get();
	mObjectCB = new ConstantBuffer<CONSTANT_BUFFER_PostMosaic>(1);
}

PostEffect::~PostEffect() {
	S_DELETE(mObjectCB);
}

bool PostEffect::ComCreateMosaic() {
	return ComCreate(0);
}

bool PostEffect::ComCreateBlur() {
	return ComCreate(1);
}

bool PostEffect::ComCreate(int no) {

	D3D12_DESCRIPTOR_HEAP_DESC uavHeapDesc = {};
	uavHeapDesc.NumDescriptors = 2;
	uavHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	uavHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	if (FAILED(dx->md3dDevice->CreateDescriptorHeap(&uavHeapDesc, IID_PPV_ARGS(&mUavHeap)))) {
		ErrorMessage("PostEffect::ComCreate Error!!"); return false;
	}
	if (FAILED(dx->createDefaultResourceTEXTURE2D_UNORDERED_ACCESS(&mOutputBuffer, dx->mClientWidth, dx->mClientHeight))) {
		ErrorMessage("PostEffect::ComCreate Error!!"); return false;
	}
	if (FAILED(dx->createDefaultResourceTEXTURE2D_UNORDERED_ACCESS(&mInputBuffer, dx->mClientWidth, dx->mClientHeight))) {
		ErrorMessage("PostEffect::ComCreate Error!!"); return false;
	}

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	uavDesc.Texture2D.MipSlice = 0;

	CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(mUavHeap->GetCPUDescriptorHandleForHeapStart());
	dx->md3dDevice->CreateUnorderedAccessView(mInputBuffer.Get(), nullptr, &uavDesc, hDescriptor);
	hDescriptor.Offset(1, dx->mCbvSrvUavDescriptorSize);
	dx->md3dDevice->CreateUnorderedAccessView(mOutputBuffer.Get(), nullptr, &uavDesc, hDescriptor);

	dx->dx_sub[com_no].ResourceBarrier(mInputBuffer.Get(),
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	dx->dx_sub[com_no].ResourceBarrier(mOutputBuffer.Get(),
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	//ルートシグネチャ
	CD3DX12_DESCRIPTOR_RANGE uavTable1, uavTable2;
	uavTable1.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);
	uavTable2.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 1);

	CD3DX12_ROOT_PARAMETER slotRootParameter[3];
	slotRootParameter[0].InitAsDescriptorTable(1, &uavTable1);//RWTexture2D(u0)
	slotRootParameter[1].InitAsDescriptorTable(1, &uavTable2);//RWTexture2D(u1)
	slotRootParameter[2].InitAsConstantBufferView(0);//mObjectCB(b0)

	mRootSignatureCom = CreateRsCompute(3, slotRootParameter);
	if (mRootSignatureCom == nullptr)return false;

	cs = dx->pComputeShader_Post[no].Get();

	//PSO
	mPSOCom = CreatePsoCompute(cs, mRootSignatureCom.Get());
	if (mPSOCom == nullptr)return false;

	return true;
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
	dx->dx_sub[com_no].ResourceBarrier(dx->mSwapChainBuffer[dx->mCurrBackBuffer].Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE);
	dx->dx_sub[com_no].ResourceBarrier(mInputBuffer.Get(),
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_DEST);
	//現在のバックバッファをインプット用バッファにコピーする
	mCommandList->CopyResource(mInputBuffer.Get(), dx->mSwapChainBuffer[dx->mCurrBackBuffer].Get());

	dx->dx_sub[com_no].ResourceBarrier(mInputBuffer.Get(),
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	CD3DX12_GPU_DESCRIPTOR_HANDLE uav(mUavHeap->GetGPUDescriptorHandleForHeapStart());
	mCommandList->SetComputeRootDescriptorTable(0, uav);
	uav.Offset(1, dx->mCbvSrvUavDescriptorSize);
	mCommandList->SetComputeRootDescriptorTable(1, uav);
	mCommandList->SetComputeRootConstantBufferView(2, mObjectCB->Resource()->GetGPUVirtualAddress());

	UINT disX = dx->mClientWidth / 32;
	UINT disY = dx->mClientHeight / 8;
	mCommandList->Dispatch(disX, disY, 1);//CS内32, 8, 1

	dx->dx_sub[com_no].ResourceBarrier(dx->mSwapChainBuffer[dx->mCurrBackBuffer].Get(),
		D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_COPY_DEST);

	dx->dx_sub[com_no].ResourceBarrier(mOutputBuffer.Get(),
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_GENERIC_READ);

	//計算後バックバッファにコピー
	mCommandList->CopyResource(dx->mSwapChainBuffer[dx->mCurrBackBuffer].Get(), mOutputBuffer.Get());

	dx->dx_sub[com_no].ResourceBarrier(mOutputBuffer.Get(),
		D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	dx->dx_sub[com_no].ResourceBarrier(dx->mSwapChainBuffer[dx->mCurrBackBuffer].Get(),
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_RENDER_TARGET);
}
