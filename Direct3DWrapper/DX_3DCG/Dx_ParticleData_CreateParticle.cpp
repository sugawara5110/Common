//*****************************************************************************************//
//**                                                                                     **//
//**                   　　　         ParticleDataクラス                                 **//
//**                                  CreateParticle関数                                 **//
//*****************************************************************************************//

#include "Dx_ParticleData.h"

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

void ParticleData::update(CONSTANT_BUFFER_P* cb, VECTOR3 pos, float angle, float size, float speed, bool tex) {
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
	if (tex)cb->size.w = 1.0f; else cb->size.w = 0.0f;
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
				P_pos[P_no].Col.as(ptex[ptexI + 0], ptex[ptexI + 1], ptex[ptexI + 2], ptex[ptexI + 3]);//色
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

void ParticleData::CreateVbObj() {
	const UINT vbByteSize = ver * sizeof(PartPos);

	Vview->VertexBufferGPU = dx->CreateDefaultBuffer(com_no, P_pos, vbByteSize, Vview->VertexBufferUploader);

	Vview->VertexByteStride = sizeof(PartPos);
	Vview->VertexBufferByteSize = vbByteSize;

	for (int i = 0; i < 2; i++) {
		Sview1[i].StreamBufferGPU = dx->CreateStreamBuffer(vbByteSize);
		Sview2[i].StreamBufferGPU = dx->CreateStreamBuffer(vbByteSize);

		Sview1[i].StreamByteStride = Sview2[i].StreamByteStride = sizeof(PartPos);
		Sview1[i].StreamBufferByteSize = Sview2[i].StreamBufferByteSize = vbByteSize;
		Sview1[i].BufferFilledSizeLocation = Sview2[i].StreamBufferGPU->GetGPUVirtualAddress();
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
	te.normal = -1;
	te.specular = -1;

	createTextureResource(0, 1, &te);
	mSrvHeap = dx->CreateDescHeap(1);
	if (mSrvHeap == nullptr)return false;
	CreateSrvTexture(mSrvHeap.Get(), 0, texture->GetAddressOf(), 1);

	//パイプラインステートオブジェクト生成(Draw)
	mPSO_draw = CreatePsoParticle(vs, ps, gs, mRootSignature_draw.Get(), dx->pVertexLayout_P, true, true);
	if (mPSO_draw == nullptr)return false;

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

void ParticleData::DrawParts1() {
	mCommandList->SetPipelineState(mPSO_com.Get());

	mCommandList->SetGraphicsRootSignature(mRootSignature_com.Get());

	mCommandList->IASetVertexBuffers(0, 1, &Vview->VertexBufferView());
	mCommandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);

	mCommandList->SetGraphicsRootConstantBufferView(0, mObjectCB->Resource()->GetGPUVirtualAddress());

	mCommandList->SOSetTargets(0, 1, &Sview1[svInd].StreamBufferView());

	mCommandList->DrawInstanced(ver, 1, 0, 0);

	svInd = 1 - svInd;
	if (firstDraw) {
		dx->dx_sub[com_no].ResourceBarrier(Vview->VertexBufferGPU.Get(),
			D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COPY_DEST);
		mCommandList->CopyResource(Vview->VertexBufferGPU.Get(), Sview1[svInd].StreamBufferGPU.Get());
		dx->dx_sub[com_no].ResourceBarrier(Vview->VertexBufferGPU.Get(),
			D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);
	}
	mCommandList->SOSetTargets(0, 1, nullptr);
}

void ParticleData::DrawParts2() {
	mCommandList->SetPipelineState(mPSO_draw.Get());

	ID3D12DescriptorHeap* descriptorHeaps[] = { mSrvHeap.Get() };
	mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	mCommandList->SetGraphicsRootSignature(mRootSignature_draw.Get());

	mCommandList->IASetVertexBuffers(0, 1, &Vview->VertexBufferView());
	mCommandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);

	mCommandList->SetGraphicsRootDescriptorTable(0, mSrvHeap->GetGPUDescriptorHandleForHeapStart());
	mCommandList->SetGraphicsRootConstantBufferView(1, mObjectCB->Resource()->GetGPUVirtualAddress());

	mCommandList->DrawInstanced(ver, 1, 0, 0);
}

void ParticleData::CbSwap(bool init) {
	if (!parInit)parInit = init;//parInit==TRUEの場合まだ初期化終了していない為フラグを書き換えない
	if (!UpOn) {
		upCount++;
		if (upCount > 1)UpOn = true;//cb,2要素初回更新終了
	}
	DrawOn = true;
}

void ParticleData::Update(VECTOR3 pos, float angle, float size, bool init, float speed) {
	update(&cbP[dx->cBuffSwap[0]], pos, angle, size, speed, texpar_on | movOn[0].m_on);
	CbSwap(init);
}

void ParticleData::DrawOff() {
	DrawOn = FALSE;
}

void ParticleData::Draw() {

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

	DrawParts1();
	if (firstDraw)DrawParts2();
	firstDraw = true;
	parInit = false;//parInit==TRUEの場合ここに来た時点で初期化終了
}

void ParticleData::Update(float size) {
	update(&cbP[dx->cBuffSwap[0]], { 0, 0, 0 }, 0, size, 1.0f, texpar_on | movOn[0].m_on);
	CbSwap(true);
}

void ParticleData::DrawBillboard() {

	if (!UpOn | !DrawOn)return;

	update2(&cbP[dx->cBuffSwap[1]], true);
	mObjectCB->CopyData(0, cbP[dx->cBuffSwap[1]]);

	DrawParts2();
}

void ParticleData::SetVertex(int i,
	float vx, float vy, float vz,
	float r, float g, float b, float a) {
	P_pos[i].CurrentPos.as(vx, vy, vz);
	P_pos[i].Col.as(r, g, b, a);
}