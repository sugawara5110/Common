//*****************************************************************************************//
//**                                                                                     **//
//**                   　  　　  　DxNeuralNetwork                                       **//
//**                                                                                     **//
//*****************************************************************************************//

#define _CRT_SECURE_NO_WARNINGS
#include "DxNeuralNetwork.h"
#include <random>
#include "ShaderNN\ShaderNeuralNetwork.h"

DxNeuralNetwork::DxNeuralNetwork(UINT *numNode, int depth, UINT split, UINT detectionnum) {

	detectionNum = detectionnum;
	Split = split;
	NumNode = numNode;
	NumNode[0] *= Split;
	if (depth > 5)Depth = 5;
	else
		Depth = depth;
	if (NumNode[Depth - 1] > 10)NumNode[Depth - 1] = 10;

	NumNodeStIndex = new UINT[Depth];
	NumWeight = new UINT[Depth - 1];
	NumWeightStIndex = new UINT[Depth - 1];

	UINT cnt = 0;
	for (int i = 0; i < Depth; i++) {
		NumNodeStIndex[i] = cnt;
		cnt += NumNode[i];
	}

	cnt = 0;
	for (int i = 0; i < Depth - 1; i++) {
		NumWeightStIndex[i] = cnt;
		cnt += NumNode[i] * NumNode[i + 1];
		NumWeight[i] = NumNode[i] * NumNode[i + 1];
	}
	weightNumAll = cnt;

	input = new float[NumNode[0] * detectionNum];
	error = new float[NumNode[0]];
	weight = new float[weightNumAll];

	output = new float[NumNode[Depth - 1] * detectionNum];
	target = new float[NumNode[Depth - 1]];
	for (UINT i = 0; i < NumNode[Depth - 1]; i++)target[i] = 0.0f;

	SetWeightInit(1.0f);

	dx = Dx12Process::GetInstance();
	mCommandList = dx->dx_sub[0].mCommandList.Get();
	mObjectCB = new ConstantBuffer<CONSTANT_BUFFER_NeuralNetwork>(1);
}

void DxNeuralNetwork::SetWeightInit(float rate) {
	std::random_device seed_gen;
	std::default_random_engine engine(seed_gen());
	//ウエイト初期値
	// 平均0.0、標準偏差 1/sqrt(NumNode)で分布させる
	std::normal_distribution<> *dist = nullptr;
	for (int k = 0; k < Depth - 1; k++) {
		dist = new std::normal_distribution<>(0.0, 1 / sqrt(NumNode[k + 1]) * rate);
		for (UINT i = 0; i < NumWeight[k]; i++) {
			double rnd = dist->operator()(engine);
			weight[NumWeightStIndex[k] + i] = (float)rnd;
		}
		S_DELETE(dist);
	}
}

DxNeuralNetwork::~DxNeuralNetwork() {

	ARR_DELETE(output);
	ARR_DELETE(target);

	ARR_DELETE(input);
	ARR_DELETE(error);
	ARR_DELETE(weight);

	ARR_DELETE(NumNodeStIndex);
	ARR_DELETE(NumWeight);
	ARR_DELETE(NumWeightStIndex);

	S_DELETE(mObjectCB);
}

void DxNeuralNetwork::SetCommandList(int no) {
	com_no = no;
	mCommandList = dx->dx_sub[com_no].mCommandList.Get();
}

