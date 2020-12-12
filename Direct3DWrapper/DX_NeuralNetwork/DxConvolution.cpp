//*****************************************************************************************//
//**                                                                                     **//
//**                              DxConvolution                                          **//
//**                                                                                     **//
//*****************************************************************************************//

#define _CRT_SECURE_NO_WARNINGS
#include "DxConvolution.h"
#include <random>
#include "ShaderNN\ShaderConvolution.h"
#define PARANUMCN 11

void DxConvolution::setOptimizerParameterFil(float LearningRate, float AttenuationRate1,
	float AttenuationRate2, float DivergencePrevention) {
	optFil->setOptimizerParameter(LearningRate, AttenuationRate1, AttenuationRate2, DivergencePrevention);
}

void DxConvolution::setOptimizerParameterBias(float LearningRate, float AttenuationRate1,
	float AttenuationRate2, float DivergencePrevention) {
	optBias->setOptimizerParameter(LearningRate, AttenuationRate1, AttenuationRate2, DivergencePrevention);
}

DxConvolution::DxConvolution(UINT width, UINT height, UINT filNum, bool deconvolutionMode, UINT inputsetnum, UINT elnumwid, UINT filstep) {

	DeconvolutionMode = deconvolutionMode;

	srand((unsigned)time(NULL));
	inputSetNum = inputsetnum;
	inputSetNumCur = inputSetNum;
	elNumWid = elnumwid;
	ElNum = elNumWid * elNumWid;
	filterStep = filstep;

	if (elNumWid % 2 == 0)MessageBoxA(0, "filter要素数は奇数のみ有効です", 0, MB_OK);
	if (elNumWid > 7)MessageBoxA(0, "filter要素数はMax7です", 0, MB_OK);
	if (filterStep > 8)MessageBoxA(0, "filterスライド数はMax8です", 0, MB_OK);
	if (filterStep != 8 && filterStep != 4 &&
		filterStep != 2 && filterStep != 1)MessageBoxA(0, "filterスライド数は2の累乗のみ有効です", 0, MB_OK);

	Width = width;
	Height = height;
	if (!DeconvolutionMode) {
		OutWid = Width / filterStep;
		OutHei = Height / filterStep;
	}
	else {
		OutWid = Width * filterStep;
		OutHei = Height * filterStep;
	}
	FilNum = filNum;
	filSize = ElNum * sizeof(float);
	input_outerrOneNum = Width * Height;
	input_outerrOneSize = input_outerrOneNum * sizeof(float);
	output_inerrOneNum = OutWid * OutHei;
	output_inerrOneSize = output_inerrOneNum * sizeof(float);
	fil = new float[FilNum * ElNum];
	input = new float[FilNum * input_outerrOneNum * inputSetNum];
	output = new float[FilNum * output_inerrOneNum * inputSetNum];
	outputError = new float[FilNum * input_outerrOneNum * inputSetNum];
	inputError = new float[FilNum * output_inerrOneNum * inputSetNum];
	for (UINT i = 0; i < FilNum * input_outerrOneNum * inputSetNum; i++)
		input[i] = 0.0f;
	for (UINT i = 0; i < FilNum * output_inerrOneNum * inputSetNum; i++)
		inputError[i] = 0.0f;

	if (!DeconvolutionMode) {
		Numdrop = FilNum * input_outerrOneNum;
	}
	else {
		Numdrop = FilNum * output_inerrOneNum;
	}
	dropout = new float[Numdrop];
	for (UINT i = 0; i < Numdrop; i++)dropout[i] = 1.0f;

	bias = new float[FilNum];
	ZeroMemory(bias, sizeof(float) * FilNum);

	dx = Dx12Process::GetInstance();
	mCommandList = dx->dx_sub[0].mCommandList.Get();
	mObjectCB = new ConstantBuffer<CONSTANT_BUFFER_Convolution>(1);
	cb.WidHei.x = (float)Width;
	cb.WidHei.y = (float)Height;
	cb.WidHei.z = (float)FilNum;
	cb.filWid_filStep.x = (float)elNumWid;
	cb.filWid_filStep.y = (float)filterStep;
	cb.Lear_inputS.y = (float)inputSetNum;
	mObjectCB->CopyData(0, cb);
}

