//*****************************************************************************************//
//**                                                                                     **//
//**                   　　　         ParticleDataクラス                                 **//
//**                                  CreateParticle関数                                 **//
//*****************************************************************************************//

#include "Dx_ParticleData.h"
#define _USE_MATH_DEFINES
#include <math.h>

ParticleData::ParticleData() {
	dx = Dx12Process::GetInstance();
	mCommandList = dx->dx_sub[0].mCommandList.Get();
	ver = 0;
	P_pos = nullptr;
}

ParticleData::~ParticleData() {
	S_DELETE(mObjectCB);
	if (P_pos != nullptr) {
		free(P_pos);
		P_pos = nullptr;
	}
}

void ParticleData::GetShaderByteCode() {
	gsSO = dx->pGeometryShader_PSO.Get();
	gs = dx->pGeometryShader_P.Get();
	vsSO = dx->pVertexShader_PSO.Get();
	vs = dx->pVertexShader_P.Get();
	ps = dx->pPixelShader_P.Get();
}

void ParticleData::update(CONSTANT_BUFFER_P* cb, VECTOR3 pos, VECTOR4 color, float angle, float size, float speed) {
	MATRIX mov;
	MATRIX rot;
	MATRIX world;

	MatrixRotationZ(&rot, angle);
	MatrixTranslation(&mov, pos.x, pos.y, pos.z);
	MatrixMultiply(&world, &rot, &mov);
	MatrixMultiply(&cb->WV, &world, &dx->mView);
	cb->Proj = dx->mProj;
	MatrixTranspose(&cb->WV);
	MatrixTranspose(&cb->Proj);
	cb->size.x = size;
	cb->size.z = speed;
	cb->AddObjColor.as(color.x, color.y, color.z, color.w);

	if (dx->DXR_CreateResource) {
		memcpy(&dxrPara.AddObjColor, &cb->AddObjColor, sizeof(VECTOR4));
		MatrixTranspose(&world);
		memcpy(dxrPara.Transform, &world, sizeof(MATRIX) * dxrPara.NumInstance);
		cb->invRot = BillboardAngleCalculation(angle);
		MatrixTranspose(&cb->invRot);
	}
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
				P_pos[P_no].normal.as(0.0f, 0.0f, -1.0f);
				P_no++;
			}
		}
	}
}

void ParticleData::GetBufferParticle(int texture_no, float size, float density) {
	mObjectCB = new ConstantBuffer<CONSTANT_BUFFER_P>(1);
	Vview = std::make_unique<VertexView>();
	Sview1 = std::make_unique<StreamView[]>(2);
	Sview2 = std::make_unique<StreamView[]>(2);
	GetVbColarray(texture_no, size, density);
}

void ParticleData::GetBufferBill(int v) {
	ver = v;
	P_pos = (PartPos*)malloc(sizeof(PartPos) * ver);
	mObjectCB = new ConstantBuffer<CONSTANT_BUFFER_P>(1);
	Vview = std::make_unique<VertexView>();
	Sview1 = std::make_unique<StreamView[]>(2);
	Sview2 = std::make_unique<StreamView[]>(2);
}

void ParticleData::createDxr() {
	dxrPara.NumMaterial = 1;
	dxrPara.NumInstance = 1;
	dxrPara.difTex = std::make_unique<ID3D12Resource* []>(1);
	dxrPara.norTex = std::make_unique<ID3D12Resource* []>(1);
	dxrPara.speTex = std::make_unique<ID3D12Resource* []>(1);
	dxrPara.diffuse = std::make_unique<VECTOR4[]>(1);
	dxrPara.specular = std::make_unique<VECTOR4[]>(1);
	dxrPara.ambient = std::make_unique<VECTOR4[]>(1);
	dxrPara.VviewDXR = std::make_unique<VertexView[]>(1);
	dxrPara.IviewDXR = std::make_unique<IndexView[]>(1);
	dxrPara.VviewDXR = std::make_unique<VertexView[]>(1);
	dxrPara.VviewDXR = std::make_unique<VertexView[]>(1);
	dxrPara.SviewDXR = std::make_unique<StreamView[]>(1);
	dxrPara.SizeLocation = std::make_unique<StreamView[]>(1);
	dxrPara.shininess = 1.0f;
	dxrPara.alphaTest = true;
}

