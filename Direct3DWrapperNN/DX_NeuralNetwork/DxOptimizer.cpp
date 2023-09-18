//*****************************************************************************************//
//**                                                                                     **//
//**                   　  　　　DxOptimizer                                             **//
//**                                                                                     **//
//*****************************************************************************************//

#include "DxOptimizer.h"
#include "ShaderNN/ShaderOptimizer.h"
#define PARANUMOP 1

DxOptimizer::DxOptimizer(UINT numNode) {
	NumNode = numNode;
	mObjectCB = new ConstantBuffer<CONSTANT_BUFFER_Optimizer>(1);

	cb.Lear_b1_b2_eps.as(0.001f, 0.9f, 0.999f, 0.00000001f);
	mObjectCB->CopyData(0, cb);
}

DxOptimizer::~DxOptimizer() {
	S_DELETE(mObjectCB);
	ARR_DELETE(shaderThreadNum);
}

void DxOptimizer::ComCreate(OptimizerName name) {
	//RWStructuredBuffer用gGradient
	CreateResourceDef(mGradientBuffer, sizeof(float) * NumNode);
	//RWStructuredBuffer用gGradientMovAve1
	CreateResourceDef(mGradientMovAve1Buffer, sizeof(float) * NumNode);
	//RWStructuredBuffer用gGradientMovAve2
	CreateResourceDef(mGradientMovAve2Buffer, sizeof(float) * NumNode);
	//RWStructuredBuffer用gWeight
	CreateResourceDef(mWeightBuffer, sizeof(float) * NumNode);
	//up用gGradientMovAve1
	CreateResourceUp(mGradientMovAve1UPBuffer, sizeof(float) * NumNode);
	//up用gGradientMovAve2
	CreateResourceUp(mGradientMovAve2UPBuffer, sizeof(float) * NumNode);

	//ルートシグネチャ
	CD3DX12_ROOT_PARAMETER slotRootParameter[5];
	for (int i = 0; i < 4; i++)
		slotRootParameter[i].InitAsUnorderedAccessView(i);//RWStructuredBuffer(ui)
	slotRootParameter[4].InitAsConstantBufferView(0);//mObjectCB(b0)
	mRootSignatureCom = CreateRsCompute(5, slotRootParameter);

	UINT tmpNum[PARANUMOP];
	tmpNum[0] = NumNode;

	char** replaceString = nullptr;
	CreateReplaceArr(&shaderThreadNum, &replaceString, PARANUMOP, tmpNum);
	char* repsh = nullptr;
	ReplaceString(&repsh, ShaderOptimizer, '?', replaceString);
	for (int i = 0; i < PARANUMOP; i++)ARR_DELETE(replaceString[i]);
	ARR_DELETE(replaceString);

	switch (name) {
	case SGD:
		pCS[0] = Dx_ShaderHolder::CompileShader(repsh, strlen(repsh), "SGDCS", "cs_5_0");
		break;
	case ADAM:
		pCS[0] = Dx_ShaderHolder::CompileShader(repsh, strlen(repsh), "AdamCS", "cs_5_0");
		break;
	}
	ARR_DELETE(repsh);
	for (int i = 0; i < OP_SHADER_NUM; i++)
		mPSOCom[i] = CreatePsoCompute(pCS[i].Get(), mRootSignatureCom.Get());

	float* initGr = new float[NumNode];
	ZeroMemory(initGr, sizeof(float) * NumNode);
	SubresourcesUp(initGr, NumNode, mGradientMovAve1Buffer, mGradientMovAve1UPBuffer);
	SubresourcesUp(initGr, NumNode, mGradientMovAve2Buffer, mGradientMovAve2UPBuffer);
	ARR_DELETE(initGr);
}

void DxOptimizer::setOptimizerParameter(float LearningRate, float AttenuationRate1,
	float AttenuationRate2, float DivergencePrevention) {
	cb.Lear_b1_b2_eps.as(LearningRate, AttenuationRate1, AttenuationRate2, DivergencePrevention);
	mObjectCB->CopyData(0, cb);
}

void DxOptimizer::SetInputGradientBuffer(ID3D12Resource* res) {
	CopyResource(mGradientBuffer.Get(), res);
}

void DxOptimizer::SetInputWeightBuffer(ID3D12Resource* res) {
	CopyResource(mWeightBuffer.Get(), res);
}

ID3D12Resource* DxOptimizer::GetOutputWeightBuffer() {
	return mWeightBuffer.Get();
}

void DxOptimizer::comOptimizer() {
	d->Bigin();
	CList->SetPipelineState(mPSOCom[0].Get());
	CList->SetComputeRootSignature(mRootSignatureCom.Get());
	CList->SetComputeRootUnorderedAccessView(0, mGradientBuffer->GetGPUVirtualAddress());
	CList->SetComputeRootUnorderedAccessView(1, mGradientMovAve1Buffer->GetGPUVirtualAddress());
	CList->SetComputeRootUnorderedAccessView(2, mGradientMovAve2Buffer->GetGPUVirtualAddress());
	CList->SetComputeRootUnorderedAccessView(3, mWeightBuffer->GetGPUVirtualAddress());
	CList->SetComputeRootConstantBufferView(4, mObjectCB->Resource()->GetGPUVirtualAddress());
	CList->Dispatch(NumNode / shaderThreadNum[0], 1, 1);
	d->End();
	cMa->RunGpu();
	cMa->WaitFence();
}