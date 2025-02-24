//*****************************************************************************************//
//**                                                                                     **//
//**                   　　　          PolygonData2D                                     **//
//**                                                                                     **//
//*****************************************************************************************//

#include "Dx_PolygonData2D.h"

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
}

PolygonData2D::PolygonData2D() {
	firstCbSet[0] = false;
	firstCbSet[1] = false;
}

PolygonData2D::~PolygonData2D() {
	S_DELETE(mObjectCB);
}

void PolygonData2D::Instancing(CoordTf::MATRIX world, CoordTf::VECTOR4 Color,
	float px, float py, float mx, float my) {

	using namespace CoordTf;

	MatrixTranspose(&world);

	Dx_Device* dev = Dx_Device::GetInstance();
	int swI = dev->cBuffSwapUpdateIndex();

	cb2[swI][ins_no].world = world;
	cb2[swI][ins_no].AddObjColor = Color;
	cb2[swI][ins_no].pXpYmXmY = { px, py, mx, my };

	ins_no++;
}

void PolygonData2D::Instancing(CoordTf::VECTOR3 pos, float angle, CoordTf::VECTOR2 size, CoordTf::VECTOR4 Color,
	float px, float py, float mx, float my) {

	using namespace CoordTf;

	VECTOR3 the3 = { 0.0f, 0.0f, angle };
	VECTOR3 sc3 = { size.x, size.y, 1.0f };
	MATRIX world = Dx_Util::calculationMatrixWorld(pos, the3, sc3);

	Instancing(world, Color, px, py, mx, my);
}

void PolygonData2D::GetVBarray2D(UINT numMaxInstance) {
	NumMaxInstance = numMaxInstance;
	mObjectCB = NEW ConstantBuffer<WVP_CB2D>(NumMaxInstance);
	cb2[0] = std::make_unique<WVP_CB2D[]>(NumMaxInstance);
	cb2[1] = std::make_unique<WVP_CB2D[]>(NumMaxInstance);
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

bool PolygonData2D::Create(int comIndex, bool blend, bool alpha, int Tex, VERTEX2* v2, int num_v2, UINT* index, int num_index) {
	GetShaderByteCode();

	const int numSrv = 1;
	const int numCbv = 0;

	UINT numDescriptors[1] = {};
	numDescriptors[0] = NumMaxInstance;
	mRootSignature = CreateRootSignature(numSrv, numCbv, 0, 0, 0, 1, numDescriptors);
	if (mRootSignature == nullptr)return false;

	TextureNo te;
	te.diffuse = Tex;
	te.normal = -1;
	te.specular = -1;

	Dx_Device* device = Dx_Device::GetInstance();
	createTextureResource(comIndex, 0, 1, &te, objName);
	mDescHeap = device->CreateDescHeap(numSrv + numCbv + NumMaxInstance);
	Dx_Device* d = device;
	D3D12_CPU_DESCRIPTOR_HANDLE hDescriptor(mDescHeap->GetCPUDescriptorHandleForHeapStart());

	for (UINT i = 0; i < NumMaxInstance; i++) {
		D3D12_GPU_VIRTUAL_ADDRESS ad = mObjectCB->getGPUVirtualAddress(i);
		UINT size = mObjectCB->getSizeInBytes();
		d->CreateCbv(hDescriptor, &ad, &size, 1);
	}

	d->CreateSrvTexture(hDescriptor, &texture[0], 1);

	const UINT vbByteSize = num_v2 * sizeof(VERTEX2);
	const UINT ibByteSize = num_index * sizeof(UINT);
	Dx_CommandManager& ma = *Dx_CommandManager::GetInstance();

	Vview->VertexBufferGPU.CreateDefaultBuffer(comIndex, v2, vbByteSize, false);

	Iview->IndexBufferGPU.CreateDefaultBuffer(comIndex, index, ibByteSize, false);

	Vview->VertexByteStride = sizeof(VERTEX2);
	Vview->VertexBufferByteSize = vbByteSize;
	Iview->IndexBufferByteSize = ibByteSize;
	Iview->IndexCount = num_index;

	//パイプラインステートオブジェクト生成
	mPSO = CreatePsoVsPs(vs, ps, mRootSignature.Get(), Dx_ShaderHolder::pVertexLayout_2D, alpha, blend, SQUARE);
	if (mPSO == nullptr)return false;

	return true;
}

void PolygonData2D::InstancingUpdate() {
	Dx_Device* dev = Dx_Device::GetInstance();
	firstCbSet[dev->cBuffSwapUpdateIndex()] = true;
	insNum[dev->cBuffSwapUpdateIndex()] = ins_no;
	ins_no = 0;
	DrawOn = true;
}

void PolygonData2D::Update(
	CoordTf::VECTOR3 pos, float angle, CoordTf::VECTOR2 size, CoordTf::VECTOR4 Color,
	float px, float py, float mx, float my) {

	Instancing(pos, angle, size, Color,
		px, py, mx, my);

	InstancingUpdate();
}

void PolygonData2D::DrawOff() {
	DrawOn = false;
}

void PolygonData2D::Draw(int comIndex) {

	Dx_Device* dev = Dx_Device::GetInstance();
	int cbInd = dev->cBuffSwapDrawOrStreamoutputIndex();

	if (!firstCbSet[cbInd] || !DrawOn)return;

	ID3D12GraphicsCommandList* mCList = Dx_CommandManager::GetInstance()->getGraphicsComListObj(comIndex)->getCommandList();

	for (int i = 0; i < insNum[cbInd]; i++) {
		mObjectCB->CopyData(i, cb2[cbInd][i]);
	}

	mCList->SetPipelineState(mPSO.Get());

	ID3D12DescriptorHeap* descriptorHeaps[] = { mDescHeap.Get() };
	mCList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	mCList->SetGraphicsRootSignature(mRootSignature.Get());

	mCList->IASetVertexBuffers(0, 1, &Vview->VertexBufferView());
	mCList->IASetIndexBuffer(&Iview->IndexBufferView());
	mCList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	mCList->SetGraphicsRootDescriptorTable(0, mDescHeap->GetGPUDescriptorHandleForHeapStart());

	mCList->DrawIndexedInstanced(
		Iview->IndexCount, insNum[cbInd], 0, 0, 0);
}



