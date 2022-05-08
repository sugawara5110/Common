//*****************************************************************************************//
//**                                                                                     **//
//**                                 DxSkinnedCom                                        **//
//**                                                                                     **//
//*****************************************************************************************//

#include "DxSkinnedCom.h"

void SkinnedCom::getBuffer(BasicPolygon* p) {
	Dx12Process* dx = Dx12Process::GetInstance();
	if (dx->DXR_CreateResource) {
		pd = p;
	}
}

bool SkinnedCom::createDescHeap(D3D12_GPU_VIRTUAL_ADDRESS ad3, UINT ad3Size) {
	Dx12Process* dx = Dx12Process::GetInstance();

	if (dx->DXR_CreateResource && pd->vs == dx->shaderH->pVertexShader_SKIN.Get()) {

		Dx_Device* device = Dx_Device::GetInstance();
		const int numDesc = numSrv + numCbv + numUav;
		descHeap = device->CreateDescHeap(numDesc);
		if (descHeap == nullptr)return false;

		D3D12_CPU_DESCRIPTOR_HANDLE hDescriptor = descHeap->GetCPUDescriptorHandleForHeapStart();

		UINT cbSize[1] = {};
		cbSize[0] = ad3Size;

		ID3D12Resource* res[1] = {};
		res[0] = pd->dpara.Vview.get()->VertexBufferGPU.Get();
		UINT resSize[1] = {};
		resSize[0] = sizeof(MY_VERTEX_S);

		device->CreateSrvBuffer(hDescriptor, res, numSrv, resSize);
		D3D12_GPU_VIRTUAL_ADDRESS ad[2] = {};
		ad[0] = ad3;
		device->CreateCbv(hDescriptor, ad, cbSize, numCbv);
		ID3D12Resource* resU[1] = {};
		resU[0] = SkinnedVer.Get();
		UINT byteStride[1] = {};
		byteStride[0] = sizeof(VERTEX_DXR);
		UINT size[1] = {};
		size[0] = pd->dpara.Vview.get()->VertexBufferByteSize / sizeof(MY_VERTEX_S);
		device->CreateUavBuffer(hDescriptor, resU, byteStride, size, numUav);
	}
	return true;
}

bool SkinnedCom::createPSO() {
	Dx12Process* dx = Dx12Process::GetInstance();

	if (dx->DXR_CreateResource && pd->vs == dx->shaderH->pVertexShader_SKIN.Get()) {
		rootSignature = pd->CreateRootSignatureCompute(numSrv, numCbv, numUav, 0, 0, 0, nullptr);
		if (rootSignature == nullptr)return false;

		ID3DBlob* cs = dx->shaderH->pVertexShader_SKIN_Com.Get();

		PSO = pd->CreatePsoCompute(cs, rootSignature.Get());
		if (PSO == nullptr)return false;
	}
	return true;
}

