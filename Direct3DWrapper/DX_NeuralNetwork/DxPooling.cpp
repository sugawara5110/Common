//*****************************************************************************************//
//**                                                                                     **//
//**                   　  　  　　DxPooling                                             **//
//**                                                                                     **//
//*****************************************************************************************//

#include "DxPooling.h"
#include "ShaderNN\ShaderPooling.h"

DxPooling::DxPooling(UINT width, UINT height, UINT poolNum, UINT detectionnum) {

	detectionNum = detectionnum;
	PoolNum = poolNum;
	Width = width;
	Height = height;
	if (Width % 2 == 1)OddNumWid = 1;
	if (Height % 2 == 1)OddNumHei = 1;

	input_outerrOneNum = Width * Height;
	output_inerrOneNum = (Width / PONUM) * (Height / PONUM);
	input_outerrOneSize = input_outerrOneNum * sizeof(float);
	output_inerrOneSize = output_inerrOneNum * sizeof(float);

	input = new float[input_outerrOneNum * PoolNum * detectionNum];
	outerror = new float[input_outerrOneNum * PoolNum];
	output = new float[output_inerrOneNum * PoolNum * detectionNum];
	inerror = new float[output_inerrOneNum * PoolNum];

	for (UINT i = 0; i < input_outerrOneNum * PoolNum * detectionNum; i++)input[i] = 0.0f;
	for (UINT i = 0; i < output_inerrOneNum * PoolNum; i++)inerror[i] = 0.0f;

	OutWid = Width / PONUM;
	OutHei = Height / PONUM;

	dx = Dx12Process::GetInstance();
	mCommandList = dx->dx_sub[0].mCommandList.Get();
	mObjectCB = new UploadBuffer<CONSTANT_BUFFER_Pooling>(dx->md3dDevice.Get(), 1, true);
	cb.WidHei.x = Width;
	cb.WidHei.y = Height;
	cb.WidHei.z = PoolNum;
	mObjectCB->CopyData(0, cb);
}

DxPooling::~DxPooling() {

	ARR_DELETE(input);
	ARR_DELETE(output);
	ARR_DELETE(inerror);
	ARR_DELETE(outerror);
	S_DELETE(mObjectCB);
}

void DxPooling::SetCommandList(int no) {
	com_no = no;
	mCommandList = dx->dx_sub[com_no].mCommandList.Get();
}

void DxPooling::ComCreate() {

	//RWStructuredBuffer用gInput
	dx->md3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(input_outerrOneSize * PoolNum * detectionNum, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS),
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(&mInputBuffer));
	//RWStructuredBuffer用gOutput
	dx->md3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(output_inerrOneSize * PoolNum * detectionNum, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS),
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(&mOutputBuffer));
	//RWStructuredBuffer用gInErr
	dx->md3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(output_inerrOneSize * PoolNum, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS),
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(&mInErrorBuffer));
	//RWStructuredBuffer用gOutErr
	dx->md3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(input_outerrOneSize * PoolNum, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS),
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(&mOutErrorBuffer));
	//up用gInput
	dx->md3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(input_outerrOneSize * PoolNum * detectionNum),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&mInputUpBuffer));
	//up用gInErr
	dx->md3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(output_inerrOneSize * PoolNum),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&mInErrorUpBuffer));
	//read用gOutput
	dx->md3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(output_inerrOneSize * PoolNum * detectionNum),
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&mOutputReadBuffer));
	//read用gOutErr
	dx->md3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(input_outerrOneSize * PoolNum),
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&mOutErrorReadBuffer));

	D3D12_SUBRESOURCE_DATA subResourceDataInput = {};
	subResourceDataInput.pData = input;
	subResourceDataInput.RowPitch = input_outerrOneNum * PoolNum * detectionNum;
	subResourceDataInput.SlicePitch = subResourceDataInput.RowPitch;

	D3D12_SUBRESOURCE_DATA subResourceDataInerror = {};
	subResourceDataInerror.pData = inerror;
	subResourceDataInerror.RowPitch = output_inerrOneNum * PoolNum;
	subResourceDataInerror.SlicePitch = subResourceDataInerror.RowPitch;

	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mInputBuffer.Get(),
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST));
	UpdateSubresources(mCommandList, mInputBuffer.Get(), mInputUpBuffer.Get(), 0, 0, 1, &subResourceDataInput);
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mInputBuffer.Get(),
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));

	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mInErrorBuffer.Get(),
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST));
	UpdateSubresources(mCommandList, mInErrorBuffer.Get(), mInErrorUpBuffer.Get(), 0, 0, 1, &subResourceDataInerror);
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mInErrorBuffer.Get(),
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));

	//ルートシグネチャ
	CD3DX12_ROOT_PARAMETER slotRootParameter[5];
	slotRootParameter[0].InitAsUnorderedAccessView(0);//RWStructuredBuffer(u0)
	slotRootParameter[1].InitAsUnorderedAccessView(1);//RWStructuredBuffer(u1)
	slotRootParameter[2].InitAsUnorderedAccessView(2);//RWStructuredBuffer(u2)
	slotRootParameter[3].InitAsUnorderedAccessView(3);//RWStructuredBuffer(u3)
	slotRootParameter[4].InitAsConstantBufferView(0);//mObjectCB(b0)
	mRootSignatureCom = CreateRsCompute(5, slotRootParameter);

	pCS[0] = CompileShader(ShaderPooling, strlen(ShaderPooling), "POFPCS", "cs_5_0");
	pCS[1] = CompileShader(ShaderPooling, strlen(ShaderPooling), "POBPCS", "cs_5_0");
	for (int i = 0; i < 2; i++)
		mPSOCom[i] = CreatePsoCompute(pCS[i].Get(), mRootSignatureCom.Get());
}