void DxNeuralNetwork::ComCreate(bool sigon) {

	for (int i = 0; i < Depth; i++) {
		cb.NumNode[i].as((float)NumNode[i], NumNodeStIndex[i], 0.0f, 0.0f);
	}
	for (int i = 0; i < Depth - 1; i++) {
		cb.NumWeight[i].x = NumWeightStIndex[i];
	}
	cb.Lear_Depth.z = (float)Depth - 1;

	weight_byteSize = weightNumAll * sizeof(float);

	//RWStructuredBuffer用gNode
	for (int i = 0; i < Depth; i++) {
		CreateResourceDef(mNodeBuffer[i], detectionNum * NumNode[i] * sizeof(float));
	}

	//RWStructuredBuffer用gWeight
	CreateResourceDef(mWeightBuffer, weight_byteSize);

	//RWStructuredBuffer用gError
	for (int i = 0; i < Depth; i++) {
		CreateResourceDef(mErrorBuffer[i], NumNode[i] * sizeof(float));
	}

	//up用gNode
	CreateResourceUp(mNodeUpBuffer, detectionNum * NumNode[0] * sizeof(float));

	//up用gWeight
	CreateResourceUp(mWeightUpBuffer, weight_byteSize);

	//read用gNode
	CreateResourceRead(mNodeReadBuffer, detectionNum * NumNode[Depth - 1] * sizeof(float));

	//read用gWeight
	CreateResourceRead(mWeightReadBuffer, weight_byteSize);

	//read用gError
	CreateResourceRead(mErrorReadBuffer, NumNode[0] * sizeof(float));

	SubresourcesUp(weight, weightNumAll, mWeightBuffer, mWeightUpBuffer);

	//ルートシグネチャ
	CD3DX12_ROOT_PARAMETER slotRootParameter[6];
	slotRootParameter[0].InitAsUnorderedAccessView(0);//RWStructuredBuffer(u0)
	slotRootParameter[1].InitAsUnorderedAccessView(1);//RWStructuredBuffer(u1)
	slotRootParameter[2].InitAsUnorderedAccessView(2);//RWStructuredBuffer(u2)
	slotRootParameter[3].InitAsUnorderedAccessView(3);//RWStructuredBuffer(u3)
	slotRootParameter[4].InitAsUnorderedAccessView(4);//RWStructuredBuffer(u4)
	slotRootParameter[5].InitAsConstantBufferView(0);//mObjectCB(b0)
	mRootSignatureCom = CreateRsCompute(6, slotRootParameter);

	if (sigon) {
		pCS[0] = CompileShader(ShaderNeuralNetwork, strlen(ShaderNeuralNetwork), "NNFPCS", "cs_5_0");
		pCS[3] = CompileShader(ShaderNeuralNetwork, strlen(ShaderNeuralNetwork), "NNBPCS1", "cs_5_0");
	}
	else {
		pCS[0] = CompileShader(ShaderNeuralNetwork, strlen(ShaderNeuralNetwork), "NNFPReLUCS", "cs_5_0");
		pCS[3] = CompileShader(ShaderNeuralNetwork, strlen(ShaderNeuralNetwork), "NNBPReLUCS1", "cs_5_0");
	}
	pCS[1] = CompileShader(ShaderNeuralNetwork, strlen(ShaderNeuralNetwork), "InTargetCS", "cs_5_0");
	pCS[2] = CompileShader(ShaderNeuralNetwork, strlen(ShaderNeuralNetwork), "NNBPCS0", "cs_5_0");
	pCS[4] = CompileShader(ShaderNeuralNetwork, strlen(ShaderNeuralNetwork), "NNInverseCS", "cs_5_0");
	for (int i = 0; i < 5; i++)
		mPSOCom[i] = CreatePsoCompute(pCS[i].Get(), mRootSignatureCom.Get());
}

void DxNeuralNetwork::ComCreateSigmoid() {
	ComCreate(true);
}

void DxNeuralNetwork::ComCreateReLU() {
	ComCreate(false);
}

