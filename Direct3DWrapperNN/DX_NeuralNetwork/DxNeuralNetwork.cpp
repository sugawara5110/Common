//*****************************************************************************************//
//**                                                                                     **//
//**                   　  　　  　DxNeuralNetwork                                       **//
//**                                                                                     **//
//*****************************************************************************************//

#define _CRT_SECURE_NO_WARNINGS
#include "DxNeuralNetwork.h"
#include <random>
#include "ShaderNN\ShaderNeuralNetwork.h"
#define PARANUM 4

DxNeuralNetwork::DxNeuralNetwork(UINT* numNode, int depth, UINT split, UINT inputsetnum) {

	mObjectCB = new ConstantBuffer<CONSTANT_BUFFER_NeuralNetwork>(1);

	srand((unsigned)time(NULL));
	inputSetNum = inputsetnum;
	inputSetNumCur = inputSetNum;
	Split = split;

	if (depth > MAX_DEPTH_NUM)Depth = MAX_DEPTH_NUM;
	else
		Depth = depth;

	NumNode = new UINT[Depth];
	for (int i = 0; i < Depth; i++)NumNode[i] = numNode[i];
}

void DxNeuralNetwork::SetDropOut() {

	d->Bigin();
	for (int k = 0; k < Depth - 1; k++) {
		for (UINT i = 0; i < NumNode[k]; i++) {
			dropout[k][i] = 1.0f;
			int rnd = (rand() % 100) + 1;
			if ((int)(dropThreshold[k] * 100.0f) >= rnd) {
				dropout[k][i] = 0.0f;
			}
		}
		SubresourcesUp(dropout[k], NumNode[k], mDropOutFBuffer[k], mDropOutFUpBuffer[k]);
	}
	d->End();
	cMa->RunGpu();
	cMa->WaitFence();
}

void DxNeuralNetwork::SetdropThreshold(float *ThresholdArr) {
	for (int k = 0; k < Depth - 1; k++) {
		dropThreshold[k] = ThresholdArr[k];
	}
}

void DxNeuralNetwork::SetWeightInitXavier(float rate) {
	std::random_device seed_gen;
	std::default_random_engine engine(seed_gen());
	//ウエイト初期値
	// 平均0.0、標準偏差 1/sqrt(NumNode)で分布させる
	std::normal_distribution<>* dist = nullptr;
	for (int k = 0; k < Depth - 1; k++) {
		dist = new std::normal_distribution<>(0.0, 1 / sqrt(NumNode[k]) * rate);
		for (UINT i = 0; i < NumWeight[k]; i++) {
			double rnd = dist->operator()(engine);
			weight[NumWeightStIndex[k] + i] = (float)rnd;
		}
		S_DELETE(dist);
	}
}

void DxNeuralNetwork::SetWeightInitHe(float rate) {
	std::random_device seed_gen;
	std::default_random_engine engine(seed_gen());
	//ウエイト初期値
	// 平均0.0、標準偏差 2/sqrt(NumNode)で分布させる
	std::normal_distribution<>* dist = nullptr;
	for (int k = 0; k < Depth - 1; k++) {
		dist = new std::normal_distribution<>(0.0, 2 / sqrt(NumNode[k]) * rate);
		for (UINT i = 0; i < NumWeight[k]; i++) {
			double rnd = dist->operator()(engine);
			weight[NumWeightStIndex[k] + i] = (float)rnd;
		}
		S_DELETE(dist);
	}
}

DxNeuralNetwork::~DxNeuralNetwork() {
	ARR_DELETE(input);
	ARR_DELETE(error);
	ARR_DELETE(weight);
	for (int i = 0; i < Depth - 1; i++) {
		ARR_DELETE(shaderThreadNum[i]);
		ARR_DELETE(dropout[i]);
	}
	ARR_DELETE(dropout);
	ARR_DELETE(dropThreshold);

	for (int i = 0; i < Depth - 2; i++)S_DELETE(ac[i]);
	ARR_DELETE(ac);
	S_DELETE(topAc);

	ARR_DELETE(NumNodeStIndex);
	ARR_DELETE(NumWeight);
	ARR_DELETE(NumWeightStIndex);

	S_DELETE(mObjectCB);
	ARR_DELETE(NumNode);
	S_DELETE(opt);
}