void DxConvolution::SetDropOut() {

	dx->Bigin(com_no);
	for (UINT i = 0; i < Numdrop; i++) {
		dropout[i] = 1.0f;
		int rnd = (rand() % 100) + 1;
		if ((int)(dropThreshold * 100.0f) >= rnd) {
			dropout[i] = 0.0f;
		}
	}
	SubresourcesUp(dropout, Numdrop, mDropOutFBuffer, mDropOutFUpBuffer);
	dx->End(com_no);
	dx->RunGpu();
	dx->WaitFence();
}

void DxConvolution::SetdropThreshold(float Threshold) {
	dropThreshold = Threshold;
}

void DxConvolution::SetWeightInitXavier(float rate) {
	std::random_device seed_gen;
	std::default_random_engine engine(seed_gen());
	//フィルタ初期値
	std::normal_distribution<> dist(0.0, 1 / sqrt(FilNum * ElNum) * rate);
	for (UINT i = 0; i < FilNum * ElNum; i++)
		fil[i] = (float)dist(engine);
}

void DxConvolution::SetWeightInitHe(float rate) {
	std::random_device seed_gen;
	std::default_random_engine engine(seed_gen());
	//フィルタ初期値
	std::normal_distribution<> dist(0.0, 2 / sqrt(FilNum * ElNum) * rate);
	for (UINT i = 0; i < FilNum * ElNum; i++)
		fil[i] = (float)dist(engine);
}

DxConvolution::~DxConvolution() {

	ARR_DELETE(input);
	ARR_DELETE(output);
	ARR_DELETE(fil);
	ARR_DELETE(outputError);
	ARR_DELETE(inputError);
	S_DELETE(mObjectCB);
	ARR_DELETE(dropout);
	ARR_DELETE(bias);
	ARR_DELETE(shaderThreadNum);
	S_DELETE(ac);
	S_DELETE(optFil);
	S_DELETE(optBias);
}

