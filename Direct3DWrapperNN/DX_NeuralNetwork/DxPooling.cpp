//*****************************************************************************************//
//**                                                                                     **//
//**                   　  　  　　DxPooling                                             **//
//**                                                                                     **//
//*****************************************************************************************//

#include "DxPooling.h"
#include "ShaderNN\ShaderPooling.h"
#define PARANUMPO 4

DxPooling::DxPooling(UINT width, UINT height, UINT poolNum, UINT inputsetnum) {

	inputSetNum = inputsetnum;
	inputSetNumCur = inputSetNum;
	PoolNum = poolNum;
	Width = width;
	Height = height;
	if (Width % 2 == 1)OddNumWid = 1;
	if (Height % 2 == 1)OddNumHei = 1;

	input_outerrOneNum = Width * Height;
	output_inerrOneNum = (Width / PONUM) * (Height / PONUM);
	input_outerrOneSize = input_outerrOneNum * sizeof(float);
	output_inerrOneSize = output_inerrOneNum * sizeof(float);

	input = new float[input_outerrOneNum * PoolNum * inputSetNum];
	outerror = new float[input_outerrOneNum * PoolNum * inputSetNum];
	output = new float[output_inerrOneNum * PoolNum * inputSetNum];
	inerror = new float[output_inerrOneNum * PoolNum * inputSetNum];

	for (UINT i = 0; i < input_outerrOneNum * PoolNum * inputSetNum; i++)input[i] = 0.0f;
	for (UINT i = 0; i < output_inerrOneNum * PoolNum * inputSetNum; i++)inerror[i] = 0.0f;

	OutWid = Width / PONUM;
	OutHei = Height / PONUM;

	mObjectCB = new ConstantBuffer<CONSTANT_BUFFER_Pooling>(1);
	cb.WidHei.x = (float)Width;
	cb.WidHei.y = (float)Height;
	cb.WidHei.z = (float)PoolNum;
	mObjectCB->CopyData(0, cb);
}

DxPooling::~DxPooling() {

	ARR_DELETE(input);
	ARR_DELETE(output);
	ARR_DELETE(inerror);
	ARR_DELETE(outerror);
	S_DELETE(mObjectCB);
	ARR_DELETE(shaderThreadNum);
}

void DxPooling::ComCreate() {
	//RWStructuredBuffer用gInput
	CreateResourceDef(mInputBuffer, input_outerrOneSize * PoolNum * inputSetNum);

	//RWStructuredBuffer用gOutput
	CreateResourceDef(mOutputBuffer, output_inerrOneSize * PoolNum * inputSetNum);

	//RWStructuredBuffer用gInErr
	CreateResourceDef(mInErrorBuffer, output_inerrOneSize * PoolNum * inputSetNum);

	//RWStructuredBuffer用gOutErr
	CreateResourceDef(mOutErrorBuffer, input_outerrOneSize * PoolNum * inputSetNum);

	//up用gInput
	CreateResourceUp(mInputUpBuffer, input_outerrOneSize * PoolNum * inputSetNum);

	//up用gInErr
	CreateResourceUp(mInErrorUpBuffer, output_inerrOneSize * PoolNum * inputSetNum);

	//read用gOutput
	CreateResourceRead(mOutputReadBuffer, output_inerrOneSize * PoolNum * inputSetNum);

	//read用gOutErr
	CreateResourceRead(mOutErrorReadBuffer, input_outerrOneSize * PoolNum * inputSetNum);

	SubresourcesUp(input, input_outerrOneNum * PoolNum * inputSetNum, mInputBuffer, mInputUpBuffer);

	SubresourcesUp(inerror, output_inerrOneNum * PoolNum * inputSetNum, mInErrorBuffer, mInErrorUpBuffer);

	//ルートシグネチャ
	CD3DX12_ROOT_PARAMETER slotRootParameter[5];
	slotRootParameter[0].InitAsUnorderedAccessView(0);//RWStructuredBuffer(u0)
	slotRootParameter[1].InitAsUnorderedAccessView(1);//RWStructuredBuffer(u1)
	slotRootParameter[2].InitAsUnorderedAccessView(2);//RWStructuredBuffer(u2)
	slotRootParameter[3].InitAsUnorderedAccessView(3);//RWStructuredBuffer(u3)
	slotRootParameter[4].InitAsConstantBufferView(0);//mObjectCB(b0)
	mRootSignatureCom = CreateRsCompute(5, slotRootParameter);

	UINT tmpNum[PARANUMPO];
	tmpNum[0] = OutWid;
	tmpNum[1] = OutHei * PoolNum;
	tmpNum[2] = OutWid;
	tmpNum[3] = OutHei * PoolNum;
	char** replaceString = nullptr;

	CreateReplaceArr(&shaderThreadNum, &replaceString, PARANUMPO, tmpNum);

	char* repsh = nullptr;
	ReplaceString(&repsh, ShaderPooling, '?', replaceString);
	for (int i = 0; i < PARANUMPO; i++)ARR_DELETE(replaceString[i]);
	ARR_DELETE(replaceString);

	pCS[0] = Dx_ShaderHolder::CompileShader(repsh, strlen(repsh), "POFPCS", "cs_5_0");
	pCS[1] = Dx_ShaderHolder::CompileShader(repsh, strlen(repsh), "POBPCS", "cs_5_0");
	ARR_DELETE(repsh);
	for (int i = 0; i < PO_SHADER_NUM; i++)
		mPSOCom[i] = CreatePsoCompute(pCS[i].Get(), mRootSignatureCom.Get());
}

