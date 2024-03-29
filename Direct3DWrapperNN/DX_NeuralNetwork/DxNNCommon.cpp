//*****************************************************************************************//
//**                                                                                     **//
//**                   　  　　　 DxNNCommon                                             **//
//**                                                                                     **//
//*****************************************************************************************//

#include "DxNNCommon.h"
#include "ShaderNN\ShaderNNCommon.h"
#include "ShaderNN\ShaderNNCommonCopy.h"

D3D12_ROOT_PARAMETER DxNNCommon::setSlotRootParameter(
	UINT ShaderRegister,
	D3D12_ROOT_PARAMETER_TYPE type,
	D3D12_DESCRIPTOR_RANGE* uavTable,
	UINT NumDescriptorRanges) {

	D3D12_ROOT_PARAMETER slotRootParameter = {};
	slotRootParameter.ParameterType = type;
	if (uavTable) {
		slotRootParameter.DescriptorTable.NumDescriptorRanges = NumDescriptorRanges;
		slotRootParameter.DescriptorTable.pDescriptorRanges = uavTable;
	}
	else {
		slotRootParameter.Descriptor.ShaderRegister = ShaderRegister;
		slotRootParameter.Descriptor.RegisterSpace = 0;
	}
	slotRootParameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	return slotRootParameter;
}

DxNNCommon::DxNNCommon() {

	cMa = Dx_CommandManager::GetInstance();
	d = cMa->getGraphicsComListObj(0);
	CList = d->getCommandList();

	D3D12_ROOT_PARAMETER slotRootParameter[3];
	slotRootParameter[0] = setSlotRootParameter(0);//RWStructuredBuffer(u0)
	slotRootParameter[1] = setSlotRootParameter(1);//RWStructuredBuffer(u1)
	slotRootParameter[2] = setSlotRootParameter(0, D3D12_ROOT_PARAMETER_TYPE_CBV);//mObjectCB(b0)
	mRootSignatureCom2Copy = CreateRsCompute(3, slotRootParameter);

	pCS2Copy[0] = Dx_ShaderHolder::CompileShader(ShaderNNCommonCopy, strlen(ShaderNNCommonCopy), "ResourceExpansionCS", "cs_5_0");
	pCS2Copy[1] = Dx_ShaderHolder::CompileShader(ShaderNNCommonCopy, strlen(ShaderNNCommonCopy), "ResourceReductionCS", "cs_5_0");

	for (int i = 0; i < Copy_SHADER_NUM; i++)
		mPSOCom2Copy[i] = CreatePsoCompute(pCS2Copy[i].Get(), mRootSignatureCom2Copy.Get());

	mObjectCB2Copy = new ConstantBuffer<CBResourceCopy>(1);
}

