//*****************************************************************************************//
//**                                                                                     **//
//**                   　　　         ParticleDataクラス                                 **//
//**                                                                                     **//
//*****************************************************************************************//

#include "Dx_ParticleData.h"
#define _USE_MATH_DEFINES
#include <math.h>

ParticleData::ParticleData() {
	dx = Dx12Process::GetInstance();
	mCommandList = dx->dx_sub[0].mCommandList.Get();
	ver = 0;
	P_pos = nullptr;

	firstCbSet[0] = false;
	firstCbSet[1] = false;
}

ParticleData::~ParticleData() {
	S_DELETE(mObjectCB);
	if (P_pos != nullptr) {
		free(P_pos);
		P_pos = nullptr;
	}
}

void ParticleData::GetShaderByteCode() {
	Dx_ShaderHolder* sh = dx->shaderH.get();
	gsSO = sh->pGeometryShader_PSO.Get();
	gs = sh->pGeometryShader_P.Get();
	vsSO = sh->pVertexShader_PSO.Get();
	vs = sh->pVertexShader_P.Get();
	ps = sh->pPixelShader_P.Get();
}

void ParticleData::update(CONSTANT_BUFFER_P* cb, CoordTf::VECTOR3 pos,
	CoordTf::VECTOR4 color, float angle, float size, float speed) {

	using namespace CoordTf;

	MATRIX mov;
	MATRIX rot;
	MATRIX world;

	MatrixRotationZ(&rot, angle);
	MatrixTranslation(&mov, pos.x, pos.y, pos.z);
	MatrixMultiply(&world, &rot, &mov);
	MatrixMultiply(&cb->WV, &world, &dx->upd[dx->cBuffSwap[0]].mView);
	memcpy(&cb->Proj, &dx->upd[dx->cBuffSwap[0]].mProj, sizeof(MATRIX));

	if (dx->DXR_CreateResource) {
		MATRIX wvp;
		MatrixMultiply(&wvp, &cb->WV, &dx->upd[dx->cBuffSwap[0]].mProj);
		MatrixTranspose(&wvp);

		memcpy(&dxrPara.updateDXR[dx->dxrBuffSwap[0]].AddObjColor, &cb->AddObjColor, sizeof(VECTOR4));
		MatrixTranspose(&world);
		memcpy(dxrPara.updateDXR[dx->dxrBuffSwap[0]].Transform,
			&world, sizeof(MATRIX) * dxrPara.updateDXR[dx->dxrBuffSwap[0]].NumInstance);
		memcpy(dxrPara.updateDXR[dx->dxrBuffSwap[0]].WVP,
			&wvp, sizeof(MATRIX) * dxrPara.updateDXR[dx->dxrBuffSwap[0]].NumInstance);

		cb->invRot = BillboardAngleCalculation(angle);
		MatrixTranspose(&cb->invRot);
	}

	MatrixTranspose(&cb->WV);
	MatrixTranspose(&cb->Proj);
	cb->size.x = size;
	cb->size.z = speed;
	cb->AddObjColor.as(color.x, color.y, color.z, color.w);
}

void ParticleData::update2(CONSTANT_BUFFER_P* cb, bool init) {
	if (init)cb->size.y = 1.0f; else cb->size.y = 0.0f;
}

void ParticleData::GetVbColarray(int texture_no, float size, float density) {

	InternalTexture* tex = &dx->texture[texture_no];

	//テクスチャの横サイズ取得
	float width = (float)tex->width;
	//テクスチャの縦サイズ取得
	float height = (float)tex->height;

	//ステップ数
	float step = 1 / size / density;

	//頂点個数カウント
	UCHAR* ptex = tex->byteArr;
	for (float j = 0; j < height; j += step) {
		UINT j1 = (UINT)j * (UINT)tex->RowPitch;//RowPitchデータの行ピッチ、行幅、または物理サイズ (バイト単位)
		for (float i = 0; i < width; i += step) {
			UINT ptexI = (UINT)i * 4 + j1;
			if (ptex[ptexI + 3] > 0)ver++;//アルファ値0より高い場合カウント
		}
	}

	//パーティクル配列確保
	P_pos = (PartPos*)malloc(sizeof(PartPos) * ver);

	//ピクセルデータ読み込み
	int P_no = 0;
	float ws = width * size / 2;//中心を0,0,0にする為
	float hs = height * size / 2;
	for (float j = 0; j < height; j += step) {
		UINT j1 = (UINT)j * (UINT)tex->RowPitch;
		for (float i = 0; i < width; i += step) {
			UINT ptexI = (UINT)i * 4 + j1;
			float yp = (float)(j * size - hs);
			if (ptex[ptexI + 3] > 0) {
				float xp = (float)(i * size - ws);
				float xst = xp + ((rand() % 500) * size) - ((rand() % 500) * size);
				float yst = yp + ((rand() % 500) * size) - ((rand() % 500) * size);
				float zst = (rand() % 500) * size;
				P_pos[P_no].CurrentPos.as(xst, yst, zst);
				P_pos[P_no].PosSt.as(xst, yst, zst);
				P_pos[P_no].PosEnd.as(xp, yp, 0.0f);//0,0,0を中心にする
				P_no++;
			}
		}
	}
}

