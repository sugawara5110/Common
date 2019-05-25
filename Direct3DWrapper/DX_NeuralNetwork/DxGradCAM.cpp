//*****************************************************************************************//
//**                                                                                     **//
//**                   　  　　　  DxGradCAM                                             **//
//**                                                                                     **//
//*****************************************************************************************//

#include "DxGradCAM.h"
#include "ShaderNN\ShaderGradCAM.h"

DxGradCAM::DxGradCAM(UINT SizeFeatureMapW, UINT SizeFeatureMapH, UINT NumGradientEl, UINT NumFil, UINT Inputsetnum) {
	dx = Dx12Process::GetInstance();

	cb.NumFil = NumFil;
	cb.SizeFeatureMapW = SizeFeatureMapW;
	cb.SizeFeatureMapH = SizeFeatureMapH;
	cb.NumConvFilElement = NumGradientEl;
	inputsetnum = Inputsetnum;
	mCommandList = dx->dx_sub[0].mCommandList.Get();
	mObjectCB = new ConstantBuffer<CBGradCAM>(1);
	mObjectCB->CopyData(0, cb);
}

DxGradCAM::~DxGradCAM() {
	S_DELETE(mObjectCB);
}

void DxGradCAM::ComCreate(UINT srcWid, UINT srcHei, float SignalStrength) {
	cb.srcWid = srcWid;
	cb.srcHei = srcHei;
	cb.SignalStrength = SignalStrength;
	mObjectCB->CopyData(0, cb);

	//RWStructuredBuffer用gInputFeatureMapBuffer
	CreateResourceDef(mInputFeatureMapBuffer, cb.SizeFeatureMapW * cb.SizeFeatureMapH * cb.NumFil * inputsetnum * sizeof(float));

	//RWStructuredBuffer用gInputGradientBuffer
	CreateResourceDef(mInputGradientBuffer, cb.NumConvFilElement * cb.NumFil * sizeof(float));

	//RWStructuredBuffer用gGlobalAveragePoolingBuffer
	CreateResourceDef(mGlobalAveragePoolingBuffer, cb.NumFil * sizeof(float));

	//RWStructuredBuffer用gOutGradCAMBuffer
	CreateResourceDef(mOutGradCAMBuffer, cb.SizeFeatureMapW * cb.SizeFeatureMapH * inputsetnum * sizeof(float));

	//RWStructuredBuffer用gOutGradCAMSynthesisBuffer(OutGradCAMBuffer合成)
	CreateResourceDef(mOutGradCAMSynthesisBuffer, cb.srcWid * cb.srcHei * sizeof(float));

	D3D12_DESCRIPTOR_HEAP_DESC uavHeapDesc = {};
	uavHeapDesc.NumDescriptors = 1;
	uavHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	uavHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	dx->md3dDevice->CreateDescriptorHeap(&uavHeapDesc, IID_PPV_ARGS(&mUavHeap));

	D3D12_RESOURCE_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Alignment = 0;
	texDesc.Width = srcWid;
	texDesc.Height = srcHei;
	texDesc.DepthOrArraySize = 1;
	texDesc.MipLevels = 1;
	texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	//RWTexture2D用gInputCol
	dx->md3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(&mInputColBuffer));

	UINT64 uploadBufferSize = GetRequiredIntermediateSize(mInputColBuffer.Get(), 0, 1);
	D3D12_HEAP_PROPERTIES HeapPropsUp;
	HeapPropsUp.Type = D3D12_HEAP_TYPE_UPLOAD;
	HeapPropsUp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	HeapPropsUp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	HeapPropsUp.CreationNodeMask = 1;
	HeapPropsUp.VisibleNodeMask = 1;

	D3D12_RESOURCE_DESC BufferDesc;
	BufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	BufferDesc.Alignment = 0;
	BufferDesc.Width = uploadBufferSize;
	BufferDesc.Height = 1;
	BufferDesc.DepthOrArraySize = 1;
	BufferDesc.MipLevels = 1;
	BufferDesc.Format = DXGI_FORMAT_UNKNOWN;
	BufferDesc.SampleDesc.Count = 1;
	BufferDesc.SampleDesc.Quality = 0;
	BufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	BufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	//up用gInputCol
	dx->md3dDevice->CreateCommittedResource(
		&HeapPropsUp,
		D3D12_HEAP_FLAG_NONE,
		&BufferDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&mInputColUpBuffer));

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	uavDesc.Texture2D.MipSlice = 0;

	CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(mUavHeap->GetCPUDescriptorHandleForHeapStart());
	dx->md3dDevice->CreateUnorderedAccessView(mInputColBuffer.Get(), nullptr, &uavDesc, hDescriptor);

	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mInputColBuffer.Get(),
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));

	//ルートシグネチャ
	CD3DX12_DESCRIPTOR_RANGE uavTable;
	uavTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 5);
	CD3DX12_ROOT_PARAMETER slotRootParameter[7];
	slotRootParameter[0].InitAsUnorderedAccessView(0);//RWStructuredBuffer(u0)
	slotRootParameter[1].InitAsUnorderedAccessView(1);//RWStructuredBuffer(u1)
	slotRootParameter[2].InitAsUnorderedAccessView(2);//RWStructuredBuffer(u2)
	slotRootParameter[3].InitAsUnorderedAccessView(3);//RWStructuredBuffer(u3)
	slotRootParameter[4].InitAsUnorderedAccessView(4);//RWStructuredBuffer(u4)
	slotRootParameter[5].InitAsDescriptorTable(1, &uavTable);//RWTexture2D(u5)
	slotRootParameter[6].InitAsConstantBufferView(0);//mObjectCB(b0)
	mRootSignatureCom = CreateRsCompute(7, slotRootParameter);

	pCS[0] = CompileShader(ShaderGradCAM, strlen(ShaderGradCAM), "ComGAP", "cs_5_0");
	pCS[1] = CompileShader(ShaderGradCAM, strlen(ShaderGradCAM), "ComGradCAM", "cs_5_0");
	pCS[2] = CompileShader(ShaderGradCAM, strlen(ShaderGradCAM), "InitGradCAMSynthesis", "cs_5_0");
	pCS[3] = CompileShader(ShaderGradCAM, strlen(ShaderGradCAM), "ComGradCAMSynthesis", "cs_5_0");
	pCS[4] = CompileShader(ShaderGradCAM, strlen(ShaderGradCAM), "ComPixelSynthesis", "cs_5_0");

	for (int i = 0; i < GC_SHADER_NUM; i++)
		mPSOCom[i] = CreatePsoCompute(pCS[i].Get(), mRootSignatureCom.Get());
}