void ParticleData::CreateVbObj() {
	const UINT vbByteSize = ver * sizeof(PartPos);

	Vview->VertexBufferGPU = dx->CreateDefaultBuffer(com_no, P_pos, vbByteSize, Vview->VertexBufferUploader, false);

	Vview->VertexByteStride = sizeof(PartPos);
	Vview->VertexBufferByteSize = vbByteSize;

	for (int i = 0; i < 2; i++) {
		Sview1[i].StreamBufferGPU = dx->CreateStreamBuffer(vbByteSize);
		Sview2[i].StreamBufferGPU = dx->CreateStreamBuffer(vbByteSize);

		Sview1[i].StreamByteStride = Sview2[i].StreamByteStride = sizeof(PartPos);
		Sview1[i].StreamBufferByteSize = Sview2[i].StreamBufferByteSize = vbByteSize;
		Sview1[i].BufferFilledSizeLocation = Sview2[i].StreamBufferGPU->GetGPUVirtualAddress();
	}

	if (dx->DXR_CreateResource) {
		createDxr();
	}
}

bool ParticleData::CreatePartsCom() {

	CD3DX12_ROOT_PARAMETER slotRootParameter_com[1];
	slotRootParameter_com[0].InitAsConstantBufferView(0);

	mRootSignature_com = CreateRsStreamOutput(1, slotRootParameter_com);
	if (mRootSignature_com == nullptr)return false;

	//パイプラインステートオブジェクト生成(STREAM_OUTPUT)
	mPSO_com = CreatePsoStreamOutput(vsSO, nullptr, nullptr, gsSO, mRootSignature_com.Get(),
		dx->pVertexLayout_P, &dx->pDeclaration_PSO,
		(UINT)dx->pDeclaration_PSO.size(),
		&Sview1[0].StreamByteStride,
		1,
		POINt);
	if (mPSO_com == nullptr)return false;

	return true;
}

bool ParticleData::CreatePartsDraw(int texpar) {

	if (texpar != -1)texpar_on = true;

	CD3DX12_DESCRIPTOR_RANGE texTable;
	texTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

	CD3DX12_ROOT_PARAMETER slotRootParameter_draw[2];
	slotRootParameter_draw[0].InitAsDescriptorTable(1, &texTable, D3D12_SHADER_VISIBILITY_ALL);// DescriptorRangeの数は1つ, DescriptorRangeの先頭アドレス
	slotRootParameter_draw[1].InitAsConstantBufferView(0);

	mRootSignature_draw = CreateRs(2, slotRootParameter_draw);
	if (mRootSignature_draw == nullptr)return false;

	TextureNo te;
	te.diffuse = texpar;
	te.normal = dx->GetTexNumber("dummyNor.");;
	te.specular = dx->GetTexNumber("dummyDifSpe.");

	createTextureResource(0, 1, &te);
	mSrvHeap = dx->CreateDescHeap(1);
	if (mSrvHeap == nullptr)return false;
	CreateSrvTexture(mSrvHeap.Get(), 0, texture->GetAddressOf(), 1);

	//パイプラインステートオブジェクト生成(Draw)
	mPSO_draw = CreatePsoParticle(vs, ps, gs, mRootSignature_draw.Get(), dx->pVertexLayout_P, true, true);
	if (mPSO_draw == nullptr)return false;

	if (dx->DXR_CreateResource) {

		UINT indCnt = ver * 6;
		IndexView& dxI = dxrPara.IviewDXR[0];
		dxI.IndexFormat = DXGI_FORMAT_R32_UINT;
		dxI.IndexBufferByteSize = indCnt * sizeof(UINT);
		dxI.IndexCount = indCnt;
		UINT* ind = new UINT[indCnt];
		for (UINT in = 0; in < indCnt; in++)ind[in] = in;
		dxI.IndexBufferGPU = dx->CreateDefaultBuffer(com_no, ind,
			dxI.IndexBufferByteSize, dxI.IndexBufferUploader, false);
		ARR_DELETE(ind);

		UINT verCnt = ver * 6;
		VertexView& dxV = dxrPara.VviewDXR[0];
		UINT bytesize = verCnt * sizeof(VERTEX_DXR);
		dxV.VertexByteStride = sizeof(VERTEX_DXR);
		dxV.VertexBufferByteSize = bytesize;
		dx->createDefaultResourceBuffer(dxV.VertexBufferGPU.GetAddressOf(),
			dxV.VertexBufferByteSize);
		dx->dx_sub[com_no].ResourceBarrier(dxV.VertexBufferGPU.Get(),
			D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_GENERIC_READ);

		StreamView& dxS = dxrPara.SviewDXR[0];
		dxS.StreamByteStride = sizeof(VERTEX_DXR);
		dxS.StreamBufferByteSize = bytesize;
		dxS.StreamBufferGPU = dx->CreateStreamBuffer(dxS.StreamBufferByteSize);

		StreamView& dxL = dxrPara.SizeLocation[0];
		dxL.StreamByteStride = sizeof(VERTEX_DXR);
		dxL.StreamBufferByteSize = bytesize;
		dxL.StreamBufferGPU = dx->CreateStreamBuffer(dxL.StreamBufferByteSize);

		dxS.BufferFilledSizeLocation = dxL.StreamBufferGPU.Get()->GetGPUVirtualAddress();

		rootSignatureDXR = CreateRsStreamOutput(2, slotRootParameter_draw);
		if (rootSignatureDXR == nullptr)return false;

		dxrPara.difTex[0] = texture[0].Get();
		dxrPara.norTex[0] = texture[1].Get();
		dxrPara.speTex[0] = texture[2].Get();

		gs = dx->pGeometryShader_P_Output.Get();
		PSO_DXR = CreatePsoStreamOutput(vs, nullptr, nullptr, gs, rootSignatureDXR.Get(),
			dx->pVertexLayout_P,
			&dx->pDeclaration_Output,
			(UINT)dx->pDeclaration_Output.size(),
			&dxrPara.SviewDXR[0].StreamByteStride,
			1,
			POINt);

		dxrPara.diffuse[0].as(0.5f, 0.5f, 0.5f, 0.5f);
		dxrPara.specular[0].as(0.5f, 0.5f, 0.5f, 0.5f);
		dxrPara.ambient[0].as(0.0f, 0.0f, 0.0f, 0.0f);
	}

	return true;
}