void ParticleData::GetBufferParticle(int texture_no, float size, float density) {
	mObjectCB = new ConstantBuffer<CONSTANT_BUFFER_P>(1);
	Vview = std::make_unique<VertexView>();
	Sview = std::make_unique<StreamView[]>(2);
	GetVbColarray(texture_no, size, density);
	if (dx->DXR_CreateResource) {
		dxrPara.create(1, 1);
	}
}

void ParticleData::GetBufferBill(int v) {
	ver = v;
	if (dx->DXR_CreateResource) {
		UpdateDXR& u0 = dxrPara.updateDXR[0];
		UpdateDXR& u1 = dxrPara.updateDXR[1];
		u0.useVertex = true;
		u1.useVertex = true;
		u0.numVertex = ver;
		u1.numVertex = ver;
		u0.v = std::make_unique<CoordTf::VECTOR3[]>(ver);
		u1.v = std::make_unique<CoordTf::VECTOR3[]>(ver);
	}
	P_pos = (PartPos*)malloc(sizeof(PartPos) * ver);
	mObjectCB = new ConstantBuffer<CONSTANT_BUFFER_P>(1);
	Vview = std::make_unique<VertexView>();
	Sview = std::make_unique<StreamView[]>(2);
	if (dx->DXR_CreateResource) {
		dxrPara.create(1, 1);
	}
}

void ParticleData::createDxr(bool alpha, bool blend) {
	dxrPara.updateDXR[0].shininess = 1.0f;
	dxrPara.updateDXR[1].shininess = 1.0f;
	dxrPara.alphaTest = alpha;
	dxrPara.alphaBlend = blend;
}

void ParticleData::CreateVbObj(bool alpha, bool blend) {
	const UINT vbByteSize = ver * sizeof(PartPos);

	Vview->VertexBufferGPU = dx->CreateDefaultBuffer(com_no, P_pos, vbByteSize, Vview->VertexBufferUploader, false);

	Vview->VertexByteStride = sizeof(PartPos);
	Vview->VertexBufferByteSize = vbByteSize;

	for (int i = 0; i < 2; i++) {
		Sview[i].StreamBufferGPU = Dx_Device::GetInstance()->CreateStreamBuffer(vbByteSize);

		Sview[i].StreamByteStride = sizeof(PartPos);
		Sview[i].StreamBufferByteSize = vbByteSize;
	}

	if (dx->DXR_CreateResource) {
		createDxr(alpha, blend);
	}
}

bool ParticleData::CreatePartsCom() {

	D3D12_ROOT_PARAMETER rootParams = {};
	rootParams.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParams.Constants.ShaderRegister = 0;
	rootParams.Constants.RegisterSpace = 0;
	rootParams.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	mRootSignature_com = CreateRsStreamOutput(1, &rootParams);
	if (mRootSignature_com == nullptr)return false;

	//パイプラインステートオブジェクト生成(STREAM_OUTPUT)
	Dx_ShaderHolder* sh = dx->shaderH.get();
	mPSO_com = CreatePsoStreamOutput(vsSO, nullptr, nullptr, gsSO, mRootSignature_com.Get(),
		sh->pVertexLayout_P, &sh->pDeclaration_PSO,
		(UINT)sh->pDeclaration_PSO.size(),
		&Sview[0].StreamByteStride,
		1,
		POINt);
	if (mPSO_com == nullptr)return false;

	return true;
}

