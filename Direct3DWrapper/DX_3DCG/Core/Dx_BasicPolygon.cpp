//*****************************************************************************************//
//**                                                                                     **//
//**                   　　　          BasicPolygon                                      **//
//**                                                                                     **//
//*****************************************************************************************//

#define _CRT_SECURE_NO_WARNINGS
#include "Dx_BasicPolygon.h"

namespace {

	void Instancing_Internal(int& insNum, int numMaxIns, WVP_CB* cbArr,
		CoordTf::VECTOR3 pos, CoordTf::VECTOR3 angle, CoordTf::VECTOR3 size, CoordTf::VECTOR4 Color,
		float px, float py, float mx, float my) {

		if (insNum >= numMaxIns) {
			Dx_Util::ErrorMessage("Error: insNum is greater than numMaxIns.");
		}

		using namespace CoordTf;

		MATRIX world = Dx_Util::calculationMatrixWorld(pos, angle, size);

		Dx_Device* dev = Dx_Device::GetInstance();
		Dx_SwapChain* sw = Dx_SwapChain::GetInstance();
		Dx_SwapChain::Update& u = sw->getUpdate(dev->cBuffSwapUpdateIndex());

		//ワールド、カメラ、射影行列、等
		cbArr[insNum].world = world;
		cbArr[insNum].wvp = world * u.mView * u.mProj;
		MatrixTranspose(&cbArr[insNum].world);
		MatrixTranspose(&cbArr[insNum].wvp);

		cbArr[insNum].AddObjColor.as(Color.x, Color.y, Color.z, Color.w);
		cbArr[insNum].pXpYmXmY = { px, py, mx, my };

		insNum++;
	}

	void InstancingUpdate_Internal(CONSTANT_BUFFER* cb, CONSTANT_BUFFER3* cb3, float disp,
		DivideArr* divArr, int numDiv, float shininess, float SmoothRange, float SmoothRatio) {

		using namespace CoordTf;

		Dx_Device* dev = Dx_Device::GetInstance();
		Dx_SwapChain* sw = Dx_SwapChain::GetInstance();
		Dx_SwapChain::Update& u = sw->getUpdate(dev->cBuffSwapUpdateIndex());
		Dx_Light::Update& ul = Dx_Light::getUpdate(dev->cBuffSwapUpdateIndex());
		PointLight& pl = ul.plight;
		DirectionLight& dl = ul.dlight;
		Fog& fog = ul.fog;

		cb->C_Pos.as(u.pos.x,
			u.pos.y,
			u.pos.z, 0.0f);
		memcpy(&cb3->GlobalAmbientLight, &Dx_Light::getGlobalAmbientLight(), sizeof(VECTOR4));
		cb3->numLight.as((float)pl.LightPcs, 0.0f, 0.0f, 0.0f);
		memcpy(cb3->pLightPos, pl.LightPos, sizeof(VECTOR4) * LIGHT_PCS);
		memcpy(cb3->pLightColor, pl.LightColor, sizeof(VECTOR4) * LIGHT_PCS);
		memcpy(cb3->pLightst, pl.Lightst, sizeof(VECTOR4) * LIGHT_PCS);
		cb3->dDirection = dl.Direction;
		cb3->dLightColor = dl.LightColor;
		cb3->dLightst.x = dl.onoff;
		cb3->FogAmo_Density.as(fog.Amount, fog.Density, fog.on_off, 0.0f);
		cb3->FogColor = fog.FogColor;
		cb->DispAmount.as(disp, float(numDiv), shininess, SmoothRange);
		cb->SmoothRatio.as(SmoothRatio, 0.0f, 0.0f, 0.0f);
		for (int i = 0; i < numDiv; i++) {
			cb->Divide[i].as(divArr[i].distance, divArr[i].divide, 0.0f, 0.0f);
		}
	}
}