void DxConvolution::ComCreate(ActivationName node, OptimizerName optName, float wInit) {

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

	//RWStructuredBuffer用gInput
	CreateResourceDef(mInputBuffer, input_outerrOneSize * FilNum * inputSetNum);

	if (!DeconvolutionMode) {
		CreateResourceDef(mInputBuffer2, input_outerrOneSize * FilNum * inputSetNum);
	}
	else {
		CreateResourceDef(mInputBuffer2, output_inerrOneSize * FilNum * inputSetNum);
	}

	UINT dropSize = Numdrop * sizeof(float);
	//RWStructuredBuffer用gDropout
	CreateResourceDef(mDropOutFBuffer, dropSize);
	//Up
	CreateResourceUp(mDropOutFUpBuffer, dropSize);
	//初期値コピー
	SubresourcesUp(dropout, Numdrop, mDropOutFBuffer, mDropOutFUpBuffer);

	//RWStructuredBuffer用gOutput
	CreateResourceDef(mOutputBuffer, output_inerrOneSize * FilNum * inputSetNum);

	//RWStructuredBuffer用gInErr
	CreateResourceDef(mInErrorBuffer, output_inerrOneSize * FilNum * inputSetNum);
	//mInErrorBufferのfilterStep倍拡大, 間は0.0fで埋める
	CreateResourceDef(mInErrorBuffer2, output_inerrOneSize * FilNum * inputSetNum * filterStep);

	//RWStructuredBuffer用gOutErr
	CreateResourceDef(mOutErrorBuffer, input_outerrOneSize * FilNum * inputSetNum);

	//RWStructuredBuffer用gFilter
	CreateResourceDef(mFilterBuffer, filSize * FilNum);
	//オプティマイザー
	optFil = new DxOptimizer(FilNum * ElNum);
	optFil->ComCreate(optName);

	//RWStructuredBuffer用gGradient
	CreateResourceDef(mGradientBuffer, filSize * FilNum);

	//up用gInput
	CreateResourceUp(mInputUpBuffer, input_outerrOneSize * FilNum * inputSetNum);

	//up用gInErr
	CreateResourceUp(mInErrorUpBuffer, output_inerrOneSize * FilNum * inputSetNum);

	//up用gFilter
	CreateResourceUp(mFilterUpBuffer, filSize * FilNum);

	//read用gOutput
	CreateResourceRead(mOutputReadBuffer, output_inerrOneSize * FilNum * inputSetNum);

	//read用gOutErr
	CreateResourceRead(mOutErrorReadBuffer, input_outerrOneSize * FilNum * inputSetNum);

	//read用gFilter
	CreateResourceRead(mFilterReadBuffer, filSize * FilNum);

	SubresourcesUp(input, input_outerrOneNum * FilNum * inputSetNum, mInputBuffer, mInputUpBuffer);

	SubresourcesUp(inputError, output_inerrOneNum * FilNum * inputSetNum, mInErrorBuffer, mInErrorUpBuffer);

	SubresourcesUp(fil, ElNum * FilNum, mFilterBuffer, mFilterUpBuffer);

	//RWStructuredBuffer用gBias
	CreateResourceDef(mBiasBuffer, sizeof(float) * FilNum);
	optBias = new DxOptimizer(FilNum);
	optBias->ComCreate(optName);

	//RWStructuredBuffer用gGradBias
	CreateResourceDef(mGradBiasBuffer, sizeof(float) * FilNum);
	//Up
	CreateResourceUp(mBiasUpBuffer, sizeof(float) * FilNum);
	//Read
	CreateResourceRead(mBiasReadBuffer, sizeof(float) * FilNum);

	SubresourcesUp(bias, FilNum, mBiasBuffer, mBiasUpBuffer);

	//ルートシグネチャ
	CD3DX12_ROOT_PARAMETER slotRootParameter[10];
	for (int i = 0; i < 9; i++)
		slotRootParameter[i].InitAsUnorderedAccessView(i);//RWStructuredBuffer(ui)
	slotRootParameter[9].InitAsConstantBufferView(0);//mObjectCB(b0)
	mRootSignatureCom = CreateRsCompute(10, slotRootParameter);

	UINT tmpNum[PARANUMCN];
	//CNFPCS0
	tmpNum[0] = Width;
	tmpNum[1] = Height * FilNum;
	//CNFPCS
	tmpNum[2] = OutWid;
	tmpNum[3] = OutHei * FilNum;
	//CNBPCS0
	tmpNum[4] = OutWid;
	tmpNum[5] = OutHei * FilNum;
	//CNBPCS1
	tmpNum[6] = Width;
	tmpNum[7] = Height * FilNum;
	//CNBPCS2
	tmpNum[8] = elNumWid;
	tmpNum[9] = elNumWid * FilNum;
	//CNBPCSBias
	tmpNum[10] = FilNum;
	char** replaceString = nullptr;

	CreateReplaceArr(&shaderThreadNum, &replaceString, PARANUMCN, tmpNum);

	char* repsh = nullptr;
	ReplaceString(&repsh, ShaderConvolution, '?', replaceString);
	for (int i = 0; i < PARANUMCN; i++)ARR_DELETE(replaceString[i]);
	ARR_DELETE(replaceString);

	pCS[0] = CompileShader(repsh, strlen(repsh), "CNFPCS0", "cs_5_0");
	pCS[1] = CompileShader(repsh, strlen(repsh), "CNFPCS", "cs_5_0");
	pCS[2] = CompileShader(repsh, strlen(repsh), "CNBPCS0", "cs_5_0");
	pCS[3] = CompileShader(repsh, strlen(repsh), "CNBPCS1", "cs_5_0");
	pCS[4] = CompileShader(repsh, strlen(repsh), "CNBPCS2", "cs_5_0");
	pCS[5] = CompileShader(repsh, strlen(repsh), "CNBPCSBias", "cs_5_0");
	pCS[6] = CompileShader(repsh, strlen(repsh), "CNInitCS", "cs_5_0");

	ARR_DELETE(repsh);
	for (int i = 0; i < CN_SHADER_NUM; i++)
		mPSOCom[i] = CreatePsoCompute(pCS[i].Get(), mRootSignatureCom.Get());

	ac = new DxActivation(FilNum * output_inerrOneNum, inputSetNum);
	ac->ComCreate(node);

	//Shaderパラメーター
	if (!DeconvolutionMode) {
		//CNFPCS0
		sstep[0].step = 1;
		sstep[0].width = Width;
		sstep[0].height = Height;
		//CNFPCS
		sstep[1].step = filterStep;
		sstep[1].width = Width;
		sstep[1].height = Height;
		//CNBPCS0
		sstep[2].step = filterStep;
		sstep[2].width = Width;
		sstep[2].height = Height;
		//CNBPCS1
		sstep[3].step = 1;
		sstep[3].width = Width;
		sstep[3].height = Height;
		//CNBPCS2
		sstep[4].step = 1;
		sstep[4].width = Width;
		sstep[4].height = Height;
		//CNBPCSBias
		sstep[5].step = 1;
		sstep[5].width = Width;
		sstep[5].height = Height;
		//CNInitCS, Fp
		sstep[6].step = 1;
		sstep[6].width = Width;
		sstep[6].height = Height;
		//CNInitCS, Bp
		sstep[7].step = 1;
		sstep[7].width = Width;
		sstep[7].height = Height;
	}
	else {
		//CNFPCS0
		sstep[0].step = filterStep;
		sstep[0].width = Width;
		sstep[0].height = Height;
		//CNFPCS
		sstep[1].step = 1;
		sstep[1].width = OutWid;
		sstep[1].height = OutHei;
		//CNBPCS0
		sstep[2].step = 1;
		sstep[2].width = OutWid;
		sstep[2].height = OutHei;
		//CNBPCS1
		sstep[3].step = filterStep;
		sstep[3].width = Width;
		sstep[3].height = Height;
		//CNBPCS2
		sstep[4].step = filterStep;
		sstep[4].width = Width;
		sstep[4].height = Height;
		//CNBPCSBias
		sstep[5].step = filterStep;
		sstep[5].width = Width;
		sstep[5].height = Height;
		//CNInitCS, Fp
		sstep[6].step = 1;
		sstep[6].width = OutWid;
		sstep[6].height = OutHei;
		//CNInitCS, Bp
		sstep[7].step = 1;
		sstep[7].width = OutWid;
		sstep[7].height = OutHei;
	}
}