void DxNeuralNetwork::ComCreate(ActivationName node, OptimizerName optName, ActivationName topNode, float wInit) {

	NumNode[0] *= Split;

	if (topNode == CrossEntropySigmoid) {
		if (NumNode[Depth - 1] > MAX_OUTPUT_NUM)NumNode[Depth - 1] = MAX_OUTPUT_NUM;
	}
	else {
		for (int i = 1; i < Depth; i++) {
			NumNode[i] *= Split;
		}
	}

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

	input = new float[NumNode[0] * inputSetNum];
	error = new float[NumNode[0] * inputSetNum];
	weight = new float[weightNumAll];
	dropThreshold = new float[Depth - 1];
	dropout = new float* [Depth - 1];
	for (int i = 0; i < Depth - 1; i++) {
		dropout[i] = new float[NumNode[i]];
		dropThreshold[i] = 0.0f;
	}
	for (int k = 0; k < Depth - 1; k++) {
		for (UINT i = 0; i < NumNode[k]; i++) {
			dropout[k][i] = 1.0f;//1.0fでニューロン有効
		}
	}

	switch (node) {
	case Sigmoid:
	case Tanh:
		SetWeightInitXavier(wInit);
		break;
	case ReLU:
	case ELU:
		SetWeightInitHe(wInit);
		break;
	}

	for (int i = 0; i < Depth; i++) {
		cb.NumNode[i].as((float)NumNode[i], (float)NumNodeStIndex[i], 0.0f, 0.0f);
	}
	for (int i = 0; i < Depth - 1; i++) {
		cb.NumWeight[i].x = (float)NumWeightStIndex[i];
	}
	cb.Lear_Depth_inputS.z = (float)Depth - 1;
	cb.Lear_Depth_inputS.w = (float)inputSetNum;

	weight_byteSize = weightNumAll * sizeof(float);

	for (int i = 0; i < Depth; i++) {
		//RWStructuredBuffer用gNode
		CreateResourceDef(mNodeBuffer[i], inputSetNum * NumNode[i] * sizeof(float));
		//RWStructuredBuffer用gError
		CreateResourceDef(mErrorBuffer[i], inputSetNum * NumNode[i] * sizeof(float));
	}

	//RWStructuredBuffer用gWeight
	CreateResourceDef(mWeightBuffer, weight_byteSize);
	//オプティマイザー
	opt = new DxOptimizer(weightNumAll);
	opt->ComCreate(optName);

	//RWStructuredBuffer用gGradient
	CreateResourceDef(mGradientBuffer, weight_byteSize);

	for (int i = 0; i < Depth - 1; i++) {
		//RWStructuredBuffer用gDropout
		CreateResourceDef(mDropOutFBuffer[i], NumNode[i] * sizeof(float));
		//Up
		CreateResourceUp(mDropOutFUpBuffer[i], NumNode[i] * sizeof(float));
		//初期値コピー
		SubresourcesUp(dropout[i], NumNode[i], mDropOutFBuffer[i], mDropOutFUpBuffer[i]);
	}

	//up用gNode
	CreateResourceUp(mNodeUpBuffer, inputSetNum * NumNode[0] * sizeof(float));

	//up用gWeight
	CreateResourceUp(mWeightUpBuffer, weight_byteSize);

	//read用gWeight
	CreateResourceRead(mWeightReadBuffer, weight_byteSize);

	//read用gError
	CreateResourceRead(mErrorReadBuffer, inputSetNum * NumNode[0] * sizeof(float));

	SubresourcesUp(weight, weightNumAll, mWeightBuffer, mWeightUpBuffer);

	//ルートシグネチャ
	CD3DX12_ROOT_PARAMETER slotRootParameter[8];
	for (int i = 0; i < 7; i++)
		slotRootParameter[i].InitAsUnorderedAccessView(i);//RWStructuredBuffer(ui)
	slotRootParameter[7].InitAsConstantBufferView(0);//mObjectCB(b0)
	mRootSignatureCom = CreateRsCompute(8, slotRootParameter);

	UINT tmpNum[PARANUM];
	UINT tmpI0 = 1;
	UINT tmpI1 = Depth - 2;
	UINT tmpI2 = Depth - 2;
	UINT tmpI3 = Depth - 1;

	for (int k = 0; k < Depth - 1; k++) {
		tmpNum[0] = NumNode[tmpI0++];
		tmpNum[1] = NumNode[tmpI1--];
		tmpNum[2] = NumNode[tmpI2--];
		tmpNum[3] = NumNode[tmpI3--];
		char** replaceString = nullptr;

		CreateReplaceArr(&shaderThreadNum[k], &replaceString, PARANUM, tmpNum);

		char* repsh = nullptr;
		ReplaceString(&repsh, ShaderNeuralNetwork, '?', replaceString);
		for (int i = 0; i < PARANUM; i++)ARR_DELETE(replaceString[i]);
		ARR_DELETE(replaceString);

		pCS[0][k] = Dx_ShaderHolder::CompileShader(repsh, strlen(repsh), "NNFPCS", "cs_5_0");
		pCS[1][k] = Dx_ShaderHolder::CompileShader(repsh, strlen(repsh), "NNBPCS0", "cs_5_0");
		pCS[2][k] = Dx_ShaderHolder::CompileShader(repsh, strlen(repsh), "NNBPCS1", "cs_5_0");
		ARR_DELETE(repsh);
		for (int i = 0; i < NN_SHADER_NUM; i++) {
			mPSOCom[i][k] = CreatePsoCompute(pCS[i][k].Get(), mRootSignatureCom.Get());
		}
	}

	ac = new DxActivation * [Depth - 2];
	for (int i = 0; i < Depth - 2; i++) {
		ac[i] = new DxActivation(NumNode[i + 1], inputSetNum);
		ac[i]->ComCreate(node);
	}
	topAc = new DxActivation(NumNode[Depth - 1], inputSetNum);
	topAc->ComCreate(topNode);
}