bool ParticleData::CreateParticle(int texpar) {
	GetShaderByteCode();
	CreateVbObj();
	if (!CreatePartsCom())return false;
	return CreatePartsDraw(texpar);
}

bool ParticleData::CreateBillboard() {
	GetShaderByteCode();
	CreateVbObj();
	return CreatePartsDraw(-1);
}

void ParticleData::DrawParts1(int com) {

	ID3D12GraphicsCommandList* mCList = dx->dx_sub[com].mCommandList.Get();

	mCList->SetPipelineState(mPSO_com.Get());

	mCList->SetGraphicsRootSignature(mRootSignature_com.Get());

	mCList->IASetVertexBuffers(0, 1, &Vview->VertexBufferView());
	mCList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);

	mCList->SetGraphicsRootConstantBufferView(0, mObjectCB->Resource()->GetGPUVirtualAddress());

	mCList->SOSetTargets(0, 1, &Sview1[svInd].StreamBufferView());

	mCList->DrawInstanced(ver, 1, 0, 0);

	svInd = 1 - svInd;
	if (firstDraw) {
		dx->dx_sub[com].ResourceBarrier(Vview->VertexBufferGPU.Get(),
			D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COPY_DEST);
		mCList->CopyResource(Vview->VertexBufferGPU.Get(), Sview1[svInd].StreamBufferGPU.Get());
		dx->dx_sub[com].ResourceBarrier(Vview->VertexBufferGPU.Get(),
			D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);
	}
	mCList->SOSetTargets(0, 1, nullptr);
}

void ParticleData::DrawParts2(int com) {

	ID3D12GraphicsCommandList* mCList = dx->dx_sub[com].mCommandList.Get();

	mCList->SetPipelineState(mPSO_draw.Get());

	ID3D12DescriptorHeap* descriptorHeaps[] = { mSrvHeap.Get() };
	mCList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	mCList->SetGraphicsRootSignature(mRootSignature_draw.Get());

	mCList->IASetVertexBuffers(0, 1, &Vview->VertexBufferView());
	mCList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);

	mCList->SetGraphicsRootDescriptorTable(0, mSrvHeap->GetGPUDescriptorHandleForHeapStart());
	mCList->SetGraphicsRootConstantBufferView(1, mObjectCB->Resource()->GetGPUVirtualAddress());

	mCList->DrawInstanced(ver, 1, 0, 0);
}