void DxNNCommon::CreareNNTexture(UINT width, UINT height, UINT num) {
	texWid = width;
	texHei = height * num;

	D3D12_DESCRIPTOR_HEAP_DESC uavHeapDesc = {};
	uavHeapDesc.NumDescriptors = 1;
	uavHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	uavHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	Dx_Device* device = Dx_Device::GetInstance();
	device->getDevice()->CreateDescriptorHeap(&uavHeapDesc, IID_PPV_ARGS(&mUavHeap2));

	D3D12_RESOURCE_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Alignment = 0;
	texDesc.Width = texWid;
	texDesc.Height = texHei;
	texDesc.DepthOrArraySize = 1;
	texDesc.MipLevels = 1;
	texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	//RWTexture2D用
	device->getDevice()->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(&mTextureBuffer));

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	uavDesc.Texture2D.MipSlice = 0;

	CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(mUavHeap2->GetCPUDescriptorHandleForHeapStart());
	device->getDevice()->CreateUnorderedAccessView(mTextureBuffer.Get(), nullptr, &uavDesc, hDescriptor);

	CList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mTextureBuffer.Get(),
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));

	//ルートシグネチャ
	CD3DX12_DESCRIPTOR_RANGE uavTable;
	uavTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 1);

	CD3DX12_ROOT_PARAMETER slotRootParameter[3];
	slotRootParameter[0].InitAsUnorderedAccessView(0);//RWStructuredBuffer(u0)
	slotRootParameter[1].InitAsDescriptorTable(1, &uavTable);//RWTexture2D(u1)
	slotRootParameter[2].InitAsConstantBufferView(0);//mObjectCB2(b0)
	mRootSignatureCom2 = CreateRsCompute(3, slotRootParameter);

	UINT tmpNum[2];
	tmpNum[0] = texWid;
	tmpNum[1] = texHei;
	char** replaceString = nullptr;

	CreateReplaceArr(&shaderThreadNum2, &replaceString, 2, tmpNum);

	char* repsh = nullptr;
	ReplaceString(&repsh, ShaderNeuralNetworkTexture, '?', replaceString);
	for (int i = 0; i < 2; i++)ARR_DELETE(replaceString[i]);
	ARR_DELETE(replaceString);

	pCS2 = Dx_ShaderHolder::CompileShader(repsh, strlen(repsh), "NNTexCopyCS", "cs_5_0");
	ARR_DELETE(repsh);
	//PSO
	mPSOCom2 = CreatePsoCompute(pCS2.Get(), mRootSignatureCom2.Get());

	mObjectCB2 = new ConstantBuffer<NNCBTexture>(1);
	cb2.Wid_Hei.as((float)texWid, (float)texHei, 0.0f, 0.0f);
	mObjectCB2->CopyData(0, cb2);
	created = true;
}

void DxNNCommon::TextureCopy(ID3D12Resource* texture, int comNo) {
	if (!created)return;

	d->Bigin();
	CList->SetPipelineState(mPSOCom2.Get());

	ID3D12DescriptorHeap* descriptorHeaps[] = { mUavHeap2.Get() };
	CList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	CList->SetComputeRootSignature(mRootSignatureCom2.Get());

	CList->SetComputeRootUnorderedAccessView(0, texture->GetGPUVirtualAddress());
	CD3DX12_GPU_DESCRIPTOR_HANDLE uav(mUavHeap2->GetGPUDescriptorHandleForHeapStart());
	CList->SetComputeRootDescriptorTable(1, uav);
	CList->SetComputeRootConstantBufferView(2, mObjectCB2->Resource()->GetGPUVirtualAddress());
	CList->Dispatch(texWid / shaderThreadNum2[0], texHei / shaderThreadNum2[1], 1);
	d->End();
	cMa->RunGpu();
	cMa->WaitFence();
}

void DxNNCommon::CreateResourceDef(Microsoft::WRL::ComPtr<ID3D12Resource>& def, UINT64 size) {
	Dx_Device::GetInstance()->getDevice()->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(size, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS),
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		nullptr,
		IID_PPV_ARGS(&def));
}

void DxNNCommon::CreateResourceUp(Microsoft::WRL::ComPtr<ID3D12Resource>& up, UINT64 size) {
	Dx_Device::GetInstance()->getDevice()->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(size),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&up));
}

void DxNNCommon::CreateResourceRead(Microsoft::WRL::ComPtr<ID3D12Resource>& re, UINT64 size) {
	Dx_Device::GetInstance()->getDevice()->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(size),
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&re));
}

void DxNNCommon::SubresourcesUp(void* pData, UINT num, Microsoft::WRL::ComPtr<ID3D12Resource>& def,
	Microsoft::WRL::ComPtr<ID3D12Resource>& up)
{
	D3D12_SUBRESOURCE_DATA subResourceData = {};
	subResourceData.pData = pData;
	subResourceData.RowPitch = num;
	subResourceData.SlicePitch = subResourceData.RowPitch;

	CList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(def.Get(),
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_DEST));
	UpdateSubresources(CList, def.Get(), up.Get(), 0, 0, 1, &subResourceData);
	CList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(def.Get(),
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
}

ID3D12Resource *DxNNCommon::GetNNTextureResource() {
	return mTextureBuffer.Get();
}

D3D12_RESOURCE_STATES DxNNCommon::GetNNTextureResourceStates() {
	return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
}