bool ParticleData::CreatePartsDraw(int texNo, bool alpha, bool blend) {

	if (texNo != -1)texpar_on = true;

	const int numSrv = 1;
	const int numCbv = 1;

	mRootSignature_draw = CreateRootSignature(numSrv, numCbv, 0, 0, 0);
	if (mRootSignature_draw == nullptr)return false;

	TextureNo te;
	te.diffuse = texNo;
	te.normal = dx->GetTexNumber("dummyNor.");;
	te.specular = dx->GetTexNumber("dummyDifSpe.");

	Dx_Device* device = Dx_Device::GetInstance();
	createTextureResource(0, 1, &te, objName);
	mDescHeap = device->CreateDescHeap(numSrv + numCbv);
	if (mDescHeap == nullptr)return false;
	Dx_Device* d = device;
	D3D12_CPU_DESCRIPTOR_HANDLE hDescriptor(mDescHeap->GetCPUDescriptorHandleForHeapStart());
	d->CreateSrvTexture(hDescriptor, texture[0].GetAddressOf(), 1);
	D3D12_GPU_VIRTUAL_ADDRESS ad = mObjectCB->Resource()->GetGPUVirtualAddress();
	UINT size = mObjectCB->getSizeInBytes();
	d->CreateCbv(hDescriptor, &ad, &size, numCbv);

	Dx_ShaderHolder* sh = dx->shaderH.get();
	//パイプラインステートオブジェクト生成(Draw)
	mPSO_draw = CreatePsoParticle(vs, ps, gs, mRootSignature_draw.Get(), sh->pVertexLayout_P, alpha, blend);
	if (mPSO_draw == nullptr)return false;

	if (dx->DXR_CreateResource) {
		UINT bytesize = 0;
		UINT indCnt = ver * 12;
		IndexView& dxI = dxrPara.IviewDXR[0];
		dxI.IndexFormat = DXGI_FORMAT_R32_UINT;
		dxI.IndexBufferByteSize = indCnt * sizeof(UINT);
		dxI.IndexCount = indCnt;
		UINT* ind = new UINT[indCnt];
		for (UINT in = 0; in < indCnt; in++)ind[in] = in;
		dxI.IndexBufferGPU = dx->CreateDefaultBuffer(com_no, ind,
			dxI.IndexBufferByteSize, dxI.IndexBufferUploader, false);
		ARR_DELETE(ind);
		for (int j = 0; j < 2; j++) {
			dxrPara.updateDXR[j].currentIndexCount[0][0] = indCnt;
			UINT verCnt = ver * 12;
			VertexView& dxV = dxrPara.updateDXR[j].VviewDXR[0][0];
			bytesize = verCnt * sizeof(VERTEX_DXR);
			dxV.VertexByteStride = sizeof(VERTEX_DXR);
			dxV.VertexBufferByteSize = bytesize;
			device->createDefaultResourceBuffer(dxV.VertexBufferGPU.GetAddressOf(),
				dxV.VertexBufferByteSize);
			dx->dx_sub[com_no].ResourceBarrier(dxV.VertexBufferGPU.Get(),
				D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_GENERIC_READ);
		}
		StreamView& dxS = dxrPara.SviewDXR[0][0];
		dxS.StreamByteStride = sizeof(VERTEX_DXR);
		dxS.StreamBufferByteSize = bytesize;
		dxS.StreamBufferGPU = device->CreateStreamBuffer(dxS.StreamBufferByteSize);

		rootSignatureDXR = CreateRootSignatureStreamOutput(1, 1, 0, false, 0, 0);
		if (rootSignatureDXR == nullptr)return false;

		dxrPara.difTex[0] = texture[0].Get();
		dxrPara.norTex[0] = texture[1].Get();
		dxrPara.speTex[0] = texture[2].Get();

		gs = sh->pGeometryShader_P_Output.Get();
		PSO_DXR = CreatePsoStreamOutput(vs, nullptr, nullptr, gs, rootSignatureDXR.Get(),
			sh->pVertexLayout_P,
			&sh->pDeclaration_Output,
			(UINT)sh->pDeclaration_Output.size(),
			&dxrPara.SviewDXR[0][0].StreamByteStride,
			1,
			POINt);

		dxrPara.diffuse[0].as(0.5f, 0.5f, 0.5f, 0.5f);
		dxrPara.specular[0].as(0.5f, 0.5f, 0.5f, 0.5f);
		dxrPara.ambient[0].as(0.0f, 0.0f, 0.0f, 0.0f);
	}

	return true;
}

void ParticleData::setMaterialType(MaterialType type) {
	dxrPara.mType[0] = type;
}

bool ParticleData::CreateParticle(int texNo, bool alpha, bool blend) {
	GetShaderByteCode();
	CreateVbObj(alpha, blend);
	dxrPara.updateF = true;
	if (!CreatePartsCom())return false;
	return CreatePartsDraw(texNo, alpha, blend);
}