BasicPolygon::BasicPolygon() {
	sg.vDiffuse.x = 1.0f;
	sg.vDiffuse.y = 1.0f;
	sg.vDiffuse.z = 1.0f;
	sg.vDiffuse.w = 0.0f;

	sg.vSpeculer.x = 0.0f;
	sg.vSpeculer.y = 0.0f;
	sg.vSpeculer.z = 0.0f;
	sg.vSpeculer.w = 0.0f;

	sg.vAmbient.x = 0.0f;
	sg.vAmbient.y = 0.0f;
	sg.vAmbient.z = 0.0f;
	sg.vAmbient.w = 0.0f;

	divArr[0].distance = 1000.0f;
	divArr[0].divide = 2;//頂点数 3 → 3 * 6 = 18
	divArr[1].distance = 500.0f;
	divArr[1].divide = 48;//頂点数 3 → 3 * 3456 = 10368
	divArr[2].distance = 300.0f;
	divArr[2].divide = 64;//頂点数 3 → 3 * 13824 = 41472
	numDiv = 3;

	firstCbSet[0] = false;
	firstCbSet[1] = false;
}

BasicPolygon::~BasicPolygon() {
	S_DELETE(mObjectCB);
	S_DELETE(mObjectCB2);
	S_DELETE(mObjectCB3);
	S_DELETE(mObjectCB_Ins);
	S_DELETE(wvp);
}

void BasicPolygon::GetShaderByteCode(PrimitiveType type, bool light, bool smooth, bool BC_On,
	ID3DBlob* changeVs, ID3DBlob* changeDs) {

	primType_create = type;
	if (primType_create == SQUARE) {
		dpara.TOPOLOGY = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	}
	if (primType_create == POINt) {
		dpara.TOPOLOGY = D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;
	}
	if (primType_create == LINE_L) {
		dpara.TOPOLOGY = D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
	}
	if (primType_create == LINE_S) {
		dpara.TOPOLOGY = D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP;
	}
	if (primType_create == CONTROL_POINT) {
		dpara.TOPOLOGY = D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST;
	}

	bool disp = false;
	if (primType_create == CONTROL_POINT)disp = true;
	if (BC_On) {
		vs = Dx_ShaderHolder::pVertexShader_BC.Get();
		ps = Dx_ShaderHolder::pPixelShader_BC.Get();
		ps_NoMap = Dx_ShaderHolder::pPixelShader_BC.Get();
		return;
	}

	if (disp) {
		vs = Dx_ShaderHolder::pVertexShader_MESH_D.Get();
		hs = Dx_ShaderHolder::pHullShaderTriangle.Get();
		ds = Dx_ShaderHolder::pDomainShaderTriangle.Get();
		if (smooth) {
			gs = Dx_ShaderHolder::pGeometryShader_Before_ds_Smooth.Get();
			gs_NoMap = Dx_ShaderHolder::pGeometryShader_Before_ds_NoNormalMap_Smooth.Get();
		}
		else {
			gs = Dx_ShaderHolder::pGeometryShader_Before_ds_Edge.Get();
			gs_NoMap = Dx_ShaderHolder::pGeometryShader_Before_ds_NoNormalMap_Edge.Get();
		}
	}
	else {
		vs = Dx_ShaderHolder::pVertexShader_MESH[0].Get();
		gs = Dx_ShaderHolder::pGeometryShader_Before_vs.Get();
		gs_NoMap = Dx_ShaderHolder::pGeometryShader_Before_vs_NoNormalMap.Get();
	}

	if (light) {
		ps = Dx_ShaderHolder::pPixelShader_3D.Get();
		ps_NoMap = Dx_ShaderHolder::pPixelShader_3D_NoNormalMap.Get();
	}
	else {
		ps = Dx_ShaderHolder::pPixelShader_Emissive.Get();
		ps_NoMap = Dx_ShaderHolder::pPixelShader_Emissive.Get();
	}

	if (changeVs)vs = changeVs;
	if (changeDs)ds = changeDs;

	if (disp) {
		dxrPara.hs = true;
	}
}

void BasicPolygon::createBufferDXR(int numMaterial, int numMaxInstance) {
	dxrPara.create(numMaterial, numMaxInstance);
}

