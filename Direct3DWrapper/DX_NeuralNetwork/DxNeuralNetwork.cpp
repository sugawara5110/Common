//*****************************************************************************************//
//**                                                                                     **//
//**                   　  　　  　DxNeuralNetwork                                       **//
//**                                                                                     **//
//*****************************************************************************************//

#define _CRT_SECURE_NO_WARNINGS
#include "DxNeuralNetwork.h"
#include <random>
#include "ShaderNN\ShaderNeuralNetwork.h"
#define PARANUM 5

DxNeuralNetwork::DxNeuralNetwork(UINT* numNode, int depth, UINT split, UINT inputsetnum) {

	srand((unsigned)time(NULL));
	inputSetNum = inputsetnum;
	inputSetNumCur = inputSetNum;
	Split = split;

	if (depth > MAX_DEPTH_NUM)Depth = MAX_DEPTH_NUM;
	else
		Depth = depth;

	NumNode = new UINT[Depth];
	for (int i = 0; i < Depth; i++)NumNode[i] = numNode[i];

	NumNode[0] *= Split;

	if (NumNode[Depth - 1] > MAX_OUTPUT_NUM)NumNode[Depth - 1] = MAX_OUTPUT_NUM;

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

	output = new float[NumNode[Depth - 1] * inputSetNum];
	target = new float[NumNode[Depth - 1]];
	for (UINT i = 0; i < NumNode[Depth - 1]; i++)target[i] = 0.0f;

	dropThreshold = new float[Depth - 1];
	dropout = new float* [Depth - 1];
	for (int i = 0; i < Depth - 1; i++) {
		dropout[i] = new float[NumNode[i]];
		dropThreshold[i] = 0.5f;
	}
	for (int k = 0; k < Depth - 1; k++) {
		for (UINT i = 0; i < NumNode[k]; i++) {
			dropout[k][i] = 1.0f;//1.0fでニューロン有効
		}
	}

	dx = Dx12Process::GetInstance();
	mCommandList = dx->dx_sub[0].mCommandList.Get();
	mObjectCB = new ConstantBuffer<CONSTANT_BUFFER_NeuralNetwork>(1);
}

void DxNeuralNetwork::SetDropOut() {

	dx->Bigin(com_no);
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
	dx->End(com_no);
	dx->WaitFenceCurrent();
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
	// 平均0.0、標準偏差 1/sqrt(NumNode)で分布させる
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

	ARR_DELETE(output);
	ARR_DELETE(target);

	ARR_DELETE(input);
	ARR_DELETE(error);
	ARR_DELETE(weight);
	for (int i = 0; i < Depth - 1; i++) {
		ARR_DELETE(shaderThreadNum[i]);
		ARR_DELETE(dropout[i]);
	}
	ARR_DELETE(dropout);
	ARR_DELETE(dropThreshold);

	ARR_DELETE(NumNodeStIndex);
	ARR_DELETE(NumWeight);
	ARR_DELETE(NumWeightStIndex);

	S_DELETE(mObjectCB);
	ARR_DELETE(NumNode);
}

