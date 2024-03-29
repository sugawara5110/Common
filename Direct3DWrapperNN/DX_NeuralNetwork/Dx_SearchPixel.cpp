//*****************************************************************************************//
//**                                                                                     **//
//**                   　  　　　 SearchPixel                                            **//
//**                                                                                     **//
//*****************************************************************************************//

#include "Dx_SearchPixel.h"
#include "ShaderNN\ShaderSearchPixel.h"

SearchPixel::SearchPixel(UINT srcwid, UINT srchei, UINT seawid, UINT seahei, float outscale, UINT step, UINT outnum, float Threshold) {

	Step = step;
	srcWidth = srcwid;
	srcHeight = srchei;
	insize = srcWidth * srcHeight * sizeof(float);
	srcPix = new float[srcWidth * srcHeight];
	seaWid = seawid;
	seaHei = seahei;
	UINT wNum = (srcWidth - seaWid + Step) / Step;//ブロック数w
	UINT hNum = (srcHeight - seaHei + Step) / Step;//ブロック数h
	searchNum = wNum * hNum;
	outNum = outnum * searchNum;
	outWid = (UINT)(wNum * seaWid * outscale);
	outHei = (UINT)(hNum * seaHei * outscale);
	outsize = outWid * outHei * sizeof(float);
	seaPix = new float[outWid * outHei];

	sdata = new SearchPixelData[wNum * hNum];
	for (UINT j = 0; j < hNum; j++) {
		for (UINT i = 0; i < wNum; i++) {
			sdata[wNum * j + i].stW = Step * i;
			sdata[wNum * j + i].stH = Step * j;
			sdata[wNum * j + i].enW = Step * i + seaWid - 1;
			sdata[wNum * j + i].enH = Step * j + seaHei - 1;
		}
	}

	outIndW = wNum * seaWid;
	outIndH = hNum * seaHei;
	outIndNum = outIndW * outIndH;
	outIndSize = outIndNum * sizeof(float);
	outInd = new float[outIndNum];
	float fw = 0.0f;
	float fh = 0.0f;
	UINT uw = 0;
	UINT uh = 0;
	float sw = (float)seaWid * outscale;
	for (UINT j = 0; j < hNum; j++) {
		for (UINT i = 0; i < wNum; i++) {
			for (UINT j1 = 0; j1 < seaHei; j1++) {
				fw = 0.0f;
				for (UINT i1 = 0; i1 < seaWid; i1++) {
					UINT ox = seaWid * i + i1;
					UINT oy = seaHei * j + j1;
					UINT ind = outIndW * oy + ox;
					outInd[ind] = sw * (float)uh + (float)uw;
					uw = (UINT)(fw += outscale);
				}
				uh = (UINT)(fh += outscale);
			}
		}
	}

	mObjectCB = new ConstantBuffer<CBSearchPixel>(1);
	cb.InWH_OutWH.as((float)srcWidth, (float)srcHeight, (float)outIndW, (float)outIndH);
	cb.seaWH_step_PDNum.as((float)seaWid, (float)seaHei, (float)Step, (float)searchNum);
	cb.Threshold.x = Threshold;
	mObjectCB->CopyData(0, cb);
}

SearchPixel::~SearchPixel() {
	ARR_DELETE(srcPix);
	ARR_DELETE(seaPix);
	ARR_DELETE(sdata);
	ARR_DELETE(outInd);
	S_DELETE(mObjectCB);
	ARR_DELETE(shaderThreadNum);
}

UINT SearchPixel::GetSearchNum() {
	return searchNum;
}