void DxNeuralNetwork::SetActivationAlpha(float alpha) {
	for (int i = 0; i < Depth - 2; i++)ac[i]->SetActivationAlpha(alpha);
}

void DxNeuralNetwork::ForwardPropagation() {
	int repInd = 0;
	cb.Lear_Depth_inputS.w = (float)inputSetNumCur;
	for (int i = 0; i < Depth - 1; i++) {
		d->Bigin();
		CList->SetPipelineState(mPSOCom[0][repInd].Get());
		CList->SetComputeRootSignature(mRootSignatureCom.Get());
		CList->SetComputeRootUnorderedAccessView(0, mNodeBuffer[i]->GetGPUVirtualAddress());
		CList->SetComputeRootUnorderedAccessView(1, mNodeBuffer[i + 1]->GetGPUVirtualAddress());
		CList->SetComputeRootUnorderedAccessView(2, mWeightBuffer->GetGPUVirtualAddress());
		CList->SetComputeRootUnorderedAccessView(5, mDropOutFBuffer[i]->GetGPUVirtualAddress());
		cb.Lear_Depth_inputS.y = (float)i;
		mObjectCB->CopyData(0, cb);
		CList->SetComputeRootConstantBufferView(7, mObjectCB->Resource()->GetGPUVirtualAddress());
		//Dispatchは1回毎にGPU処理完了させる事
		CList->Dispatch(NumNode[i + 1] / shaderThreadNum[repInd][0], 1, inputSetNumCur);
		repInd++;
		d->End();
		cMa->RunGpu();
		cMa->WaitFence();

		if (i == Depth - 2) {
			topAc->SetInputResource(mNodeBuffer[Depth - 1].Get());
			topAc->ForwardPropagation(inputSetNumCur);
			CopyResource(mNodeBuffer[Depth - 1].Get(), topAc->GetOutputResource());
		}
		else {
			ac[i]->SetInputResource(mNodeBuffer[i + 1].Get());
			ac[i]->ForwardPropagation(inputSetNumCur);
			CopyResource(mNodeBuffer[i + 1].Get(), ac[i]->GetOutputResource());
		}
	}
}

void DxNeuralNetwork::BackPropagationNoWeightUpdate() {
	int repInd = 0;
	//逆伝播
	for (int i = Depth - 2; i >= 0; i--) {
		if (i == Depth - 2) {
			topAc->BackPropagation();
			CopyResource(mErrorBuffer[Depth - 1].Get(), topAc->GetOutErrorResource());
		}
		else {
			ac[i]->SetInErrorResource(mErrorBuffer[i + 1].Get());
			ac[i]->BackPropagation();
			CopyResource(mErrorBuffer[i + 1].Get(), ac[i]->GetOutErrorResource());
		}

		d->Bigin();
		CList->SetPipelineState(mPSOCom[1][repInd].Get());
		CList->SetComputeRootSignature(mRootSignatureCom.Get());
		CList->SetComputeRootUnorderedAccessView(0, mNodeBuffer[i]->GetGPUVirtualAddress());
		CList->SetComputeRootUnorderedAccessView(1, mNodeBuffer[i + 1]->GetGPUVirtualAddress());
		CList->SetComputeRootUnorderedAccessView(2, mWeightBuffer->GetGPUVirtualAddress());
		CList->SetComputeRootUnorderedAccessView(3, mErrorBuffer[i]->GetGPUVirtualAddress());
		CList->SetComputeRootUnorderedAccessView(4, mErrorBuffer[i + 1]->GetGPUVirtualAddress());
		CList->SetComputeRootUnorderedAccessView(5, mDropOutFBuffer[i]->GetGPUVirtualAddress());
		cb.Lear_Depth_inputS.y = (float)i;
		mObjectCB->CopyData(0, cb);
		CList->SetComputeRootConstantBufferView(7, mObjectCB->Resource()->GetGPUVirtualAddress());
		CList->Dispatch(NumNode[i] / shaderThreadNum[repInd][1], 1, inputSetNumCur);
		repInd++;
		d->End();
		cMa->RunGpu();
		cMa->WaitFence();
	}

	d->Bigin();
	CList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mErrorBuffer[0].Get(),
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE));
	CList->CopyResource(mErrorReadBuffer.Get(), mErrorBuffer[0].Get());
	CList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mErrorBuffer[0].Get(),
		D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
	d->End();
	cMa->RunGpu();
	cMa->WaitFence();
}