void DxConvolution::SetActivationAlpha(float alpha) {
	ac->SetActivationAlpha(alpha);
}

void DxConvolution::FirstInput(float el, UINT ElNum, UINT inputsetInd) {
	for (UINT i = 0; i < FilNum; i++)InputEl(el - 0.5f, i, ElNum, inputsetInd);
}

void DxConvolution::InputEl(float el, UINT arrNum, UINT ElNum, UINT inputsetInd) {
	input[input_outerrOneNum * FilNum * inputsetInd + arrNum * input_outerrOneNum + ElNum] = el;
	firstIn = true;
}

void DxConvolution::Input(float* inArr, UINT arrNum, UINT inputsetInd) {
	memcpy(&input[input_outerrOneNum * FilNum * inputsetInd + arrNum * input_outerrOneNum], inArr, input_outerrOneSize);
	firstIn = true;
}

void DxConvolution::InputError(float* inArr, UINT arrNum, UINT inputsetInd) {
	memcpy(&inputError[inputsetInd * output_inerrOneNum * FilNum + arrNum * output_inerrOneNum], inArr, output_inerrOneSize);
	firstInErr = true;
}

void DxConvolution::SetGPUVirtualAddressExpIn() {
	mCommandList->SetComputeRootSignature(mRootSignatureCom.Get());
	mCommandList->SetComputeRootUnorderedAccessView(0, mInputBuffer->GetGPUVirtualAddress());
	mCommandList->SetComputeRootUnorderedAccessView(1, mInputBuffer2->GetGPUVirtualAddress());
	mCommandList->SetComputeRootConstantBufferView(9, mObjectCB->Resource()->GetGPUVirtualAddress());
}

void DxConvolution::SetGPUVirtualAddressExpErr() {
	mCommandList->SetComputeRootSignature(mRootSignatureCom.Get());
	mCommandList->SetComputeRootUnorderedAccessView(2, mInErrorBuffer->GetGPUVirtualAddress());
	mCommandList->SetComputeRootUnorderedAccessView(3, mInErrorBuffer2->GetGPUVirtualAddress());
	mCommandList->SetComputeRootConstantBufferView(9, mObjectCB->Resource()->GetGPUVirtualAddress());
}

