//*****************************************************************************************//
//**                                                                                     **//
//**                   　　　          PolygonDataクラス                                 **//
//**                                   GetVBarray関数                                    **//
//*****************************************************************************************//

#define _CRT_SECURE_NO_WARNINGS
#include "Dx12ProcessCore.h"

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

	sg.uvSwitch.x = 0.0f;

	divArr[0].distance = 1000.0f;
	divArr[0].divide = 2;//頂点数 3 → 3 * 6 = 18
	divArr[1].distance = 500.0f;
	divArr[1].divide = 48;//頂点数 3 → 3 * 3456 = 10368
	divArr[2].distance = 300.0f;
	divArr[2].divide = 96;//頂点数 3 → 3 * 13824 = 41472
	numDiv = 3;

	firstCbSet[0] = false;
	firstCbSet[1] = false;
}

PolygonData::~PolygonData() {
	S_DELETE(mObjectCB);
	S_DELETE(mObjectCB1);
	S_DELETE(mObjectCB_Ins);
}

ID3D12PipelineState* PolygonData::GetPipelineState() {
	return dpara.PSO[0].Get();
}

void PolygonData::getBuffer(int numMaterial, int numMaxInstance, DivideArr* divarr, int numdiv) {
	if (numdiv > 0) {
		numDiv = numdiv;
		memcpy(divArr, divarr, sizeof(DivideArr) * numdiv);
	}
	mObjectCB = new ConstantBuffer<CONSTANT_BUFFER>(1);
	dpara.NumMaterial = numMaterial;
	mObjectCB1 = new ConstantBuffer<CONSTANT_BUFFER2>(dpara.NumMaterial);
	mObjectCB_Ins = new ConstantBuffer<cbInstanceID>(dpara.NumMaterial * numMaxInstance);
	dpara.material = std::make_unique<MY_MATERIAL_S[]>(dpara.NumMaterial);
	dpara.PSO = std::make_unique<ComPtr<ID3D12PipelineState>[]>(dpara.NumMaterial);
	dpara.Iview = std::make_unique<IndexView[]>(dpara.NumMaterial);
	if (dx->DXR_CreateResource) {
		dpara.PSO_DXR = std::make_unique<ComPtr<ID3D12PipelineState>[]>(dpara.NumMaterial);
		createBufferDXR(dpara.NumMaterial, numMaxInstance);
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

void PolygonData::GetVBarray(PrimitiveType type, int numMaxInstance) {

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

	getBuffer(1, numMaxInstance);
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

	//パイプラインステートオブジェクト生成
	dpara.rootSignature = CreateRootSignature(numSrv, numCbv, numUav, 1, 3);
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

	if (!dx->DXR_CreateResource || vs == dx->pVertexShader_SKIN.Get())return true;

	if (hs) {
		gs = dx->pGeometryShader_Before_ds_Output.Get();
		dpara.rootSignatureDXR = CreateRootSignatureStreamOutput(numSrv, numCbv, numUav, true, 1, 3);
	}
	else {
		gs = dx->pGeometryShader_Before_vs_Output.Get();
		dpara.rootSignatureDXR = CreateRootSignatureStreamOutput(numSrv, numCbv, numUav, false, 1, 3);
	}
	if (dpara.rootSignature == nullptr)return false;

	for (int i = 0; i < dpara.NumMaterial; i++) {
		if (dpara.Iview[i].IndexCount <= 0)continue;
		dpara.PSO_DXR[i] = CreatePsoStreamOutput(vs, hs, ds, gs, dpara.rootSignatureDXR.Get(),
			vertexLayout,
			&dx->pDeclaration_Output,
			(UINT)dx->pDeclaration_Output.size(),
			&dxrPara.SviewDXR[i][0].StreamByteStride,
			1,
			primType_create);
	}

	return true;
}

void PolygonData::setTextureDXR() {
	if (!dx->DXR_CreateResource)return;

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
	setTextureDXR();

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
	}
	return true;
}

void PolygonData::createBufferDXR(int numMaterial, int numMaxInstance) {
	dxrPara.NumMaterial = numMaterial;
	dxrPara.create(numMaterial, numMaxInstance);
}

void PolygonData::createParameterDXR(bool alpha, float divideBufferMagnification) {
	dxrPara.alphaTest = alpha;
	int NumMaterial = dxrPara.NumMaterial;

	if (hs || vs == dx->pVertexShader_SKIN.Get())dxrPara.updateF = true;

	if (!dx->DXR_CreateResource || vs == dx->pVertexShader_SKIN.Get())return;

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
		UINT* ind = new UINT[indCnt];
		for (UINT in = 0; in < indCnt; in++)ind[in] = in;
		dxI.IndexBufferGPU = dx->CreateDefaultBuffer(com_no, ind,
			bytesize, dxI.IndexBufferUploader, false);
		ARR_DELETE(ind);
		for (UINT t = 0; t < dxrPara.NumMaxInstance; t++) {
			for (int j = 0; j < 2; j++) {
				dxrPara.updateDXR[j].currentIndexCount[i][t] = indCnt;
				VertexView& dxV = dxrPara.updateDXR[j].VviewDXR[i][t];
				bytesize = indCnt * sizeof(VERTEX_DXR);
				dxV.VertexByteStride = sizeof(VERTEX_DXR);
				dxV.VertexBufferByteSize = bytesize;
				dx->createDefaultResourceBuffer(dxV.VertexBufferGPU.GetAddressOf(),
					dxV.VertexBufferByteSize, D3D12_RESOURCE_STATE_GENERIC_READ);
			}
			StreamView& dxS = dxrPara.SviewDXR[i][t];
			dxS.StreamByteStride = sizeof(VERTEX_DXR);
			dxS.StreamBufferByteSize = bytesize;
			dxS.StreamBufferGPU = dx->CreateStreamBuffer(dxS.StreamBufferByteSize);
		}
	}
}