void DxNeuralNetwork::ForwardPropagation(UINT detectionnum) {
	for (int i = 0; i < Depth - 1; i++) {
		dx->Bigin(com_no);
		mCommandList->SetPipelineState(mPSOCom[0].Get());
		mCommandList->SetComputeRootSignature(mRootSignatureCom.Get());
		mCommandList->SetComputeRootUnorderedAccessView(0, mNodeBuffer[i]->GetGPUVirtualAddress());
		mCommandList->SetComputeRootUnorderedAccessView(1, mNodeBuffer[i + 1]->GetGPUVirtualAddress());
		mCommandList->SetComputeRootUnorderedAccessView(2, mWeightBuffer->GetGPUVirtualAddress());
		cb.Lear_Depth.x = learningRate;
		cb.Lear_Depth.y = i;
		for (UINT i1 = 0; i1 < NumNode[Depth - 1]; i1++)
			cb.Target[i1].x = target[i1];
		mObjectCB->CopyData(0, cb);
		mCommandList->SetComputeRootConstantBufferView(5, mObjectCB->Resource()->GetGPUVirtualAddress());
		//Dispatchは1回毎にGPU処理完了させる事
		mCommandList->Dispatch(NumNode[i + 1], 1, detectionnum);
		dx->End(com_no);
		dx->WaitFenceCurrent();
	}
	dx->Bigin(com_no);
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mNodeBuffer[Depth - 1].Get(),
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE));
	mCommandList->CopyResource(mNodeReadBuffer.Get(), mNodeBuffer[Depth - 1].Get());
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mNodeBuffer[Depth - 1].Get(),
		D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
	dx->End(com_no);
	dx->WaitFenceCurrent();
}

void DxNeuralNetwork::BackPropagation() {
	//target入力
	dx->Bigin(com_no);
	mCommandList->SetPipelineState(mPSOCom[1].Get());
	mCommandList->SetComputeRootSignature(mRootSignatureCom.Get());
	mCommandList->SetComputeRootUnorderedAccessView(0, mNodeBuffer[Depth - 1]->GetGPUVirtualAddress());
	mCommandList->SetComputeRootUnorderedAccessView(2, mWeightBuffer->GetGPUVirtualAddress());
	mCommandList->SetComputeRootUnorderedAccessView(3, mErrorBuffer[Depth - 1]->GetGPUVirtualAddress());
	cb.Lear_Depth.y = Depth - 1;
	mObjectCB->CopyData(0, cb);
	mCommandList->SetComputeRootConstantBufferView(5, mObjectCB->Resource()->GetGPUVirtualAddress());
	mCommandList->Dispatch(NumNode[Depth - 1], 1, 1);
	dx->End(com_no);
	dx->WaitFenceCurrent();

	//逆伝搬
	for (int i = Depth - 2; i >= 0; i--) {
		dx->Bigin(com_no);
		mCommandList->SetPipelineState(mPSOCom[2].Get());
		mCommandList->SetComputeRootSignature(mRootSignatureCom.Get());
		mCommandList->SetComputeRootUnorderedAccessView(2, mWeightBuffer->GetGPUVirtualAddress());
		mCommandList->SetComputeRootUnorderedAccessView(3, mErrorBuffer[i]->GetGPUVirtualAddress());
		mCommandList->SetComputeRootUnorderedAccessView(4, mErrorBuffer[i + 1]->GetGPUVirtualAddress());
		cb.Lear_Depth.y = i;
		mObjectCB->CopyData(0, cb);
		mCommandList->SetComputeRootConstantBufferView(5, mObjectCB->Resource()->GetGPUVirtualAddress());
		mCommandList->Dispatch(NumNode[i], 1, 1);
		dx->End(com_no);
		dx->WaitFenceCurrent();
	}

	//weight値更新
	for (int i = Depth - 2; i >= 0; i--) {
		dx->Bigin(com_no);
		mCommandList->SetPipelineState(mPSOCom[3].Get());
		mCommandList->SetComputeRootSignature(mRootSignatureCom.Get());
		mCommandList->SetComputeRootUnorderedAccessView(0, mNodeBuffer[i]->GetGPUVirtualAddress());
		mCommandList->SetComputeRootUnorderedAccessView(1, mNodeBuffer[i + 1]->GetGPUVirtualAddress());
		mCommandList->SetComputeRootUnorderedAccessView(2, mWeightBuffer->GetGPUVirtualAddress());
		mCommandList->SetComputeRootUnorderedAccessView(4, mErrorBuffer[i + 1]->GetGPUVirtualAddress());
		cb.Lear_Depth.y = i;
		mObjectCB->CopyData(0, cb);
		mCommandList->SetComputeRootConstantBufferView(5, mObjectCB->Resource()->GetGPUVirtualAddress());
		mCommandList->Dispatch(NumNode[i], NumNode[i + 1], 1);
		dx->End(com_no);
		dx->WaitFenceCurrent();
	}

	dx->Bigin(com_no);
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mErrorBuffer[0].Get(),
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE));
	mCommandList->CopyResource(mErrorReadBuffer.Get(), mErrorBuffer[0].Get());
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mErrorBuffer[0].Get(),
		D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));

	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mWeightBuffer.Get(),
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE));
	mCommandList->CopyResource(mWeightReadBuffer.Get(), mWeightBuffer.Get());
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mWeightBuffer.Get(),
		D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
	dx->End(com_no);
	dx->WaitFenceCurrent();
}

