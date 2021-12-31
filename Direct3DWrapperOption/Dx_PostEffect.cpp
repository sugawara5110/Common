//*****************************************************************************************//
//**                                                                                     **//
//**                   �@�@�@       PostEffect�N���X                                     **//
//**                                                                                     **//
//*****************************************************************************************//

#include "Dx_PostEffect.h"
#include "Shader/ShaderPostEffect.h"

namespace {
	ComPtr<ID3DBlob> pComputeShader_Post[3] = {};

	bool createShaderDone = false;
}

void PostEffect::createShader() {

	if (createShaderDone)return;
	//�|�X�g�G�t�F�N�g
	pComputeShader_Post[0] = CompileShader(ShaderPostEffect, strlen(ShaderPostEffect), "MosaicCS", "cs_5_1");
	pComputeShader_Post[1] = CompileShader(ShaderPostEffect, strlen(ShaderPostEffect), "BlurCS", "cs_5_1");
	pComputeShader_Post[2] = CompileShader(ShaderPostEffect, strlen(ShaderPostEffect), "DepthOfFieldCS", "cs_5_1");

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
	mDescHeap = device->CreateDescHeap(4);
	if (!mDescHeap) {
		Dx_Util::ErrorMessage("PostEffect::ComCreate Error!!"); return false;
	}
	if (FAILED(device->createDefaultResourceTEXTURE2D_UNORDERED_ACCESS(&mOutputBuffer, dx->getClientWidth(), dx->getClientHeight()))) {
		Dx_Util::ErrorMessage("PostEffect::ComCreate Error!!"); return false;
	}
	if (FAILED(device->createDefaultResourceTEXTURE2D_UNORDERED_ACCESS(&mInputBuffer, dx->getClientWidth(), dx->getClientHeight()))) {
		Dx_Util::ErrorMessage("PostEffect::ComCreate Error!!"); return false;
	}
	mOutputBuffer.Get()->SetName(Dx_Util::charToLPCWSTR("mOutputBuffer", objName));
	mInputBuffer.Get()->SetName(Dx_Util::charToLPCWSTR("mInputBuffer", objName));

	D3D12_CPU_DESCRIPTOR_HANDLE hDescriptor = mDescHeap->GetCPUDescriptorHandleForHeapStart();

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	uavDesc.Texture2D.MipSlice = 0;

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	srvDesc.Format = dx->getDepthStencilSrvFormat();
	srvDesc.Texture2D.MipLevels = GetDepthStencilBuffer()->GetDesc().MipLevels;

	device->getDevice()->CreateShaderResourceView(GetDepthStencilBuffer(), &srvDesc, hDescriptor);
	hDescriptor.ptr += dx->getCbvSrvUavDescriptorSize();

	D3D12_GPU_VIRTUAL_ADDRESS ad = mObjectCB->Resource()->GetGPUVirtualAddress();
	UINT size = mObjectCB->getSizeInBytes();
	device->CreateCbv(hDescriptor, &ad, &size, 1);
	device->getDevice()->CreateUnorderedAccessView(mInputBuffer.Get(), nullptr, &uavDesc, hDescriptor);
	hDescriptor.ptr += dx->getCbvSrvUavDescriptorSize();
	device->getDevice()->CreateUnorderedAccessView(mOutputBuffer.Get(), nullptr, &uavDesc, hDescriptor);

	comObj->ResourceBarrier(mInputBuffer.Get(),
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	comObj->ResourceBarrier(mOutputBuffer.Get(),
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	//���[�g�V�O�l�`��
	mRootSignatureCom = CreateRootSignatureCompute(1, 1, 2, 0, 0, 0, nullptr);
	if (mRootSignatureCom == nullptr)return false;

	createShader();
	cs = pComputeShader_Post[no].Get();

	//PSO
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

	//�[�x�o�b�t�@
	d.delayResourceBarrierBefore(GetDepthStencilBuffer(),
		D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_DEPTH_READ);
	//�o�b�N�o�b�t�@���R�s�[���ɂ���
	d.delayResourceBarrierBefore(GetSwapChainBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE);
	d.delayResourceBarrierBefore(mInputBuffer.Get(),
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_DEST);
	d.RunDelayResourceBarrierBefore();
	//���݂̃o�b�N�o�b�t�@���C���v�b�g�p�o�b�t�@�ɃR�s�[����
	mCList->CopyResource(mInputBuffer.Get(), GetSwapChainBuffer());

	d.ResourceBarrier(mInputBuffer.Get(),
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	D3D12_GPU_DESCRIPTOR_HANDLE des = mDescHeap->GetGPUDescriptorHandleForHeapStart();
	mCList->SetComputeRootDescriptorTable(0, des);

	UINT disX = dx->getClientWidth() / 32;
	UINT disY = dx->getClientHeight() / 8;
	mCList->Dispatch(disX, disY, 1);//CS��32, 8, 1

	//�[�x�o�b�t�@
	d.delayResourceBarrierBefore(GetDepthStencilBuffer(),
		D3D12_RESOURCE_STATE_DEPTH_READ, D3D12_RESOURCE_STATE_DEPTH_WRITE);

	d.delayResourceBarrierBefore(GetSwapChainBuffer(),
		D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_COPY_DEST);
	d.delayResourceBarrierBefore(mOutputBuffer.Get(),
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_GENERIC_READ);
	d.RunDelayResourceBarrierBefore();
	//�v�Z��o�b�N�o�b�t�@�ɃR�s�[
	mCList->CopyResource(GetSwapChainBuffer(), mOutputBuffer.Get());

	d.delayResourceBarrierBefore(mOutputBuffer.Get(),
		D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	d.delayResourceBarrierBefore(GetSwapChainBuffer(),
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_RENDER_TARGET);
	d.RunDelayResourceBarrierBefore();
}
