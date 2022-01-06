//*****************************************************************************************//
//**                                                                                     **//
//**                   　　　          Dx12ProcessCoreクラス                             **//
//**                                                                                     **//
//*****************************************************************************************//

#define _CRT_SECURE_NO_WARNINGS
#include "Dx12ProcessCore.h"
#include <WindowsX.h>
#include <locale.h>

ComPtr<ID3D12Resource> StreamView::UpresetBuffer = nullptr;
ComPtr<ID3D12Resource> StreamView::resetBuffer = nullptr;

Dx12Process *Dx12Process::dx = nullptr;
std::mutex Dx12Process::mtx;
std::mutex Dx12Process::mtxGraphicsQueue;
std::mutex Dx12Process::mtxComputeQueue;

void Dx12Process::InstanceCreate() {

	if (dx == nullptr)dx = new Dx12Process();
}

Dx12Process *Dx12Process::GetInstance() {

	if (dx != nullptr)return dx;
	return nullptr;
}

void Dx12Process::DeleteInstance() {

	if (dx != nullptr) {
		delete dx;
		dx = nullptr;
	}
}

Dx12Process::~Dx12Process() {
	ARR_DELETE(texture);

#if defined(DEBUG) || defined(_DEBUG) 
	//ReportLiveDeviceObjectsを呼び出したタイミングでの
	//生存オブジェクトを調べる
	if (ReportLiveDeviceObjectsOn) {
		ID3D12DebugDevice* debugInterface;
		if (SUCCEEDED(Dx_Device::GetInstance()->getDevice()->QueryInterface(&debugInterface)))
		{
			debugInterface->ReportLiveDeviceObjects(D3D12_RLDO_DETAIL | D3D12_RLDO_IGNORE_INTERNAL);
			debugInterface->Release();
		}
	}
#endif

	Dx_Device::DeleteInstance();
}

void Dx12Process::RunGpuNotLock() {
	//クローズ後リストに加える
	static ID3D12CommandList* cmdsLists[COM_NO] = {};
	UINT cnt = 0;
	for (int i = 0; i < COM_NO; i++) {
		if (dx_sub[i].mComState != CLOSE)continue;
		cmdsLists[cnt++] = dx_sub[i].mCommandList.Get();
		dx_sub[i].mComState = USED;
	}

	graphicsQueue.setCommandList(cnt, cmdsLists);
}

void Dx12Process::WaitFenceNotLock() {
	graphicsQueue.waitFence();
}

void Dx12Process::RunGpu() {
	mtxGraphicsQueue.lock();
	RunGpuNotLock();
	mtxGraphicsQueue.unlock();
}

void Dx12Process::WaitFence() {
	mtxGraphicsQueue.lock();
	WaitFenceNotLock();
	mtxGraphicsQueue.unlock();
}

void Dx12Process::BiginCom(int com_no) {
	dx_subCom[com_no].Bigin();
}

void Dx12Process::EndCom(int com_no) {
	dx_subCom[com_no].End();
}

void Dx12Process::RunGpuNotLockCom() {
	static ID3D12CommandList* cmdsLists[COM_NO] = {};
	UINT cnt = 0;
	for (int i = 0; i < COM_NO; i++) {
		if (dx_subCom[i].mComState != CLOSE)continue;
		cmdsLists[cnt++] = dx_subCom[i].mCommandList.Get();
		dx_subCom[i].mComState = USED;
	}

	computeQueue.setCommandList(cnt, cmdsLists);
}

void Dx12Process::WaitFenceNotLockCom() {
	computeQueue.waitFence();
}

void Dx12Process::RunGpuCom() {
	mtxComputeQueue.lock();
	RunGpuNotLockCom();
	mtxComputeQueue.unlock();
}

void Dx12Process::WaitFenceCom() {
	mtxComputeQueue.lock();
	WaitFenceNotLockCom();
	mtxComputeQueue.unlock();
}

