//*****************************************************************************************//
//**                                                                                     **//
//**                   Å@Å@Å@         PostProcessing                                     **//
//**                                                                                     **//
//*****************************************************************************************//

#include "PostProcessing.h"

PostProcessing::~PostProcessing() {
	S_DELETE(mObjectCB);
}

bool PostProcessing::Initialize(std::vector<Dx_Resource*>& po) {

	NotUseDepthBuffer = true;
	mObjectCB = NEW ConstantBuffer<ToneMapCB>(1);
	cb.Exposure = 1.0f;

	const int numSrv = 1;

	mRootSignature = CreateRootSignature(numSrv, 0, 0, 1, 0, 0, nullptr);
	if (mRootSignature == nullptr)return false;

	Dx_Device* device = Dx_Device::GetInstance();
	mDescHeap = device->CreateDescHeap((int)po.size());

	D3D12_CPU_DESCRIPTOR_HANDLE hDescriptor(mDescHeap->GetCPUDescriptorHandleForHeapStart());

	for (size_t i = 0; i < po.size(); i++) {
		po[i]->CreateSrvTexture(hDescriptor);
	}

	mPSO = CreatePsoVsPs(Dx_ShaderHolder::pVertexShader_Post.Get(), Dx_ShaderHolder::pPixelShader_Post.Get(), mRootSignature.Get(), false, false);
	if (mPSO == nullptr)return false;

	return true;
}

ComPtr <ID3D12PipelineState> PostProcessing::CreatePsoVsPs(
	ID3DBlob* vs, ID3DBlob* ps,
	ID3D12RootSignature* mRootSignature,
	bool alpha, bool blend) {

	return CreatePSO(vs, nullptr, nullptr, ps, nullptr, mRootSignature, nullptr,
		false, nullptr, 0, nullptr, 0, alpha, blend, SQUARE);
}

void PostProcessing::setExposure(float exposure) {
	cb.Exposure = exposure;
}

void PostProcessing::Execute(uint32_t comIndex, uint32_t resourceIndex) {

	ID3D12GraphicsCommandList* mCList = Dx_CommandManager::GetInstance()->getGraphicsComListObj(comIndex)->getCommandList();

	mObjectCB->CopyData(0, cb);

	mCList->SetPipelineState(mPSO.Get());

	ID3D12DescriptorHeap* descriptorHeaps[] = { mDescHeap.Get() };
	mCList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	mCList->SetGraphicsRootSignature(mRootSignature.Get());

	mCList->SetGraphicsRootConstantBufferView(1, mObjectCB->Resource()->GetGPUVirtualAddress());

	mCList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	D3D12_GPU_DESCRIPTOR_HANDLE heap(mDescHeap->GetGPUDescriptorHandleForHeapStart());
	Dx_Device* dev = Dx_Device::GetInstance();
	heap.ptr += dev->getCbvSrvUavDescriptorSize() * resourceIndex;

	mCList->SetGraphicsRootDescriptorTable(0, heap);

	mCList->DrawInstanced(3, 1, 0, 0);
}