void DxConvolution::SetGPUVirtualAddress() {
	mCommandList->SetComputeRootSignature(mRootSignatureCom.Get());
	mCommandList->SetComputeRootUnorderedAccessView(0, mInputBuffer2->GetGPUVirtualAddress());
	mCommandList->SetComputeRootUnorderedAccessView(1, mOutputBuffer->GetGPUVirtualAddress());
	mCommandList->SetComputeRootUnorderedAccessView(2, mInErrorBuffer2->GetGPUVirtualAddress());
	mCommandList->SetComputeRootUnorderedAccessView(3, mOutErrorBuffer->GetGPUVirtualAddress());
	mCommandList->SetComputeRootUnorderedAccessView(4, mFilterBuffer->GetGPUVirtualAddress());
	mCommandList->SetComputeRootUnorderedAccessView(5, mDropOutFBuffer->GetGPUVirtualAddress());
	mCommandList->SetComputeRootUnorderedAccessView(6, mBiasBuffer->GetGPUVirtualAddress());
	mCommandList->SetComputeRootUnorderedAccessView(7, mGradientBuffer->GetGPUVirtualAddress());
	mCommandList->SetComputeRootUnorderedAccessView(8, mGradBiasBuffer->GetGPUVirtualAddress());
	mCommandList->SetComputeRootConstantBufferView(9, mObjectCB->Resource()->GetGPUVirtualAddress());
}

void DxConvolution::setshaderStep(UINT index) {
	cb.filWid_filStep.y = (float)sstep[index].step;
	cb.WidHei.x = (float)sstep[index].width;
	cb.WidHei.y = (float)sstep[index].height;
	mObjectCB->CopyData(0, cb);
}

void DxConvolution::ForwardPropagation() {
	cb.Lear_inputS.y = (float)inputSetNumCur;
	mObjectCB->CopyData(0, cb);

	//zero初期化
	setshaderStep(6);
	dx->Bigin(com_no);
	mCommandList->SetPipelineState(mPSOCom[6].Get());
	mCommandList->SetComputeRootSignature(mRootSignatureCom.Get());
	mCommandList->SetComputeRootUnorderedAccessView(0, mInputBuffer2->GetGPUVirtualAddress());
	mCommandList->SetComputeRootConstantBufferView(9, mObjectCB->Resource()->GetGPUVirtualAddress());
	mCommandList->Dispatch((UINT)cb.WidHei.x, (UINT)cb.WidHei.y * FilNum, inputSetNumCur);
	dx->End(com_no);
	dx->RunGpu();
	dx->WaitFence();

	//input拡大
	setshaderStep(0);
	dx->Bigin(com_no);
	mCommandList->SetPipelineState(mPSOCom[0].Get());
	SetGPUVirtualAddressExpIn();
	mCommandList->Dispatch(Width / shaderThreadNum[0], Height * FilNum / shaderThreadNum[1], inputSetNumCur);
	dx->End(com_no);
	dx->RunGpu();
	dx->WaitFence();

	setshaderStep(1);
	dx->Bigin(com_no);
	mCommandList->SetPipelineState(mPSOCom[1].Get());
	SetGPUVirtualAddress();
	mCommandList->Dispatch(OutWid / shaderThreadNum[2], OutHei * FilNum / shaderThreadNum[3], inputSetNumCur);
	dx->End(com_no);
	dx->RunGpu();
	dx->WaitFence();

	ac->SetInputResource(mOutputBuffer.Get());
	ac->ForwardPropagation(inputSetNumCur);
	CopyResource(mOutputBuffer.Get(), ac->GetOutputResource());

	dx->Bigin(com_no);
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mOutputBuffer.Get(),
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE));
	mCommandList->CopyResource(mOutputReadBuffer.Get(), mOutputBuffer.Get());
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mOutputBuffer.Get(),
		D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
	dx->End(com_no);
	dx->RunGpu();
	dx->WaitFence();
}

void DxConvolution::BackPropagation0() {
	ac->SetInErrorResource(mInErrorBuffer.Get());
	ac->BackPropagation();
	CopyResource(mInErrorBuffer.Get(), ac->GetOutErrorResource());

	//zero初期化
	setshaderStep(7);
	dx->Bigin(com_no);
	mCommandList->SetPipelineState(mPSOCom[6].Get());
	mCommandList->SetComputeRootSignature(mRootSignatureCom.Get());
	mCommandList->SetComputeRootUnorderedAccessView(0, mInErrorBuffer2->GetGPUVirtualAddress());
	mCommandList->SetComputeRootConstantBufferView(9, mObjectCB->Resource()->GetGPUVirtualAddress());
	mCommandList->Dispatch((UINT)cb.WidHei.x, (UINT)cb.WidHei.y * FilNum, inputSetNumCur);
	dx->End(com_no);
	dx->RunGpu();
	dx->WaitFence();

	//inErr拡大
	setshaderStep(2);
	dx->Bigin(com_no);
	mCommandList->SetPipelineState(mPSOCom[2].Get());
	SetGPUVirtualAddressExpErr();
	mCommandList->Dispatch(OutWid / shaderThreadNum[4], OutHei * FilNum / shaderThreadNum[5], inputSetNumCur);
	dx->End(com_no);
	dx->RunGpu();
	dx->WaitFence();

	setshaderStep(3);
	dx->Bigin(com_no);
	mCommandList->SetPipelineState(mPSOCom[3].Get());
	SetGPUVirtualAddress();
	mCommandList->Dispatch(Width / shaderThreadNum[6], Height * FilNum / shaderThreadNum[7], inputSetNumCur);
	dx->End(com_no);
	dx->RunGpu();
	dx->WaitFence();

	dx->Bigin(com_no);
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mOutErrorBuffer.Get(),
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE));
	mCommandList->CopyResource(mOutErrorReadBuffer.Get(), mOutErrorBuffer.Get());
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mOutErrorBuffer.Get(),
		D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
	dx->End(com_no);
	dx->RunGpu();
	dx->WaitFence();
}