int Dx12Process::GetTexNumber(CHAR* fileName) {

	fileName = Dx_Util::GetNameFromPass(fileName);

	for (int i = 0; i < texNum; i++) {
		if (texture[i].texName == '\0')continue;
		char str[50];
		char str1[50];
		strcpy(str, texture[i].texName);
		strcpy(str1, fileName);
		int i1 = -1;
		while (str[++i1] != '\0' && str[i1] != '.' && str[i1] == str1[i1]);
		if (str[i1] == '.' && str1[i1] == '.')return i;
	}

	return -1;
}

HRESULT Dx12Process::createTexture(int com_no, UCHAR* byteArr, DXGI_FORMAT format,
	ID3D12Resource** up, ID3D12Resource** def,
	int width, LONG_PTR RowPitch, int height) {

	HRESULT hr = Dx_Device::GetInstance()->textureInit(width, height, up, def, format,
		D3D12_RESOURCE_STATE_COMMON);
	if (FAILED(hr)) {
		return hr;
	}

	dx_sub[com_no].ResourceBarrier(*def, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
	hr = CopyResourcesToGPU(com_no, *up, *def, byteArr, RowPitch);
	dx_sub[com_no].ResourceBarrier(*def, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);

	return hr;
}

void Dx12Process::createTextureArr(int numTexArr, int resourceIndex, char* texName,
	UCHAR* byteArr, DXGI_FORMAT format,
	int width, LONG_PTR RowPitch, int height) {

	if (!texture) {
		texNum = numTexArr + 2;//dummyNor,dummyDifSpe分
		texture = new InternalTexture[texNum];
		UCHAR dm[8 * 4 * 8] = {};
		UCHAR ndm[4] = { 128,128,255,0 };
		for (int i = 0; i < 8 * 4 * 8; i += 4)
			memcpy(&dm[i], ndm, sizeof(UCHAR) * 4);
		texture[0].setParameter(DXGI_FORMAT_R8G8B8A8_UNORM, 8, 8 * 4, 8);
		texture[0].setName("dummyNor.");
		texture[0].setData(dm);

		UCHAR sdm[4] = { 255,255,255,255 };
		for (int i = 0; i < 8 * 4 * 8; i += 4)
			memcpy(&dm[i], sdm, sizeof(UCHAR) * 4);
		texture[1].setParameter(DXGI_FORMAT_R8G8B8A8_UNORM, 8, 8 * 4, 8);
		texture[1].setName("dummyDifSpe.");
		texture[1].setData(dm);
	}

	InternalTexture* tex = &texture[resourceIndex + 2];
	tex->setParameter(format, width, RowPitch, height);
	tex->setName(texName);
	tex->setData(byteArr);
}

bool Dx12Process::Initialize(HWND hWnd, int width, int height) {

	mClientWidth = width;
	mClientHeight = height;

#if defined(DEBUG) || defined(_DEBUG) 
	//デバッグ中はデバッグレイヤーを有効化する
	{
		ComPtr<ID3D12Debug> debugController;
		if (FAILED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
			Dx_Util::ErrorMessage("D3D12GetDebugInterface Error");
			return false;
		}
		debugController->EnableDebugLayer();
	}
#endif

	//ファクトリ生成
	//アダプターの列挙、スワップ チェーンの作成、
	//および全画面表示モードとの間の切り替えに使用される Alt + 
	//Enter キー シーケンスとのウィンドウの関連付けを行うオブジェクトを生成するために使用
	if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&mdxgiFactory)))) {
		Dx_Util::ErrorMessage("CreateDXGIFactory1 Error");
		return false;
	}

	//デバイス生成
	Dx_Device::InstanceCreate();
	Dx_Device* device = Dx_Device::GetInstance();
	HRESULT hardwareResult = device->createDevice();

	//↑ハードウエア処理不可の場合ソフトウエア処理にする,ソフトウエア処理デバイス生成
	if (FAILED(hardwareResult))
	{
		ComPtr<IDXGIAdapter> pWarpAdapter;
		if (FAILED(mdxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&pWarpAdapter)))) {
			Dx_Util::ErrorMessage("EnumWarpAdapter Error");
			return false;
		}

		ID3D12Device5* dev = device->getDevice();
		if (FAILED(D3D12CreateDevice(
			pWarpAdapter.Get(),
			D3D_FEATURE_LEVEL_11_0,
			IID_PPV_ARGS(&dev)))) {
			Dx_Util::ErrorMessage("D3D12CreateDevice Error");
			return false;
		}
	}
	else if (DXR_CreateResource) {
		D3D12_FEATURE_DATA_D3D12_OPTIONS5 features5 = {};
		HRESULT hr = device->getDevice()->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &features5, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS5));
		if (FAILED(hr) || features5.RaytracingTier == D3D12_RAYTRACING_TIER_NOT_SUPPORTED)
		{
			Dx_Util::ErrorMessage("DXR not supported");
			DXR_CreateResource = false;
			return false;
		}
	}

	//Descriptor のアドレス計算で使用
	mRtvDescriptorSize = device->getDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	mDsvDescriptorSize = device->getDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	mCbvSrvUavDescriptorSize = device->getDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	//MultiSampleレベルチェック
	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msQualityLevels;
	msQualityLevels.Format = mBackBufferFormat;
	msQualityLevels.SampleCount = 4;
	msQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
	msQualityLevels.NumQualityLevels = 0;
	if (FAILED(device->getDevice()->CheckFeatureSupport(
		D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
		&msQualityLevels,     //機能のサポートを示すデータが格納
		sizeof(msQualityLevels)))) {
		Dx_Util::ErrorMessage("CheckFeatureSupport Error");
		return false;
	}

	m4xMsaaQuality = msQualityLevels.NumQualityLevels;
	assert(m4xMsaaQuality > 0 && "Unexpected MSAA quality level.");

	//コマンドキュー生成
	if (!graphicsQueue.Create(device->getDevice(), false))return false;
	if (!computeQueue.Create(device->getDevice(), true))return false;

	for (int i = 0; i < COM_NO; i++) {
		//コマンドアロケータ,コマンドリスト生成
		if (!dx_sub[i].ListCreate(false, device->getDevice()))return false;
		dx_sub[i].createResourceBarrierList();
		dx_sub[i].createUavResourceBarrierList();
		if (!dx_subCom[i].ListCreate(true, device->getDevice()))return false;
		dx_subCom[i].createUavResourceBarrierList();
	}

	//初期化
	mSwapChain.Reset();

	//スワップチェイン生成
	DXGI_SWAP_CHAIN_DESC1 sd = {};
	sd.Width = mClientWidth;
	sd.Height = mClientHeight;
	sd.Format = mBackBufferFormat;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.BufferCount = SwapChainBufferCount;
	sd.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	sd.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	sd.Scaling = DXGI_SCALING_STRETCH;
	sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	sd.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	ComPtr<IDXGISwapChain1> swapChain;
	if (FAILED(mdxgiFactory->CreateSwapChainForHwnd(
		graphicsQueue.mCommandQueue.Get(),
		hWnd,
		&sd,
		nullptr,
		nullptr,
		swapChain.GetAddressOf()))) {
		Dx_Util::ErrorMessage("CreateSwapChain Error");
		return false;
	}
	swapChain->QueryInterface(IID_PPV_ARGS(mSwapChain.GetAddressOf()));

	//Descriptor:何のバッファかを記述される
	//スワップチェインをRenderTargetとして使用するためのDescriptorHeapを作成(Descriptorの記録場所)
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.NumDescriptors = SwapChainBufferCount;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;   //RenderTargetView
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE; //シェーダからアクセスしないのでNONEでOK
	rtvHeapDesc.NodeMask = 0;
	if (FAILED(device->getDevice()->CreateDescriptorHeap(
		&rtvHeapDesc, IID_PPV_ARGS(mRtvHeap.GetAddressOf())))) {
		Dx_Util::ErrorMessage("CreateDescriptorHeap Error");
		return false;
	}

	//深度ステンシルビュ-DescriptorHeapを作成
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	dsvHeapDesc.NodeMask = 0;
	if (FAILED(device->getDevice()->CreateDescriptorHeap(
		&dsvHeapDesc, IID_PPV_ARGS(mDsvHeap.GetAddressOf())))) {
		Dx_Util::ErrorMessage("CreateDescriptorHeap Error");
		return false;
	}

	//レンダーターゲットビューデスクリプターヒープの開始ハンドル取得
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(mRtvHeap->GetCPUDescriptorHandleForHeapStart());
	for (UINT i = 0; i < SwapChainBufferCount; i++) {
		//スワップチェインバッファ取得
		if (FAILED(mSwapChain->GetBuffer(i, IID_PPV_ARGS(&mSwapChainBuffer[i])))) {
			Dx_Util::ErrorMessage("GetSwapChainBuffer Error");
			return false;
		}
		//レンダーターゲットビュー(Descriptor)生成,DescriptorHeapにDescriptorが記録される
		device->getDevice()->CreateRenderTargetView(mSwapChainBuffer[i].Get(), nullptr, rtvHeapHandle);
		//ヒープ位置オフセット(2個あるので2個目記録位置にDescriptorSize分オフセット)
		rtvHeapHandle.ptr += mRtvDescriptorSize;
	}

	D3D12_RESOURCE_DESC depthStencilDesc = {};
	depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	depthStencilDesc.Alignment = 0;
	depthStencilDesc.Width = mClientWidth;
	depthStencilDesc.Height = mClientHeight;
	depthStencilDesc.DepthOrArraySize = 1;
	depthStencilDesc.MipLevels = 1;
	depthStencilDesc.Format = mDepthStencilResFormat;
	depthStencilDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	depthStencilDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_HEAP_PROPERTIES depthStencilHeapProps = {};
	depthStencilHeapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
	depthStencilHeapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	depthStencilHeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	depthStencilHeapProps.CreationNodeMask = 1;
	depthStencilHeapProps.VisibleNodeMask = 1;

	D3D12_CLEAR_VALUE optClear = {};
	optClear.Format = mDepthStencilFormat;
	optClear.DepthStencil.Depth = 1.0f;
	optClear.DepthStencil.Stencil = 0;
	//深度ステンシルバッファ領域確保
	if (FAILED(device->getDevice()->CreateCommittedResource(
		&depthStencilHeapProps,
		D3D12_HEAP_FLAG_NONE,
		&depthStencilDesc,
		D3D12_RESOURCE_STATE_COMMON,
		&optClear,
		IID_PPV_ARGS(mDepthStencilBuffer.GetAddressOf())))) {
		Dx_Util::ErrorMessage("CreateCommittedResource DepthStencil Error");
		return false;
	}

	//深度ステンシルビューデスクリプターヒープの開始ハンドル取得
	D3D12_CPU_DESCRIPTOR_HANDLE mDsvHeapHeapHandle(mDsvHeap->GetCPUDescriptorHandleForHeapStart());
	//深度ステンシルビュー生成
	D3D12_DEPTH_STENCIL_VIEW_DESC depthdesc = {};
	depthdesc.Format = mDepthStencilFormat;
	depthdesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	depthdesc.Flags = D3D12_DSV_FLAG_NONE;
	device->getDevice()->CreateDepthStencilView(mDepthStencilBuffer.Get(), &depthdesc, mDsvHeapHeapHandle);

	dx_sub[0].Bigin();

	//深度ステンシルバッファ,リソースバリア共有→深度書き込み
	dx_sub[0].ResourceBarrier(mDepthStencilBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE);

	StreamView::createResetBuffer(0);

	dx_sub[0].End();
	RunGpu();
	WaitFence();

	//ビューポート
	mScreenViewport.TopLeftX = 0;
	mScreenViewport.TopLeftY = 0;
	mScreenViewport.Width = static_cast<float>(mClientWidth);
	mScreenViewport.Height = static_cast<float>(mClientHeight);
	mScreenViewport.MinDepth = 0.0f;
	mScreenViewport.MaxDepth = 1.0f;

	mScissorRect = { 0, 0, mClientWidth, mClientHeight };

	MatrixIdentity(&upd[0].mProj);
	MatrixIdentity(&upd[1].mProj);
	MatrixIdentity(&upd[0].mView);
	MatrixIdentity(&upd[1].mView);
	//カメラ画角
	ViewY_theta = 55.0f;
	// アスペクト比の計算
	aspect = (float)mScreenViewport.Width / (float)mScreenViewport.Height;
	//nearプレーン
	NearPlane = 1.0f;
	//farプレーン
	FarPlane = 10000.0f;
	MatrixPerspectiveFovLH(&upd[0].mProj, ViewY_theta, aspect, NearPlane, FarPlane);
	MatrixPerspectiveFovLH(&upd[1].mProj, ViewY_theta, aspect, NearPlane, FarPlane);

	//ビューポート行列作成(3D座標→2D座標変換に使用)
	MatrixViewPort(&Vp, mClientWidth, mClientHeight);

	//ポイントライト構造体初期化
	ResetPointLight();

	//平行光源初期化
	upd[0].dlight.Direction.as(0.0f, 0.0f, 0.0f, 0.0f);
	upd[0].dlight.LightColor.as(1.0f, 1.0f, 1.0f, 1.0f);
	upd[0].dlight.onoff = 0.0f;
	upd[1].dlight.Direction.as(0.0f, 0.0f, 0.0f, 0.0f);
	upd[1].dlight.LightColor.as(1.0f, 1.0f, 1.0f, 1.0f);
	upd[1].dlight.onoff = 0.0f;

	//フォグ初期化
	fog.FogColor.as(1.0f, 1.0f, 1.0f, 1.0f);
	fog.Amount = 0.0f;
	fog.Density = 0.0f;
	fog.on_off = 0.0f;

	shaderH = std::make_unique<Dx_ShaderHolder>();
	return shaderH.get()->CreateShaderByteCode();
}

