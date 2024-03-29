//*****************************************************************************************//
//**                                                                                     **//
//**                   　　　          PolygonData2D                                     **//
//**                                                                                     **//
//*****************************************************************************************//

#include "Dx_PolygonData2D.h"

float PolygonData2D::magnificationX = 1.0f;
float PolygonData2D::magnificationY = 1.0f;

void PolygonData2D::Pos2DCompute(CoordTf::VECTOR3* p) {
	using namespace CoordTf;
	MATRIX VP, VP_VP;
	Dx_SwapChain* sw = Dx_SwapChain::GetInstance();
	Dx_Device* dev = Dx_Device::GetInstance();
	//入力3D座標から変換行列取得
	MatrixMultiply(&VP, &sw->getUpdate(dev->cBuffSwapUpdateIndex()).mView, &sw->getUpdate(dev->cBuffSwapUpdateIndex()).mProj);
	//ビューポート行列作成(3D座標→2D座標変換に使用)
	MATRIX Vp = {};
	MatrixViewPort(&Vp, sw->getClientWidth(), sw->getClientHeight());
	MatrixMultiply(&VP_VP, &VP, &Vp);
	//変換後の座標取得
	VectorMatrixMultiply(p, &VP_VP);
	p->x /= magnificationX;
	p->y /= magnificationY;
}

void PolygonData2D::SetMagnification(float x, float y) {
	magnificationX = x;
	magnificationY = y;
}

PolygonData2D::PolygonData2D() {
	d2varray = nullptr;
	d2varrayI = nullptr;
	magX = magnificationX;
	magY = magnificationY;

	firstCbSet[0] = false;
	firstCbSet[1] = false;
}

void PolygonData2D::DisabledMagnification() {
	magX = 1.0f;
	magY = 1.0f;
}

PolygonData2D::~PolygonData2D() {
	free(d2varray);
	d2varray = nullptr;
	free(d2varrayI);
	d2varrayI = nullptr;
	S_DELETE(mObjectCB);
}

void PolygonData2D::InstancedSetConstBf(float x, float y, float r, float g, float b, float a, float sizeX, float sizeY) {
	InstancedSetConstBf(x, y, 0.0f, r, g, b, a, sizeX, sizeY);
}

void PolygonData2D::InstancedSetConstBf(float x, float y, float z, float r, float g, float b, float a, float sizeX, float sizeY) {

	Dx_SwapChain* sw = Dx_SwapChain::GetInstance();
	Dx_Device* dev = Dx_Device::GetInstance();
	int swI = dev->cBuffSwapUpdateIndex();
	cb2[swI].Pos[ins_no].as(x * magX, y * magY, z, 0.0f);
	cb2[swI].Color[ins_no].as(r, g, b, a);
	cb2[swI].sizeXY[ins_no].as(sizeX * magX, sizeY * magY, 0.0f, 0.0f);
	cb2[swI].WidHei.as((float)sw->getClientWidth(), (float)sw->getClientHeight(), 0.0f, 0.0f);
	ins_no++;
}

void PolygonData2D::SetConstBf(CONSTANT_BUFFER2D* cb, float x, float y, float z, float r, float g, float b, float a, float sizeX, float sizeY) {

	Dx_SwapChain* sw = Dx_SwapChain::GetInstance();
	cb->Pos[ins_no].as(x * magX, y * magY, z, 0.0f);
	cb->Color[ins_no].as(r, g, b, a);
	cb->sizeXY[ins_no].as(sizeX * magX, sizeY * magY, 0.0f, 0.0f);
	cb->WidHei.as((float)sw->getClientWidth(), (float)sw->getClientHeight(), 0.0f, 0.0f);
	ins_no++;
}

ID3D12PipelineState *PolygonData2D::GetPipelineState() {
	return mPSO.Get();
}

void PolygonData2D::GetVBarray2D(int pcs) {
	ver = pcs * 4;
	d2varray = (MY_VERTEX2*)malloc(sizeof(MY_VERTEX2) * ver);
	d2varrayI = (std::uint16_t*)malloc(sizeof(std::uint16_t) * (int)(ver * 1.5));
	mObjectCB = NEW ConstantBuffer<CONSTANT_BUFFER2D>(1);
	Vview = std::make_unique<VertexView>();
	Iview = std::make_unique<IndexView>();
}

void PolygonData2D::TexOn() {
	tex_on = true;
}

void PolygonData2D::GetShaderByteCode() {

	if (!tex_on) {
		vs = Dx_ShaderHolder::pVertexShader_2D.Get();
		ps = Dx_ShaderHolder::pPixelShader_2D.Get();
	}
	else {
		vs = Dx_ShaderHolder::pVertexShader_2DTC.Get();
		ps = Dx_ShaderHolder::pPixelShader_2DTC.Get();
	}
}