void SearchPixel::ComCreate() {
	//up用gInput
	CreateResourceUp(mInputUpBuffer, insize);

	//RWStructuredBuffer用gInput
	CreateResourceDef(mInputBuffer, insize);

	//RWStructuredBuffer用gOutput
	CreateResourceDef(mOutputBuffer, outsize);

	//read用gOutput
	CreateResourceRead(mOutputReadBuffer, outsize);

	//up用gInPixPos
	CreateResourceUp(mInPixPosUpBuffer, searchNum * sizeof(SearchPixelData));

	//RWStructuredBuffer用gInPixPos
	CreateResourceDef(mInPixPosBuffer, searchNum * sizeof(SearchPixelData));

	//up用gOutInd
	CreateResourceUp(mOutIndUpBuffer, outIndSize);

	//RWStructuredBuffer用gOutInd
	CreateResourceDef(mOutIndBuffer, outIndSize);

	//up用gNNOutput
	CreateResourceUp(mNNOutputUpBuffer, outNum * sizeof(float));

	//RWStructuredBuffer用gNNOutput
	CreateResourceDef(mNNOutputBuffer, outNum * sizeof(float));

	D3D12_DESCRIPTOR_HEAP_DESC uavHeapDesc = {};
	uavHeapDesc.NumDescriptors = 1;
	uavHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	uavHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	Dx_Device* device = Dx_Device::GetInstance();
	device->getDevice()->CreateDescriptorHeap(&uavHeapDesc, IID_PPV_ARGS(&mUavHeap));

	D3D12_RESOURCE_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Alignment = 0;
	texDesc.Width = srcWidth;
	texDesc.Height = srcHeight;
	texDesc.DepthOrArraySize = 1;
	texDesc.MipLevels = 1;
	texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	//RWTexture2D用gInputCol
	device->getDevice()->CreateCommittedResource(
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
	device->getDevice()->CreateCommittedResource(
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
	device->getDevice()->CreateUnorderedAccessView(mInputColBuffer.Get(), nullptr, &uavDesc, hDescriptor);

	CList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mInputColBuffer.Get(),
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));

	SubresourcesUp(sdata, searchNum, mInPixPosBuffer, mInPixPosUpBuffer);

	SubresourcesUp(outInd, (UINT)outIndSize, mOutIndBuffer, mOutIndUpBuffer);

	D3D12_DESCRIPTOR_RANGE uavTable;
	uavTable.BaseShaderRegister = 5;
	uavTable.NumDescriptors = 1;
	uavTable.RegisterSpace = 0;
	uavTable.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
	uavTable.OffsetInDescriptorsFromTableStart = 0;

	D3D12_ROOT_PARAMETER slotRootParameter[7];
	slotRootParameter[0] = setSlotRootParameter(0);//RWStructuredBuffer(u0)
	slotRootParameter[1] = setSlotRootParameter(1);//RWStructuredBuffer(u1)
	slotRootParameter[2] = setSlotRootParameter(2);//RWStructuredBuffer(u2)
	slotRootParameter[3] = setSlotRootParameter(3);//RWStructuredBuffer(u3)
	slotRootParameter[4] = setSlotRootParameter(4);//RWStructuredBuffer(u4)
	slotRootParameter[5] = setSlotRootParameter(
		0, D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE, &uavTable, 1);//RWTexture2D(u5)
	slotRootParameter[6] = setSlotRootParameter(0, D3D12_ROOT_PARAMETER_TYPE_CBV);//mObjectCB(b0)

	mRootSignatureCom = CreateRsCompute(7, slotRootParameter);

	UINT tmpNum[SEA_SHADER_NUM * 2];
	tmpNum[0] = outIndW;
	tmpNum[1] = outIndH;
	tmpNum[2] = srcWidth;
	tmpNum[3] = srcHeight;
	char** replaceString = nullptr;

	CreateReplaceArr(&shaderThreadNum, &replaceString, SEA_SHADER_NUM * 2, tmpNum);

	char* repsh = nullptr;
	ReplaceString(&repsh, ShaderSearchPixel, '?', replaceString);
	for (int i = 0; i < SEA_SHADER_NUM * 2; i++)ARR_DELETE(replaceString[i]);
	ARR_DELETE(replaceString);

	pCS[0] = Dx_ShaderHolder::CompileShader(repsh, strlen(repsh), "InPixCS", "cs_5_0");
	pCS[1] = Dx_ShaderHolder::CompileShader(repsh, strlen(repsh), "OutPixCS", "cs_5_0");
	ARR_DELETE(repsh);
	for (int i = 0; i < SEA_SHADER_NUM; i++)
		mPSOCom[i] = CreatePsoCompute(pCS[i].Get(), mRootSignatureCom.Get());
}

void SearchPixel::SetPixel(float* pi) {
	memcpy(srcPix, pi, insize);

	d->Bigin();
	SubresourcesUp(srcPix, srcWidth * srcHeight, mInputBuffer, mInputUpBuffer);
	d->End();
	cMa->RunGpu();
	cMa->WaitFence();
}