void DxConvolution::BackPropagationNoWeightUpdate() {
	BackPropagation0();

	setshaderStep(4);
	dx->Bigin(com_no);
	mCommandList->SetPipelineState(mPSOCom[4].Get());
	SetGPUVirtualAddress();
	mCommandList->Dispatch(elNumWid / shaderThreadNum[8], elNumWid * FilNum / shaderThreadNum[9], 1);
	dx->End(com_no);
	dx->RunGpu();
	dx->WaitFence();
}

void DxConvolution::BackPropagation() {
	BackPropagation0();

	setshaderStep(4);
	dx->Bigin(com_no);
	mCommandList->SetPipelineState(mPSOCom[4].Get());
	SetGPUVirtualAddress();
	mCommandList->Dispatch(elNumWid / shaderThreadNum[8], elNumWid * FilNum / shaderThreadNum[9], 1);
	dx->End(com_no);
	dx->RunGpu();
	dx->WaitFence();

	optFil->SetInputGradientBuffer(mGradientBuffer.Get());
	optFil->SetInputWeightBuffer(mFilterBuffer.Get());
	optFil->comOptimizer();
	CopyResource(mFilterBuffer.Get(), optFil->GetOutputWeightBuffer());

	setshaderStep(5);
	dx->Bigin(com_no);
	mCommandList->SetPipelineState(mPSOCom[5].Get());
	SetGPUVirtualAddress();
	mCommandList->Dispatch(FilNum / shaderThreadNum[10], 1, 1);
	dx->End(com_no);
	dx->RunGpu();
	dx->WaitFence();

	optBias->SetInputGradientBuffer(mGradBiasBuffer.Get());
	optBias->SetInputWeightBuffer(mBiasBuffer.Get());
	optBias->comOptimizer();
	CopyResource(mBiasBuffer.Get(), optBias->GetOutputWeightBuffer());

	dx->Bigin(com_no);
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mFilterBuffer.Get(),
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE));
	mCommandList->CopyResource(mFilterReadBuffer.Get(), mFilterBuffer.Get());
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mFilterBuffer.Get(),
		D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));

	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mBiasBuffer.Get(),
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE));
	mCommandList->CopyResource(mBiasReadBuffer.Get(), mBiasBuffer.Get());
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mBiasBuffer.Get(),
		D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
	dx->End(com_no);
	dx->RunGpu();
	dx->WaitFence();
}

void DxConvolution::Query() {
	//TestInput();
	InputResourse();
	SetDropOut();
	inputSetNumCur = inputSetNum;
	ForwardPropagation();
	CopyOutputResourse();
	TextureCopy(mFilterBuffer.Get(), com_no);
	//TestOutput();
}

void DxConvolution::Training() {
	//TestInErr();
	//TestFilter();
	InputErrResourse();
	BackPropagation();
	CopyOutputErrResourse();
	CopyFilterResourse();
	CopyBiasResourse();
	//TestFilter();
	//TestOutErr();
}

void DxConvolution::Detection(UINT inputsetnum) {
	InputResourse();
	inputSetNumCur = inputsetnum;
	ForwardPropagation();
	CopyOutputResourse();
	TextureCopy(mFilterBuffer.Get(), com_no);
}

void DxConvolution::Test() {
	InputResourse();
	SetDropOut();
	inputSetNumCur = 1;
	ForwardPropagation();
	CopyOutputResourse();
}