void DxPooling::FirstInput(float el, UINT ElNum, UINT detectionInd) {
	for (UINT i = 0; i < PoolNum; i++)InputEl(el - 0.5f, i, ElNum, detectionInd);
	firstIn = true;
}

void DxPooling::Input(float *inArr, UINT arrNum, UINT detectionInd) {
	memcpy(&input[input_outerrOneNum * PoolNum * detectionInd + arrNum * input_outerrOneNum], inArr, input_outerrOneSize);
}

void DxPooling::InputEl(float el, UINT arrNum, UINT ElNum, UINT detectionInd) {
	input[input_outerrOneNum * PoolNum * detectionInd + arrNum * input_outerrOneNum + ElNum] = el;
}

void DxPooling::ForwardPropagation(UINT detectionnum) {
	dx->Bigin(com_no);
	mCommandList->SetPipelineState(mPSOCom[0].Get());
	mCommandList->SetComputeRootSignature(mRootSignatureCom.Get());
	mCommandList->SetComputeRootUnorderedAccessView(0, mInputBuffer->GetGPUVirtualAddress());
	mCommandList->SetComputeRootUnorderedAccessView(1, mOutputBuffer->GetGPUVirtualAddress());
	mCommandList->SetComputeRootUnorderedAccessView(2, mInErrorBuffer->GetGPUVirtualAddress());
	mCommandList->SetComputeRootUnorderedAccessView(3, mOutErrorBuffer->GetGPUVirtualAddress());
	mCommandList->SetComputeRootConstantBufferView(4, mObjectCB->Resource()->GetGPUVirtualAddress());
	mCommandList->Dispatch(OutWid, OutHei * PoolNum, detectionnum);
	dx->End(com_no);
	dx->WaitFenceCurrent();

	dx->Bigin(com_no);
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mOutputBuffer.Get(),
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE));
	mCommandList->CopyResource(mOutputReadBuffer.Get(), mOutputBuffer.Get());
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mOutputBuffer.Get(),
		D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
	dx->End(com_no);
	dx->WaitFenceCurrent();
}

void DxPooling::BackPropagation() {
	dx->Bigin(com_no);
	mCommandList->SetPipelineState(mPSOCom[1].Get());
	mCommandList->SetComputeRootSignature(mRootSignatureCom.Get());
	mCommandList->SetComputeRootUnorderedAccessView(0, mInputBuffer->GetGPUVirtualAddress());
	mCommandList->SetComputeRootUnorderedAccessView(1, mOutputBuffer->GetGPUVirtualAddress());
	mCommandList->SetComputeRootUnorderedAccessView(2, mInErrorBuffer->GetGPUVirtualAddress());
	mCommandList->SetComputeRootUnorderedAccessView(3, mOutErrorBuffer->GetGPUVirtualAddress());
	mCommandList->SetComputeRootConstantBufferView(4, mObjectCB->Resource()->GetGPUVirtualAddress());
	mCommandList->Dispatch(OutWid, OutHei * PoolNum, 1);
	dx->End(com_no);
	dx->WaitFenceCurrent();

	dx->Bigin(com_no);
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mOutErrorBuffer.Get(),
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE));
	mCommandList->CopyResource(mOutErrorReadBuffer.Get(), mOutErrorBuffer.Get());
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mOutErrorBuffer.Get(),
		D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
	dx->End(com_no);
	dx->WaitFenceCurrent();
}

void DxPooling::Query() {
	InputResourse();
	ForwardPropagation(1);
	//CopyOutputResourse();
	TextureCopy(mOutputBuffer.Get(), com_no);
}