void BasicPolygon::setTextureDXR() {

	for (int i = 0; i < dpara.NumMaterial; i++) {
		dxrPara.difTex[i] = texture[i * 3 + 0];
		dxrPara.norTex[i] = texture[i * 3 + 1];
		dxrPara.speTex[i] = texture[i * 3 + 2];
	}
}

void BasicPolygon::CbSwap() {
	Dx_Device* dev = Dx_Device::GetInstance();
	firstCbSet[dev->cBuffSwapUpdateIndex()] = true;
	insNum[dev->cBuffSwapUpdateIndex()] = ins_no;
	ins_no = 0;
	DrawOn = true;
}

void BasicPolygon::draw(int comIndex, drawPara& para) {

	Dx_Device* dev = Dx_Device::GetInstance();
	Dx_CommandListObj* cObj = Dx_CommandManager::GetInstance()->getGraphicsComListObj(comIndex);

	ID3D12GraphicsCommandList* mCList = cObj->getCommandList();
	ID3D12DescriptorHeap* descriptorHeaps[] = { para.descHeap.Get() };
	mCList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
	mCList->SetGraphicsRootSignature(para.rootSignature.Get());
	mCList->IASetVertexBuffers(0, 1, &(para.Vview)->VertexBufferView());
	cbInstanceID cI = {};
	cI.instanceID.as(0.0f, 0.0f, 0.0f, 0.0f);
	mObjectCB_Ins->CopyData(0, cI);
	mCList->SetGraphicsRootConstantBufferView(3, mObjectCB_Ins->Resource()->GetGPUVirtualAddress());
	mCList->SetGraphicsRootConstantBufferView(2, mObjectCB3->Resource()->GetGPUVirtualAddress());

	D3D12_GPU_DESCRIPTOR_HANDLE heap(para.descHeap.Get()->GetGPUDescriptorHandleForHeapStart());
	for (int i = 0; i < para.NumMaterial; i++) {
		//使用されていないマテリアル対策
		if (para.Iview[i].IndexCount <= 0)continue;

		mCList->IASetIndexBuffer(&(para.Iview[i]).IndexBufferView());
		mCList->IASetPrimitiveTopology(para.TOPOLOGY);
		mCList->SetPipelineState(para.PSO[i].Get());
		mCList->SetGraphicsRootDescriptorTable(0, heap);
		heap.ptr += dev->getCbvSrvUavDescriptorSize() * para.numDesc;
		mCList->SetGraphicsRootConstantBufferView(1, mObjectCB2->getGPUVirtualAddress(i));
		mCList->DrawIndexedInstanced(para.Iview[i].IndexCount, para.insNum, 0, 0, 0);
	}
}

void BasicPolygon::ParameterDXR_Update() {
	Dx_Device* dev = Dx_Device::GetInstance();
	UpdateDXR& ud = dxrPara.updateDXR[dev->dxrBuffSwapIndex()];
	ud.NumInstance = dpara.insNum;
	ud.shininess = cb[dev->cBuffSwapDrawOrStreamoutputIndex()].DispAmount.z;
	for (UINT i = 0; i < ud.NumInstance; i++) {
		ud.Transform[i] = cbWVP[dev->cBuffSwapDrawOrStreamoutputIndex()][i].world;
		ud.WVP[i] = cbWVP[dev->cBuffSwapDrawOrStreamoutputIndex()][i].wvp;
		ud.AddObjColor[i] = cbWVP[dev->cBuffSwapDrawOrStreamoutputIndex()][i].AddObjColor;
		ud.pXpYmXmY[i] = cbWVP[dev->cBuffSwapDrawOrStreamoutputIndex()][i].pXpYmXmY;
	}
}

