//*****************************************************************************************//
//**                                                                                     **//
//**                   　  　　　  DxActivation                                          **//
//**                                                                                     **//
//*****************************************************************************************//

#include "DxActivation.h"
#include "ShaderNN\ShaderActivation.h"
#define PARANUMAC 1

DxActivation::DxActivation(UINT numNode, UINT inputsetnum) {
	NumNode = numNode;
	inputSetNum = inputsetnum;
	inputSetNumCur = inputsetnum;
	output = new float[NumNode * inputSetNum];
	outerr = new float[NumNode * inputSetNum];
	ZeroMemory(output, sizeof(float) * NumNode * inputSetNum);
	ZeroMemory(outerr, sizeof(float) * NumNode * inputSetNum);

	mObjectCB = new ConstantBuffer<CONSTANT_BUFFER_Activation>(1);
	cb.ActivationAlpha = 0.0f;
	cb.NumNode = NumNode;
	mObjectCB->CopyData(0, cb);
}

DxActivation::~DxActivation() {
	ARR_DELETE(output);
	ARR_DELETE(outerr);
	S_DELETE(mObjectCB);
	ARR_DELETE(shaderThreadNum);
}

void DxActivation::ComCreate(ActivationName name) {
	//RWStructuredBuffer用gNode
	CreateResourceDef(mNodeBuffer, sizeof(float) * NumNode * inputSetNum);
	//read用gNode
	CreateResourceRead(mNodeReadBuffer, sizeof(float) * NumNode * inputSetNum);
	//RWStructuredBuffer用gErr
	CreateResourceDef(mErrorBuffer, sizeof(float) * NumNode * inputSetNum);
	///read用gErr
	CreateResourceRead(mErrorReadBuffer, sizeof(float) * NumNode * inputSetNum);
	//ルートシグネチャ
	CD3DX12_ROOT_PARAMETER slotRootParameter[3];
	slotRootParameter[0].InitAsUnorderedAccessView(0);//RWStructuredBuffer(u0)
	slotRootParameter[1].InitAsUnorderedAccessView(1);//RWStructuredBuffer(u1)
	slotRootParameter[2].InitAsConstantBufferView(0);//mObjectCB(b0)
	mRootSignatureCom = CreateRsCompute(3, slotRootParameter);

	UINT tmpNum[PARANUMAC];
	tmpNum[0] = NumNode;

	char** replaceString = nullptr;
	CreateReplaceArr(&shaderThreadNum, &replaceString, PARANUMAC, tmpNum);
	char* repsh = nullptr;
	ReplaceString(&repsh, ShaderActivation, '?', replaceString);
	for (int i = 0; i < PARANUMAC; i++)ARR_DELETE(replaceString[i]);
	ARR_DELETE(replaceString);

	switch (name) {
	case CrossEntropySigmoid:
		pCS[0] = CompileShader(repsh, strlen(repsh), "FPsigmoidCS", "cs_5_0");
		pCS[1] = CompileShader(repsh, strlen(repsh), "BPSigmoidCECS", "cs_5_0");
		break;
	case Sigmoid:
		pCS[0] = CompileShader(repsh, strlen(repsh), "FPsigmoidCS", "cs_5_0");
		pCS[1] = CompileShader(repsh, strlen(repsh), "BPSigmoidCS", "cs_5_0");
		break;
	case ReLU:
		pCS[0] = CompileShader(repsh, strlen(repsh), "FPReLUCS", "cs_5_0");
		pCS[1] = CompileShader(repsh, strlen(repsh), "BPReLUCS", "cs_5_0");
		break;
	case ELU:
		pCS[0] = CompileShader(repsh, strlen(repsh), "FPELUCS", "cs_5_0");
		pCS[1] = CompileShader(repsh, strlen(repsh), "BPELUCS", "cs_5_0");
		break;
	case Tanh:
		pCS[0] = CompileShader(repsh, strlen(repsh), "FPtanhCS", "cs_5_0");
		pCS[1] = CompileShader(repsh, strlen(repsh), "BPtanhCS", "cs_5_0");
		break;
	}
	ARR_DELETE(repsh);
	for (int i = 0; i < AC_SHADER_NUM; i++)
		mPSOCom[i] = CreatePsoCompute(pCS[i].Get(), mRootSignatureCom.Get());
}

void DxActivation::SetActivationAlpha(float alpha) {
	cb.ActivationAlpha = alpha;
	mObjectCB->CopyData(0, cb);
}

void DxActivation::SetTarget(float* tar) {
	if (NumNode > MAX_OUTPUT_NUM)return;
	for (UINT i = 0; i < NumNode; i++)cb.Target[i].x = tar[i];
	mObjectCB->CopyData(0, cb);
}

void DxActivation::SetTargetEl(float el, UINT ElNum) {
	cb.Target[ElNum].x = el;
	mObjectCB->CopyData(0, cb);
}