void DxNeuralNetwork::ComCreate(bool sigon) {

	for (int i = 0; i < Depth; i++) {
		cb.NumNode[i].as((float)NumNode[i], (float)NumNodeStIndex[i], 0.0f, 0.0f);
	}
	for (int i = 0; i < Depth - 1; i++) {
		cb.NumWeight[i].x = (float)NumWeightStIndex[i];
	}
	cb.Lear_Depth_inputS.z = (float)Depth - 1;
	cb.Lear_Depth_inputS.w = (float)inputSetNum;
	cb.LeakyReLUAlpha = 0.0f;//初期値のままだと通常ReLU(ReLU使用時)

	weight_byteSize = weightNumAll * sizeof(float);

	for (int i = 0; i < Depth; i++) {
		//RWStructuredBuffer用gNode
		CreateResourceDef(mNodeBuffer[i], inputSetNum * NumNode[i] * sizeof(float));
		//RWStructuredBuffer用gError
		CreateResourceDef(mErrorBuffer[i], inputSetNum * NumNode[i] * sizeof(float));
	}

	//RWStructuredBuffer用gWeight
	CreateResourceDef(mWeightBuffer, weight_byteSize);

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

	//read用gNode
	CreateResourceRead(mNodeReadBuffer, inputSetNum * NumNode[Depth - 1] * sizeof(float));

	//read用gWeight
	CreateResourceRead(mWeightReadBuffer, weight_byteSize);

	//read用gError
	CreateResourceRead(mErrorReadBuffer, inputSetNum * NumNode[0] * sizeof(float));

	SubresourcesUp(weight, weightNumAll, mWeightBuffer, mWeightUpBuffer);

	//ルートシグネチャ
	CD3DX12_ROOT_PARAMETER slotRootParameter[7];
	slotRootParameter[0].InitAsUnorderedAccessView(0);//RWStructuredBuffer(u0)
	slotRootParameter[1].InitAsUnorderedAccessView(1);//RWStructuredBuffer(u1)
	slotRootParameter[2].InitAsUnorderedAccessView(2);//RWStructuredBuffer(u2)
	slotRootParameter[3].InitAsUnorderedAccessView(3);//RWStructuredBuffer(u3)
	slotRootParameter[4].InitAsUnorderedAccessView(4);//RWStructuredBuffer(u4)
	slotRootParameter[5].InitAsUnorderedAccessView(5);//RWStructuredBuffer(u5)
	slotRootParameter[6].InitAsConstantBufferView(0);//mObjectCB(b0)
	mRootSignatureCom = CreateRsCompute(7, slotRootParameter);

	UINT tmpNum[PARANUM];
	UINT tmpI0 = 1;
	UINT tmpI1 = Depth - 1;//これだけ固定
	UINT tmpI2 = Depth - 2;
	UINT tmpI3 = Depth - 2;
	UINT tmpI4 = Depth - 1;

	for (int k = 0; k < Depth - 1; k++) {
		tmpNum[0] = NumNode[tmpI0++];
		tmpNum[1] = NumNode[tmpI1];
		tmpNum[2] = NumNode[tmpI2--];
		tmpNum[3] = NumNode[tmpI3--];
		tmpNum[4] = NumNode[tmpI4--];
		char** replaceString = nullptr;

		CreateReplaceArr(&shaderThreadNum[k], &replaceString, PARANUM, tmpNum);

		char* repsh = nullptr;
		ReplaceString(&repsh, ShaderNeuralNetwork, '?', replaceString);
		for (int i = 0; i < PARANUM; i++)ARR_DELETE(replaceString[i]);
		ARR_DELETE(replaceString);

		if (sigon) {
			pCS[0][k] = CompileShader(repsh, strlen(repsh), "NNFPCS", "cs_5_0");
			pCS[2][k] = CompileShader(repsh, strlen(repsh), "NNBPCS0", "cs_5_0");
		}
		else {
			pCS[0][k] = CompileShader(repsh, strlen(repsh), "NNFPReLUCS", "cs_5_0");
			pCS[2][k] = CompileShader(repsh, strlen(repsh), "NNBPReLUCS0", "cs_5_0");
		}
		pCS[1][k] = CompileShader(repsh, strlen(repsh), "InTargetCS", "cs_5_0");
		pCS[3][k] = CompileShader(repsh, strlen(repsh), "NNBPCS1", "cs_5_0");
		ARR_DELETE(repsh);
		for (int i = 0; i < NN_SHADER_NUM; i++) {
			mPSOCom[i][k] = CreatePsoCompute(pCS[i][k].Get(), mRootSignatureCom.Get());
		}
	}
}

void DxNeuralNetwork::ComCreateSigmoid() {
	SetWeightInitXavier(1.0f);
	ComCreate(true);
}

void DxNeuralNetwork::ComCreateReLU() {
	SetWeightInitHe(1.0f);
	ComCreate(false);
}

void DxNeuralNetwork::SetLeakyReLUAlpha(float alpha) {
	cb.LeakyReLUAlpha = alpha;
}