void BasicPolygon::streamOutput(int comIndex, drawPara& para, ParameterDXR& dxr) {

	Dx_CommandListObj& d = *Dx_CommandManager::GetInstance()->getGraphicsComListObj(comIndex);

	ID3D12GraphicsCommandList* mCList = d.getCommandList();

	Dx_Device* dev = Dx_Device::GetInstance();

	ID3D12DescriptorHeap* descriptorHeaps[] = { para.descHeap.Get() };
	UpdateDXR& ud = dxr.updateDXR[dev->dxrBuffSwapIndex()];
	mCList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
	mCList->SetGraphicsRootSignature(para.rootSignatureDXR.Get());
	mCList->IASetVertexBuffers(0, 1, &(para.Vview)->VertexBufferView());
	cbInstanceID cI = {};
	cI.instanceID.as(0.0f, 0.0f, 0.0f, 0.0f);
	mObjectCB_Ins->CopyData(0, cI);
	mCList->SetGraphicsRootConstantBufferView(3, mObjectCB_Ins->Resource()->GetGPUVirtualAddress());

	D3D12_GPU_DESCRIPTOR_HANDLE heap(para.descHeap.Get()->GetGPUDescriptorHandleForHeapStart());
	for (int i = 0; i < para.NumMaterial; i++) {
		//使用されていないマテリアル対策
		if (para.Iview[i].IndexCount <= 0)continue;

		mCList->IASetIndexBuffer(&(para.Iview[i]).IndexBufferView());
		mCList->IASetPrimitiveTopology(para.TOPOLOGY);
		mCList->SetPipelineState(para.PSO_DXR[i].Get());
		int loop = 1;
		if (dxrPara.hs) {
			loop = ud.NumInstance;
		}
		for (int t = 0; t < loop; t++) {

			if (dxrPara.hs) {
				cbInstanceID cI = {};
				cI.instanceID.as((float)t, 1.0f, 0.0f, 0.0f);
				mObjectCB_Ins->CopyData(t, cI);
				mCList->SetGraphicsRootConstantBufferView(3, mObjectCB_Ins->getGPUVirtualAddress(t));
			}

			mCList->SetGraphicsRootDescriptorTable(0, heap);
			mCList->SOSetTargets(0, 1, &dxr.SviewDXR[i][t].StreamBufferView());
			mCList->DrawIndexedInstanced(para.Iview[i].IndexCount, 1, 0, 0, 0);

			ud.VviewDXR[i][t].VertexBufferGPU.delayCopyResource(comIndex, &dxr.SviewDXR[i][t].StreamBufferGPU);

			dxr.SviewDXR[i][t].outputReadBack(comIndex);
			dxr.SviewDXR[i][t].ResetFilledSizeBuffer(comIndex);
		}
		heap.ptr += dev->getCbvSrvUavDescriptorSize() * para.numDesc;
		ud.firstSet = true;
	}
}

void BasicPolygon::getBuffer(int numMaterial, int numMaxInstance, DivideArr* divarr, int numdiv) {
	if (numdiv > 0) {
		numDiv = numdiv;
		memcpy(divArr, divarr, sizeof(DivideArr) * numdiv);
	}

	Dx_Device* dev = Dx_Device::GetInstance();

	dpara.NumMaxInstance = numMaxInstance;
	cbWVP[0] = std::make_unique<WVP_CB[]>(numMaxInstance);
	cbWVP[1] = std::make_unique<WVP_CB[]>(numMaxInstance);
	mObjectCB = NEW ConstantBuffer<CONSTANT_BUFFER>(1);
	dpara.NumMaterial = numMaterial;
	mObjectCB2 = NEW ConstantBuffer<CONSTANT_BUFFER2>(dpara.NumMaterial);
	mObjectCB3 = NEW ConstantBuffer<CONSTANT_BUFFER3>(1);
	mObjectCB_Ins = NEW ConstantBuffer<cbInstanceID>(dpara.NumMaterial * numMaxInstance);
	wvp = NEW ConstantBuffer<WVP_CB>(numMaxInstance);

	dpara.material = std::make_unique<MY_MATERIAL_S[]>(dpara.NumMaterial);
	dpara.PSO = std::make_unique<ComPtr<ID3D12PipelineState>[]>(dpara.NumMaterial);
	dpara.Iview = std::make_unique<IndexView[]>(dpara.NumMaterial);
	if (dev->getDxrCreateResourceState()) {
		dpara.PSO_DXR = std::make_unique<ComPtr<ID3D12PipelineState>[]>(dpara.NumMaterial);
		createBufferDXR(dpara.NumMaterial, numMaxInstance);
	}
}