void DxPooling::Training() {
	//InputErrResourse();
	BackPropagation();
	//CopyOutputErrResourse();
}

void DxPooling::Detection(UINT detectionnum) {
	InputResourse();
	ForwardPropagation(detectionnum);
	TextureCopy(mOutputBuffer.Get(), com_no);
}

void DxPooling::InputResourse() {
	if (!firstIn)return;
	D3D12_SUBRESOURCE_DATA subResourceData = {};
	subResourceData.pData = input;
	subResourceData.RowPitch = input_outerrOneNum * PoolNum * detectionNum;
	subResourceData.SlicePitch = subResourceData.RowPitch;
	dx->Bigin(com_no);
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mInputBuffer.Get(),
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_DEST));
	UpdateSubresources(mCommandList, mInputBuffer.Get(), mInputUpBuffer.Get(), 0, 0, 1, &subResourceData);
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mInputBuffer.Get(),
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
	dx->End(com_no);
	dx->WaitFenceCurrent();
	firstIn = false;
}

void DxPooling::InputErrResourse() {
	D3D12_SUBRESOURCE_DATA subResourceData = {};
	subResourceData.pData = inerror;
	subResourceData.RowPitch = output_inerrOneNum * PoolNum;
	subResourceData.SlicePitch = subResourceData.RowPitch;
	dx->Bigin(com_no);
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mInErrorBuffer.Get(),
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_DEST));
	UpdateSubresources(mCommandList, mInErrorBuffer.Get(), mInErrorUpBuffer.Get(), 0, 0, 1, &subResourceData);
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mInErrorBuffer.Get(),
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
	dx->End(com_no);
	dx->WaitFenceCurrent();
}

void DxPooling::CopyOutputResourse() {
	D3D12_RANGE range;
	range.Begin = 0;
	range.End = detectionNum * output_inerrOneSize * PoolNum;
	float *out = nullptr;
	mOutputReadBuffer->Map(0, &range, reinterpret_cast<void**>(&out));
	memcpy(output, out, detectionNum * output_inerrOneSize * PoolNum);
	mOutputReadBuffer->Unmap(0, nullptr);
}

void DxPooling::CopyOutputErrResourse() {
	D3D12_RANGE range;
	range.Begin = 0;
	range.End = input_outerrOneSize;
	float *out = nullptr;
	mOutErrorReadBuffer->Map(0, &range, reinterpret_cast<void**>(&out));
	memcpy(outerror, out, input_outerrOneSize * PoolNum);
	mOutErrorReadBuffer->Unmap(0, nullptr);
}

float *DxPooling::Output(UINT arrNum, UINT detectionInd) {
	return &output[detectionNum * output_inerrOneNum * PoolNum + arrNum * output_inerrOneNum];
}

float DxPooling::OutputEl(UINT arrNum, UINT ElNum, UINT detectionInd) {
	return output[detectionNum * output_inerrOneNum * PoolNum + arrNum * output_inerrOneNum + ElNum];
}

void DxPooling::InputError(float *inArr, UINT arrNum) {
	memcpy(&inerror[arrNum * output_inerrOneNum], inArr, output_inerrOneSize);
}

void DxPooling::InputErrorEl(float el, UINT arrNum, UINT ElNum) {
	inerror[arrNum * output_inerrOneNum + ElNum] = el;
}

float *DxPooling::GetError(UINT arrNum) {
	return &outerror[arrNum * input_outerrOneNum];
}

float DxPooling::GetErrorEl(UINT arrNum, UINT ElNum) {
	return outerror[arrNum * input_outerrOneNum + ElNum];
}

void DxPooling::SetInputResource(ID3D12Resource *res) {
	dx->Bigin(com_no);
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mInputBuffer.Get(),
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_DEST));
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(res,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE));

	mCommandList->CopyResource(mInputBuffer.Get(), res);

	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(res,
		D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mInputBuffer.Get(),
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
	dx->End(com_no);
	dx->WaitFenceCurrent();
}

void DxPooling::SetInErrorResource(ID3D12Resource *res) {
	dx->Bigin(com_no);
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mInErrorBuffer.Get(),
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_DEST));
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(res,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE));

	mCommandList->CopyResource(mInErrorBuffer.Get(), res);

	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(res,
		D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mInErrorBuffer.Get(),
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
	dx->End(com_no);
	dx->WaitFenceCurrent();
}

ID3D12Resource *DxPooling::GetOutErrorResource() {
	return mOutErrorBuffer.Get();
}

ID3D12Resource *DxPooling::GetOutputResource() {
	return mOutputBuffer.Get();
}

UINT DxPooling::GetOutWidth() {
	return OutWid;
}

UINT DxPooling::GetOutHeight() {
	return OutHei;
}