void DxNeuralNetwork::ForwardPropagation() {
	int repInd = 0;
	for (int i = 0; i < Depth - 1; i++) {
		dx->Bigin(com_no);
		mCommandList->SetPipelineState(mPSOCom[0][repInd].Get());
		mCommandList->SetComputeRootSignature(mRootSignatureCom.Get());
		mCommandList->SetComputeRootUnorderedAccessView(0, mNodeBuffer[i]->GetGPUVirtualAddress());
		mCommandList->SetComputeRootUnorderedAccessView(1, mNodeBuffer[i + 1]->GetGPUVirtualAddress());
		mCommandList->SetComputeRootUnorderedAccessView(2, mWeightBuffer->GetGPUVirtualAddress());
		mCommandList->SetComputeRootUnorderedAccessView(5, mDropOutFBuffer[i]->GetGPUVirtualAddress());
		cb.Lear_Depth_inputS.x = learningRate;
		cb.Lear_Depth_inputS.y = (float)i;
		cb.Lear_Depth_inputS.w = (float)inputSetNumCur;
		for (UINT i1 = 0; i1 < NumNode[Depth - 1]; i1++)
			cb.Target[i1].x = target[i1];
		mObjectCB->CopyData(0, cb);
		mCommandList->SetComputeRootConstantBufferView(6, mObjectCB->Resource()->GetGPUVirtualAddress());
		//Dispatchは1回毎にGPU処理完了させる事
		mCommandList->Dispatch(NumNode[i + 1] / shaderThreadNum[repInd][0], 1, inputSetNumCur);
		repInd++;
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

void DxNeuralNetwork::BackPropagationNoWeightUpdate() {
	//target入力
	dx->Bigin(com_no);
	mCommandList->SetPipelineState(mPSOCom[1][0].Get());
	mCommandList->SetComputeRootSignature(mRootSignatureCom.Get());
	mCommandList->SetComputeRootUnorderedAccessView(1, mNodeBuffer[Depth - 1]->GetGPUVirtualAddress());
	mCommandList->SetComputeRootUnorderedAccessView(2, mWeightBuffer->GetGPUVirtualAddress());
	mCommandList->SetComputeRootUnorderedAccessView(4, mErrorBuffer[Depth - 1]->GetGPUVirtualAddress());
	cb.Lear_Depth_inputS.y = (float)(Depth - 1);
	mObjectCB->CopyData(0, cb);
	mCommandList->SetComputeRootConstantBufferView(6, mObjectCB->Resource()->GetGPUVirtualAddress());
	mCommandList->Dispatch(NumNode[Depth - 1] / shaderThreadNum[0][1], 1, inputSetNumCur);
	dx->End(com_no);
	dx->WaitFenceCurrent();

	int repInd = 0;
	//逆伝播
	for (int i = Depth - 2; i >= 0; i--) {
		dx->Bigin(com_no);
		mCommandList->SetPipelineState(mPSOCom[2][repInd].Get());
		mCommandList->SetComputeRootSignature(mRootSignatureCom.Get());
		mCommandList->SetComputeRootUnorderedAccessView(0, mNodeBuffer[i]->GetGPUVirtualAddress());
		mCommandList->SetComputeRootUnorderedAccessView(1, mNodeBuffer[i + 1]->GetGPUVirtualAddress());
		mCommandList->SetComputeRootUnorderedAccessView(2, mWeightBuffer->GetGPUVirtualAddress());
		mCommandList->SetComputeRootUnorderedAccessView(3, mErrorBuffer[i]->GetGPUVirtualAddress());
		mCommandList->SetComputeRootUnorderedAccessView(4, mErrorBuffer[i + 1]->GetGPUVirtualAddress());
		mCommandList->SetComputeRootUnorderedAccessView(5, mDropOutFBuffer[i]->GetGPUVirtualAddress());
		cb.Lear_Depth_inputS.y = (float)i;
		mObjectCB->CopyData(0, cb);
		mCommandList->SetComputeRootConstantBufferView(6, mObjectCB->Resource()->GetGPUVirtualAddress());
		mCommandList->Dispatch(NumNode[i] / shaderThreadNum[repInd][2], 1, inputSetNumCur);
		repInd++;
		dx->End(com_no);
		dx->WaitFenceCurrent();
	}

	dx->Bigin(com_no);
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mErrorBuffer[0].Get(),
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE));
	mCommandList->CopyResource(mErrorReadBuffer.Get(), mErrorBuffer[0].Get());
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mErrorBuffer[0].Get(),
		D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
	dx->End(com_no);
	dx->WaitFenceCurrent();
}