void DxConvolution::TestFilter() {
	for (UINT i = 0; i < ElNum * FilNum; i++) {
		if (fil[i] > 3000 || fil[i] < -3000) {
			char st0[50];
			sprintf(st0, "fil[%d]異常値です", i);
			MessageBoxA(0, st0, 0, MB_OK);
			char st1[50];
			sprintf(st1, "%f", fil[i]);
			MessageBoxA(0, st1, 0, MB_OK);
			return;
		}
	}
}

void DxConvolution::TestInput() {
	for (UINT i = 0; i < input_outerrOneNum * FilNum; i++) {
		if (input[i] > 3000 || input[i] < -3000) {
			char st0[50];
			sprintf(st0, "input[%d]異常値です", i);
			MessageBoxA(0, st0, 0, MB_OK);
			char st1[50];
			sprintf(st1, "%f", input[i]);
			MessageBoxA(0, st1, 0, MB_OK);
			return;
		}
	}
}

void DxConvolution::TestInErr() {
	for (UINT i = 0; i < output_inerrOneNum * FilNum; i++) {
		if (inputError[i] > 3000 || inputError[i] < -3000) {
			char st0[50];
			sprintf(st0, "inputError[%d]異常値です", i);
			MessageBoxA(0, st0, 0, MB_OK);
			char st1[50];
			sprintf(st1, "%f", inputError[i]);
			MessageBoxA(0, st1, 0, MB_OK);
			return;
		}
	}
}

void DxConvolution::TestOutput() {
	for (UINT i = 0; i < output_inerrOneNum * FilNum; i++) {
		if (output[i] > 3000 || output[i] < -3000) {
			char st0[50];
			sprintf(st0, "output[%d]異常値です", i);
			MessageBoxA(0, st0, 0, MB_OK);
			char st1[50];
			sprintf(st1, "%f", output[i]);
			MessageBoxA(0, st1, 0, MB_OK);
			return;
		}
	}
}

void DxConvolution::TestOutErr() {
	for (UINT i = 0; i < input_outerrOneNum * FilNum; i++) {
		if (outputError[i] > 2000000 || outputError[i] < -2000000) {
			char st0[50];
			sprintf(st0, "outputError[%d]異常値です", i);
			MessageBoxA(0, st0, 0, MB_OK);
			char st1[50];
			sprintf(st1, "%f", outputError[i]);
			MessageBoxA(0, st1, 0, MB_OK);
			return;
		}
	}
}

void DxConvolution::InputResourse() {
	if (!firstIn)return;
	dx->Bigin(com_no);
	SubresourcesUp(input, input_outerrOneNum * FilNum * inputSetNum, mInputBuffer, mInputUpBuffer);
	dx->End(com_no);
	dx->RunGpu();
	dx->WaitFence();
	firstIn = false;
}

void DxConvolution::InputErrResourse() {
	if (!firstInErr)return;
	dx->Bigin(com_no);
	SubresourcesUp(inputError, output_inerrOneNum * FilNum * inputSetNum, mInErrorBuffer, mInErrorUpBuffer);
	dx->End(com_no);
	dx->RunGpu();
	dx->WaitFence();
	firstInErr = false;
}

void DxConvolution::CopyOutputResourse() {
	D3D12_RANGE range;
	range.Begin = 0;
	range.End = output_inerrOneSize * FilNum * inputSetNum;
	float *out = nullptr;
	mOutputReadBuffer->Map(0, &range, reinterpret_cast<void**>(&out));
	memcpy(output, out, output_inerrOneSize * FilNum * inputSetNum);
	mOutputReadBuffer->Unmap(0, nullptr);
}

void DxConvolution::CopyOutputErrResourse() {
	D3D12_RANGE range;
	range.Begin = 0;
	range.End = input_outerrOneSize * FilNum * inputSetNum;
	float *out = nullptr;
	mOutErrorReadBuffer->Map(0, &range, reinterpret_cast<void**>(&out));
	memcpy(outputError, out, input_outerrOneSize * FilNum * inputSetNum);
	mOutErrorReadBuffer->Unmap(0, nullptr);
}

void DxConvolution::CopyFilterResourse() {
	D3D12_RANGE range;
	range.Begin = 0;
	range.End = filSize * FilNum;
	float *fi = nullptr;
	mFilterReadBuffer->Map(0, &range, reinterpret_cast<void**>(&fi));
	memcpy(fil, fi, filSize * FilNum);
	mFilterReadBuffer->Unmap(0, nullptr);
}