void DxGradCAM::SetFeatureMap(ID3D12Resource* res) {
	CopyResource(mInputFeatureMapBuffer.Get(), res);
}

void DxGradCAM::SetGradient(ID3D12Resource* res) {
	CopyResource(mInputGradientBuffer.Get(), res);
}

void DxGradCAM::ComGAP() {
	dx->Bigin(com_no);
	mCommandList->SetPipelineState(mPSOCom[0].Get());
	mCommandList->SetComputeRootSignature(mRootSignatureCom.Get());
	mCommandList->SetComputeRootUnorderedAccessView(1, mInputGradientBuffer->GetGPUVirtualAddress());
	mCommandList->SetComputeRootUnorderedAccessView(2, mGlobalAveragePoolingBuffer->GetGPUVirtualAddress());
	mCommandList->SetComputeRootConstantBufferView(6, mObjectCB->Resource()->GetGPUVirtualAddress());
	mCommandList->Dispatch(cb.NumFil, 1, 1);
	dx->End(com_no);
	dx->WaitFenceCurrent();
}

void DxGradCAM::ComGradCAM(UINT inputsetnum) {
	dx->Bigin(com_no);
	mCommandList->SetPipelineState(mPSOCom[1].Get());
	mCommandList->SetComputeRootSignature(mRootSignatureCom.Get());
	mCommandList->SetComputeRootUnorderedAccessView(0, mInputFeatureMapBuffer->GetGPUVirtualAddress());
	mCommandList->SetComputeRootUnorderedAccessView(2, mGlobalAveragePoolingBuffer->GetGPUVirtualAddress());
	mCommandList->SetComputeRootUnorderedAccessView(3, mOutGradCAMBuffer->GetGPUVirtualAddress());
	mCommandList->SetComputeRootConstantBufferView(6, mObjectCB->Resource()->GetGPUVirtualAddress());
	mCommandList->Dispatch(cb.SizeFeatureMapW * cb.SizeFeatureMapH, 1, inputsetnum);
	dx->End(com_no);
	dx->WaitFenceCurrent();
}