bool PolygonData2D::CreateBox(int comIndex, float x, float y, float z,
	float sizex, float sizey,
	float r, float g, float b, float a,
	bool blend, bool alpha, int noTex) {

	//verが4の時のみ実行可
	if (ver != 4)return false;
	d2varray[0].Pos.as(x * magX, y * magY, z);
	d2varray[0].color.as(r, g, b, a);
	d2varray[0].tex.as(0.0f, 0.0f);

	d2varray[1].Pos.as(x * magX, (y + sizey) * magY, z);
	d2varray[1].color.as(r, g, b, a);
	d2varray[1].tex.as(0.0f, 1.0f);

	d2varray[2].Pos.as((x + sizex) * magX, (y + sizey) * magY, z);
	d2varray[2].color.as(r, g, b, a);
	d2varray[2].tex.as(1.0f, 1.0f);

	d2varray[3].Pos.as((x + sizex) * magX, y * magY, z);
	d2varray[3].color.as(r, g, b, a);
	d2varray[3].tex.as(1.0f, 0.0f);

	d2varrayI[0] = 0;
	d2varrayI[1] = 3;
	d2varrayI[2] = 1;
	d2varrayI[3] = 1;
	d2varrayI[4] = 3;
	d2varrayI[5] = 2;

	return Create(comIndex, blend, alpha, noTex);
}

bool PolygonData2D::Create(int comIndex, bool blend, bool alpha, int noTex) {

	GetShaderByteCode();

	const int numSrv = 1;
	const int numCbv = 1;

	mRootSignature = CreateRootSignature(numSrv, numCbv, 0, 0, 0, 0, nullptr);
	if (mRootSignature == nullptr)return false;

	TextureNo te;
	te.diffuse = noTex;
	te.normal = -1;
	te.specular = -1;

	Dx_Device* device = Dx_Device::GetInstance();
	createTextureResource(comIndex, 0, 1, &te, objName);
	mDescHeap = device->CreateDescHeap(numSrv + numCbv);
	Dx_Device* d = device;
	D3D12_CPU_DESCRIPTOR_HANDLE hDescriptor(mDescHeap->GetCPUDescriptorHandleForHeapStart());
	d->CreateSrvTexture(hDescriptor, &texture[0], 1);
	D3D12_GPU_VIRTUAL_ADDRESS ad = mObjectCB->Resource()->GetGPUVirtualAddress();
	UINT size = mObjectCB->getSizeInBytes();
	d->CreateCbv(hDescriptor, &ad, &size, numCbv);

	const UINT vbByteSize = ver * sizeof(MY_VERTEX2);
	const UINT ibByteSize = (int)(ver * 1.5) * sizeof(std::uint16_t);
	Dx_CommandManager& ma = *Dx_CommandManager::GetInstance();

	Vview->VertexBufferGPU.CreateDefaultBuffer(comIndex, d2varray, vbByteSize, false);

	Iview->IndexBufferGPU.CreateDefaultBuffer(comIndex, d2varrayI, ibByteSize, false);

	Vview->VertexByteStride = sizeof(MY_VERTEX2);
	Vview->VertexBufferByteSize = vbByteSize;
	Iview->IndexFormat = DXGI_FORMAT_R16_UINT;
	Iview->IndexBufferByteSize = ibByteSize;
	Iview->IndexCount = (int)(ver * 1.5);

	//パイプラインステートオブジェクト生成
	mPSO = CreatePsoVsPs(vs, ps, mRootSignature.Get(), Dx_ShaderHolder::pVertexLayout_2D, alpha, blend, SQUARE);
	if (mPSO == nullptr)return false;

	return true;
}

void PolygonData2D::CbSwap() {
	Dx_Device* dev = Dx_Device::GetInstance();
	firstCbSet[dev->cBuffSwapUpdateIndex()] = true;
	insNum[dev->cBuffSwapUpdateIndex()] = ins_no;
	ins_no = 0;
	DrawOn = true;
}

void PolygonData2D::InstanceUpdate() {
	CbSwap();
}

void PolygonData2D::Update(float x, float y, float r, float g, float b, float a, float sizeX, float sizeY) {
	Update(x, y, 0.0f, r, g, b, a, sizeX, sizeY);
}

void PolygonData2D::Update(float x, float y, float z, float r, float g, float b, float a, float sizeX, float sizeY) {
	Dx_Device* dev = Dx_Device::GetInstance();
	SetConstBf(&cb2[dev->cBuffSwapUpdateIndex()], x, y, z, r, g, b, a, sizeX, sizeY);
	CbSwap();
}

void PolygonData2D::DrawOff() {
	DrawOn = false;
}

void PolygonData2D::Draw(int comIndex) {

	Dx_Device* dev = Dx_Device::GetInstance();
	if (!firstCbSet[dev->cBuffSwapDrawOrStreamoutputIndex()] | !DrawOn)return;

	ID3D12GraphicsCommandList* mCList = Dx_CommandManager::GetInstance()->getGraphicsComListObj(comIndex)->getCommandList();

	mObjectCB->CopyData(0, cb2[dev->cBuffSwapDrawOrStreamoutputIndex()]);

	mCList->SetPipelineState(mPSO.Get());

	ID3D12DescriptorHeap* descriptorHeaps[] = { mDescHeap.Get() };
	mCList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	mCList->SetGraphicsRootSignature(mRootSignature.Get());

	mCList->IASetVertexBuffers(0, 1, &Vview->VertexBufferView());
	mCList->IASetIndexBuffer(&Iview->IndexBufferView());
	mCList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	mCList->SetGraphicsRootDescriptorTable(0, mDescHeap->GetGPUDescriptorHandleForHeapStart());

	mCList->DrawIndexedInstanced(
		Iview->IndexCount, insNum[dev->cBuffSwapDrawOrStreamoutputIndex()], 0, 0, 0);
}



