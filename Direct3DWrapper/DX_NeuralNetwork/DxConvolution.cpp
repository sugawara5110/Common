//*****************************************************************************************//
//**                                                                                     **//
//**                              DxConvolution                                          **//
//**                                                                                     **//
//*****************************************************************************************//

#include "DxConvolution.h"
#include <random>
#include "ShaderNN\ShaderConvolution.h"

void DxConvolution::SetLearningLate(float rate, float biasrate) {
	learningRate = rate;
	learningBiasRate = biasrate;
}

DxConvolution::DxConvolution(UINT width, UINT height, UINT filNum, UINT inputsetnum, UINT elnumwid, UINT filstep) {

	srand((unsigned)time(NULL));
	inputSetNum = inputsetnum;
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
	OutWid = Width / filterStep;
	OutHei = Height / filterStep;
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

	dropout = new float[FilNum * input_outerrOneNum];
	for (UINT i = 0; i < FilNum * input_outerrOneNum; i++)dropout[i] = 1.0f;

	bias = new float[FilNum];
	ZeroMemory(bias, sizeof(float) * FilNum);

	SetWeightInit(0.2f);

	dx = Dx12Process::GetInstance();
	mCommandList = dx->dx_sub[0].mCommandList.Get();
	mObjectCB = new ConstantBuffer<CONSTANT_BUFFER_Convolution>(1);
	cb.WidHei.x = Width;
	cb.WidHei.y = Height;
	cb.WidHei.z = FilNum;
	cb.filWid_filStep.x = elNumWid;
	cb.filWid_filStep.y = filterStep;
	cb.Lear_inputS.y = inputSetNum;
	mObjectCB->CopyData(0, cb);
}

void DxConvolution::SetDropOut() {

	dx->Bigin(com_no);
	for (UINT i = 0; i < FilNum * input_outerrOneNum; i++) {
		dropout[i] = 1.0f;
		int rnd = (rand() % 100) + 1;
		if ((int)(dropThreshold * 100.0f) >= rnd) {
			dropout[i] = 0.0f;
		}
	}
	SubresourcesUp(dropout, FilNum * input_outerrOneNum, mDropOutFBuffer, mDropOutFUpBuffer);
	dx->End(com_no);
	dx->WaitFenceCurrent();
}

void DxConvolution::SetdropThreshold(float Threshold) {
	dropThreshold = Threshold;
}

void DxConvolution::SetWeightInit(float rate) {
	std::random_device seed_gen;
	std::default_random_engine engine(seed_gen());
	//フィルタ初期値
	std::normal_distribution<> dist(0.0, rate);
	for (UINT i = 0; i < FilNum * ElNum; i++)
		fil[i] = dist(engine);
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
}

void DxConvolution::SetCommandList(int no) {
	com_no = no;
	mCommandList = dx->dx_sub[com_no].mCommandList.Get();
}