void DxConvolution::CopyBiasResourse() {
	D3D12_RANGE range;
	range.Begin = 0;
	range.End = sizeof(float) * FilNum;
	float *bi = nullptr;
	mBiasReadBuffer->Map(0, &range, reinterpret_cast<void**>(&bi));
	memcpy(bias, bi, sizeof(float) * FilNum);
	mBiasReadBuffer->Unmap(0, nullptr);
}

float *DxConvolution::Output(UINT arrNum, UINT inputsetInd) {
	return &output[output_inerrOneNum * FilNum * inputsetInd + arrNum * output_inerrOneNum];
}

float DxConvolution::OutputEl(UINT arrNum, UINT ElNum, UINT inputsetInd) {
	return output[output_inerrOneNum * FilNum * inputsetInd + arrNum * output_inerrOneNum + ElNum];
}

float DxConvolution::OutputFilter(UINT arrNum, UINT elNum) {
	return fil[arrNum * ElNum + elNum];
}

float *DxConvolution::GetError(UINT arrNum, UINT inputsetInd) {
	return &outputError[inputsetInd * input_outerrOneNum * FilNum + arrNum * input_outerrOneNum];
}

float DxConvolution::GetErrorEl(UINT arrNum, UINT ElNum, UINT inputsetInd) {
	return outputError[inputsetInd * input_outerrOneNum * FilNum + arrNum * input_outerrOneNum + ElNum];
}

void DxConvolution::SaveData(UINT Num, char* pass) {
	char pass1[16];
	sprintf_s(pass1, sizeof(char) * 16, pass, Num + 1);
	SaveData(pass1);
}

void DxConvolution::LoadData(UINT Num, char* pass) {
	char pass1[16];
	sprintf_s(pass1, sizeof(char) * 16, pass, Num + 1);
	LoadData(pass1);
}

void DxConvolution::SaveData(char* pass) {
	FILE* fp;
	fp = fopen(pass, "wb");
	float* bifil = new float[ElNum * FilNum + FilNum];

	int Ind = 0;
	for (UINT k = 0; k < FilNum; k++) {
		for (UINT i = 0; i < ElNum; i++) {
			bifil[Ind++] = fil[k * ElNum + i];
		}
		bifil[Ind++] = bias[k];
	}
	size_t size = (ElNum * FilNum + FilNum) * sizeof(float);
	fwrite(bifil, size, 1, fp);
	fclose(fp);
	ARR_DELETE(bifil);
}

void DxConvolution::LoadData(char* pass) {
	FILE* fp;
	fp = fopen(pass, "rb");
	float* bifil = new float[ElNum * FilNum + FilNum];

	int Ind = 0;
	size_t size = (ElNum * FilNum + FilNum) * sizeof(float);
	fread(bifil, size, 1, fp);
	for (UINT k = 0; k < FilNum; k++) {
		for (UINT i = 0; i < ElNum; i++) {
			fil[k * ElNum + i] = bifil[Ind++];
		}
		bias[k] = bifil[Ind++];
	}
	fclose(fp);
	ARR_DELETE(bifil);
	dx->Bigin(com_no);
	SubresourcesUp(fil, ElNum * FilNum, mFilterBuffer, mFilterUpBuffer);
	SubresourcesUp(bias, FilNum, mBiasBuffer, mBiasUpBuffer);
	dx->End(com_no);
	dx->RunGpu();
	dx->WaitFence();
}

void DxConvolution::SetInputResource(ID3D12Resource* res) {
	CopyResource(mInputBuffer.Get(), res);
}

void DxConvolution::SetInErrorResource(ID3D12Resource* res) {
	CopyResource(mInErrorBuffer.Get(), res);
}

ID3D12Resource *DxConvolution::GetOutErrorResource() {
	return mOutErrorBuffer.Get();
}

ID3D12Resource *DxConvolution::GetOutputResource() {
	return mOutputBuffer.Get();
}

ID3D12Resource *DxConvolution::GetFilter() {
	return mFilterBuffer.Get();
}

ID3D12Resource* DxConvolution::GetGradient() {
	return mGradientBuffer.Get();
}

UINT DxConvolution::GetOutWidth() {
	return OutWid;
}

UINT DxConvolution::GetOutHeight() {
	return OutHei;
}