//*****************************************************************************************//
//**                                                                                     **//
//**                                 DxSkinnedCom                                        **//
//**                                                                                     **//
//*****************************************************************************************//

#include "DxSkinnedCom.h"

void SkinnedCom::getBuffer(BasicPolygon* p) {
	Dx_Device* dev = Dx_Device::GetInstance();
	if (dev->getDxrCreateResourceState()) {
		pd = p;
	}
}

bool SkinnedCom::createDescHeap(D3D12_GPU_VIRTUAL_ADDRESS ad3, UINT ad3Size) {

	Dx_Device* dev = Dx_Device::GetInstance();

	if (dev->getDxrCreateResourceState() && pd->vs == Dx_ShaderHolder::pVertexShader_SKIN.Get()) {

		Dx_Device* device = Dx_Device::GetInstance();
		const int numDesc = numSrv + numCbv + numUav;
		descHeap = device->CreateDescHeap(numDesc);
		if (descHeap == nullptr)return false;

		D3D12_CPU_DESCRIPTOR_HANDLE hDescriptor = descHeap->GetCPUDescriptorHandleForHeapStart();

		UINT resSize = sizeof(MY_VERTEX_S);
		pd->dpara.Vview.get()->VertexBufferGPU.CreateSrvBuffer(hDescriptor, resSize);

		D3D12_GPU_VIRTUAL_ADDRESS ad[1] = {};
		ad[0] = ad3;
		UINT cbSize[1] = {};
		cbSize[0] = ad3Size;
		device->CreateCbv(hDescriptor, ad, cbSize, numCbv);

		UINT byteStride = sizeof(VERTEX_DXR);
		UINT size = pd->dpara.Vview.get()->VertexBufferByteSize / sizeof(MY_VERTEX_S);
		SkinnedVer.CreateUavBuffer(hDescriptor, byteStride, size);
	}
	return true;
}

bool SkinnedCom::createPSO() {

	Dx_Device* dev = Dx_Device::GetInstance();

	if (dev->getDxrCreateResourceState() && pd->vs == Dx_ShaderHolder::pVertexShader_SKIN.Get()) {
		rootSignature = pd->CreateRootSignatureCompute(numSrv, numCbv, numUav, 0, 0, 0, nullptr);
		if (rootSignature == nullptr)return false;

		ID3DBlob* cs = Dx_ShaderHolder::pVertexShader_SKIN_Com.Get();

		PSO = pd->CreatePsoCompute(cs, rootSignature.Get());
		if (PSO == nullptr)return false;
	}
	return true;
}

bool SkinnedCom::createParameterDXR(int comIndex) {

	Dx_Device* dev = Dx_Device::GetInstance();

	if (dev->getDxrCreateResourceState() && pd->vs == Dx_ShaderHolder::pVertexShader_SKIN.Get()) {

		int NumMaterial = pd->dxrPara.NumMaterial;
		Dx_Device* device = Dx_Device::GetInstance();

		Dx_CommandListObj& d = *Dx_CommandManager::GetInstance()->getGraphicsComListObj(comIndex);

		for (int i = 0; i < NumMaterial; i++) {
			if (pd->dpara.Iview[i].IndexCount <= 0)continue;
			UINT bytesize = 0;
			IndexView& dxI = pd->dxrPara.IviewDXR[i];
			UINT indCnt = pd->dpara.Iview[i].IndexCount;
			bytesize = indCnt * sizeof(UINT);
			dxI.IndexFormat = DXGI_FORMAT_R32_UINT;
			dxI.IndexBufferByteSize = bytesize;
			dxI.IndexCount = indCnt;
			if (FAILED(dxI.IndexBufferGPU.createDefaultResourceBuffer(dxI.IndexBufferByteSize))) {
				Dx_Util::ErrorMessage("SkinnedCom::createParameterDXR Error!!");
				return false;
			}

			dxI.IndexBufferGPU.CopyResource(comIndex, &pd->dpara.Iview[i].IndexBufferGPU);

			for (int j = 0; j < 2; j++) {
				pd->dxrPara.updateDXR[j].currentIndexCount[i][0] = indCnt;
				VertexView& dxV = pd->dxrPara.updateDXR[j].VviewDXR[i][0];
				UINT vCnt = pd->dpara.Vview.get()->VertexBufferByteSize / sizeof(MY_VERTEX_S);
				bytesize = vCnt * sizeof(VERTEX_DXR);
				dxV.VertexByteStride = sizeof(VERTEX_DXR);
				dxV.VertexBufferByteSize = bytesize;
				if (FAILED(dxV.VertexBufferGPU.createDefaultResourceBuffer(dxV.VertexBufferByteSize))) {
					Dx_Util::ErrorMessage("SkinnedCom::createParameterDXR Error!!");
					return false;
				}

				dxV.VertexBufferGPU.ResourceBarrier(comIndex, D3D12_RESOURCE_STATE_GENERIC_READ);
			}
			if (!SkinnedVer.getResource()) {
				if (FAILED(SkinnedVer.createDefaultResourceBuffer_UNORDERED_ACCESS(bytesize))) {
					Dx_Util::ErrorMessage("SkinnedCom::createParameterDXR Error!!");
					return false;
				}

				SkinnedVer.ResourceBarrier(comIndex, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			}
		}
	}
	return true;
}

void SkinnedCom::skinning(int comIndex) {

	Dx_Device* dev = Dx_Device::GetInstance();

	Dx_CommandListObj& d = *Dx_CommandManager::GetInstance()->getGraphicsComListObj(comIndex);
	ID3D12GraphicsCommandList* mCList = d.getCommandList();

	mCList->SetPipelineState(PSO.Get());
	ID3D12DescriptorHeap* descriptorHeaps[] = { descHeap.Get() };
	mCList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
	mCList->SetComputeRootSignature(rootSignature.Get());

	UINT numVer = pd->dpara.Vview.get()->VertexBufferByteSize / sizeof(MY_VERTEX_S);
	D3D12_GPU_DESCRIPTOR_HANDLE des = descHeap->GetGPUDescriptorHandleForHeapStart();
	UpdateDXR& ud = pd->dxrPara.updateDXR[dev->dxrBuffSwapIndex()];

	mCList->SetComputeRootDescriptorTable(0, des);
	mCList->Dispatch(numVer, 1, 1);

	for (int i = 0; i < pd->dpara.NumMaterial; i++) {
		//使用されていないマテリアル対策
		if (pd->dpara.Iview[i].IndexCount <= 0)continue;

		ud.VviewDXR[i][0].VertexBufferGPU.delayCopyResource(comIndex, &SkinnedVer);
	}

	ud.firstSet = true;
}

void SkinnedCom::Skinning(int comIndex) {

	Dx_Device* dev = Dx_Device::GetInstance();

	if (pd->vs == Dx_ShaderHolder::pVertexShader_SKIN.Get()) {

		UpdateDXR& ud = pd->dxrPara.updateDXR[dev->dxrBuffSwapIndex()];
		ud.InstanceMaskChange(pd->DrawOn);

		if (!pd->firstCbSet[dev->cBuffSwapDrawOrStreamoutputIndex()])return;

		pd->mObjectCB->CopyData(0, pd->cb[dev->cBuffSwapDrawOrStreamoutputIndex()]);
		pd->dpara.insNum = pd->insNum[dev->cBuffSwapDrawOrStreamoutputIndex()];
		pd->ParameterDXR_Update();
		if (pd->dxrPara.updateF || !pd->dxrPara.updateDXR[dev->dxrBuffSwapIndex()].createAS) {
			skinning(comIndex);
		}
	}
}