void SearchPixel::SetPixel3ch(ID3D12Resource* pi) {

	d->Bigin();
	CList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mInputColBuffer.Get(),
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_DEST));
	CList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(pi,
		D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COPY_SOURCE));

	CList->CopyResource(mInputColBuffer.Get(), pi);

	CList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(pi,
		D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_GENERIC_READ));
	CList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mInputColBuffer.Get(),
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
	d->End();
	cMa->RunGpu();
	cMa->WaitFence();
}

void SearchPixel::SetPixel3ch(BYTE* pi) {

	d->Bigin();
	SubresourcesUp(pi, srcWidth * 4, mInputColBuffer, mInputColUpBuffer);
	d->End();
	cMa->RunGpu();
	cMa->WaitFence();
}

void SearchPixel::SetNNoutput(float* in) {

	d->Bigin();
	SubresourcesUp(in, outNum, mNNOutputBuffer, mNNOutputUpBuffer);
	d->End();
	cMa->RunGpu();
	cMa->WaitFence();
}

void SearchPixel::SetNNoutput(ID3D12Resource* in) {
	CopyResource(mNNOutputBuffer.Get(), in);
}

void SearchPixel::SeparationTexture() {

	d->Bigin();
	CList->SetPipelineState(mPSOCom[0].Get());
	CList->SetComputeRootSignature(mRootSignatureCom.Get());
	CList->SetComputeRootUnorderedAccessView(0, mInPixPosBuffer->GetGPUVirtualAddress());
	CList->SetComputeRootUnorderedAccessView(1, mInputBuffer->GetGPUVirtualAddress());
	CList->SetComputeRootUnorderedAccessView(2, mOutputBuffer->GetGPUVirtualAddress());
	CList->SetComputeRootUnorderedAccessView(3, mOutIndBuffer->GetGPUVirtualAddress());
	CList->SetComputeRootConstantBufferView(6, mObjectCB->Resource()->GetGPUVirtualAddress());
	CList->Dispatch(outIndW / shaderThreadNum[0], outIndH / shaderThreadNum[1], 1);
	d->End();
	cMa->RunGpu();
	cMa->WaitFence();
	TextureCopy(mOutputBuffer.Get(), 0);

	d->Bigin();
	CList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mOutputBuffer.Get(),
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE));
	CList->CopyResource(mOutputReadBuffer.Get(), mOutputBuffer.Get());
	CList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mOutputBuffer.Get(),
		D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
	d->End();
	cMa->RunGpu();
	cMa->WaitFence();

	D3D12_RANGE range;
	range.Begin = 0;
	range.End = outsize;
	float* out = nullptr;
	mOutputReadBuffer->Map(0, &range, reinterpret_cast<void**>(&out));
	memcpy(seaPix, out, outsize);
	mOutputReadBuffer->Unmap(0, nullptr);
}

void SearchPixel::TextureDraw() {

	d->Bigin();
	CList->SetPipelineState(mPSOCom[1].Get());

	ID3D12DescriptorHeap* descriptorHeaps[] = { mUavHeap.Get() };
	CList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	CList->SetComputeRootSignature(mRootSignatureCom.Get());

	CList->SetComputeRootUnorderedAccessView(0, mInPixPosBuffer->GetGPUVirtualAddress());
	CList->SetComputeRootUnorderedAccessView(4, mNNOutputBuffer->GetGPUVirtualAddress());
	CD3DX12_GPU_DESCRIPTOR_HANDLE uav(mUavHeap->GetGPUDescriptorHandleForHeapStart());
	CList->SetComputeRootDescriptorTable(5, uav);
	CList->SetComputeRootConstantBufferView(6, mObjectCB->Resource()->GetGPUVirtualAddress());
	CList->Dispatch(srcWidth / shaderThreadNum[2], srcHeight / shaderThreadNum[3], 1);
	d->End();
	cMa->RunGpu();
	cMa->WaitFence();
}

ID3D12Resource *SearchPixel::GetOutputResource() {
	return mInputColBuffer.Get();
}

UINT SearchPixel::GetOutWid() {
	return outWid;
}

UINT SearchPixel::GetOutHei() {
	return outHei;
}

float SearchPixel::GetOutputEl(UINT Num) {
	return seaPix[Num];
}