void BasicPolygon::getVertexBuffer(UINT VertexByteStride, UINT numVertex) {
	dpara.Vview = std::make_unique<VertexView>();
	const UINT byteSize = VertexByteStride * numVertex;
	dpara.Vview->VertexByteStride = VertexByteStride;
	dpara.Vview->VertexBufferByteSize = byteSize;
}

void BasicPolygon::getIndexBuffer(int materialIndex, UINT IndexBufferByteSize, UINT numIndex) {
	dpara.Iview[materialIndex].IndexFormat = DXGI_FORMAT_R32_UINT;
	dpara.Iview[materialIndex].IndexBufferByteSize = IndexBufferByteSize;
	dpara.Iview[materialIndex].IndexCount = numIndex;
}

void BasicPolygon::setColorDXR(int materialIndex, CONSTANT_BUFFER2& sg) {

	dxrPara.diffuse[materialIndex].as(sg.vDiffuse.x, sg.vDiffuse.y, sg.vDiffuse.z, sg.vDiffuse.w);
	dxrPara.specular[materialIndex].as(sg.vSpeculer.x, sg.vSpeculer.y, sg.vSpeculer.z, sg.vSpeculer.w);
	dxrPara.ambient[materialIndex].as(sg.vAmbient.x, sg.vAmbient.y, sg.vAmbient.z, sg.vAmbient.w);
}

void BasicPolygon::createDefaultBuffer(int comIndex, void* vertexArr, UINT** indexArr) {
	dpara.Vview->VertexBufferGPU.CreateDefaultBuffer(comIndex, vertexArr,
		dpara.Vview->VertexBufferByteSize, false);

	for (int i = 0; i < dpara.NumMaterial; i++) {
		if (dpara.Iview[i].IndexCount <= 0)continue;
		dpara.Iview[i].IndexBufferGPU.CreateDefaultBuffer(comIndex, indexArr[i],
			dpara.Iview[i].IndexBufferByteSize, false);
	}
}

bool BasicPolygon::createPSO(std::vector<D3D12_INPUT_ELEMENT_DESC>& vertexLayout,
	const int numSrv, const int numCbv, const int numUav, bool blend, bool alpha) {

	//パイプラインステートオブジェクト生成
	UINT numDescriptors[1] = {};
	numDescriptors[0] = dpara.NumMaxInstance;
	dpara.rootSignature = CreateRootSignature(numSrv, numCbv, numUav, 3, 2, 1, numDescriptors);
	if (dpara.rootSignature == nullptr) {
		Dx_Util::ErrorMessage("BasicPolygon::createPSO Error");
		return false;
	}

	for (int i = 0; i < dpara.NumMaterial; i++) {
		if (dpara.material[i].nortex_no < 0)
			dpara.PSO[i] = CreatePsoVsHsDsPs(vs, hs, ds, ps_NoMap, gs_NoMap, dpara.rootSignature.Get(),
				vertexLayout, alpha, blend, primType_create);
		else
			dpara.PSO[i] = CreatePsoVsHsDsPs(vs, hs, ds, ps, gs, dpara.rootSignature.Get(),
				vertexLayout, alpha, blend, primType_create);

		if (dpara.PSO[i] == nullptr) {
			Dx_Util::ErrorMessage("BasicPolygon::createPSO Error");
			return false;
		}
	}

	return true;
}

bool BasicPolygon::createTexResource(int comIndex) {

	Dx_TextureHolder* dx = Dx_TextureHolder::GetInstance();
	TextureNo* te = NEW TextureNo[dpara.NumMaterial];
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
	if (FAILED(createTextureResource(comIndex, 0, tCnt, te, objName))) {
		Dx_Util::ErrorMessage("BasicPolygon::createTexResource Error");
		return false;
	}
	ARR_DELETE(te);
	return true;
}