void DxNeuralNetwork::BackPropagation() {
	BackPropagationNoWeightUpdate();

	int repInd = 0;
	//weight値更新
	for (int i = Depth - 2; i >= 0; i--) {
		dx->Bigin(com_no);
		mCommandList->SetPipelineState(mPSOCom[3][repInd].Get());
		mCommandList->SetComputeRootSignature(mRootSignatureCom.Get());
		mCommandList->SetComputeRootUnorderedAccessView(0, mNodeBuffer[i]->GetGPUVirtualAddress());
		mCommandList->SetComputeRootUnorderedAccessView(1, mNodeBuffer[i + 1]->GetGPUVirtualAddress());
		mCommandList->SetComputeRootUnorderedAccessView(2, mWeightBuffer->GetGPUVirtualAddress());
		mCommandList->SetComputeRootUnorderedAccessView(4, mErrorBuffer[i + 1]->GetGPUVirtualAddress());
		cb.Lear_Depth_inputS.y = (float)i;
		mObjectCB->CopyData(0, cb);
		mCommandList->SetComputeRootConstantBufferView(6, mObjectCB->Resource()->GetGPUVirtualAddress());
		mCommandList->Dispatch(NumNode[i] / shaderThreadNum[repInd][3],
			NumNode[i + 1] / shaderThreadNum[repInd][4], 1);
		repInd++;
		dx->End(com_no);
		dx->WaitFenceCurrent();
	}

	dx->Bigin(com_no);
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
	range.End = inputSetNum * NumNode[Depth - 1] * sizeof(float);
	D3D12_SUBRESOURCE_DATA subResource;
	mNodeReadBuffer->Map(0, &range, reinterpret_cast<void**>(&subResource));
	float *nod = (float*)subResource.pData;
	for (UINT i = 0; i < inputSetNum * NumNode[Depth - 1]; i++) {
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
	range.End = inputSetNum * NumNode[0] * sizeof(float);
	D3D12_SUBRESOURCE_DATA subResource;
	mErrorReadBuffer->Map(0, &range, reinterpret_cast<void**>(&subResource));
	float *err = (float*)subResource.pData;
	for (UINT i = 0; i < inputSetNum * NumNode[0]; i++) {
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

void DxNeuralNetwork::FirstInput(float el, UINT ElNum, UINT inputsetInd) {
	for (UINT i = 0; i < Split; i++)InputArrayEl(el, i, ElNum, inputsetInd);
	firstIn = true;
}

void DxNeuralNetwork::InputArray(float *inArr, UINT arrNum, UINT inputsetInd) {
	memcpy(&input[NumNode[0] * inputsetInd + (NumNode[0] / Split) * arrNum], inArr, sizeof(float) * (NumNode[0] / Split));
}

void DxNeuralNetwork::InputArrayEl(float el, UINT arrNum, UINT ElNum, UINT inputsetInd) {
	input[NumNode[0] * inputsetInd + (NumNode[0] / Split) * arrNum + ElNum] = el;
}

void DxNeuralNetwork::InputResourse() {
	if (!firstIn)return;
	dx->Bigin(com_no);
	SubresourcesUp(input, NumNode[0] * inputSetNum, mNodeBuffer[0], mNodeUpBuffer);
	dx->End(com_no);
	dx->WaitFenceCurrent();
	firstIn = false;
}

void DxNeuralNetwork::GetOutput(float *out, UINT inputsetInd) {
	for (UINT i = 0; i < NumNode[Depth - 1]; i++) {
		out[i] = output[NumNode[Depth - 1] * inputsetInd + i];
	}
}

float DxNeuralNetwork::GetOutputEl(UINT ElNum, UINT inputsetInd) {
	return output[NumNode[Depth - 1] * inputsetInd + ElNum];
}

void DxNeuralNetwork::Query(UINT inputsetnum) {
	InputResourse();
	inputSetNumCur = inputsetnum;
	ForwardPropagation();
	CopyOutputResourse();
}

void DxNeuralNetwork::QueryAndBackPropagation(UINT inputsetnum) {
	InputResourse();
	inputSetNumCur = inputsetnum;
	ForwardPropagation();
	CopyOutputResourse();
	BackPropagationNoWeightUpdate();
}

void DxNeuralNetwork::Training() {
	InputResourse();
	SetDropOut();
	inputSetNumCur = inputSetNum;
	ForwardPropagation();
	CopyOutputResourse();
	comCrossEntropyError(true);
	BackPropagation();
	CopyWeightResourse();
	//CopyErrorResourse();
	TextureCopy(mErrorBuffer[0].Get(), com_no);
}

void DxNeuralNetwork::comCrossEntropyError(bool f) {
	float err = 0.0f;
	for (UINT j = 0; j < inputSetNumCur; j++) {
		for (UINT i = 0; i < NumNode[Depth - 1]; i++) {
			UINT outInd = NumNode[Depth - 1] * j + i;
			err += -target[i] * log2(output[outInd] + 0.0000001f) - (1.0f - target[i]) *
				log2(1.0f - output[outInd] + 0.0000001f);
		}
	}
	if (f)crossEntropyError = err / (float)inputSetNumCur;
	else crossEntropyErrorTest = err / (float)inputSetNumCur;
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
	CopyOutputResourse();
	comCrossEntropyError(false);
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