void ParticleData::DrawParts2StreamOutput(int com) {

	ID3D12GraphicsCommandList* mCList = dx->dx_sub[com].mCommandList.Get();

	dx->Bigin(com);

	mCList->SetPipelineState(PSO_DXR.Get());

	ID3D12DescriptorHeap* descriptorHeaps[] = { mSrvHeap.Get() };
	mCList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	mCList->SetGraphicsRootSignature(rootSignatureDXR.Get());

	mCList->IASetVertexBuffers(0, 1, &Vview->VertexBufferView());
	mCList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);

	mCList->SetGraphicsRootDescriptorTable(0, mSrvHeap->GetGPUDescriptorHandleForHeapStart());
	mCList->SetGraphicsRootConstantBufferView(1, mObjectCB->Resource()->GetGPUVirtualAddress());

	mCList->SOSetTargets(0, 1, &dxrPara.SviewDXR[0].StreamBufferView());

	mCList->DrawInstanced(ver, 1, 0, 0);

	dx->dx_sub[com].ResourceBarrier(dxrPara.VviewDXR[0].VertexBufferGPU.Get(),
		D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COPY_DEST);
	dx->dx_sub[com].ResourceBarrier(dxrPara.SviewDXR[0].StreamBufferGPU.Get(),
		D3D12_RESOURCE_STATE_STREAM_OUT, D3D12_RESOURCE_STATE_COPY_SOURCE);

	mCList->CopyResource(dxrPara.VviewDXR[0].VertexBufferGPU.Get(), dxrPara.SviewDXR[0].StreamBufferGPU.Get());

	dx->dx_sub[com].ResourceBarrier(dxrPara.VviewDXR[0].VertexBufferGPU.Get(),
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);
	dx->dx_sub[com].ResourceBarrier(dxrPara.SviewDXR[0].StreamBufferGPU.Get(),
		D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_STREAM_OUT);

	mCList->SOSetTargets(0, 1, nullptr);

	dx->End(com);
	dx->WaitFence();
}

void ParticleData::CbSwap(bool init) {
	if (!parInit)parInit = init;//parInit==TRUEの場合まだ初期化終了していない為フラグを書き換えない
	if (!UpOn) {
		upCount++;
		if (upCount > 1)UpOn = true;//cb,2要素初回更新終了
	}
	DrawOn = true;
}

void ParticleData::Update(VECTOR3 pos, VECTOR4 color, float angle, float size, bool init, float speed) {
	update(&cbP[dx->cBuffSwap[0]], pos, color, angle, size, speed);
	CbSwap(init);
}

void ParticleData::DrawOff() {
	DrawOn = FALSE;
}

void ParticleData::Draw(int com) {

	if (!UpOn | !DrawOn)return;

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

void ParticleData::Update(float size, VECTOR4 color) {
	update(&cbP[dx->cBuffSwap[0]], { 0, 0, 0 }, color, 0, size, 1.0f);
	CbSwap(true);
}

void ParticleData::DrawBillboard(int com) {

	if (!UpOn | !DrawOn)return;

	update2(&cbP[dx->cBuffSwap[1]], true);
	mObjectCB->CopyData(0, cbP[dx->cBuffSwap[1]]);

	DrawParts2(com);
}

void ParticleData::DrawBillboard() {
	DrawBillboard(com_no);
}

void ParticleData::SetVertex(int i, VECTOR3 pos, VECTOR3 nor) {
	P_pos[i].CurrentPos.as(pos.x, pos.y, pos.z);
	P_pos[i].normal.as(nor.x, nor.y, nor.z);
}

void ParticleData::StreamOutput(int com) {

	if (!UpOn | !DrawOn)return;

	bool init = parInit;
	//一回のinit == TRUE で二つのstreamOutを初期化
	if (init) { streamInitcount = 1; }
	else {
		if (streamInitcount > 2) { streamInitcount = 0; }
		if (streamInitcount != 0) { init = true; streamInitcount++; }
	}
	update2(&cbP[dx->cBuffSwap[1]], init);
	mObjectCB->CopyData(0, cbP[dx->cBuffSwap[1]]);

	dx->Bigin(com);
	DrawParts1(com);
	dx->End(com);
	dx->WaitFence();
	if (firstDraw)DrawParts2StreamOutput(com);
	firstDraw = true;
	parInit = false;//parInit==TRUEの場合ここに来た時点で初期化終了
}

void ParticleData::StreamOutputBillboard(int com) {

	if (!UpOn | !DrawOn)return;

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

static float AngleCalculation360(float distA, float distB) {
	static const float radian1 = 180.0f / (float)M_PI;
	float angle = (float)atan(distA / distB) * radian1;
	if (distA >= 0 && distB < 0) {
		return angle + 360.0f;
	}
	if (distB >= 0) {
		return angle + 180.0f;
	}
	return angle;
}

MATRIX ParticleData::BillboardAngleCalculation(float angle) {
	//float distanceZ = dx->posZ - dx->dirZ;
	float distanceY = dx->posY - dx->dirY;
	float distanceX = dx->posX - dx->dirX;

	float angleZ = (float)fmod(AngleCalculation360(distanceX, distanceY) + angle, 360.0f);
	//float angleY = AngleCalculation360(distanceZ, distanceX);
	//float angleX = AngleCalculation360(distanceZ, distanceY);

	MATRIX rotZ, inv;
	MatrixRotationZ(&rotZ, angleZ);
	MatrixInverse(&inv, &rotZ);

	return inv;
}