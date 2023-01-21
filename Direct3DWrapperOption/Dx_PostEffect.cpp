//*****************************************************************************************//
//**                                                                                     **//
//**                   　　　       PostEffectクラス                                     **//
//**                                                                                     **//
//*****************************************************************************************//

#include "Dx_PostEffect.h"
#include "Shader/ShaderPostEffect.h"
#include <stdio.h>
#include <math.h>

namespace {
	ComPtr<ID3DBlob> pComputeShader_Post[3] = {};

	bool createShaderDone = false;
}

void PostEffect::createShader() {

	if (createShaderDone)return;

	LPSTR csName[3] = {
		"MosaicCS",
		"BlurCS",
		"DepthOfFieldCS"
	};

	for (int i = 0; i < _countof(pComputeShader_Post); i++) {
		pComputeShader_Post[i] = CompileShader(ShaderPostEffect, strlen(ShaderPostEffect), csName[i], "cs_5_1");
	}

	createShaderDone = true;
}

PostEffect::PostEffect() {
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

bool PostEffect::ComCreateDepthOfField() {
	return ComCreate(2);
}

bool PostEffect::ComCreate(int no) {

	Dx_Device* device = Dx_Device::GetInstance();
	const int numSrv = 2;
	const int numUav = 1;
	const int numDesc = numSrv + numUav;
	mDescHeap = device->CreateDescHeap(numDesc);
	if (!mDescHeap) {
		Dx_Util::ErrorMessage("PostEffect::ComCreate Error!!"); return false;
	}
	if (FAILED(device->createDefaultResourceTEXTURE2D_UNORDERED_ACCESS(mOutputBuffer.GetAddressOf(), dx->getClientWidth(), dx->getClientHeight()))) {
		Dx_Util::ErrorMessage("PostEffect::ComCreate Error!!"); return false;
	}
	if (FAILED(device->createDefaultResourceTEXTURE2D(mInputBuffer.GetAddressOf(),
		dx->getClientWidth(), dx->getClientHeight(), DXGI_FORMAT_R8G8B8A8_UNORM))) {

		Dx_Util::ErrorMessage("PostEffect::ComCreate Error!!"); return false;
	}
	mOutputBuffer.Get()->SetName(Dx_Util::charToLPCWSTR("mOutputBuffer0", objName));
	mInputBuffer.Get()->SetName(Dx_Util::charToLPCWSTR("mInputBuffer", objName));

	mDepthHandleGPU = mDescHeap->GetGPUDescriptorHandleForHeapStart();
	mInputHandleGPU = mDepthHandleGPU;
	mOutputHandleGPU = mDepthHandleGPU;
	mInputHandleGPU.ptr += dx->getCbvSrvUavDescriptorSize() * 1;
	mOutputHandleGPU.ptr += dx->getCbvSrvUavDescriptorSize() * 2;

	D3D12_CPU_DESCRIPTOR_HANDLE hDescriptor = mDescHeap->GetCPUDescriptorHandleForHeapStart();
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	srvDesc.Format = dx->getDepthStencilSrvFormat();
	srvDesc.Texture2D.MipLevels = GetDepthStencilBuffer()->GetDesc().MipLevels;

	device->getDevice()->CreateShaderResourceView(GetDepthStencilBuffer(), &srvDesc, hDescriptor);
	hDescriptor.ptr += dx->getCbvSrvUavDescriptorSize();

	device->CreateSrvTexture(hDescriptor, mInputBuffer.GetAddressOf(), 1);

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	uavDesc.Texture2D.MipSlice = 0;

	device->getDevice()->CreateUnorderedAccessView(mOutputBuffer.Get(), nullptr, &uavDesc, hDescriptor);
	hDescriptor.ptr += dx->getCbvSrvUavDescriptorSize();

	comObj->ResourceBarrier(mInputBuffer.Get(),
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_GENERIC_READ);

	comObj->ResourceBarrier(mOutputBuffer.Get(),
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	//ルートシグネチャ
	const int numCbv = 1;
	const int numPara = numDesc + numCbv;
	D3D12_ROOT_PARAMETER rootParameter[numPara] = {};
	D3D12_ROOT_PARAMETER& r = rootParameter[0];
	r.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	r.Descriptor.ShaderRegister = 0;
	r.Descriptor.RegisterSpace = 0;
	r.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	D3D12_DESCRIPTOR_RANGE sRange[numSrv] = {};
	for (int i = 0; i < numSrv; i++) {
		D3D12_DESCRIPTOR_RANGE& s = sRange[i];
		s.BaseShaderRegister = i;
		s.NumDescriptors = 1;
		s.RegisterSpace = 0;
		s.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		s.OffsetInDescriptorsFromTableStart = 0;
		D3D12_ROOT_PARAMETER& r = rootParameter[i + numCbv];
		r.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		r.DescriptorTable.NumDescriptorRanges = 1;
		r.DescriptorTable.pDescriptorRanges = &s;
		r.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	}

	D3D12_DESCRIPTOR_RANGE uRange[numUav] = {};
	for (int i = 0; i < numUav; i++) {
		D3D12_DESCRIPTOR_RANGE& u = uRange[i];
		u.BaseShaderRegister = i;
		u.NumDescriptors = 1;
		u.RegisterSpace = 0;
		u.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
		u.OffsetInDescriptorsFromTableStart = 0;
		D3D12_ROOT_PARAMETER& r = rootParameter[i + numCbv + numSrv];
		r.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		r.DescriptorTable.NumDescriptorRanges = 1;
		r.DescriptorTable.pDescriptorRanges = &u;
		r.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	}

	mRootSignatureCom = CreateRsCompute(numPara, rootParameter);
	if (mRootSignatureCom == nullptr)return false;

	createShader();
	cs = pComputeShader_Post[no].Get();
	mPSOCom = CreatePsoCompute(cs, mRootSignatureCom.Get());
	if (mPSOCom == nullptr)return false;

	return true;
}

void PostEffect::ComputeMosaic(int com, bool On, int size) {
	Compute(com, On, size, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
}

void PostEffect::ComputeBlur(int com, bool On, float blurX, float blurY, float blurLevel) {
	Compute(com, On, 0, blurX, blurY, blurLevel, 0.0f, 0.0f);
}

void PostEffect::ComputeDepthOfField(int com, bool On, float blurLevel, float focusDepth, float focusRange) {
	Compute(com, On, 0, 0.0f, 0.0f, blurLevel, focusDepth, focusRange);
}

void PostEffect::ComputeMosaic(bool On, int size) {
	ComputeMosaic(com_no, On, size);
}

void PostEffect::ComputeBlur(bool On, float blurX, float blurY, float blurLevel) {
	ComputeBlur(com_no, On, blurX, blurY, blurLevel);
}

void PostEffect::ComputeDepthOfField(bool On, float blurLevel, float focusDepth, float focusRange) {
	ComputeDepthOfField(com_no, On, blurLevel, focusDepth, focusRange);
}

void PostEffect::Compute(int com, bool On, int size,
	float blurX, float blurY, float blurLevel,
	float focusDepth, float focusRange) {

	if (!On)return;

	SetCommandList(com);
	ID3D12GraphicsCommandList* mCList = mCommandList;
	Dx_CommandListObj& d = *comObj;

	CONSTANT_BUFFER_PostMosaic cb = {};
	cb.mosaicSize.x = (float)size;
	cb.blur.x = blurX;
	cb.blur.y = blurY;
	cb.blur.z = blurLevel;
	cb.blur.w = focusDepth;
	cb.focusRange = focusRange;
	mObjectCB->CopyData(0, cb);

	mCList->SetPipelineState(mPSOCom.Get());

	ID3D12DescriptorHeap* descriptorHeaps[] = { mDescHeap.Get() };
	mCList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
	mCList->SetComputeRootSignature(mRootSignatureCom.Get());
	mCList->SetComputeRootConstantBufferView(0, mObjectCB->Resource()->GetGPUVirtualAddress());

	//深度バッファ
	d.delayResourceBarrierBefore(GetDepthStencilBuffer(),
		D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_DEPTH_READ);
	//バックバッファをコピー元にする
	d.delayResourceBarrierBefore(GetSwapChainBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE);
	d.delayResourceBarrierBefore(mInputBuffer.Get(),
		D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COPY_DEST);
	d.RunDelayResourceBarrierBefore();
	//現在のバックバッファをインプット用バッファにコピーする
	mCList->CopyResource(mInputBuffer.Get(), GetSwapChainBuffer());

	d.ResourceBarrier(mInputBuffer.Get(),
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);

	mCList->SetComputeRootDescriptorTable(1, mDepthHandleGPU);
	mCList->SetComputeRootDescriptorTable(2, mInputHandleGPU);
	mCList->SetComputeRootDescriptorTable(3, mOutputHandleGPU);

	D3D12_RESOURCE_BARRIER ba = {};
	ba.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;

	UINT disX = dx->getClientWidth() / 32;
	UINT disY = dx->getClientHeight() / 8;
	mCList->Dispatch(disX, disY, 1);//CS内32, 8, 1
	ba.UAV.pResource = mOutputBuffer.Get();
	mCList->ResourceBarrier(1, &ba);

	//深度バッファ
	d.delayResourceBarrierBefore(GetDepthStencilBuffer(),
		D3D12_RESOURCE_STATE_DEPTH_READ, D3D12_RESOURCE_STATE_DEPTH_WRITE);

	d.delayResourceBarrierBefore(GetSwapChainBuffer(),
		D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_COPY_DEST);
	d.delayResourceBarrierBefore(mOutputBuffer.Get(),
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_GENERIC_READ);
	d.RunDelayResourceBarrierBefore();
	//計算後バックバッファにコピー
	mCList->CopyResource(GetSwapChainBuffer(), mOutputBuffer.Get());

	d.delayResourceBarrierBefore(mOutputBuffer.Get(),
		D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	d.delayResourceBarrierBefore(GetSwapChainBuffer(),
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_RENDER_TARGET);
	d.RunDelayResourceBarrierBefore();
}