void DxNeuralNetwork::BackPropagation() {
	BackPropagationNoWeightUpdate();

	int repInd = 0;
	//勾配値更新
	for (int i = Depth - 2; i >= 0; i--) {
		d->Bigin();
		CList->SetPipelineState(mPSOCom[2][repInd].Get());
		CList->SetComputeRootSignature(mRootSignatureCom.Get());
		CList->SetComputeRootUnorderedAccessView(0, mNodeBuffer[i]->GetGPUVirtualAddress());
		CList->SetComputeRootUnorderedAccessView(1, mNodeBuffer[i + 1]->GetGPUVirtualAddress());
		CList->SetComputeRootUnorderedAccessView(2, mWeightBuffer->GetGPUVirtualAddress());
		CList->SetComputeRootUnorderedAccessView(4, mErrorBuffer[i + 1]->GetGPUVirtualAddress());
		CList->SetComputeRootUnorderedAccessView(6, mGradientBuffer->GetGPUVirtualAddress());
		cb.Lear_Depth_inputS.y = (float)i;
		mObjectCB->CopyData(0, cb);
		CList->SetComputeRootConstantBufferView(7, mObjectCB->Resource()->GetGPUVirtualAddress());
		CList->Dispatch(NumNode[i] / shaderThreadNum[repInd][2],
			NumNode[i + 1] / shaderThreadNum[repInd][3], 1);
		repInd++;
		d->End();
		cMa->RunGpu();
		cMa->WaitFence();
	}
	//オプティマイザー
	opt->SetInputGradientBuffer(mGradientBuffer.Get());
	opt->SetInputWeightBuffer(mWeightBuffer.Get());
	opt->comOptimizer();
	CopyResource(mWeightBuffer.Get(), opt->GetOutputWeightBuffer());

	d->Bigin();
	CList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mWeightBuffer.Get(),
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE));
	CList->CopyResource(mWeightReadBuffer.Get(), mWeightBuffer.Get());
	CList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mWeightBuffer.Get(),
		D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
	d->End();
	cMa->RunGpu();
	cMa->WaitFence();
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
	range.End = inputSetNum * NumNode[0] * sizeof(float);
	D3D12_SUBRESOURCE_DATA subResource;
	mErrorReadBuffer->Map(0, &range, reinterpret_cast<void**>(&subResource));
	float *err = (float*)subResource.pData;
	for (UINT i = 0; i < inputSetNum * NumNode[0]; i++) {
		error[i] = err[i];
	}
	mErrorReadBuffer->Unmap(0, nullptr);
}

void DxNeuralNetwork::setOptimizerParameter(float LearningRate, float AttenuationRate1,
	float AttenuationRate2, float DivergencePrevention) {
	opt->setOptimizerParameter(LearningRate, AttenuationRate1,
		AttenuationRate2, DivergencePrevention);
}

void DxNeuralNetwork::SetTarget(float* tar) {
	topAc->SetTarget(tar);
}

void DxNeuralNetwork::SetTargetEl(float el, UINT ElNum) {
	topAc->SetTargetEl(el, ElNum);
}

void DxNeuralNetwork::FirstInput(float el, UINT ElNum, UINT inputsetInd) {
	for (UINT i = 0; i < Split; i++)InputArrayEl(el, i, ElNum, inputsetInd);
}

void DxNeuralNetwork::InputArray(float* inArr, UINT arrNum, UINT inputsetInd) {
	memcpy(&input[NumNode[0] * inputsetInd + (NumNode[0] / Split) * arrNum], inArr, sizeof(float) * (NumNode[0] / Split));
	firstIn = true;
}