bool BasicPolygon::setDescHeap(const int numSrvTex, const int numSrvBuf,
	ID3D12Resource** buffer, UINT* StructureByteStride,
	const int numCbv, D3D12_GPU_VIRTUAL_ADDRESS ad3, UINT ad3Size) {

	Dx_Device* device = Dx_Device::GetInstance();

	dpara.numDesc = numSrvTex + numSrvBuf + (numCbv + dpara.NumMaxInstance);
	int numHeap = dpara.NumMaterial * dpara.numDesc;
	dpara.descHeap = device->CreateDescHeap(numHeap);
	if (dpara.descHeap == nullptr) {
		Dx_Util::ErrorMessage("BasicPolygon::setDescHeap Error");
		return false;
	}
	const int numMaxCB = 2;
	UINT cbSize[numMaxCB] = {};
	cbSize[0] = mObjectCB->getSizeInBytes();
	cbSize[1] = ad3Size;
	D3D12_CPU_DESCRIPTOR_HANDLE hDescriptor(dpara.descHeap->GetCPUDescriptorHandleForHeapStart());

	for (int m = 0; m < dpara.NumMaterial; m++) {
		Dx_Device* d = device;

		//ConstantBuffer<WVPCB> wvp[] : register(b0, space1)
		for (UINT i = 0; i < dpara.NumMaxInstance; i++) {
			D3D12_GPU_VIRTUAL_ADDRESS ad2[1] = { wvp->getGPUVirtualAddress(i) };
			UINT size2[1] = { wvp->getSizeInBytes() };
			device->CreateCbv(hDescriptor, ad2, size2, 1);
		}

		d->CreateSrvTexture(hDescriptor, &texture[numSrvTex * m], numSrvTex);
		d->CreateSrvBuffer(hDescriptor, buffer, numSrvBuf, StructureByteStride);
		D3D12_GPU_VIRTUAL_ADDRESS ad[numMaxCB] = {};
		//cbuffer global : register(b0, space0)
		ad[0] = mObjectCB->Resource()->GetGPUVirtualAddress();
		//その他ボーン等
		ad[1] = ad3;
		d->CreateCbv(hDescriptor, ad, cbSize, numCbv);
	}
	return true;
}

void BasicPolygon::setParameterDXR(bool alpha) {
	dxrPara.alphaTest = alpha;
	if (hs)dxrPara.tessellationF = true;
}

bool BasicPolygon::createStreamOutputResource(int comIndex, float divideBufferMagnification) {

	int NumMaterial = dxrPara.NumMaterial;

	int numDispPolygon = 1;//テセレーション分割数(1ポリゴン)
	if (hs) {
		float maxDiv = 0.0f;
		const float minDiv = 2.0f;
		float minDivPolygon = 6 * divideBufferMagnification;
		for (int i = 0; i < numDiv; i++) {
			if (divArr[i].divide >= minDiv && (int)divArr[i].divide % 2 == 1)divArr[i].divide += 1.0f;//偶数にする
			if (divArr[i].divide < minDiv)divArr[i].divide = minDiv;
			if (maxDiv < divArr[i].divide)maxDiv = divArr[i].divide;
		}
		int mag = (int)maxDiv / (int)minDiv;
		numDispPolygon = (int)mag * (int)mag * (int)minDivPolygon;//分割最大数(1ポリゴン)
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
		UINT* ind = NEW UINT[indCnt];
		for (UINT in = 0; in < indCnt; in++)ind[in] = in;
		dxI.IndexBufferGPU.CreateDefaultBuffer(comIndex, ind, bytesize, false);
		ARR_DELETE(ind);
		for (UINT t = 0; t < dxrPara.NumMaxInstance; t++) {
			for (int j = 0; j < 2; j++) {
				dxrPara.updateDXR[j].currentIndexCount[i][t] = indCnt;
				VertexView& dxV = dxrPara.updateDXR[j].VviewDXR[i][t];
				bytesize = indCnt * sizeof(VERTEX_DXR);
				dxV.VertexByteStride = sizeof(VERTEX_DXR);
				dxV.VertexBufferByteSize = bytesize;
				if (FAILED(dxV.VertexBufferGPU.createDefaultResourceBuffer(dxV.VertexBufferByteSize))) {
					Dx_Util::ErrorMessage("BasicPolygon::createParameterDXR Error");
					return false;
				}
				dxV.VertexBufferGPU.ResourceBarrier(comIndex, D3D12_RESOURCE_STATE_GENERIC_READ);
			}
			StreamView& dxS = dxrPara.SviewDXR[i][t];
			dxS.StreamByteStride = sizeof(VERTEX_DXR);
			dxS.StreamBufferByteSize = bytesize;
			if (FAILED(dxS.StreamBufferGPU.createDefaultResourceBuffer(dxS.StreamBufferByteSize))) {
				Dx_Util::ErrorMessage("BasicPolygon::createParameterDXR Error");
				return false;
			}
			dxS.StreamBufferGPU.ResourceBarrier(comIndex, D3D12_RESOURCE_STATE_STREAM_OUT);
		}
	}

	return true;
}