void DxPooling::FirstInput(float el, UINT ElNum, UINT inputsetInd) {
	for (UINT i = 0; i < PoolNum; i++)InputEl(el - 0.5f, i, ElNum, inputsetInd);
}

void DxPooling::Input(float* inArr, UINT arrNum, UINT inputsetInd) {
	memcpy(&input[input_outerrOneNum * PoolNum * inputsetInd + arrNum * input_outerrOneNum], inArr, input_outerrOneSize);
	firstIn = true;
}

void DxPooling::InputEl(float el, UINT arrNum, UINT ElNum, UINT inputsetInd) {
	input[input_outerrOneNum * PoolNum * inputsetInd + arrNum * input_outerrOneNum + ElNum] = el;
	firstIn = true;
}

void DxPooling::ForwardPropagation() {
	d->Bigin();
	CList->SetPipelineState(mPSOCom[0].Get());
	CList->SetComputeRootSignature(mRootSignatureCom.Get());
	CList->SetComputeRootUnorderedAccessView(0, mInputBuffer->GetGPUVirtualAddress());
	CList->SetComputeRootUnorderedAccessView(1, mOutputBuffer->GetGPUVirtualAddress());
	CList->SetComputeRootUnorderedAccessView(2, mInErrorBuffer->GetGPUVirtualAddress());
	CList->SetComputeRootUnorderedAccessView(3, mOutErrorBuffer->GetGPUVirtualAddress());
	CList->SetComputeRootConstantBufferView(4, mObjectCB->Resource()->GetGPUVirtualAddress());
	CList->Dispatch(OutWid / shaderThreadNum[0], OutHei * PoolNum / shaderThreadNum[1], inputSetNumCur);
	d->End();
	cMa->RunGpu();
	cMa->WaitFence();

	d->Bigin();
	CList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mOutputBuffer.Get(),
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE));
	CList->CopyResource(mOutputReadBuffer.Get(), mOutputBuffer.Get());
	CList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mOutputBuffer.Get(),
		D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
	d->End();
	cMa->RunGpu();
	cMa->WaitFence();
}

void DxPooling::BackPropagation() {
	d->Bigin();
	CList->SetPipelineState(mPSOCom[1].Get());
	CList->SetComputeRootSignature(mRootSignatureCom.Get());
	CList->SetComputeRootUnorderedAccessView(0, mInputBuffer->GetGPUVirtualAddress());
	CList->SetComputeRootUnorderedAccessView(1, mOutputBuffer->GetGPUVirtualAddress());
	CList->SetComputeRootUnorderedAccessView(2, mInErrorBuffer->GetGPUVirtualAddress());
	CList->SetComputeRootUnorderedAccessView(3, mOutErrorBuffer->GetGPUVirtualAddress());
	CList->SetComputeRootConstantBufferView(4, mObjectCB->Resource()->GetGPUVirtualAddress());
	CList->Dispatch(OutWid / shaderThreadNum[2], OutHei * PoolNum / shaderThreadNum[3], inputSetNumCur);
	d->End();
	cMa->RunGpu();
	cMa->WaitFence();

	d->Bigin();
	CList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mOutErrorBuffer.Get(),
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE));
	CList->CopyResource(mOutErrorReadBuffer.Get(), mOutErrorBuffer.Get());
	CList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mOutErrorBuffer.Get(),
		D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
	d->End();
	cMa->RunGpu();
	cMa->WaitFence();
}