DxNNCommon::~DxNNCommon() {
	S_DELETE(mObjectCB2);
	S_DELETE(mObjectCB2Copy);
	ARR_DELETE(shaderThreadNum2);
}

#define PLACEHOLDERNUM 3
void DxNNCommon::CreateReplaceArr(int **shaderThreadNum, char ***replaceArr, UINT arrNum, UINT *srcNumArr) {

	int *shaderthreadnum = new int[arrNum];
	char **replacestring = new char*[arrNum];
	for (UINT i = 0; i < arrNum; i++)replacestring[i] = new char[PLACEHOLDERNUM + 1];

	for (UINT k = 0; k < arrNum; k++) {
		for (UINT i = maxThreadNum; i > 0; i--) {
			if (srcNumArr[k] % i == 0) {
				snprintf(replacestring[k], PLACEHOLDERNUM + 1, "%d", i);
				size_t len = strlen(replacestring[k]);
				for (size_t ln = len; ln < PLACEHOLDERNUM; ln++) {
					replacestring[k][ln] = ' ';//余った要素は空白で埋める
				}
				replacestring[k][PLACEHOLDERNUM] = '\0';
				shaderthreadnum[k] = i;
				break;
			}
		}
	}
	*shaderThreadNum = shaderthreadnum;
	*replaceArr = replacestring;
}

void DxNNCommon::ReplaceString(char **destination, char *source, char placeholderStartPoint, char **replaceArr) {

	bool search = false;
	int repInd = 0;
	int strInd = 0;

	char *sou = source;
	UINT strCnt = 0;
	while (*sou != '\0') {
		strCnt++;
		sou++;
	}
	char *dst = new char[strCnt + 1];
	strCnt = 0;

	do {
		if (source[strCnt] == placeholderStartPoint)search = true;
		if (search) {
			if (replaceArr[repInd][strInd] != '\0') {
				dst[strCnt] = replaceArr[repInd][strInd++];
			}
			else {
				search = false;
				repInd++;
				strInd = 0;
				dst[strCnt] = source[strCnt];
			}
		}
		else {
			dst[strCnt] = source[strCnt];
		}
	} while (source[++strCnt] != '\0');
	dst[strCnt] = '\0';
	*destination = dst;
}

void DxNNCommon::SetMaxThreadNum(UINT num) {
	maxThreadNum = num;
	if (maxThreadNum > 32)maxThreadNum = 32;
}

void DxNNCommon::CopyResource(ID3D12Resource* dest, ID3D12Resource* src) {

	UINT destNumNode = (UINT)(dest->GetDesc().Width * dest->GetDesc().Height / sizeof(float));
	UINT srcNumNode = (UINT)(src->GetDesc().Width * src->GetDesc().Height / sizeof(float));

	if (destNumNode == srcNumNode) {
		d->Bigin();
		CList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(dest,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_DEST));
		CList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(src,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE));

		CList->CopyResource(dest, src);

		CList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(src,
			D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
		CList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(dest,
			D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
		d->End();
		cMa->RunGpu();
		cMa->WaitFence();
		return;
	}

	UINT index = 0;
	if (destNumNode > srcNumNode) {
		index = 0;
		cb2Copy.NumNode = srcNumNode / inputSetNum;
	}
	else {
		index = 1;
		cb2Copy.NumNode = destNumNode / inputSetNum;
	}
	mObjectCB2Copy->CopyData(0, cb2Copy);

	d->Bigin();
	CList->SetPipelineState(mPSOCom2Copy[index].Get());
	CList->SetComputeRootSignature(mRootSignatureCom2Copy.Get());
	CList->SetComputeRootUnorderedAccessView(0, src->GetGPUVirtualAddress());
	CList->SetComputeRootUnorderedAccessView(1, dest->GetGPUVirtualAddress());
	CList->SetComputeRootConstantBufferView(2, mObjectCB2Copy->Resource()->GetGPUVirtualAddress());
	CList->Dispatch(cb2Copy.NumNode, 1, inputSetNum);
	d->End();
	cMa->RunGpu();
	cMa->WaitFence();
}