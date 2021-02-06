//*****************************************************************************************//
//**                                                                                     **//
//**                   　　　        DxSkinnedCom                                        **//
//**                                                                                     **//
//*****************************************************************************************//

#include "DxSkinnedCom.h"

void SkinnedCom::getBuffer(PolygonData* p, int numMaterial) {
	Dx12Process* dx = Dx12Process::GetInstance();
	if (dx->DXR_CreateResource) {
		pd = p;
		int NumMaterial = pd->dpara.NumMaterial;
		mObjectCB = new ConstantBuffer<uvSW>(NumMaterial);
		sw = std::make_unique<uvSW[]>(NumMaterial);
		SkinnedVer = std::make_unique<ComPtr<ID3D12Resource>[]>(NumMaterial);
	}
}

bool SkinnedCom::createDescHeap(D3D12_GPU_VIRTUAL_ADDRESS ad3, UINT ad3Size) {
	Dx12Process* dx = Dx12Process::GetInstance();

	if (dx->DXR_CreateResource && pd->vs == dx->shaderH->pVertexShader_SKIN.Get()) {
		int NumMaterial = pd->dpara.NumMaterial;

		const int numDesc = numSrv + numCbv + numUav;
		NumDesc = numDesc;
		const int numHeap = numDesc * NumMaterial;
		descHeap = dx->device->CreateDescHeap(numHeap);
		if (descHeap == nullptr)return false;

		UINT cbSize[2] = {};
		cbSize[0] = ad3Size;
		cbSize[1] = mObjectCB->getSizeInBytes();

		for (int i = 0; i < NumMaterial; i++) {
			mObjectCB->CopyData(i, sw[i]);
			ID3D12Resource* res[1];
			res[0] = pd->dpara.Vview.get()->VertexBufferGPU.Get();
			UINT resSize[1];
			resSize[0] = sizeof(MY_VERTEX_S);
			pd->CreateSrvBuffer(descHeap.Get(), numDesc * i, res, numSrv, resSize);
			D3D12_GPU_VIRTUAL_ADDRESS ad[2];
			ad[0] = ad3;
			ad[1] = mObjectCB->Resource()->GetGPUVirtualAddress() + cbSize[1] * i;
			pd->CreateCbv(descHeap.Get(), numDesc * i + numSrv, ad, cbSize, numCbv);
			ID3D12Resource* resU[1];
			resU[0] = SkinnedVer[i].Get();
			UINT byteStride[1];
			byteStride[0] = sizeof(VERTEX_DXR);
			UINT size[1];
			size[0] = pd->dpara.Vview.get()->VertexBufferByteSize / sizeof(MY_VERTEX_S);
			pd->CreateUavBuffer(descHeap.Get(), numDesc * i + numSrv + numCbv,
				resU, byteStride, size, numUav);
		}
	}
	return true;
}

bool SkinnedCom::createPSO() {
	Dx12Process* dx = Dx12Process::GetInstance();

	if (dx->DXR_CreateResource && pd->vs == dx->shaderH->pVertexShader_SKIN.Get()) {
		rootSignature = pd->CreateRootSignatureCompute(numSrv, numCbv, numUav, 0, 0);
		if (rootSignature == nullptr)return false;

		ID3DBlob* cs = pd->dx->shaderH->pVertexShader_SKIN_Com.Get();

		PSO = pd->CreatePsoCompute(cs, rootSignature.Get());
		if (PSO == nullptr)return false;
	}
	return true;
}

bool SkinnedCom::createParameterDXR() {
	Dx12Process* dx = Dx12Process::GetInstance();

	if (dx->DXR_CreateResource && pd->vs == dx->shaderH->pVertexShader_SKIN.Get()) {

		int NumMaterial = pd->dxrPara.NumMaterial;

		for (int i = 0; i < NumMaterial; i++) {
			if (pd->dpara.Iview[i].IndexCount <= 0)continue;
			UINT bytesize = 0;
			IndexView& dxI = pd->dxrPara.IviewDXR[i];
			UINT indCnt = pd->dpara.Iview[i].IndexCount;
			bytesize = indCnt * sizeof(UINT);
			dxI.IndexFormat = DXGI_FORMAT_R32_UINT;
			dxI.IndexBufferByteSize = bytesize;
			dxI.IndexCount = indCnt;
			if (FAILED(dx->device->createDefaultResourceBuffer(dxI.IndexBufferGPU.GetAddressOf(),
				dxI.IndexBufferByteSize, D3D12_RESOURCE_STATE_GENERIC_READ))) {
				ErrorMessage("SkinnedCom::createParameterDXR Error!!");
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
				if (FAILED(dx->device->createDefaultResourceBuffer(dxV.VertexBufferGPU.GetAddressOf(),
					dxV.VertexBufferByteSize, D3D12_RESOURCE_STATE_GENERIC_READ))) {
					ErrorMessage("SkinnedCom::createParameterDXR Error!!");
					return false;
				}
			}
			if (FAILED(dx->device->createDefaultResourceBuffer_UNORDERED_ACCESS(SkinnedVer[i].GetAddressOf(),
				bytesize, D3D12_RESOURCE_STATE_UNORDERED_ACCESS))) {
				ErrorMessage("SkinnedCom::createParameterDXR Error!!");
				return false;
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
	for (int i = 0; i < pd->dpara.NumMaterial; i++) {
		//使用されていないマテリアル対策
		if (pd->dpara.Iview[i].IndexCount <= 0)continue;

		mCList->SetComputeRootDescriptorTable(0, des);
		mCList->Dispatch(numVer, 1, 1);
		des.ptr += dx->mCbvSrvUavDescriptorSize * NumDesc;

		d.delayResourceBarrierBefore(ud.VviewDXR[i][0].VertexBufferGPU.Get(),
			D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COPY_DEST);
		d.delayResourceBarrierBefore(SkinnedVer[i].Get(),
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
		d.delayCopyResource(ud.VviewDXR[i][0].VertexBufferGPU.Get(), SkinnedVer[i].Get());
		d.delayResourceBarrierAfter(ud.VviewDXR[i][0].VertexBufferGPU.Get(),
			D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);
		d.delayResourceBarrierAfter(SkinnedVer[i].Get(),
			D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

		ud.firstSet = true;
	}
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