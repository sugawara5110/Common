//*****************************************************************************************//
//**                                                                                     **//
//**                   �@�@�@          PolygonData�N���X                                 **//
//**                                   GetVBarray�֐�                                    **//
//*****************************************************************************************//

#define _CRT_SECURE_NO_WARNINGS
#include "Dx12ProcessCore.h"

void SkinnedCom::getBuffer(PolygonData* p, int numMaterial) {
	pd = p;
	int NumMaterial = pd->dpara.NumMaterial;
	mObjectCB = new ConstantBuffer<uvSW>(NumMaterial);
	sw = std::make_unique<uvSW[]>(NumMaterial);
}

void SkinnedCom::createDescHeap(D3D12_GPU_VIRTUAL_ADDRESS ad3, UINT ad3Size) {
	Dx12Process* dx = Dx12Process::GetInstance();
	int NumMaterial = pd->dpara.NumMaterial;

	const int numDesc = numSrv + numCbv + numUav;
	NumDesc = numDesc;
	const int numHeap = numDesc * NumMaterial;
	descHeap = dx->CreateDescHeap(numHeap);

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
		resU[0] = VviewDXR.Get();
		UINT byteStride[1];
		byteStride[0] = sizeof(VERTEX_DXR);
		UINT size[1];
		size[0] = pd->dpara.Vview.get()->VertexBufferByteSize / sizeof(MY_VERTEX_S);
		pd->CreateUavBuffer(descHeap.Get(), numDesc * i + numSrv + numCbv,
			resU, byteStride, size, numUav);
	}
}

bool SkinnedCom::createPSO() {

	rootSignature = pd->CreateRootSignatureCompute(numSrv, numCbv, numUav);
	if (rootSignature == nullptr)return false;

	ID3DBlob* cs = pd->dx->pVertexShader_SKIN_Com.Get();

	PSO = pd->CreatePsoCompute(cs, rootSignature.Get());
	if (PSO == nullptr)return false;

	return true;
}

bool SkinnedCom::createParameterDXR() {
	Dx12Process* dx = Dx12Process::GetInstance();
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
		if (FAILED(dx->createDefaultResourceBuffer(dxI.IndexBufferGPU.GetAddressOf(),
			dxI.IndexBufferByteSize, D3D12_RESOURCE_STATE_GENERIC_READ))) {
			ErrorMessage("SkinnedCom::createParameterDXR Error!!");
			return false;
		}

		dx->dx_sub[pd->com_no].CopyResourceGENERIC_READ(dxI.IndexBufferGPU.Get(),
			pd->dpara.Iview[i].IndexBufferGPU.Get());

		for (int j = 0; j < 2; j++) {
			pd->dxrPara.updateDXR[j].currentIndexCount[i] = indCnt;
			VertexView& dxV = pd->dxrPara.updateDXR[j].VviewDXR[i];
			UINT vCnt = pd->dpara.Vview.get()->VertexBufferByteSize / sizeof(MY_VERTEX_S);
			bytesize = vCnt * sizeof(VERTEX_DXR);
			dxV.VertexByteStride = sizeof(VERTEX_DXR);
			dxV.VertexBufferByteSize = bytesize;
			if (FAILED(dx->createDefaultResourceBuffer(dxV.VertexBufferGPU.GetAddressOf(),
				dxV.VertexBufferByteSize, D3D12_RESOURCE_STATE_GENERIC_READ))) {
				ErrorMessage("SkinnedCom::createParameterDXR Error!!");
				return false;
			}
		}
		if (FAILED(dx->createDefaultResourceBuffer_UNORDERED_ACCESS(VviewDXR.GetAddressOf(),
			bytesize, D3D12_RESOURCE_STATE_UNORDERED_ACCESS))) {
			ErrorMessage("SkinnedCom::createParameterDXR Error!!");
			return false;
		}
	}
	return true;
}