void DxPooling::Query() {
	InputResourse();
	inputSetNumCur = inputSetNum;
	ForwardPropagation();
	CopyOutputResourse();
	TextureCopy(mOutputBuffer.Get(), 0);
}

void DxPooling::Training() {
	//InputErrResourse();
	BackPropagation();
	//CopyOutputErrResourse();
}

void DxPooling::Detection(UINT inputsetnum) {
	InputResourse();
	inputSetNumCur = inputsetnum;
	ForwardPropagation();
	CopyOutputResourse();
	TextureCopy(mOutputBuffer.Get(), 0);
}

void DxPooling::Test() {
	InputResourse();
	inputSetNumCur = 1;
	ForwardPropagation();
	CopyOutputResourse();
}

void DxPooling::InputResourse() {
	if (!firstIn)return;
	d->Bigin();
	SubresourcesUp(input, input_outerrOneNum * PoolNum * inputSetNum, mInputBuffer, mInputUpBuffer);
	d->End();
	cMa->RunGpu();
	cMa->WaitFence();
	firstIn = false;
}

void DxPooling::InputErrResourse() {
	d->Bigin();
	SubresourcesUp(inerror, output_inerrOneNum * PoolNum * inputSetNum, mInErrorBuffer, mInErrorUpBuffer);
	d->End();
	cMa->RunGpu();
	cMa->WaitFence();
}

void DxPooling::CopyOutputResourse() {
	D3D12_RANGE range;
	range.Begin = 0;
	range.End = inputSetNum * output_inerrOneSize * PoolNum;
	float *out = nullptr;
	mOutputReadBuffer->Map(0, &range, reinterpret_cast<void**>(&out));
	memcpy(output, out, inputSetNum * output_inerrOneSize * PoolNum);
	mOutputReadBuffer->Unmap(0, nullptr);
}

void DxPooling::CopyOutputErrResourse() {
	D3D12_RANGE range;
	range.Begin = 0;
	range.End = inputSetNum * input_outerrOneSize * PoolNum;
	float *out = nullptr;
	mOutErrorReadBuffer->Map(0, &range, reinterpret_cast<void**>(&out));
	memcpy(outerror, out, inputSetNum * input_outerrOneSize * PoolNum);
	mOutErrorReadBuffer->Unmap(0, nullptr);
}

float *DxPooling::Output(UINT arrNum, UINT inputsetInd) {
	return &output[inputsetInd * output_inerrOneNum * PoolNum + arrNum * output_inerrOneNum];
}

float DxPooling::OutputEl(UINT arrNum, UINT ElNum, UINT inputsetInd) {
	return output[inputsetInd * output_inerrOneNum * PoolNum + arrNum * output_inerrOneNum + ElNum];
}

void DxPooling::InputError(float *inArr, UINT arrNum, UINT inputsetInd) {
	memcpy(&inerror[inputsetInd * output_inerrOneNum * PoolNum + arrNum * output_inerrOneNum], inArr, output_inerrOneSize);
}

void DxPooling::InputErrorEl(float el, UINT arrNum, UINT ElNum, UINT inputsetInd) {
	inerror[inputsetInd * output_inerrOneNum * PoolNum + arrNum * output_inerrOneNum + ElNum] = el;
}

float *DxPooling::GetError(UINT arrNum, UINT inputsetInd) {
	return &outerror[inputsetInd * input_outerrOneNum * PoolNum + arrNum * input_outerrOneNum];
}

float DxPooling::GetErrorEl(UINT arrNum, UINT ElNum, UINT inputsetInd) {
	return outerror[inputsetInd * input_outerrOneNum * PoolNum + arrNum * input_outerrOneNum + ElNum];
}

void DxPooling::SetInputResource(ID3D12Resource* res) {
	CopyResource(mInputBuffer.Get(), res);
}

void DxPooling::SetInErrorResource(ID3D12Resource* res) {
	CopyResource(mInErrorBuffer.Get(), res);
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