bool SkinnedCom::createParameterDXR() {
	Dx12Process* dx = Dx12Process::GetInstance();

	if (dx->DXR_CreateResource && pd->vs == dx->shaderH->pVertexShader_SKIN.Get()) {

		int NumMaterial = pd->dxrPara.NumMaterial;
		Dx_Device* device = Dx_Device::GetInstance();

		for (int i = 0; i < NumMaterial; i++) {
			if (pd->dpara.Iview[i].IndexCount <= 0)continue;
			UINT bytesize = 0;
			IndexView& dxI = pd->dxrPara.IviewDXR[i];
			UINT indCnt = pd->dpara.Iview[i].IndexCount;
			bytesize = indCnt * sizeof(UINT);
			dxI.IndexFormat = DXGI_FORMAT_R32_UINT;
			dxI.IndexBufferByteSize = bytesize;
			dxI.IndexCount = indCnt;
			if (FAILED(device->createDefaultResourceBuffer(dxI.IndexBufferGPU.GetAddressOf(),
				dxI.IndexBufferByteSize, D3D12_RESOURCE_STATE_GENERIC_READ))) {
				Dx_Util::ErrorMessage("SkinnedCom::createParameterDXR Error!!");
				return false;
			}

			dx->dx_sub[pd->com_no].CopyResourceGENERIC_READ(dxI.IndexBufferGPU.Get(),
				pd->dpara.Iview[i].IndexBufferGPU.Get());

			for (int j = 0; j < 2; j++) {
				pd->dxrPara.updateDXR[j].currentIndexCount[i][0] = indCnt;
				VertexView& dxV = pd->dxrPara.updateDXR[j].VviewDXR[i][0];
				UINT vCnt = pd->dpara.Vview.get()->VertexBufferByteSize / sizeof(MY_VERTEX_S);
				bytesize = vCnt * sizeof(VERTEX_DXR);
				dxV.VertexByteStride = sizeof(VERTEX_DXR);
				dxV.VertexBufferByteSize = bytesize;
				if (FAILED(device->createDefaultResourceBuffer(dxV.VertexBufferGPU.GetAddressOf(),
					dxV.VertexBufferByteSize, D3D12_RESOURCE_STATE_GENERIC_READ))) {
					Dx_Util::ErrorMessage("SkinnedCom::createParameterDXR Error!!");
					return false;
				}
			}
			if (!SkinnedVer) {
				if (FAILED(device->createDefaultResourceBuffer_UNORDERED_ACCESS(SkinnedVer.GetAddressOf(),
					bytesize, D3D12_RESOURCE_STATE_UNORDERED_ACCESS))) {
					Dx_Util::ErrorMessage("SkinnedCom::createParameterDXR Error!!");
					return false;
				}
			}
		}
	}
	return true;
}

void SkinnedCom::skinning(int comNo) {
	Dx12Process* dx = Dx12Process::GetInstance();

	ID3D12GraphicsCommandList* mCList = pd->dx->dx_sub[comNo].mCommandList.Get();
	Dx_CommandListObj& d = dx->dx_sub[comNo];
	mCList->SetPipelineState(PSO.Get());
	ID3D12DescriptorHeap* descriptorHeaps[] = { descHeap.Get() };
	mCList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
	mCList->SetComputeRootSignature(rootSignature.Get());

	UINT numVer = pd->dpara.Vview.get()->VertexBufferByteSize / sizeof(MY_VERTEX_S);
	D3D12_GPU_DESCRIPTOR_HANDLE des = descHeap->GetGPUDescriptorHandleForHeapStart();
	UpdateDXR& ud = pd->dxrPara.updateDXR[dx->dxrBuffSwap[0]];

	mCList->SetComputeRootDescriptorTable(0, des);
	mCList->Dispatch(numVer, 1, 1);

	d.delayResourceBarrierBefore(SkinnedVer.Get(),
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
	for (int i = 0; i < pd->dpara.NumMaterial; i++) {
		//使用されていないマテリアル対策
		if (pd->dpara.Iview[i].IndexCount <= 0)continue;

		d.delayResourceBarrierBefore(ud.VviewDXR[i][0].VertexBufferGPU.Get(),
			D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COPY_DEST);
		d.delayCopyResource(ud.VviewDXR[i][0].VertexBufferGPU.Get(), SkinnedVer.Get());
		d.delayResourceBarrierAfter(ud.VviewDXR[i][0].VertexBufferGPU.Get(),
			D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);
	}
	d.delayResourceBarrierAfter(SkinnedVer.Get(),
		D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	ud.firstSet = true;
}

void SkinnedCom::Skinning(int comNo) {
	Dx12Process* dx = Dx12Process::GetInstance();

	if (pd->vs == dx->shaderH->pVertexShader_SKIN.Get()) {

		UpdateDXR& ud = pd->dxrPara.updateDXR[dx->dxrBuffSwap[0]];
		ud.InstanceMaskChange(pd->DrawOn);

		if (!pd->firstCbSet[dx->cBuffSwap[1]])return;

		pd->mObjectCB->CopyData(0, pd->cb[dx->cBuffSwap[1]]);
		pd->dpara.insNum = pd->insNum[dx->cBuffSwap[1]];
		pd->ParameterDXR_Update();
		if (pd->dxrPara.updateF || !pd->dxrPara.updateDXR[dx->dxrBuffSwap[0]].createAS) {
			skinning(comNo);
		}
	}
}