void PolygonData::setColorDXR(int materialIndex, CONSTANT_BUFFER2& sg) {
	if (!dx->DXR_CreateResource)return;

	dxrPara.diffuse[materialIndex].as(sg.vDiffuse.x, sg.vDiffuse.y, sg.vDiffuse.z, sg.vDiffuse.w);
	dxrPara.specular[materialIndex].as(sg.vSpeculer.x, sg.vSpeculer.y, sg.vSpeculer.z, sg.vSpeculer.w);
	dxrPara.ambient[materialIndex].as(sg.vAmbient.x, sg.vAmbient.y, sg.vAmbient.z, sg.vAmbient.w);
}

bool PolygonData::Create(bool light, int tNo, int nortNo, int spetNo, bool blend, bool alpha,
	float divideBufferMagnification) {

	dpara.material[0].diftex_no = tNo;
	dpara.material[0].nortex_no = nortNo;
	dpara.material[0].spetex_no = spetNo;
	GetShaderByteCode(light, tNo);
	mObjectCB1->CopyData(0, sg);

	createDefaultBuffer(ver, index, true);
	createParameterDXR(alpha, divideBufferMagnification);
	setColorDXR(0, sg);

	const int numSrvTex = 3;
	const int numCbv = 2;
	int numUav = 0;
	if (tNo == -1 && !movOn[0].m_on) {
		if (!createPSO(dx->pVertexLayout_3DBC, numSrvTex, numCbv, numUav, blend, alpha))return false;
	}
	else {
		if (!createPSO(dx->pVertexLayout_MESH, numSrvTex, numCbv, numUav, blend, alpha))return false;
		if (!createPSO_DXR(dx->pVertexLayout_MESH, numSrvTex, numCbv, numUav))return false;
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
	cbInstanceID cI = {};
	cI.instanceID.as(0.0f, 0.0f, 0.0f, 0.0f);
	mObjectCB_Ins->CopyData(0, cI);
	mCList->SetGraphicsRootConstantBufferView(1, mObjectCB_Ins->Resource()->GetGPUVirtualAddress());

	D3D12_GPU_DESCRIPTOR_HANDLE heap(para.descHeap.Get()->GetGPUDescriptorHandleForHeapStart());
	for (int i = 0; i < para.NumMaterial; i++) {
		//使用されていないマテリアル対策
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
	Dx12Process_sub& d = dx->dx_sub[com];
	ID3D12DescriptorHeap* descriptorHeaps[] = { para.descHeap.Get() };
	UpdateDXR& ud = dxr.updateDXR[dx->dxrBuffSwap[0]];
	mCList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
	mCList->SetGraphicsRootSignature(para.rootSignatureDXR.Get());
	mCList->IASetVertexBuffers(0, 1, &(para.Vview)->VertexBufferView());
	cbInstanceID cI = {};
	cI.instanceID.as(0.0f, 0.0f, 0.0f, 0.0f);
	mObjectCB_Ins->CopyData(0, cI);
	mCList->SetGraphicsRootConstantBufferView(1, mObjectCB_Ins->Resource()->GetGPUVirtualAddress());

	D3D12_GPU_DESCRIPTOR_HANDLE heap(para.descHeap.Get()->GetGPUDescriptorHandleForHeapStart());
	for (int i = 0; i < para.NumMaterial; i++) {
		//使用されていないマテリアル対策
		if (para.Iview[i].IndexCount <= 0)continue;

		mCList->IASetIndexBuffer(&(para.Iview[i]).IndexBufferView());
		mCList->IASetPrimitiveTopology(para.TOPOLOGY);
		mCList->SetPipelineState(para.PSO_DXR[i].Get());
		int loop = 1;
		if (hs) {
			loop = ud.NumInstance;
			dxr.hs = true;
		}
		for (int t = 0; t < loop; t++) {

			if (hs) {
				cbInstanceID cI = {};
				cI.instanceID.as((float)t, 1.0f, 0.0f, 0.0f);
				mObjectCB_Ins->CopyData(t, cI);
				mCList->SetGraphicsRootConstantBufferView(1, mObjectCB_Ins->getGPUVirtualAddress(t));
			}

			mCList->SetGraphicsRootDescriptorTable(0, heap);
			mCList->SOSetTargets(0, 1, &dxr.SviewDXR[i][t].StreamBufferView());
			mCList->DrawIndexedInstanced(para.Iview[i].IndexCount, 1, 0, 0, 0);

			d.delayResourceBarrierBefore(ud.VviewDXR[i][t].VertexBufferGPU.Get(),
				D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COPY_DEST);
			d.delayResourceBarrierBefore(dxr.SviewDXR[i][t].StreamBufferGPU.Get(),
				D3D12_RESOURCE_STATE_STREAM_OUT, D3D12_RESOURCE_STATE_COPY_SOURCE);

			d.delayCopyResource(ud.VviewDXR[i][t].VertexBufferGPU.Get(),
				dxr.SviewDXR[i][t].StreamBufferGPU.Get());

			d.delayResourceBarrierAfter(ud.VviewDXR[i][t].VertexBufferGPU.Get(),
				D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);
			d.delayResourceBarrierAfter(dxr.SviewDXR[i][t].StreamBufferGPU.Get(),
				D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_STREAM_OUT);

			dxr.SviewDXR[i][t].outputReadBack(com);
			dxr.SviewDXR[i][t].ResetFilledSizeBuffer(com);
		}
		heap.ptr += dx->mCbvSrvUavDescriptorSize * para.numDesc;
		ud.firstSet = true;
	}
}

void PolygonData::ParameterDXR_Update() {
	UpdateDXR& ud = dxrPara.updateDXR[dx->dxrBuffSwap[0]];
	ud.NumInstance = dpara.insNum;
	ud.shininess = cb[dx->cBuffSwap[1]].DispAmount.z;
	memcpy(&ud.AddObjColor, &cb[dx->cBuffSwap[1]].AddObjColor, sizeof(VECTOR4));
	memcpy(ud.Transform,
		&cb[dx->cBuffSwap[1]].World, sizeof(MATRIX) * ud.NumInstance);
	memcpy(ud.WVP,
		&cb[dx->cBuffSwap[1]].WVP, sizeof(MATRIX) * ud.NumInstance);
}

void PolygonData::Draw(int com) {
	if (!firstCbSet[dx->cBuffSwap[1]] | !DrawOn)return;

	mObjectCB->CopyData(0, cb[dx->cBuffSwap[1]]);
	dpara.insNum = insNum[dx->cBuffSwap[1]];
	draw(com, dpara);
}

void PolygonData::StreamOutput(int com) {

	if (vs != dx->pVertexShader_SKIN.Get()) {

		UpdateDXR& ud = dxrPara.updateDXR[dx->dxrBuffSwap[0]];
		ud.InstanceMaskChange(DrawOn);

		if (!firstCbSet[dx->cBuffSwap[1]])return;

		mObjectCB->CopyData(0, cb[dx->cBuffSwap[1]]);
		dpara.insNum = insNum[dx->cBuffSwap[1]];
		ParameterDXR_Update();
		if (dxrPara.updateF || !dxrPara.updateDXR[dx->dxrBuffSwap[0]].createAS) {
			streamOutput(com, dpara, dxrPara);
		}
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

void PolygonData::UpdateDxrDivideBuffer() {
	if (hs) {
		UpdateDXR& ud = dxrPara.updateDXR[dx->dxrBuffSwap[0]];
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