void DxNeuralNetwork::InputArrayEl(float el, UINT arrNum, UINT ElNum, UINT inputsetInd) {
	input[NumNode[0] * inputsetInd + (NumNode[0] / Split) * arrNum + ElNum] = el;
	firstIn = true;
}

void DxNeuralNetwork::InputResourse() {
	if (!firstIn)return;
	d->Bigin();
	SubresourcesUp(input, NumNode[0] * inputSetNum, mNodeBuffer[0], mNodeUpBuffer);
	d->End();
	cMa->RunGpu();
	cMa->WaitFence();
	firstIn = false;
}

void DxNeuralNetwork::GetOutput(float* out, UINT inputsetInd) {
	topAc->GetOutput(out, inputsetInd);
}

float DxNeuralNetwork::GetOutputEl(UINT ElNum, UINT inputsetInd) {
	return topAc->GetOutputEl(ElNum, inputsetInd);
}

void DxNeuralNetwork::Query(UINT inputsetnum) {
	InputResourse();
	inputSetNumCur = inputsetnum;
	ForwardPropagation();
}

void DxNeuralNetwork::QueryAndBackPropagation(UINT inputsetnum) {
	InputResourse();
	inputSetNumCur = inputsetnum;
	ForwardPropagation();
	BackPropagationNoWeightUpdate();
}

void DxNeuralNetwork::Training() {
	TrainingFp();
	TrainingBp();
}

void DxNeuralNetwork::TrainingFp() {
	InputResourse();
	SetDropOut();
	inputSetNumCur = inputSetNum;
	ForwardPropagation();
	topAc->comCrossEntropyError();
	crossEntropyError = topAc->GetcrossEntropyError();
}

void DxNeuralNetwork::TrainingBp() {
	BackPropagation();
	CopyWeightResourse();
	CopyErrorResourse();
	TextureCopy(mErrorBuffer[0].Get(), 0);
}

void DxNeuralNetwork::TrainingBpNoWeightUpdate() {
	BackPropagationNoWeightUpdate();
	CopyWeightResourse();
	CopyErrorResourse();
	TextureCopy(mErrorBuffer[0].Get(), 0);
}

float DxNeuralNetwork::GetcrossEntropyError() {
	return crossEntropyError;
}

float DxNeuralNetwork::GetcrossEntropyErrorTest() {
	return crossEntropyErrorTest;
}

void DxNeuralNetwork::Test() {
	InputResourse();
	SetDropOut();
	inputSetNumCur = 1;
	ForwardPropagation();
	topAc->comCrossEntropyError();
	crossEntropyErrorTest = topAc->GetcrossEntropyError();
}

float *DxNeuralNetwork::GetError(UINT arrNum, UINT inputsetInd) {
	return &error[(NumNode[0] / Split) * arrNum + inputsetInd * NumNode[0]];
}

float DxNeuralNetwork::GetErrorEl(UINT arrNum, UINT ElNum, UINT inputsetInd) {
	return error[(NumNode[0] / Split) * arrNum + inputsetInd * NumNode[0] + ElNum];
}

void DxNeuralNetwork::SetInputResource(ID3D12Resource* res) {
	CopyResource(mNodeBuffer[0].Get(), res);
}

void DxNeuralNetwork::SetInErrorResource(ID3D12Resource* res) {
	topAc->SetInErrorResource(res);
}

ID3D12Resource *DxNeuralNetwork::GetOutErrorResource() {
	return mErrorBuffer[0].Get();
}

ID3D12Resource* DxNeuralNetwork::GetOutputResource() {
	return topAc->GetOutputResource();
}

void DxNeuralNetwork::SaveData(char* pass) {
	FILE* fp = fopen(pass, "wb");
	float* weightArr = new float[weightNumAll];

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

void DxNeuralNetwork::LoadData(char* pass) {
	FILE* fp = fopen(pass, "rb");
	float* weightArr = new float[weightNumAll];
	fread(weightArr, sizeof(float) * weightNumAll, 1, fp);
	fclose(fp);

	UINT cnt = 0;
	for (int k = 0; k < Depth - 1; k++) {
		for (UINT i = 0; i < NumWeight[k]; i++) {
			weight[NumWeightStIndex[k] + i] = weightArr[cnt++];
		}
	}
	ARR_DELETE(weightArr);

	d->Bigin();
	SubresourcesUp(weight, weightNumAll, mWeightBuffer, mWeightUpBuffer);
	d->End();
	cMa->RunGpu();
	cMa->WaitFence();
}

float* DxNeuralNetwork::getWeightArr() {
	return weight;
}

float DxNeuralNetwork::getWeight(int depthIndex, int elIndex) {
	return weight[NumWeightStIndex[depthIndex] + elIndex];
}