bool BasicPolygon::createPSO_DXR(std::vector<D3D12_INPUT_ELEMENT_DESC>& vertexLayout,
	const int numSrv, const int numCbv, const int numUav, bool smooth) {

	UINT numDescriptors[1] = {};
	numDescriptors[0] = dpara.NumMaxInstance;

	if (hs) {
		if (smooth)
			gs = Dx_ShaderHolder::pGeometryShader_Before_ds_Output_Smooth.Get();
		else
			gs = Dx_ShaderHolder::pGeometryShader_Before_ds_Output_Edge.Get();

		dpara.rootSignatureDXR = CreateRootSignatureStreamOutput(numSrv, numCbv, numUav, true, 3, 2, 1, numDescriptors);
	}
	else {
		gs = Dx_ShaderHolder::pGeometryShader_Before_vs_Output.Get();
		dpara.rootSignatureDXR = CreateRootSignatureStreamOutput(numSrv, numCbv, numUav, false, 3, 2, 1, numDescriptors);
	}
	if (dpara.rootSignatureDXR == nullptr) {
		Dx_Util::ErrorMessage("BasicPolygon::createPSO_DXR Error");
		return false;
	}

	ID3DBlob* vs_dxr = vs;
	if (vs == Dx_ShaderHolder::pVertexShader_MESH[0].Get()) {
		vs_dxr = Dx_ShaderHolder::pVertexShader_MESH[1].Get();
	}

	for (int i = 0; i < dpara.NumMaterial; i++) {
		if (dpara.Iview[i].IndexCount <= 0)continue;
		dpara.PSO_DXR[i] = CreatePsoStreamOutput(vs_dxr, hs, ds, gs, dpara.rootSignatureDXR.Get(),
			vertexLayout,
			&Dx_ShaderHolder::pDeclaration_Output,
			(UINT)Dx_ShaderHolder::pDeclaration_Output.size(),
			&dxrPara.SviewDXR[i][0].StreamByteStride,
			1,
			primType_create);

		if (dpara.PSO_DXR[i] == nullptr) {
			Dx_Util::ErrorMessage("BasicPolygon::createPSO_DXR Error");
			return false;
		}
	}

	return true;
}

void BasicPolygon::Instancing(
	CoordTf::VECTOR3 pos, CoordTf::VECTOR3 angle, CoordTf::VECTOR3 size, CoordTf::VECTOR4 Color,
	float px, float py, float mx, float my) {

	Dx_Device* dev = Dx_Device::GetInstance();
	dxrPara.createOutlineSize(dev->cBuffSwapUpdateIndex(), size, ins_no);
	Instancing_Internal(ins_no, dpara.NumMaxInstance, cbWVP[dev->cBuffSwapUpdateIndex()].get(), pos, angle, size, Color, px, py, mx, my);
}