void DxConvolution::ComCreate(bool sigon) {
	//RWStructuredBuffer用gInput
	CreateResourceDef(mInputBuffer, input_outerrOneSize * FilNum * inputSetNum);

	//RWStructuredBuffer用gDropout
	CreateResourceDef(mDropOutFBuffer, input_outerrOneSize * FilNum);
	//Up
	CreateResourceUp(mDropOutFUpBuffer, input_outerrOneSize * FilNum);
	//初期値コピー
	SubresourcesUp(dropout, input_outerrOneNum * FilNum, mDropOutFBuffer, mDropOutFUpBuffer);

	//RWStructuredBuffer用gOutput
	CreateResourceDef(mOutputBuffer, output_inerrOneSize * FilNum * inputSetNum);

	//RWStructuredBuffer用gInErr
	CreateResourceDef(mInErrorBuffer, output_inerrOneSize * FilNum * inputSetNum);

	//RWStructuredBuffer用gOutErr
	CreateResourceDef(mOutErrorBuffer, input_outerrOneSize * FilNum * inputSetNum);

	//RWStructuredBuffer用gFilter
	CreateResourceDef(mFilterBuffer, filSize * FilNum);

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
	//Up
	CreateResourceUp(mBiasUpBuffer, sizeof(float) * FilNum);
	//Read
	CreateResourceRead(mBiasReadBuffer, sizeof(float) * FilNum);

	SubresourcesUp(bias, FilNum, mBiasBuffer, mBiasUpBuffer);

	//ルートシグネチャ
	CD3DX12_ROOT_PARAMETER slotRootParameter[8];
	slotRootParameter[0].InitAsUnorderedAccessView(0);//RWStructuredBuffer(u0)
	slotRootParameter[1].InitAsUnorderedAccessView(1);//RWStructuredBuffer(u1)
	slotRootParameter[2].InitAsUnorderedAccessView(2);//RWStructuredBuffer(u2)
	slotRootParameter[3].InitAsUnorderedAccessView(3);//RWStructuredBuffer(u3)
	slotRootParameter[4].InitAsUnorderedAccessView(4);//RWStructuredBuffer(u4)
	slotRootParameter[5].InitAsUnorderedAccessView(5);//RWStructuredBuffer(u5)
	slotRootParameter[6].InitAsUnorderedAccessView(6);//RWStructuredBuffer(u6)
	slotRootParameter[7].InitAsConstantBufferView(0);//mObjectCB(b0)
	mRootSignatureCom = CreateRsCompute(8, slotRootParameter);

	if (sigon) {
		pCS[0] = CompileShader(ShaderConvolution, strlen(ShaderConvolution), "CNFPCS", "cs_5_0");
		pCS[3] = CompileShader(ShaderConvolution, strlen(ShaderConvolution), "CNBPCS2", "cs_5_0");
	}
	else {
		pCS[0] = CompileShader(ShaderConvolution, strlen(ShaderConvolution), "CNFPReLUCS", "cs_5_0");
		pCS[3] = CompileShader(ShaderConvolution, strlen(ShaderConvolution), "CNBPReLUCS2", "cs_5_0");
	}
	pCS[1] = CompileShader(ShaderConvolution, strlen(ShaderConvolution), "CNBPCS0", "cs_5_0");
	pCS[2] = CompileShader(ShaderConvolution, strlen(ShaderConvolution), "CNBPCS1", "cs_5_0");
	pCS[4] = CompileShader(ShaderConvolution, strlen(ShaderConvolution), "CNBPCSBias", "cs_5_0");

	for (int i = 0; i < CN_SHADER_NUM; i++)
		mPSOCom[i] = CreatePsoCompute(pCS[i].Get(), mRootSignatureCom.Get());
}

void DxConvolution::ComCreateSigmoid() {
	ComCreate(true);
}

void DxConvolution::ComCreateReLU() {
	ComCreate(false);
}

void DxConvolution::FirstInput(float el, UINT ElNum, UINT inputsetInd) {
	for (UINT i = 0; i < FilNum; i++)InputEl(el - 0.5f, i, ElNum, inputsetInd);
	firstIn = true;
}

void DxConvolution::InputEl(float el, UINT arrNum, UINT ElNum, UINT inputsetInd) {
	input[input_outerrOneNum * FilNum * inputsetInd + arrNum * input_outerrOneNum + ElNum] = el;
}

void DxConvolution::Input(float *inArr, UINT arrNum, UINT inputsetInd) {
	memcpy(&input[input_outerrOneNum * FilNum * inputsetInd + arrNum * input_outerrOneNum], inArr, input_outerrOneSize);
}

void DxConvolution::InputError(float *inArr, UINT arrNum, UINT inputsetInd) {
	memcpy(&inputError[inputsetInd * output_inerrOneNum * FilNum + arrNum * output_inerrOneNum], inArr, output_inerrOneSize);
}