void DxActivation::ForwardPropagation(UINT inputsetnum) {
	inputSetNumCur = inputsetnum;
	dx->Bigin(com_no);
	mCommandList->SetPipelineState(mPSOCom[0].Get());
	mCommandList->SetComputeRootSignature(mRootSignatureCom.Get());
	mCommandList->SetComputeRootUnorderedAccessView(0, mNodeBuffer->GetGPUVirtualAddress());
	mCommandList->SetComputeRootUnorderedAccessView(1, mErrorBuffer->GetGPUVirtualAddress());
	mCommandList->SetComputeRootConstantBufferView(2, mObjectCB->Resource()->GetGPUVirtualAddress());
	mCommandList->Dispatch(NumNode / shaderThreadNum[0], 1, inputSetNumCur);
	dx->End(com_no);
	dx->RunGpu();
	dx->WaitFence();

	dx->Bigin(com_no);
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mNodeBuffer.Get(),
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE));
	mCommandList->CopyResource(mNodeReadBuffer.Get(), mNodeBuffer.Get());
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mNodeBuffer.Get(),
		D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
	dx->End(com_no);
	dx->RunGpu();
	dx->WaitFence();
	CopyOutputResourse();
}

void DxActivation::comCrossEntropyError() {
	if (NumNode > MAX_OUTPUT_NUM)return;
	float err = 0.0f;
	for (UINT j = 0; j < inputSetNumCur; j++) {
		for (UINT i = 0; i < NumNode; i++) {
			UINT outInd = NumNode * j + i;
			err += -cb.Target[i].x * log2(output[outInd] + 0.0000001f) - (1.0f - cb.Target[i].x) *
				log2(1.0f - output[outInd] + 0.0000001f);
		}
	}
	crossEntropyError = err / (float)inputSetNumCur;
}

float DxActivation::GetcrossEntropyError() {
	return crossEntropyError;
}

void DxActivation::BackPropagation() {
	dx->Bigin(com_no);
	mCommandList->SetPipelineState(mPSOCom[1].Get());
	mCommandList->SetComputeRootSignature(mRootSignatureCom.Get());
	mCommandList->SetComputeRootUnorderedAccessView(0, mNodeBuffer->GetGPUVirtualAddress());
	mCommandList->SetComputeRootUnorderedAccessView(1, mErrorBuffer->GetGPUVirtualAddress());
	mCommandList->SetComputeRootConstantBufferView(2, mObjectCB->Resource()->GetGPUVirtualAddress());
	mCommandList->Dispatch(NumNode / shaderThreadNum[0], 1, inputSetNumCur);
	dx->End(com_no);
	dx->RunGpu();
	dx->WaitFence();

	dx->Bigin(com_no);
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mErrorBuffer.Get(),
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE));
	mCommandList->CopyResource(mErrorReadBuffer.Get(), mErrorBuffer.Get());
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mErrorBuffer.Get(),
		D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
	dx->End(com_no);
	dx->RunGpu();
	dx->WaitFence();
	CopyOutErrResourse();
}

void DxActivation::CopyOutputResourse() {
	D3D12_RANGE range;
	range.Begin = 0;
	range.End = inputSetNum * NumNode * sizeof(float);
	D3D12_SUBRESOURCE_DATA subResource;
	mNodeReadBuffer->Map(0, &range, reinterpret_cast<void**>(&subResource));
	float* nod = (float*)subResource.pData;
	for (UINT i = 0; i < inputSetNum * NumNode; i++) {
		output[i] = nod[i];
	}
	mNodeReadBuffer->Unmap(0, nullptr);
}

void DxActivation::CopyOutErrResourse() {
	D3D12_RANGE range;
	range.Begin = 0;
	range.End = inputSetNum * NumNode * sizeof(float);
	D3D12_SUBRESOURCE_DATA subResource;
	mErrorReadBuffer->Map(0, &range, reinterpret_cast<void**>(&subResource));
	float* nod = (float*)subResource.pData;
	for (UINT i = 0; i < inputSetNum * NumNode; i++) {
		outerr[i] = nod[i];
	}
	mErrorReadBuffer->Unmap(0, nullptr);
}

void DxActivation::SetInputResource(ID3D12Resource* res) {
	CopyResource(mNodeBuffer.Get(), res);
}

void DxActivation::SetInErrorResource(ID3D12Resource* res) {
	CopyResource(mErrorBuffer.Get(), res);
}

ID3D12Resource* DxActivation::GetOutputResource() {
	return mNodeBuffer.Get();
}

ID3D12Resource* DxActivation::GetOutErrorResource() {
	return mErrorBuffer.Get();
}

void DxActivation::GetOutput(float* out, UINT inputsetInd) {
	for (UINT i = 0; i < NumNode; i++) {
		out[i] = output[NumNode * inputsetInd + i];
	}
}

float DxActivation::GetOutputEl(UINT ElNum, UINT inputsetInd) {
	return output[NumNode * inputsetInd + ElNum];
}

void DxActivation::GetOutErr(float* out, UINT inputsetInd) {
	for (UINT i = 0; i < NumNode; i++) {
		out[i] = outerr[NumNode * inputsetInd + i];
	}
}

float DxActivation::GetOutErrEl(UINT ElNum, UINT inputsetInd) {
	return outerr[NumNode * inputsetInd + ElNum];
}