void BasicPolygon::InstancingUpdate(float disp, float SmoothRange, float SmoothRatio, float shininess) {

	Dx_Device* dev = Dx_Device::GetInstance();
	InstancingUpdate_Internal(&cb[dev->cBuffSwapUpdateIndex()], &cb3[dev->cBuffSwapUpdateIndex()],
		disp, divArr, numDiv, shininess, SmoothRange, SmoothRatio);

	CbSwap();
}

void BasicPolygon::Update(
	CoordTf::VECTOR3 pos, CoordTf::VECTOR3 angle, CoordTf::VECTOR3 size, CoordTf::VECTOR4 Color,
	float disp, float SmoothRange, float SmoothRatio, float shininess,
	float px, float py, float mx, float my) {

	Instancing(pos, angle, size, Color, px, py, mx, my);
	InstancingUpdate(disp, SmoothRange, SmoothRatio, shininess);
}

void BasicPolygon::Draw(int comIndex) {
	Dx_Device* dev = Dx_Device::GetInstance();
	if (!firstCbSet[dev->cBuffSwapDrawOrStreamoutputIndex()] || !DrawOn)return;

	mObjectCB->CopyData(0, cb[dev->cBuffSwapDrawOrStreamoutputIndex()]);
	mObjectCB3->CopyData(0, cb3[dev->cBuffSwapDrawOrStreamoutputIndex()]);
	dpara.insNum = insNum[dev->cBuffSwapDrawOrStreamoutputIndex()];
	for (UINT i = 0; i < dpara.insNum; i++) {
		wvp->CopyData(i, cbWVP[dev->cBuffSwapDrawOrStreamoutputIndex()][i]);
	}
	draw(comIndex, dpara);
}

void BasicPolygon::StreamOutput(int comIndex) {

	Dx_Device* dev = Dx_Device::GetInstance();

	UpdateDXR& ud = dxrPara.updateDXR[dev->dxrBuffSwapIndex()];
	ud.InstanceMaskChange(DrawOn);

	if (!firstCbSet[dev->cBuffSwapDrawOrStreamoutputIndex()])return;

	mObjectCB->CopyData(0, cb[dev->cBuffSwapDrawOrStreamoutputIndex()]);
	dpara.insNum = insNum[dev->cBuffSwapDrawOrStreamoutputIndex()];
	for (UINT i = 0; i < dpara.insNum; i++) {
		wvp->CopyData(i, cbWVP[dev->cBuffSwapDrawOrStreamoutputIndex()][i]);
	}
	ParameterDXR_Update();
	if (dxrPara.updateF || !dxrPara.updateDXR[dev->dxrBuffSwapIndex()].createAS) {
		streamOutput(comIndex, dpara, dxrPara);
	}
}

void BasicPolygon::DrawOff() {
	DrawOn = false;
}

ParameterDXR* BasicPolygon::getParameter() {
	for (int i = 0; i < dxrPara.NumMaterial; i++) {
		if (dxrPara.IviewDXR[i].IndexCount <= 0) {
			dxrPara.NumMaterial = i;
			break;
		}
	}
	return &dxrPara;
}

void BasicPolygon::UpdateDxrDivideBuffer() {
	if (hs) {
		Dx_Device* dev = Dx_Device::GetInstance();
		UpdateDXR& ud = dxrPara.updateDXR[dev->dxrBuffSwapIndex()];
		for (int i = 0; i < dpara.NumMaterial; i++) {
			//使用されていないマテリアル対策
			if (dpara.Iview[i].IndexCount <= 0)continue;
			for (UINT t = 0; t < ud.NumInstance; t++) {
				dxrPara.SviewDXR[i][t].outputFilledSize();
				UINT sCnt = dxrPara.SviewDXR[i][t].FilledSize / sizeof(VERTEX_DXR);
				ud.currentIndexCount[i][t] = sCnt;
			}
		}
	}
}