void DxGradCAM::SetPixel3ch(ID3D12Resource* pi) {
	dx->Bigin(com_no);
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mInputColBuffer.Get(),
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_DEST));
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(pi,
		D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COPY_SOURCE));

	mCommandList->CopyResource(mInputColBuffer.Get(), pi);

	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(pi,
		D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_GENERIC_READ));
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mInputColBuffer.Get(),
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
	dx->End(com_no);
	dx->WaitFenceCurrent();
}

void DxGradCAM::SetPixel3ch(BYTE* pi) {
	dx->Bigin(com_no);
	SubresourcesUp(pi, cb.srcWid * 4, mInputColBuffer, mInputColUpBuffer);
	dx->End(com_no);
	dx->WaitFenceCurrent();
}

void DxGradCAM::GradCAMSynthesis(UINT srcConvMapW, UINT srcConvMapH, UINT MapSlide) {
	cb.srcConvMapW = srcConvMapW;
	cb.srcConvMapH = srcConvMapH;
	cb.MapSlide = MapSlide;
	cb.NumFeatureMapW = (cb.srcWid - cb.srcConvMapW + cb.MapSlide) / cb.MapSlide;//ブロック数w
	cb.NumFeatureMapH = (cb.srcHei - cb.srcConvMapH + cb.MapSlide) / cb.MapSlide;//ブロック数h
	mObjectCB->CopyData(0, cb);

	dx->Bigin(com_no);
	mCommandList->SetPipelineState(mPSOCom[2].Get());
	mCommandList->SetComputeRootSignature(mRootSignatureCom.Get());
	mCommandList->SetComputeRootUnorderedAccessView(4, mOutGradCAMSynthesisBuffer->GetGPUVirtualAddress());
	mCommandList->SetComputeRootConstantBufferView(6, mObjectCB->Resource()->GetGPUVirtualAddress());
	mCommandList->Dispatch(cb.srcWid, cb.srcHei, 1);
	dx->End(com_no);
	dx->WaitFenceCurrent();

	dx->Bigin(com_no);
	mCommandList->SetPipelineState(mPSOCom[3].Get());
	mCommandList->SetComputeRootSignature(mRootSignatureCom.Get());
	mCommandList->SetComputeRootUnorderedAccessView(3, mOutGradCAMBuffer->GetGPUVirtualAddress());
	mCommandList->SetComputeRootUnorderedAccessView(4, mOutGradCAMSynthesisBuffer->GetGPUVirtualAddress());
	mCommandList->SetComputeRootConstantBufferView(6, mObjectCB->Resource()->GetGPUVirtualAddress());
	mCommandList->Dispatch(cb.srcConvMapW, cb.srcConvMapH, 1);
	dx->End(com_no);
	dx->WaitFenceCurrent();

	dx->Bigin(com_no);
	mCommandList->SetPipelineState(mPSOCom[4].Get());

	ID3D12DescriptorHeap * descriptorHeaps[] = { mUavHeap.Get() };
	mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	mCommandList->SetComputeRootSignature(mRootSignatureCom.Get());
	mCommandList->SetComputeRootUnorderedAccessView(4, mOutGradCAMSynthesisBuffer->GetGPUVirtualAddress());
	CD3DX12_GPU_DESCRIPTOR_HANDLE uav(mUavHeap->GetGPUDescriptorHandleForHeapStart());
	mCommandList->SetComputeRootDescriptorTable(5, uav);
	mCommandList->SetComputeRootConstantBufferView(6, mObjectCB->Resource()->GetGPUVirtualAddress());
	mCommandList->Dispatch(cb.srcWid, cb.srcHei, 1);
	dx->End(com_no);
	dx->WaitFenceCurrent();
}

ID3D12Resource* DxGradCAM::GetPixel() {
	return mInputColBuffer.Get();
}