void DxNeuralNetwork::CopyOutputResourse() {
	D3D12_RANGE range;
	range.Begin = 0;
	range.End = detectionNum * NumNode[Depth - 1] * sizeof(float);
	D3D12_SUBRESOURCE_DATA subResource;
	mNodeReadBuffer->Map(0, &range, reinterpret_cast<void**>(&subResource));
	float *nod = (float*)subResource.pData;
	for (UINT i = 0; i < detectionNum * NumNode[Depth - 1]; i++) {
		output[i] = nod[i];
	}
	mNodeReadBuffer->Unmap(0, nullptr);
}

void DxNeuralNetwork::CopyWeightResourse() {
	D3D12_RANGE range;
	range.Begin = 0;
	range.End = weight_byteSize;
	float *wei = nullptr;
	mWeightReadBuffer->Map(0, &range, reinterpret_cast<void**>(&wei));
	UINT subInd = 0;
	for (int k = 0; k < Depth - 1; k++) {
		for (UINT i = 0; i < NumWeight[k]; i++) {
			weight[NumWeightStIndex[k] + i] = wei[subInd++];
		}
	}
	mWeightReadBuffer->Unmap(0, nullptr);
}
//↓↑どちらの書き方でもOK
void DxNeuralNetwork::CopyErrorResourse() {
	D3D12_RANGE range;
	range.Begin = 0;
	range.End = NumNode[0] * sizeof(float);
	D3D12_SUBRESOURCE_DATA subResource;
	mErrorReadBuffer->Map(0, &range, reinterpret_cast<void**>(&subResource));
	float *err = (float*)subResource.pData;
	for (UINT i = 0; i < NumNode[0]; i++) {
		error[i] = err[i];
	}
	mErrorReadBuffer->Unmap(0, nullptr);
}

void DxNeuralNetwork::SetLearningLate(float rate) {
	learningRate = rate;
}

void DxNeuralNetwork::SetTarget(float *tar) {
	memcpy(target, tar, sizeof(float) * NumNode[Depth - 1]);
}

void DxNeuralNetwork::SetTargetEl(float el, UINT ElNum) {
	target[ElNum] = el;
}

void DxNeuralNetwork::FirstInput(float el, UINT ElNum, UINT detectionInd) {
	for (UINT i = 0; i < Split; i++)InputArrayEl(el, i, ElNum, detectionInd);
	firstIn = true;
}

void DxNeuralNetwork::InputArray(float *inArr, UINT arrNum, UINT detectionInd) {
	memcpy(&input[NumNode[0] * detectionInd + (NumNode[0] / Split) * arrNum], inArr, sizeof(float) * (NumNode[0] / Split));
}

void DxNeuralNetwork::InputArrayEl(float el, UINT arrNum, UINT ElNum, UINT detectionInd) {
	input[NumNode[0] * detectionInd + (NumNode[0] / Split) * arrNum + ElNum] = el;
}

void DxNeuralNetwork::InputResourse() {
	if (!firstIn)return;
	dx->Bigin(com_no);
	SubresourcesUp(input, NumNode[0] * detectionNum, mNodeBuffer[0], mNodeUpBuffer);
	dx->End(com_no);
	dx->WaitFenceCurrent();
	firstIn = false;
}

void DxNeuralNetwork::GetOutput(float *out, UINT detectionInd) {
	for (UINT i = 0; i < NumNode[Depth - 1]; i++) {
		out[i] = output[NumNode[Depth - 1] * detectionInd + i];
	}
}