void Dx12Process::setPerspectiveFov(float ViewAngle, float nearPlane, float farPlane) {
	ViewY_theta = ViewAngle;
	NearPlane = nearPlane;
	FarPlane = farPlane;
	MatrixPerspectiveFovLH(&upd[0].mProj, ViewY_theta, aspect, NearPlane, FarPlane);
	MatrixPerspectiveFovLH(&upd[1].mProj, ViewY_theta, aspect, NearPlane, FarPlane);
}

void Dx12Process::BiginDraw(int com_no, bool clearBackBuffer) {
	dx_sub[com_no].mCommandList->RSSetViewports(1, &mScreenViewport);
	dx_sub[com_no].mCommandList->RSSetScissorRects(1, &mScissorRect);

	dx_sub[com_no].ResourceBarrier(mSwapChainBuffer[mCurrBackBuffer].Get(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = mRtvHeap->GetCPUDescriptorHandleForHeapStart();
	rtvHandle.ptr += mCurrBackBuffer * mRtvDescriptorSize;

	if (clearBackBuffer) {
		dx_sub[com_no].mCommandList->ClearRenderTargetView(rtvHandle,
			DirectX::Colors::Black, 0, nullptr);

		dx_sub[com_no].mCommandList->ClearDepthStencilView(mDsvHeap->GetCPUDescriptorHandleForHeapStart(),
			D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
	}

	dx_sub[com_no].mCommandList->OMSetRenderTargets(1, &rtvHandle,
		true, &mDsvHeap->GetCPUDescriptorHandleForHeapStart());
}

void Dx12Process::EndDraw(int com_no) {
	dx_sub[com_no].ResourceBarrier(mSwapChainBuffer[mCurrBackBuffer].Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
}

void Dx12Process::Bigin(int com_no) {
	dx_sub[com_no].Bigin();
}

void Dx12Process::End(int com_no) {
	dx_sub[com_no].End();
}

void Dx12Process::DrawScreen() {
	// swap the back and front buffers
	mSwapChain->Present(0, 0);
	mCurrBackBuffer = mSwapChain->GetCurrentBackBufferIndex();
}

void Dx12Process::Cameraset(CoordTf::VECTOR3 pos, CoordTf::VECTOR3 dir, CoordTf::VECTOR3 up) {
	//カメラの位置と方向を設定
	MatrixLookAtLH(&upd[cBuffSwap[0]].mView,
		pos, //カメラの位置
		dir, //カメラの方向を向ける点
		up); //カメラの上の方向(通常視点での上方向を1.0fにする)
	//シェーダー計算用座標登録(視点からの距離で使う)
	upd[cBuffSwap[0]].pos.as(pos.x, pos.y, pos.z);
	upd[cBuffSwap[0]].dir.as(dir.x, dir.y, dir.z);
	upd[cBuffSwap[0]].up.as(up.x, up.y, up.z);
}

void Dx12Process::ResetPointLight() {
	for (int i = 0; i < LIGHT_PCS; i++) {
		upd[0].plight.LightPos[i].as(0.0f, 0.0f, 0.0f, 0.0f);
		upd[0].plight.LightColor[i].as(0.0f, 0.0f, 0.0f, 0.0f);
		upd[0].plight.Lightst[i].as(0.0f, 1.0f, 0.001f, 0.001f);
		upd[1].plight.LightPos[i].as(0.0f, 0.0f, 0.0f, 0.0f);
		upd[1].plight.LightColor[i].as(0.0f, 0.0f, 0.0f, 0.0f);
		upd[1].plight.Lightst[i].as(0.0f, 1.0f, 0.001f, 0.001f);
	}
	upd[0].lightNum = 0;
	upd[1].lightNum = 0;
}

bool Dx12Process::PointLightPosSet(int Idx, CoordTf::VECTOR3 pos, CoordTf::VECTOR4 Color, bool on_off,
	float range, CoordTf::VECTOR3 atten) {

	if (Idx > LIGHT_PCS - 1 || Idx < 0) {
		Dx_Util::ErrorMessage("lightNumの値が範囲外です");
		return false;
	}

	if (Idx + 1 > upd[cBuffSwap[0]].lightNum && on_off)upd[cBuffSwap[0]].lightNum = Idx + 1;

	float onoff;
	if (on_off)onoff = 1.0f; else onoff = 0.0f;
	upd[cBuffSwap[0]].plight.LightPos[Idx].as(pos.x, pos.y, pos.z, onoff);
	upd[cBuffSwap[0]].plight.LightColor[Idx].as(Color.x, Color.y, Color.z, Color.w);
	upd[cBuffSwap[0]].plight.Lightst[Idx].as(range, atten.x, atten.y, atten.z);
	upd[cBuffSwap[0]].plight.LightPcs = upd[cBuffSwap[0]].lightNum;

	return true;
}

void Dx12Process::DirectionLight(float x, float y, float z, float r, float g, float b) {
	upd[cBuffSwap[0]].dlight.Direction.as(x, y, z, 0.0f);
	upd[cBuffSwap[0]].dlight.LightColor.as(r, g, b, 0.0f);
}

void Dx12Process::SetDirectionLight(bool onoff) {
	float f = 0.0f;
	if (onoff)f = 1.0f;
	upd[0].dlight.onoff = f;
	upd[1].dlight.onoff = f;
}

void Dx12Process::Fog(float r, float g, float b, float amount, float density, bool onoff) {

	if (!onoff) {
		fog.on_off = 0.0f;
		return;
	}
	fog.on_off = 1.0f;
	fog.FogColor.as(r, g, b, 1.0f);
	fog.Amount = amount;
	fog.Density = density;
}

HRESULT Dx12Process::CopyResourcesToGPU(int com_no, ID3D12Resource* up, ID3D12Resource* def,
	const void* initData, LONG_PTR RowPitch) {

	bool copyTexture = false;
	D3D12_RESOURCE_DESC desc = def->GetDesc();
	if (desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE1D ||
		desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D ||
		desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D) {
		copyTexture = true;
	}

	D3D12_SUBRESOURCE_DATA subResourceData = {};
	subResourceData.pData = initData;

	D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint;
	D3D12_TEXTURE_COPY_LOCATION dest, src;

	if (copyTexture) {
		UINT64  total_bytes = 0;
		Dx_Device::GetInstance()->getDevice()->GetCopyableFootprints(&desc, 0, 1, 0, &footprint, nullptr, nullptr, &total_bytes);

		memset(&dest, 0, sizeof(dest));
		dest.pResource = def;
		dest.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		dest.SubresourceIndex = 0;

		memset(&src, 0, sizeof(src));
		src.pResource = up;
		src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
		src.PlacedFootprint = footprint;

		subResourceData.RowPitch = footprint.Footprint.RowPitch;
	}
	else {
		subResourceData.RowPitch = RowPitch;
	}

	D3D12_SUBRESOURCE_DATA mapResource;
	HRESULT hr = up->Map(0, nullptr, reinterpret_cast<void**>(&mapResource));
	if (FAILED(hr)) {
		return hr;
	}

	UCHAR* destResource = (UCHAR*)mapResource.pData;
	mapResource.RowPitch = subResourceData.RowPitch;
	UCHAR* srcResource = (UCHAR*)subResourceData.pData;

	UINT copyDestWsize = (UINT)mapResource.RowPitch;
	UINT copySrcWsize = (UINT)RowPitch;

	if (copyDestWsize == copySrcWsize) {
		memcpy(destResource, srcResource, sizeof(UCHAR) * copySrcWsize * desc.Height);
	}
	else {
		for (UINT s = 0; s < desc.Height; s++) {
			memcpy(&destResource[copyDestWsize * s], &srcResource[copySrcWsize * s], sizeof(UCHAR) * copySrcWsize);
		}
	}

	up->Unmap(0, nullptr);

	if (copyTexture) {
		dx_sub[com_no].mCommandList.Get()->CopyTextureRegion(&dest, 0, 0, 0, &src, nullptr);
	}
	else {
		dx_sub[com_no].mCommandList.Get()->CopyBufferRegion(def, 0, up, 0, copySrcWsize);
	}

	return S_OK;
}

ComPtr<ID3D12Resource> Dx12Process::CreateDefaultBuffer(
	int com_no,
	const void* initData,
	UINT64 byteSize,
	ComPtr<ID3D12Resource>& uploadBuffer, bool uav)
{
	ComPtr<ID3D12Resource> defaultBuffer;
	HRESULT hr;
	D3D12_RESOURCE_STATES after = D3D12_RESOURCE_STATE_GENERIC_READ;
	Dx_Device* device = Dx_Device::GetInstance();
	if (uav) {
		hr = device->createDefaultResourceBuffer_UNORDERED_ACCESS(defaultBuffer.GetAddressOf(), byteSize);
		after = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	}
	else {
		hr = device->createDefaultResourceBuffer(defaultBuffer.GetAddressOf(), byteSize);
	}
	if (FAILED(hr)) {
		return nullptr;
	}

	UINT64 uploadBufferSize = device->getRequiredIntermediateSize(defaultBuffer.Get());
	hr = device->createUploadResource(uploadBuffer.GetAddressOf(), uploadBufferSize);
	if (FAILED(hr)) {
		return nullptr;
	}

	dx_sub[com_no].ResourceBarrier(defaultBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
	hr = CopyResourcesToGPU(com_no, uploadBuffer.Get(), defaultBuffer.Get(), initData, byteSize);
	if (FAILED(hr)) {
		return nullptr;
	}
	dx_sub[com_no].ResourceBarrier(defaultBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, after);

	return defaultBuffer;
}

void Dx12Process::Instancing(int& insNum, int numMaxIns, WVP_CB* cbArr,
	CoordTf::VECTOR3 pos, CoordTf::VECTOR3 angle, CoordTf::VECTOR3 size, CoordTf::VECTOR4 Color) {

	if (insNum >= numMaxIns) {
		Dx_Util::ErrorMessage("Error: insNum is greater than numMaxIns.");
	}

	using namespace CoordTf;

	MATRIX mov;
	MATRIX rotZ, rotY, rotX, rotZY, rotZYX;
	MATRIX scale;
	MATRIX scro;
	MATRIX world;
	MATRIX WV;

	//拡大縮小
	MatrixScaling(&scale, size.x, size.y, size.z);
	//表示位置
	MatrixRotationZ(&rotZ, angle.z);
	MatrixRotationY(&rotY, angle.y);
	MatrixRotationX(&rotX, angle.x);
	MatrixMultiply(&rotZY, &rotZ, &rotY);
	MatrixMultiply(&rotZYX, &rotZY, &rotX);
	MatrixTranslation(&mov, pos.x, pos.y, pos.z);
	MatrixMultiply(&scro, &scale, &rotZYX);
	MatrixMultiply(&world, &scro, &mov);

	//ワールド、カメラ、射影行列、等
	cbArr[insNum].world = world;
	MatrixMultiply(&WV, &world, &upd[cBuffSwap[0]].mView);
	MatrixMultiply(&cbArr[insNum].wvp, &WV, &upd[cBuffSwap[0]].mProj);
	MatrixTranspose(&cbArr[insNum].world);
	MatrixTranspose(&cbArr[insNum].wvp);

	cbArr[insNum].AddObjColor.as(Color.x, Color.y, Color.z, Color.w);

	insNum++;
}

void Dx12Process::InstancingUpdate(CONSTANT_BUFFER* cb, float disp,
	float px, float py, float mx, float my, DivideArr* divArr, int numDiv, float shininess, float SmoothRange) {

	using namespace CoordTf;

	cb->C_Pos.as(upd[cBuffSwap[0]].pos.x,
		upd[cBuffSwap[0]].pos.y,
		upd[cBuffSwap[0]].pos.z, 0.0f);
	memcpy(&cb->GlobalAmbientLight, &GlobalAmbientLight, sizeof(VECTOR4));
	cb->numLight.as((float)upd[cBuffSwap[0]].plight.LightPcs, 0.0f, 0.0f, 0.0f);
	memcpy(cb->pLightPos, upd[cBuffSwap[0]].plight.LightPos, sizeof(VECTOR4) * LIGHT_PCS);
	memcpy(cb->pLightColor, upd[cBuffSwap[0]].plight.LightColor, sizeof(VECTOR4) * LIGHT_PCS);
	memcpy(cb->pLightst, upd[cBuffSwap[0]].plight.Lightst, sizeof(VECTOR4) * LIGHT_PCS);
	cb->dDirection = upd[cBuffSwap[0]].dlight.Direction;
	cb->dLightColor = upd[cBuffSwap[0]].dlight.LightColor;
	cb->dLightst.x = upd[cBuffSwap[0]].dlight.onoff;
	cb->FogAmo_Density.as(fog.Amount, fog.Density, fog.on_off, 0.0f);
	cb->FogColor = fog.FogColor;
	cb->DispAmount.as(disp, float(numDiv), shininess, SmoothRange);
	cb->pXpYmXmY.as(px, py, mx, my);
	for (int i = 0; i < numDiv; i++) {
		cb->Divide[i].as(divArr[i].distance, divArr[i].divide, 0.0f, 0.0f);
	}
}

float Dx12Process::GetViewY_theta() {
	return ViewY_theta;
}

float Dx12Process::Getaspect() {
	return aspect;
}

float Dx12Process::GetNearPlane() {
	return NearPlane;
}

float Dx12Process::GetFarPlane() {
	return FarPlane;
}

//移動量一定化
DWORD T_float::f[2] = { timeGetTime() };
DWORD T_float::time[2] = { 0 };
DWORD T_float::time_fps[2] = { 0 };//FPS計測用
int T_float::frame[2] = { 0 };     //FPS計測使用
int T_float::up = 0;
char T_float::str[50];     //ウインドウ枠文字表示使用
float T_float::adj = 1.0f;

void T_float::GetTime(HWND hWnd) {
	time[0] = timeGetTime() - f[0];
	f[0] = timeGetTime();

	//FPS計測
	frame[0]++;
	sprintf(str, "     Ctrl:決定  Delete:キャンセル  fps=%d  ups=%d", frame[0], up);
	if (timeGetTime() - time_fps[0] > 1000)
	{
		time_fps[0] = timeGetTime();
		frame[0] = 0;
		char Name[100] = { 0 };
		GetClassNameA(hWnd, Name, sizeof(Name));
		strcat(Name, str);
		SetWindowTextA(hWnd, Name);
	}
}

void T_float::GetTimeUp(HWND hWnd) {
	time[1] = timeGetTime() - f[1];
	f[1] = timeGetTime();

	//UPS計測
	frame[1]++;
	if (timeGetTime() - time_fps[1] > 1000)
	{
		time_fps[1] = timeGetTime();
		up = frame[1];
		frame[1] = 0;
	}
}

void T_float::AddAdjust(float ad) {
	adj = ad;
}

int T_float::GetUps() {
	return up;
}

float T_float::Add(float f) {
	float r = ((float)time[1] * f) / 2.0f * adj;
	if (r <= 0.0f)return 0.01f;
	if (r >= 1000000.0f)return 1000000.0f;
	return r;
}
