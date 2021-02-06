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

	D3D12_DESCRIPTOR_HEAP_DESC HeapDesc = {};
	HeapDesc.NumDescriptors = 3;
	HeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	HeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	if (FAILED(dx->getDevice()->CreateDescriptorHeap(&HeapDesc, IID_PPV_ARGS(&mDescHeap)))) {
		ErrorMessage("PostEffect::ComCreate Error!!"); return false;
	}
	if (FAILED(dx->device->createDefaultResourceTEXTURE2D_UNORDERED_ACCESS(&mOutputBuffer, dx->mClientWidth, dx->mClientHeight))) {
		ErrorMessage("PostEffect::ComCreate Error!!"); return false;
	}
	if (FAILED(dx->device->createDefaultResourceTEXTURE2D_UNORDERED_ACCESS(&mInputBuffer, dx->mClientWidth, dx->mClientHeight))) {
		ErrorMessage("PostEffect::ComCreate Error!!"); return false;
	}

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	uavDesc.Texture2D.MipSlice = 0;

	D3D12_CPU_DESCRIPTOR_HANDLE hDescriptor = mDescHeap->GetCPUDescriptorHandleForHeapStart();
	D3D12_GPU_VIRTUAL_ADDRESS ad = mObjectCB->Resource()->GetGPUVirtualAddress();
	UINT size = mObjectCB->getSizeInBytes();
	CreateCbv(mDescHeap.Get(), 0, &ad, &size, 1);
	hDescriptor.ptr += dx->mCbvSrvUavDescriptorSize;
	dx->getDevice()->CreateUnorderedAccessView(mInputBuffer.Get(), nullptr, &uavDesc, hDescriptor);
	hDescriptor.ptr += dx->mCbvSrvUavDescriptorSize;
	dx->getDevice()->CreateUnorderedAccessView(mOutputBuffer.Get(), nullptr, &uavDesc, hDescriptor);

	dx->dx_sub[com_no].ResourceBarrier(mInputBuffer.Get(),
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	dx->dx_sub[com_no].ResourceBarrier(mOutputBuffer.Get(),
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	//ルートシグネチャ
	mRootSignatureCom = CreateRootSignatureCompute(0, 1, 2, 0, 0);
	if (mRootSignatureCom == nullptr)return false;

	cs = dx->shaderH->pComputeShader_Post[no].Get();

	//PSO
	mPSOCom = CreatePsoCompute(cs, mRootSignatureCom.Get());
	if (mPSOCom == nullptr)return false;

	return true;
}

void PostEffect::ComputeMosaic(int com, bool On, int size) {
	Compute(com, On, size, 0.0f, 0.0f, 0.0f);
}

void PostEffect::ComputeBlur(int com, bool On, float blurX, float blurY, float blurLevel) {
	Compute(com, On, 0, blurX, blurY, blurLevel);
}

void PostEffect::ComputeMosaic(bool On, int size) {
	ComputeMosaic(com_no, On, size);
}

void PostEffect::ComputeBlur(bool On, float blurX, float blurY, float blurLevel) {
	ComputeBlur(com_no, On, blurX, blurY, blurLevel);
}

void PostEffect::Compute(int com, bool On, int size, float blurX, float blurY, float blurLevel) {

	if (!On)return;

	ID3D12GraphicsCommandList* mCList = dx->dx_sub[com].mCommandList.Get();
	Dx_CommandListObj& d = dx->dx_sub[com];

	CONSTANT_BUFFER_PostMosaic cb;
	cb.mosaicSize.x = (float)size;
	cb.blur.x = blurX;
	cb.blur.y = blurY;
	cb.blur.z = blurLevel;
	mObjectCB->CopyData(0, cb);

	mCList->SetPipelineState(mPSOCom.Get());

	ID3D12DescriptorHeap* descriptorHeaps[] = { mDescHeap.Get() };
	mCList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
	mCList->SetComputeRootSignature(mRootSignatureCom.Get());

	//バックバッファをコピー元にする
	d.delayResourceBarrierBefore(dx->mSwapChainBuffer[dx->mCurrBackBuffer].Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE);
	d.delayResourceBarrierBefore(mInputBuffer.Get(),
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_DEST);
	d.RunDelayResourceBarrierBefore();
	//現在のバックバッファをインプット用バッファにコピーする
	mCList->CopyResource(mInputBuffer.Get(), dx->mSwapChainBuffer[dx->mCurrBackBuffer].Get());

	d.ResourceBarrier(mInputBuffer.Get(),
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	D3D12_GPU_DESCRIPTOR_HANDLE des = mDescHeap->GetGPUDescriptorHandleForHeapStart();
	mCList->SetComputeRootDescriptorTable(0, des);

	UINT disX = dx->mClientWidth / 32;
	UINT disY = dx->mClientHeight / 8;
	mCList->Dispatch(disX, disY, 1);//CS内32, 8, 1

	d.delayResourceBarrierBefore(dx->mSwapChainBuffer[dx->mCurrBackBuffer].Get(),
		D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_COPY_DEST);
	d.delayResourceBarrierBefore(mOutputBuffer.Get(),
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_GENERIC_READ);
	d.RunDelayResourceBarrierBefore();
	//計算後バックバッファにコピー
	mCList->CopyResource(dx->mSwapChainBuffer[dx->mCurrBackBuffer].Get(), mOutputBuffer.Get());

	d.delayResourceBarrierBefore(mOutputBuffer.Get(),
		D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	d.delayResourceBarrierBefore(dx->mSwapChainBuffer[dx->mCurrBackBuffer].Get(),
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_RENDER_TARGET);
	d.RunDelayResourceBarrierBefore();
}