void DxConvolution::ForwardPropagation(UINT inputsetnum) {
	cb.Lear_inputS.x = learningRate;
	cb.Lear_inputS.z = learningBiasRate;
	mObjectCB->CopyData(0, cb);
	dx->Bigin(com_no);
	mCommandList->SetPipelineState(mPSOCom[0].Get());
	mCommandList->SetComputeRootSignature(mRootSignatureCom.Get());
	mCommandList->SetComputeRootUnorderedAccessView(0, mInputBuffer->GetGPUVirtualAddress());
	mCommandList->SetComputeRootUnorderedAccessView(1, mOutputBuffer->GetGPUVirtualAddress());
	mCommandList->SetComputeRootUnorderedAccessView(2, mInErrorBuffer->GetGPUVirtualAddress());
	mCommandList->SetComputeRootUnorderedAccessView(3, mOutErrorBuffer->GetGPUVirtualAddress());
	mCommandList->SetComputeRootUnorderedAccessView(4, mFilterBuffer->GetGPUVirtualAddress());
	mCommandList->SetComputeRootUnorderedAccessView(5, mDropOutFBuffer->GetGPUVirtualAddress());
	mCommandList->SetComputeRootUnorderedAccessView(6, mBiasBuffer->GetGPUVirtualAddress());
	mCommandList->SetComputeRootConstantBufferView(7, mObjectCB->Resource()->GetGPUVirtualAddress());
	mCommandList->Dispatch(OutWid, OutHei * FilNum, inputsetnum);
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

void DxConvolution::BackPropagation() {
	dx->Bigin(com_no);
	mCommandList->SetPipelineState(mPSOCom[1].Get());
	mCommandList->SetComputeRootSignature(mRootSignatureCom.Get());
	mCommandList->SetComputeRootUnorderedAccessView(0, mInputBuffer->GetGPUVirtualAddress());
	mCommandList->SetComputeRootUnorderedAccessView(1, mOutputBuffer->GetGPUVirtualAddress());
	mCommandList->SetComputeRootUnorderedAccessView(2, mInErrorBuffer->GetGPUVirtualAddress());
	mCommandList->SetComputeRootUnorderedAccessView(3, mOutErrorBuffer->GetGPUVirtualAddress());
	mCommandList->SetComputeRootUnorderedAccessView(4, mFilterBuffer->GetGPUVirtualAddress());
	mCommandList->SetComputeRootConstantBufferView(7, mObjectCB->Resource()->GetGPUVirtualAddress());
	mCommandList->Dispatch(Width, Height * FilNum, inputSetNum);
	dx->End(com_no);
	dx->WaitFenceCurrent();

	dx->Bigin(com_no);
	mCommandList->SetPipelineState(mPSOCom[2].Get());
	mCommandList->SetComputeRootSignature(mRootSignatureCom.Get());
	mCommandList->SetComputeRootUnorderedAccessView(0, mInputBuffer->GetGPUVirtualAddress());
	mCommandList->SetComputeRootUnorderedAccessView(1, mOutputBuffer->GetGPUVirtualAddress());
	mCommandList->SetComputeRootUnorderedAccessView(2, mInErrorBuffer->GetGPUVirtualAddress());
	mCommandList->SetComputeRootUnorderedAccessView(3, mOutErrorBuffer->GetGPUVirtualAddress());
	mCommandList->SetComputeRootUnorderedAccessView(4, mFilterBuffer->GetGPUVirtualAddress());
	mCommandList->SetComputeRootUnorderedAccessView(5, mDropOutFBuffer->GetGPUVirtualAddress());
	mCommandList->SetComputeRootConstantBufferView(7, mObjectCB->Resource()->GetGPUVirtualAddress());
	mCommandList->Dispatch(OutWid, OutHei * FilNum, inputSetNum);
	dx->End(com_no);
	dx->WaitFenceCurrent();

	dx->Bigin(com_no);
	mCommandList->SetPipelineState(mPSOCom[3].Get());
	mCommandList->SetComputeRootSignature(mRootSignatureCom.Get());
	mCommandList->SetComputeRootUnorderedAccessView(0, mInputBuffer->GetGPUVirtualAddress());
	mCommandList->SetComputeRootUnorderedAccessView(1, mOutputBuffer->GetGPUVirtualAddress());
	mCommandList->SetComputeRootUnorderedAccessView(2, mInErrorBuffer->GetGPUVirtualAddress());
	mCommandList->SetComputeRootUnorderedAccessView(3, mOutErrorBuffer->GetGPUVirtualAddress());
	mCommandList->SetComputeRootUnorderedAccessView(4, mFilterBuffer->GetGPUVirtualAddress());
	mCommandList->SetComputeRootConstantBufferView(7, mObjectCB->Resource()->GetGPUVirtualAddress());
	mCommandList->Dispatch(elNumWid, elNumWid * FilNum, 1);
	dx->End(com_no);
	dx->WaitFenceCurrent();

	dx->Bigin(com_no);
	mCommandList->SetPipelineState(mPSOCom[4].Get());
	mCommandList->SetComputeRootSignature(mRootSignatureCom.Get());
	mCommandList->SetComputeRootUnorderedAccessView(2, mInErrorBuffer->GetGPUVirtualAddress());
	mCommandList->SetComputeRootUnorderedAccessView(6, mBiasBuffer->GetGPUVirtualAddress());
	mCommandList->SetComputeRootConstantBufferView(7, mObjectCB->Resource()->GetGPUVirtualAddress());
	mCommandList->Dispatch(FilNum, 1, 1);
	dx->End(com_no);
	dx->WaitFenceCurrent();

	dx->Bigin(com_no);
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mOutErrorBuffer.Get(),
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE));
	mCommandList->CopyResource(mOutErrorReadBuffer.Get(), mOutErrorBuffer.Get());
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mOutErrorBuffer.Get(),
		D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));

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
	dx->WaitFenceCurrent();
}