float DxNeuralNetwork::GetOutputEl(UINT ElNum, UINT detectionInd) {
	return output[NumNode[Depth - 1] * detectionInd + ElNum];
}

void DxNeuralNetwork::Query(UINT detectionnum) {
	InputResourse();
	ForwardPropagation(detectionnum);
	CopyOutputResourse();
	InverseQuery();
	TextureCopy(mNodeBuffer[0].Get(), com_no);
}

void DxNeuralNetwork::Training() {
	InputResourse();
	ForwardPropagation(1);
	CopyOutputResourse();

	BackPropagation();
	CopyWeightResourse();
	//CopyErrorResourse();

	//↓BackPropagation()の直前に実行しない事
	InverseQuery();
	TextureCopy(mNodeBuffer[0].Get(), com_no);
}

float *DxNeuralNetwork::GetError(UINT arrNum) {
	return &error[(NumNode[0] / Split) * arrNum];
}

float DxNeuralNetwork::GetErrorEl(UINT arrNum, UINT ElNum) {
	return error[(NumNode[0] / Split) * arrNum + ElNum];
}

void DxNeuralNetwork::InverseQuery() {
	//逆入力
	for (int i = Depth - 1; i >= 1; i--) {
		dx->Bigin(com_no);
		mCommandList->SetPipelineState(mPSOCom[4].Get());
		mCommandList->SetComputeRootSignature(mRootSignatureCom.Get());
		mCommandList->SetComputeRootUnorderedAccessView(0, mNodeBuffer[i - 1]->GetGPUVirtualAddress());
		mCommandList->SetComputeRootUnorderedAccessView(1, mNodeBuffer[i]->GetGPUVirtualAddress());
		mCommandList->SetComputeRootUnorderedAccessView(2, mWeightBuffer->GetGPUVirtualAddress());
		cb.Lear_Depth.y = i;
		mObjectCB->CopyData(0, cb);
		mCommandList->SetComputeRootConstantBufferView(5, mObjectCB->Resource()->GetGPUVirtualAddress());
		mCommandList->Dispatch(NumNode[i - 1], 1, 1);
		dx->End(com_no);
		dx->WaitFenceCurrent();
	}
}

void DxNeuralNetwork::SetInputResource(ID3D12Resource *res) {
	dx->Bigin(com_no);
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mNodeBuffer[0].Get(),
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_DEST));
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(res,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE));

	mCommandList->CopyResource(mNodeBuffer[0].Get(), res);

	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(res,
		D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mNodeBuffer[0].Get(),
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
	dx->End(com_no);
	dx->WaitFenceCurrent();
}

ID3D12Resource *DxNeuralNetwork::GetOutErrorResource() {
	return mErrorBuffer[0].Get();
}

ID3D12Resource *DxNeuralNetwork::GetOutputResource() {
	return mNodeBuffer[Depth - 1].Get();
}

void DxNeuralNetwork::SaveData() {
	FILE *fp = fopen("save/save.da", "wb");
	float *weightArr = new float[weightNumAll];

	UINT cnt = 0;
	for (int k = 0; k < Depth - 1; k++) {
		for (UINT i = 0; i < NumWeight[k]; i++) {
			weightArr[cnt++] = weight[NumWeightStIndex[k] + i];
		}
	}

	fwrite(weightArr, sizeof(float) * weightNumAll, 1, fp);
	fclose(fp);
	ARR_DELETE(weightArr);
}

void DxNeuralNetwork::LoadData() {
	FILE *fp = fopen("save/save.da", "rb");
	float *weightArr = new float[weightNumAll];
	fread(weightArr, sizeof(float) * weightNumAll, 1, fp);
	fclose(fp);

	UINT cnt = 0;
	for (int k = 0; k < Depth - 1; k++) {
		for (UINT i = 0; i < NumWeight[k]; i++) {
			weight[NumWeightStIndex[k] + i] = weightArr[cnt++];
		}
	}
	ARR_DELETE(weightArr);

	dx->Bigin(com_no);
	SubresourcesUp(weight, weightNumAll, mWeightBuffer, mWeightUpBuffer);
	dx->End(com_no);
	dx->WaitFenceCurrent();
}