bool ParticleData::CreateBillboard(bool alpha, bool blend) {
	GetShaderByteCode();
	CreateVbObj(alpha, blend);
	dxrPara.updateF = true;
	return CreatePartsDraw(-1, alpha, blend);
}

void ParticleData::DrawParts1(int com) {

	ID3D12GraphicsCommandList* mCList = dx->dx_sub[com].mCommandList.Get();

	mCList->SetPipelineState(mPSO_com.Get());

	mCList->SetGraphicsRootSignature(mRootSignature_com.Get());

	mCList->IASetVertexBuffers(0, 1, &Vview->VertexBufferView());
	mCList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);

	mCList->SetGraphicsRootConstantBufferView(0, mObjectCB->Resource()->GetGPUVirtualAddress());

	mCList->SOSetTargets(0, 1, &Sview[svInd].StreamBufferView());

	mCList->DrawInstanced(ver, 1, 0, 0);

	svInd = 1 - svInd;
	if (firstDraw) {
		dx->dx_sub[com].ResourceBarrier(Vview->VertexBufferGPU.Get(),
			D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COPY_DEST);
		mCList->CopyResource(Vview->VertexBufferGPU.Get(), Sview[svInd].StreamBufferGPU.Get());
		dx->dx_sub[com].ResourceBarrier(Vview->VertexBufferGPU.Get(),
			D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);
	}
	Sview[svInd].ResetFilledSizeBuffer(com);
}

void ParticleData::DrawParts2(int com) {

	ID3D12GraphicsCommandList* mCList = dx->dx_sub[com].mCommandList.Get();

	mCList->SetPipelineState(mPSO_draw.Get());

	ID3D12DescriptorHeap* descriptorHeaps[] = { mDescHeap.Get() };
	mCList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	mCList->SetGraphicsRootSignature(mRootSignature_draw.Get());

	mCList->IASetVertexBuffers(0, 1, &Vview->VertexBufferView());
	mCList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);

	mCList->SetGraphicsRootDescriptorTable(0, mDescHeap->GetGPUDescriptorHandleForHeapStart());

	mCList->DrawInstanced(ver, 1, 0, 0);
}

void ParticleData::DrawParts2StreamOutput(int com) {

	ID3D12GraphicsCommandList* mCList = dx->dx_sub[com].mCommandList.Get();
	Dx_CommandListObj& d = dx->dx_sub[com];

	mCList->SetPipelineState(PSO_DXR.Get());

	ID3D12DescriptorHeap* descriptorHeaps[] = { mDescHeap.Get() };
	mCList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	mCList->SetGraphicsRootSignature(rootSignatureDXR.Get());

	mCList->IASetVertexBuffers(0, 1, &Vview->VertexBufferView());
	mCList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);

	mCList->SetGraphicsRootDescriptorTable(0, mDescHeap->GetGPUDescriptorHandleForHeapStart());

	UpdateDXR& ud = dxrPara.updateDXR[dx->dxrBuffSwap[0]];
	mCList->SOSetTargets(0, 1, &dxrPara.SviewDXR[0][0].StreamBufferView());

	mCList->DrawInstanced(ver, 1, 0, 0);

	d.delayResourceBarrierBefore(ud.VviewDXR[0][0].VertexBufferGPU.Get(),
		D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COPY_DEST);
	d.delayResourceBarrierBefore(dxrPara.SviewDXR[0][0].StreamBufferGPU.Get(),
		D3D12_RESOURCE_STATE_STREAM_OUT, D3D12_RESOURCE_STATE_COPY_SOURCE);

	d.delayCopyResource(ud.VviewDXR[0][0].VertexBufferGPU.Get(), dxrPara.SviewDXR[0][0].StreamBufferGPU.Get());

	d.delayResourceBarrierAfter(ud.VviewDXR[0][0].VertexBufferGPU.Get(),
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);
	d.delayResourceBarrierAfter(dxrPara.SviewDXR[0][0].StreamBufferGPU.Get(),
		D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_STREAM_OUT);

	dxrPara.SviewDXR[0][0].ResetFilledSizeBuffer(com);

	ud.firstSet = true;
}

void ParticleData::CbSwap(bool init) {
	if (!parInit)parInit = init;//parInit==TRUEの場合まだ初期化終了していない為フラグを書き換えない

	firstCbSet[dx->cBuffSwap[0]] = true;
	DrawOn = true;
}