void DxConvolution::Query() {
	//TestInput();
	InputResourse();
	SetDropOut();
	ForwardPropagation(inputSetNum);
	//CopyOutputResourse();
	TextureCopy(mFilterBuffer.Get(), com_no);
	//TestOutput();
}

void DxConvolution::Training() {
	//TestInErr();
	//TestFilter();
	//InputErrResourse();//直接リソースをコピーの場合使用しない(カラの配列がコピーされてしまう)
	BackPropagation();
	//CopyOutputErrResourse();
	CopyFilterResourse();
	CopyBiasResourse();
	//TestFilter();
	//TestOutErr();
}

void DxConvolution::Detection(UINT inputsetnum) {
	InputResourse();
	ForwardPropagation(inputsetnum);
	TextureCopy(mFilterBuffer.Get(), com_no);
}

void DxConvolution::Test() {
	InputResourse();
	SetDropOut();
	ForwardPropagation(1);
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
	dx->WaitFenceCurrent();
	firstIn = false;
}

void DxConvolution::InputErrResourse() {
	dx->Bigin(com_no);
	SubresourcesUp(inputError, output_inerrOneNum * FilNum * inputSetNum, mInErrorBuffer, mInErrorUpBuffer);
	dx->End(com_no);
	dx->WaitFenceCurrent();
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

void DxConvolution::SaveData(UINT Num) {
	FILE *fp;
	char pass[16];

	sprintf_s(pass, sizeof(char) * 16, "save/save%d.da", Num + 1);
	fp = fopen(pass, "wb");

	float *bifil = new float[ElNum * FilNum + FilNum];

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

void DxConvolution::LoadData(UINT Num) {
	FILE *fp;
	char pass[16];

	sprintf_s(pass, sizeof(char) * 16, "save/save%d.da", Num + 1);
	fp = fopen(pass, "rb");

	float *bifil = new float[ElNum * FilNum + FilNum];

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
	dx->WaitFenceCurrent();
}

void DxConvolution::SetInputResource(ID3D12Resource *res) {
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

void DxConvolution::SetInErrorResource(ID3D12Resource *res) {
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

ID3D12Resource *DxConvolution::GetOutErrorResource() {
	return mOutErrorBuffer.Get();
}

ID3D12Resource *DxConvolution::GetOutputResource() {
	return mOutputBuffer.Get();
}

ID3D12Resource *DxConvolution::GetFilter() {
	return mFilterBuffer.Get();
}

UINT DxConvolution::GetOutWidth() {
	return OutWid;
}

UINT DxConvolution::GetOutHeight() {
	return OutHei;
}