void SkinnedCom::Skinning(int comNo) {
	Dx12Process* dx = Dx12Process::GetInstance();
	ID3D12GraphicsCommandList* mCList = pd->dx->dx_sub[comNo].mCommandList.Get();
	mCList->SetPipelineState(PSO.Get());
	ID3D12DescriptorHeap* descriptorHeaps[] = { descHeap.Get() };
	mCList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
	mCList->SetComputeRootSignature(rootSignature.Get());

	UINT numVer = pd->dpara.Vview.get()->VertexBufferByteSize / sizeof(MY_VERTEX_S);
	D3D12_GPU_DESCRIPTOR_HANDLE des = descHeap->GetGPUDescriptorHandleForHeapStart();
	UpdateDXR& ud = pd->dxrPara.updateDXR[dx->dxrBuffSwap[0]];
	for (int i = 0; i < pd->dpara.NumMaterial; i++) {
		//�g�p����Ă��Ȃ��}�e���A���΍�
		if (pd->dpara.Iview[i].IndexCount <= 0)continue;

		mCList->SetComputeRootDescriptorTable(0, des);
		mCList->Dispatch(numVer, 1, 1);
		des.ptr += dx->mCbvSrvUavDescriptorSize * NumDesc;

		dx->dx_sub[comNo].ResourceBarrier(ud.VviewDXR[i].VertexBufferGPU.Get(),
			D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COPY_DEST);
		dx->dx_sub[comNo].ResourceBarrier(VviewDXR.Get(),
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
		mCList->CopyResource(ud.VviewDXR[i].VertexBufferGPU.Get(), VviewDXR.Get());
		dx->dx_sub[comNo].ResourceBarrier(ud.VviewDXR[i].VertexBufferGPU.Get(),
			D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);
		dx->dx_sub[comNo].ResourceBarrier(VviewDXR.Get(),
			D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

		D3D12_RESOURCE_BARRIER uavBarrier = {};
		uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
		uavBarrier.UAV.pResource = VviewDXR.Get();
		mCList->ResourceBarrier(1, &uavBarrier);
		ud.firstSet = true;
	}
}

PolygonData::PolygonData() {
	dx = Dx12Process::GetInstance();
	mCommandList = dx->dx_sub[0].mCommandList.Get();

	sg.vDiffuse.x = 0.7f;
	sg.vDiffuse.y = 0.7f;
	sg.vDiffuse.z = 0.7f;
	sg.vDiffuse.w = 0.0f;

	sg.vSpeculer.x = 0.3f;
	sg.vSpeculer.y = 0.3f;
	sg.vSpeculer.z = 0.3f;
	sg.vSpeculer.w = 0.0f;

	sg.vAmbient.x = 0.0f;
	sg.vAmbient.y = 0.0f;
	sg.vAmbient.z = 0.0f;
	sg.vAmbient.w = 0.0f;

	divArr[0].distance = 1000.0f;
	divArr[0].divide = 2;//���_�� 3 �� 3 * 6 = 18
	divArr[1].distance = 500.0f;
	divArr[1].divide = 48;//���_�� 3 �� 3 * 3456 = 10368
	divArr[2].distance = 300.0f;
	divArr[2].divide = 96;//���_�� 3 �� 3 * 13824 = 41472

	firstCbSet[0] = false;
	firstCbSet[1] = false;
}

PolygonData::~PolygonData() {
	S_DELETE(mObjectCB);
	S_DELETE(mObjectCB1);
}

ID3D12PipelineState* PolygonData::GetPipelineState() {
	return dpara.PSO[0].Get();
}

void PolygonData::getBuffer(int numMaterial, DivideArr* divarr, int numdiv) {
	memcpy(divArr, divarr, sizeof(DivideArr) * numdiv);
	mObjectCB = new ConstantBuffer<CONSTANT_BUFFER>(1);
	dpara.NumMaterial = numMaterial;
	mObjectCB1 = new ConstantBuffer<CONSTANT_BUFFER2>(dpara.NumMaterial);
	dpara.material = std::make_unique<MY_MATERIAL_S[]>(dpara.NumMaterial);
	dpara.PSO = std::make_unique<ComPtr<ID3D12PipelineState>[]>(dpara.NumMaterial);
	dpara.Iview = std::make_unique<IndexView[]>(dpara.NumMaterial);
	if (dx->DXR_CreateResource) {
		dpara.PSO_DXR = std::make_unique<ComPtr<ID3D12PipelineState>[]>(dpara.NumMaterial);
		createBufferDXR(dpara.NumMaterial);
		sk.getBuffer(this, numMaterial);
	}
}

void PolygonData::getVertexBuffer(UINT VertexByteStride, UINT numVertex) {
	dpara.Vview = std::make_unique<VertexView>();
	const UINT byteSize = VertexByteStride * numVertex;
	dpara.Vview->VertexByteStride = VertexByteStride;
	dpara.Vview->VertexBufferByteSize = byteSize;
}

void PolygonData::getIndexBuffer(int materialIndex, UINT IndexBufferByteSize, UINT numIndex) {
	dpara.Iview[materialIndex].IndexFormat = DXGI_FORMAT_R32_UINT;
	dpara.Iview[materialIndex].IndexBufferByteSize = IndexBufferByteSize;
	dpara.Iview[materialIndex].IndexCount = numIndex;
}

void PolygonData::GetVBarray(PrimitiveType type) {

	primType_create = type;
	if (type == SQUARE) {
		dpara.TOPOLOGY = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	}
	if (type == POINt) {
		dpara.TOPOLOGY = D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;
	}
	if (type == LINE_L) {
		dpara.TOPOLOGY = D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
	}
if (type == LINE_S) {
	dpara.TOPOLOGY = D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP;
}
if (type == CONTROL_POINT) {
	dpara.TOPOLOGY = D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST;
}

getBuffer(1);
}

void PolygonData::GetShaderByteCode(bool light, int tNo) {
	bool disp = false;
	if (primType_create == CONTROL_POINT)disp = true;
	if (tNo == -1 && movOn[0].m_on == false) {
		vs = dx->pVertexShader_BC.Get();
		ps = dx->pPixelShader_BC.Get();
		ps_NoMap = dx->pPixelShader_BC.Get();
		return;
	}
	if (!disp && light) {
		vs = dx->pVertexShader_TC.Get();
		gs = dx->pGeometryShader_Before_vs.Get();
		gs_NoMap = dx->pGeometryShader_Before_vs_NoNormalMap.Get();
		ps = dx->pPixelShader_3D.Get();
		ps_NoMap = dx->pPixelShader_3D_NoNormalMap.Get();
		return;
	}
	if (!disp && !light) {
		vs = dx->pVertexShader_TC.Get();
		gs = dx->pGeometryShader_Before_vs.Get();
		gs_NoMap = dx->pGeometryShader_Before_vs_NoNormalMap.Get();
		ps = dx->pPixelShader_Emissive.Get();
		ps_NoMap = dx->pPixelShader_Emissive.Get();
		return;
	}
	if (disp && light) {
		vs = dx->pVertexShader_MESH_D.Get();
		ps = dx->pPixelShader_3D.Get();
		ps_NoMap = dx->pPixelShader_3D_NoNormalMap.Get();
		hs = dx->pHullShaderTriangle.Get();
		ds = dx->dx->pDomainShaderTriangle.Get();
		gs = dx->pGeometryShader_Before_ds.Get();
		gs_NoMap = dx->pGeometryShader_Before_ds_NoNormalMap.Get();
		return;
	}
	if (disp && !light) {
		vs = dx->pVertexShader_MESH_D.Get();
		ps = dx->pPixelShader_Emissive.Get();
		ps_NoMap = dx->pPixelShader_Emissive.Get();
		hs = dx->pHullShaderTriangle.Get();
		ds = dx->dx->pDomainShaderTriangle.Get();
		gs = dx->pGeometryShader_Before_ds.Get();
		gs_NoMap = dx->pGeometryShader_Before_ds_NoNormalMap.Get();
		return;
	}
}

void PolygonData::SetCol(float difR, float difG, float difB, float speR, float speG, float speB,
	float amR, float amG, float amB) {
	sg.vDiffuse.x = difR;
	sg.vDiffuse.y = difG;
	sg.vDiffuse.z = difB;

	sg.vSpeculer.x = speR;
	sg.vSpeculer.y = speG;
	sg.vSpeculer.z = speB;

	sg.vAmbient.x = amR;
	sg.vAmbient.y = amG;
	sg.vAmbient.z = amB;
}

bool PolygonData::Create(bool light, int tNo, bool blend, bool alpha) {
	return Create(light, tNo, -1, -1, blend, alpha);
}

bool PolygonData::createPSO(std::vector<D3D12_INPUT_ELEMENT_DESC>& vertexLayout,
	const int numSrv, const int numCbv, const int numUav, bool blend, bool alpha) {

	//�p�C�v���C���X�e�[�g�I�u�W�F�N�g����
	dpara.rootSignature = CreateRootSignature(numSrv, numCbv, numUav);
	if (dpara.rootSignature == nullptr)return false;

	for (int i = 0; i < dpara.NumMaterial; i++) {
		if (dpara.material[i].nortex_no < 0)
			dpara.PSO[i] = CreatePsoVsHsDsPs(vs, hs, ds, ps_NoMap, gs_NoMap, dpara.rootSignature.Get(),
				vertexLayout, alpha, blend, primType_create);
		else
			dpara.PSO[i] = CreatePsoVsHsDsPs(vs, hs, ds, ps, gs, dpara.rootSignature.Get(),
				vertexLayout, alpha, blend, primType_create);

		if (dpara.PSO[i] == nullptr)return false;
	}

	return true;
}

bool PolygonData::createPSO_DXR(std::vector<D3D12_INPUT_ELEMENT_DESC>& vertexLayout,
	const int numSrv, const int numCbv, const int numUav) {

	if (vs == dx->pVertexShader_SKIN.Get()) {
		sk.createPSO();
		return true;
	}

	if (hs) {
		gs = dx->pGeometryShader_Before_ds_Output.Get();
		dpara.rootSignatureDXR = CreateRootSignatureStreamOutput(numSrv, numCbv, numUav, true);
	}
	else {
		gs = dx->pGeometryShader_Before_vs_Output.Get();
		dpara.rootSignatureDXR = CreateRootSignatureStreamOutput(numSrv, numCbv, numUav, false);
	}
	if (dpara.rootSignature == nullptr)return false;

	for (int i = 0; i < dpara.NumMaterial; i++) {
		if (dpara.Iview[i].IndexCount <= 0)continue;
		dpara.PSO_DXR[i] = CreatePsoStreamOutput(vs, hs, ds, gs, dpara.rootSignatureDXR.Get(),
			vertexLayout,
			&dx->pDeclaration_Output,
			(UINT)dx->pDeclaration_Output.size(),
			&dxrPara.SviewDXR[i].StreamByteStride,
			1,
			primType_create);
	}

	return true;
}

void PolygonData::setTextureDXR() {
	for (int i = 0; i < dpara.NumMaterial; i++) {
		dxrPara.difTex[i] = texture[i * 3 + 0].Get();
		dxrPara.norTex[i] = texture[i * 3 + 1].Get();
		dxrPara.speTex[i] = texture[i * 3 + 2].Get();
	}
}

bool PolygonData::setDescHeap(const int numSrvTex,
	const int numSrvBuf, ID3D12Resource** buffer, UINT* StructureByteStride,
	const int numCbv, D3D12_GPU_VIRTUAL_ADDRESS ad3, UINT ad3Size) {

	TextureNo* te = new TextureNo[dpara.NumMaterial];
	int tCnt = 0;
	for (int i = 0; i < dpara.NumMaterial; i++) {
		if (dpara.material[i].diftex_no < 0)te[tCnt].diffuse = dx->GetTexNumber("dummyDifSpe.");
		else
			te[tCnt].diffuse = dpara.material[i].diftex_no;

		if (dpara.material[i].nortex_no < 0)te[tCnt].normal = dx->GetTexNumber("dummyNor.");
		else
			te[tCnt].normal = dpara.material[i].nortex_no;

		if (dpara.material[i].spetex_no < 0)te[tCnt].specular = dx->GetTexNumber("dummyDifSpe.");
		else
			te[tCnt].specular = dpara.material[i].spetex_no;
		tCnt++;
	}
	createTextureResource(0, tCnt, te);
	if (dx->DXR_CreateResource) {
		setTextureDXR();
		if (vs == dx->pVertexShader_SKIN.Get()) {
			sk.createDescHeap(ad3, ad3Size);
		}
	}
	int numUav = 0;
	if (hs)numUav = 1;
	dpara.numDesc = numSrvTex + numSrvBuf + numCbv + numUav;
	int numHeap = dpara.NumMaterial * dpara.numDesc;
	dpara.descHeap = dx->CreateDescHeap(numHeap);
	ARR_DELETE(te);
	if (dpara.descHeap == nullptr)return false;
	const int numMaxCB = 3;
	UINT cbSize[numMaxCB] = {};
	cbSize[0] = mObjectCB->getSizeInBytes();
	cbSize[1] = mObjectCB1->getSizeInBytes();
	cbSize[2] = ad3Size;
	for (int i = 0; i < dpara.NumMaterial; i++) {
		CreateSrvTexture(dpara.descHeap.Get(), dpara.numDesc * i, texture[numSrvTex * i].GetAddressOf(), numSrvTex);
		CreateSrvBuffer(dpara.descHeap.Get(), dpara.numDesc * i + numSrvTex, buffer, numSrvBuf, StructureByteStride);
		D3D12_GPU_VIRTUAL_ADDRESS ad[numMaxCB];
		ad[0] = mObjectCB->Resource()->GetGPUVirtualAddress();
		ad[1] = mObjectCB1->Resource()->GetGPUVirtualAddress() + cbSize[1] * i;
		ad[2] = ad3;
		CreateCbv(dpara.descHeap.Get(), dpara.numDesc * i + numSrvTex + numSrvBuf, ad, cbSize, numCbv);
		if (hs) {
			ID3D12Resource* res[1];
			res[0] = dpara.mDivideBuffer[i].Get();
			UINT byteStride[1];
			byteStride[0] = sizeof(UINT);
			UINT size[1];
			size[0] = dpara.Iview[i].IndexCount / 3;
			CreateUavBuffer(dpara.descHeap.Get(), dpara.numDesc * i + numSrvTex + numSrvBuf + numCbv,
				res, byteStride, size, 1);
		}
	}
	return true;
}

void PolygonData::createBufferDXR(int numMaterial) {
	dxrPara.NumMaterial = numMaterial;
	dxrPara.create(numMaterial);
}

void PolygonData::createParameterDXR(bool alpha) {
	dxrPara.alphaTest = alpha;
	int NumMaterial = dxrPara.NumMaterial;

	if (hs || vs == dx->pVertexShader_SKIN.Get())dxrPara.updateF = true;

	if (vs == dx->pVertexShader_SKIN.Get()) {
		sk.createParameterDXR();
		return;
	}

	int numDispPolygon = 1;//�e�Z���[�V����������(1�|���S��)
	if (hs) {
		float maxDiv = 0.0f;
		const float minDiv = 2.0f;
		const float minDivPolygon = 6;
		for (int i = 0; i < numDiv; i++) {
			if (divArr[i].divide >= minDiv && (int)divArr[i].divide % 2 == 1)divArr[i].divide += 1.0f;//�����ɂ���
			if (divArr[i].divide < minDiv)divArr[i].divide = minDiv;
			if (maxDiv < divArr[i].divide)maxDiv = divArr[i].divide;
		}
		int mag = (int)maxDiv / (int)minDiv;
		numDispPolygon = (int)mag * (int)mag * (int)minDivPolygon;//�����ő吔(1�|���S��)
	}

	for (int i = 0; i < NumMaterial; i++) {
		if (dpara.Iview[i].IndexCount <= 0)continue;
		UINT bytesize = 0;
		IndexView& dxI = dxrPara.IviewDXR[i];
		UINT indCnt = dpara.Iview[i].IndexCount * numDispPolygon;
		bytesize = indCnt * sizeof(UINT);
		dxI.IndexFormat = DXGI_FORMAT_R32_UINT;
		dxI.IndexBufferByteSize = bytesize;
		dxI.IndexCount = indCnt;
		UINT* ind = new UINT[indCnt];
		for (UINT in = 0; in < indCnt; in++)ind[in] = in;
		dxI.IndexBufferGPU = dx->CreateDefaultBuffer(com_no, ind,
			bytesize, dxI.IndexBufferUploader, false);
		ARR_DELETE(ind);
		for (int j = 0; j < 2; j++) {
			dxrPara.updateDXR[j].currentIndexCount[i] = indCnt;
			VertexView& dxV = dxrPara.updateDXR[j].VviewDXR[i];
			bytesize = indCnt * sizeof(VERTEX_DXR);
			dxV.VertexByteStride = sizeof(VERTEX_DXR);
			dxV.VertexBufferByteSize = bytesize;
			dx->createDefaultResourceBuffer(dxV.VertexBufferGPU.GetAddressOf(),
				dxV.VertexBufferByteSize);
			dx->dx_sub[com_no].ResourceBarrier(dxV.VertexBufferGPU.Get(),
				D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_GENERIC_READ);
		}
		StreamView& dxS = dxrPara.SviewDXR[i];
		dxS.StreamByteStride = sizeof(VERTEX_DXR);
		dxS.StreamBufferByteSize = bytesize;
		dxS.StreamBufferGPU = dx->CreateStreamBuffer(dxS.StreamBufferByteSize);

		StreamView& dxL = dxrPara.SizeLocation[i];
		dxL.StreamByteStride = sizeof(VERTEX_DXR);
		dxL.StreamBufferByteSize = bytesize;
		dxL.StreamBufferGPU = dx->CreateStreamBuffer(dxL.StreamBufferByteSize);

		dxS.BufferFilledSizeLocation = dxL.StreamBufferGPU.Get()->GetGPUVirtualAddress();
	}
}

void PolygonData::setColorDXR(int materialIndex, CONSTANT_BUFFER2& sg) {
	dxrPara.diffuse[materialIndex].as(sg.vDiffuse.x, sg.vDiffuse.y, sg.vDiffuse.z, sg.vDiffuse.w);
	dxrPara.specular[materialIndex].as(sg.vSpeculer.x, sg.vSpeculer.y, sg.vSpeculer.z, sg.vSpeculer.w);
	dxrPara.ambient[materialIndex].as(sg.vAmbient.x, sg.vAmbient.y, sg.vAmbient.z, sg.vAmbient.w);
}

void PolygonData::createDivideBuffer() {

	dpara.mDivideBuffer = std::make_unique<ComPtr<ID3D12Resource>[]>(dpara.NumMaterial);
	dpara.mDivideReadBuffer = std::make_unique<ComPtr<ID3D12Resource>[]>(dpara.NumMaterial);

	for (int i = 0; i < dpara.NumMaterial; i++) {
		if (dpara.Iview[i].IndexCount <= 0)continue;
		int numPolygon = dpara.Iview[i].IndexCount / 3;
		HRESULT hr = dx->createDefaultResourceBuffer_UNORDERED_ACCESS(dpara.mDivideBuffer[i].GetAddressOf(), numPolygon * sizeof(UINT),
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		if (FAILED(hr)) {
			ErrorMessage("PolygonData::createDivideBuffer Error!!");
		}

		D3D12_HEAP_PROPERTIES heap = {};
		heap.Type = D3D12_HEAP_TYPE_READBACK;
		heap.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		heap.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		heap.CreationNodeMask = 1;
		heap.VisibleNodeMask = 1;

		D3D12_RESOURCE_DESC desc = {};
		desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		desc.Alignment = 0;
		desc.Width = numPolygon * sizeof(UINT);
		desc.Height = 1;
		desc.DepthOrArraySize = 1;
		desc.MipLevels = 1;
		desc.Format = DXGI_FORMAT_UNKNOWN;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		desc.Flags = D3D12_RESOURCE_FLAG_NONE;

		hr = dx->md3dDevice->CreateCommittedResource(
			&heap,
			D3D12_HEAP_FLAG_NONE,
			&desc,
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(dpara.mDivideReadBuffer[i].GetAddressOf()));
		if (FAILED(hr)) {
			ErrorMessage("PolygonData::createDivideBuffer Error!!");
		}
	}
}

bool PolygonData::Create(bool light, int tNo, int nortNo, int spetNo, bool blend, bool alpha) {
	dpara.material[0].diftex_no = tNo;
	dpara.material[0].nortex_no = nortNo;
	dpara.material[0].spetex_no = spetNo;
	GetShaderByteCode(light, tNo);
	mObjectCB1->CopyData(0, sg);

	if (hs) {
		createDivideBuffer();
	}
	createDefaultBuffer(ver, index, true);
	if (dx->DXR_CreateResource) {
		createParameterDXR(alpha);
		setColorDXR(0, sg);
	}
	const int numSrvTex = 3;
	const int numCbv = 2;
	int numUav = 0;
	if (hs)numUav = 1;
	if (tNo == -1 && !movOn[0].m_on) {
		if (!createPSO(dx->pVertexLayout_3DBC, numSrvTex, numCbv, numUav, blend, alpha))return false;
	}
	else {
		if (!createPSO(dx->pVertexLayout_MESH, numSrvTex, numCbv, numUav, blend, alpha))return false;
	}

	if (dx->DXR_CreateResource) {
		if (tNo == -1 && !movOn[0].m_on) {
			if (!createPSO_DXR(dx->pVertexLayout_3DBC, numSrvTex, numCbv, numUav))return false;
		}
		else {
			if (!createPSO_DXR(dx->pVertexLayout_MESH, numSrvTex, numCbv, numUav))return false;
		}
	}

	return setDescHeap(numSrvTex, 0, nullptr, nullptr, numCbv, 0, 0);
}

void PolygonData::Instancing(VECTOR3 pos, VECTOR3 angle, VECTOR3 size) {
	dx->Instancing(ins_no, &cb[dx->cBuffSwap[0]], pos, angle, size);
}

void PolygonData::CbSwap() {
	firstCbSet[dx->cBuffSwap[0]] = true;
	insNum[dx->cBuffSwap[0]] = ins_no;
	ins_no = 0;
	DrawOn = true;
}

void PolygonData::InstancingUpdate(VECTOR4 Color, float disp, float shininess, float px, float py, float mx, float my) {
	dx->InstancingUpdate(&cb[dx->cBuffSwap[0]], Color, disp, px, py, mx, my, divArr, numDiv, shininess);
	CbSwap();
}

void PolygonData::Update(VECTOR3 pos, VECTOR4 Color, VECTOR3 angle, VECTOR3 size,
	float disp, float shininess, float px, float py, float mx, float my) {

	dx->Instancing(ins_no, &cb[dx->cBuffSwap[0]], pos, angle, size);
	dx->InstancingUpdate(&cb[dx->cBuffSwap[0]], Color, disp, px, py, mx, my, divArr, numDiv, shininess);
	CbSwap();
}

void PolygonData::DrawOff() {
	DrawOn = false;
}

void PolygonData::draw(int com, drawPara& para) {

	ID3D12GraphicsCommandList* mCList = dx->dx_sub[com].mCommandList.Get();

	ID3D12DescriptorHeap* descriptorHeaps[] = { para.descHeap.Get() };
	mCList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
	mCList->SetGraphicsRootSignature(para.rootSignature.Get());
	mCList->IASetVertexBuffers(0, 1, &(para.Vview)->VertexBufferView());

	D3D12_GPU_DESCRIPTOR_HANDLE heap(para.descHeap.Get()->GetGPUDescriptorHandleForHeapStart());
	for (int i = 0; i < para.NumMaterial; i++) {
		//�g�p����Ă��Ȃ��}�e���A���΍�
		if (para.Iview[i].IndexCount <= 0)continue;

		mCList->IASetIndexBuffer(&(para.Iview[i]).IndexBufferView());
		mCList->IASetPrimitiveTopology(para.TOPOLOGY);
		mCList->SetPipelineState(para.PSO[i].Get());
		mCList->SetGraphicsRootDescriptorTable(0, heap);
		heap.ptr += dx->mCbvSrvUavDescriptorSize * para.numDesc;
		mCList->DrawIndexedInstanced(para.Iview[i].IndexCount, para.insNum, 0, 0, 0);
	}
}

void PolygonData::streamOutput(int com, drawPara& para, ParameterDXR& dxr) {

	ID3D12GraphicsCommandList* mCList = dx->dx_sub[com].mCommandList.Get();
	ID3D12DescriptorHeap* descriptorHeaps[] = { para.descHeap.Get() };
	D3D12_GPU_DESCRIPTOR_HANDLE heap(para.descHeap.Get()->GetGPUDescriptorHandleForHeapStart());
	for (int i = 0; i < para.NumMaterial; i++) {
		//�g�p����Ă��Ȃ��}�e���A���΍�
		if (para.Iview[i].IndexCount <= 0)continue;

		dx->Bigin(com);

		mCList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
		mCList->SetGraphicsRootSignature(para.rootSignatureDXR.Get());
		mCList->IASetVertexBuffers(0, 1, &(para.Vview)->VertexBufferView());
		mCList->IASetIndexBuffer(&(para.Iview[i]).IndexBufferView());
		mCList->IASetPrimitiveTopology(para.TOPOLOGY);
		mCList->SetPipelineState(para.PSO_DXR[i].Get());
		mCList->SetGraphicsRootDescriptorTable(0, heap);
		heap.ptr += dx->mCbvSrvUavDescriptorSize * para.numDesc;

		UpdateDXR& ud = dxr.updateDXR[dx->dxrBuffSwap[0]];
		mCList->SOSetTargets(0, 1, &dxr.SviewDXR[i].StreamBufferView());

		mCList->DrawIndexedInstanced(para.Iview[i].IndexCount, para.insNum, 0, 0, 0);

		dx->dx_sub[com].ResourceBarrier(ud.VviewDXR[i].VertexBufferGPU.Get(),
			D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COPY_DEST);
		dx->dx_sub[com].ResourceBarrier(dxr.SviewDXR[i].StreamBufferGPU.Get(),
			D3D12_RESOURCE_STATE_STREAM_OUT, D3D12_RESOURCE_STATE_COPY_SOURCE);

		mCList->CopyResource(ud.VviewDXR[i].VertexBufferGPU.Get(),
			dxr.SviewDXR[i].StreamBufferGPU.Get());

		dx->dx_sub[com].ResourceBarrier(ud.VviewDXR[i].VertexBufferGPU.Get(),
			D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);
		dx->dx_sub[com].ResourceBarrier(dxr.SviewDXR[i].StreamBufferGPU.Get(),
			D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_STREAM_OUT);

		mCList->SOSetTargets(0, 1, nullptr);

		ud.firstSet = true;

		dx->End(com);
		dx->WaitFence();

		if (hs) {
			dx->Bigin(com);
			dx->dx_sub[com].ResourceBarrier(dpara.mDivideBuffer[i].Get(),
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
			mCList->CopyResource(dpara.mDivideReadBuffer[i].Get(), dpara.mDivideBuffer[i].Get());
			dx->dx_sub[com].ResourceBarrier(dpara.mDivideBuffer[i].Get(),
				D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			dx->End(com);
			dx->WaitFence();

			D3D12_RANGE range;
			range.Begin = 0;
			UINT numPo = dpara.Iview[i].IndexCount / 3;
			range.End = numPo * sizeof(UINT);
			UINT* div = nullptr;
			dpara.mDivideReadBuffer[i].Get()->Map(0, &range, reinterpret_cast<void**>(&div));
			const UINT minDiv = 2;
			const UINT minDivPolygon = 6;
			UINT verCnt = 0;
			for (UINT d = 0; d < numPo; d++) {
				UINT mag = div[d] / minDiv;
				UINT ver = mag * mag * minDivPolygon * 3;
				verCnt += ver;
			}
			dpara.mDivideReadBuffer[i].Get()->Unmap(0, nullptr);
			ud.currentIndexCount[i] = verCnt;
		}
	}
}

void PolygonData::ParameterDXR_Update() {
	UpdateDXR& ud = dxrPara.updateDXR[dx->dxrBuffSwap[0]];
	ud.NumInstance = dpara.insNum;
	ud.shininess = cb[dx->cBuffSwap[1]].DispAmount.z;
	memcpy(&ud.AddObjColor, &cb[dx->cBuffSwap[1]].AddObjColor, sizeof(VECTOR4));
	memcpy(ud.Transform,
		&cb[dx->cBuffSwap[1]].World, sizeof(MATRIX) * ud.NumInstance);
}

void PolygonData::Draw(int com) {
	if (!firstCbSet[dx->cBuffSwap[1]] | !DrawOn)return;

	mObjectCB->CopyData(0, cb[dx->cBuffSwap[1]]);
	dpara.insNum = insNum[dx->cBuffSwap[1]];
	draw(com, dpara);
}

void PolygonData::StreamOutput(int com) {
	if (!firstCbSet[dx->cBuffSwap[1]] | !DrawOn)return;

	mObjectCB->CopyData(0, cb[dx->cBuffSwap[1]]);
	dpara.insNum = insNum[dx->cBuffSwap[1]];
	ParameterDXR_Update();
	if (dxrPara.updateF || !dxrPara.updateDXR[dx->dxrBuffSwap[0]].createAS) {
		if (vs == dx->pVertexShader_SKIN.Get())
			sk.Skinning(com);
		else
			streamOutput(com, dpara, dxrPara);
	}
}

void PolygonData::Draw() {
	Draw(com_no);
}

void PolygonData::StreamOutput() {
	StreamOutput(com_no);
}

ParameterDXR* PolygonData::getParameter() {
	for (int i = 0; i < dxrPara.NumMaterial; i++) {
		if (dxrPara.IviewDXR[i].IndexCount <= 0) {
			dxrPara.NumMaterial = i;
			break;
		}
	}
	return &dxrPara;
}