void ParticleData::Update(CoordTf::VECTOR3 pos, CoordTf::VECTOR4 color, float angle, float size, bool init, float speed) {
	update(&cbP[dx->cBuffSwap[0]], pos, color, angle, size, speed);
	CbSwap(init);
}

void ParticleData::DrawOff() {
	DrawOn = false;
}

void ParticleData::Draw(int com) {

	if (!firstCbSet[dx->cBuffSwap[1]] | !DrawOn)return;

	bool init = parInit;
	//一回のinit == TRUE で二つのstreamOutを初期化
	if (init) { streamInitcount = 1; }
	else {
		if (streamInitcount > 2) { streamInitcount = 0; }
		if (streamInitcount != 0) { init = true; streamInitcount++; }
	}
	update2(&cbP[dx->cBuffSwap[1]], init);
	mObjectCB->CopyData(0, cbP[dx->cBuffSwap[1]]);

	DrawParts1(com);
	if (firstDraw)DrawParts2(com);
	firstDraw = true;
	parInit = false;//parInit==TRUEの場合ここに来た時点で初期化終了
}

void ParticleData::Draw() {
	Draw(com_no);
}

void ParticleData::Update(float size, CoordTf::VECTOR4 color) {
	update(&cbP[dx->cBuffSwap[0]], { 0, 0, 0 }, color, 0, size, 1.0f);
	CbSwap(true);
}

void ParticleData::DrawBillboard(int com) {

	if (!firstCbSet[dx->cBuffSwap[1]] | !DrawOn)return;

	update2(&cbP[dx->cBuffSwap[1]], true);
	mObjectCB->CopyData(0, cbP[dx->cBuffSwap[1]]);

	DrawParts2(com);
}

void ParticleData::DrawBillboard() {
	DrawBillboard(com_no);
}

void ParticleData::SetVertex(int i, CoordTf::VECTOR3 pos) {
	P_pos[i].CurrentPos.as(pos.x, pos.y, pos.z);
	if (dx->DXR_CreateResource) {
		dxrPara.updateDXR[0].v[i].as(pos.x, pos.y, pos.z);
		dxrPara.updateDXR[1].v[i].as(pos.x, pos.y, pos.z);
	}
}

void ParticleData::StreamOutput(int com) {

	UpdateDXR& ud = dxrPara.updateDXR[dx->dxrBuffSwap[0]];
	ud.InstanceMaskChange(DrawOn);

	if (!firstCbSet[dx->cBuffSwap[1]])return;

	bool init = parInit;
	//一回のinit == TRUE で二つのstreamOutを初期化
	if (init) { streamInitcount = 1; }
	else {
		if (streamInitcount > 2) { streamInitcount = 0; }
		if (streamInitcount != 0) { init = true; streamInitcount++; }
	}
	update2(&cbP[dx->cBuffSwap[1]], init);
	mObjectCB->CopyData(0, cbP[dx->cBuffSwap[1]]);

	DrawParts1(com);

	if (firstDraw)DrawParts2StreamOutput(com);
	firstDraw = true;
	parInit = false;//parInit==TRUEの場合ここに来た時点で初期化終了
}

void ParticleData::StreamOutputBillboard(int com) {

	UpdateDXR& ud = dxrPara.updateDXR[dx->dxrBuffSwap[0]];
	ud.InstanceMaskChange(DrawOn);

	if (!firstCbSet[dx->cBuffSwap[1]])return;

	update2(&cbP[dx->cBuffSwap[1]], true);
	mObjectCB->CopyData(0, cbP[dx->cBuffSwap[1]]);
	DrawParts2StreamOutput(com);
}

void ParticleData::StreamOutput() {
	StreamOutput(com_no);
}

void ParticleData::StreamOutputBillboard() {
	StreamOutputBillboard(com_no);
}

ParameterDXR* ParticleData::getParameter() {
	return &dxrPara;
}

CoordTf::MATRIX ParticleData::BillboardAngleCalculation(float angle) {

	using namespace CoordTf;
	MATRIX rotZ;
	MatrixRotationZ(&rotZ, angle);

	MATRIX vi = dx->upd[dx->cBuffSwap[0]].mView;
	vi._41 = 0.0f;
	vi._42 = 0.0f;
	vi._43 = 0.0f;

	MATRIX rZvi;
	MatrixMultiply(&rZvi, &rotZ, &vi);
	MATRIX inv;
	MatrixInverse(&inv, &rZvi);

	return inv;
}