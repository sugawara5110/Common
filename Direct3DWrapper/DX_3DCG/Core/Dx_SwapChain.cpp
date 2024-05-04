//*****************************************************************************************//
//**                                                                                     **//
//**                   　　　          Dx_SwapChain                                      **//
//**                                                                                     **//
//*****************************************************************************************//

#include "Dx_SwapChain.h"

Dx_SwapChain* Dx_SwapChain::sw = nullptr;

void Dx_SwapChain::InstanceCreate() {
	if (!sw)sw = NEW Dx_SwapChain();
}

Dx_SwapChain* Dx_SwapChain::GetInstance() {
	return sw;
}

void Dx_SwapChain::DeleteInstance() {
	if (sw) {
		delete sw;
		sw = nullptr;
	}
}

void Dx_SwapChain::Initialize(HWND hWnd, int width, int height) {

	mClientWidth = width;
	mClientHeight = height;

	Dx_Device* device = Dx_Device::GetInstance();

	//Descriptor のアドレス計算で使用
	mRtvDescriptorSize = device->getDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	mDsvDescriptorSize = device->getDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

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
		throw std::runtime_error("CheckFeatureSupport Error");
	}

	m4xMsaaQuality = msQualityLevels.NumQualityLevels;
	if (m4xMsaaQuality <= 0) {
		throw std::runtime_error("Unexpected MSAA quality level.");
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

	Dx_CommandManager* comMa = Dx_CommandManager::GetInstance();

	ComPtr<IDXGISwapChain1> swapChain;
	if (FAILED(device->getFactory()->CreateSwapChainForHwnd(
		comMa->getGraphicsQueue(),
		hWnd,
		&sd,
		nullptr,
		nullptr,
		swapChain.GetAddressOf()))) {
		throw std::runtime_error("CreateSwapChain Error");
	}
	swapChain->QueryInterface(IID_PPV_ARGS(mSwapChain.GetAddressOf()));

	//スワップチェインをRenderTargetとして使用するためのDescriptorHeapを作成(Descriptorの記録場所)
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.NumDescriptors = SwapChainBufferCount;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;   //RenderTargetView
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE; //シェーダからアクセスしないのでNONEでOK
	rtvHeapDesc.NodeMask = 0;
	if (FAILED(device->getDevice()->CreateDescriptorHeap(
		&rtvHeapDesc, IID_PPV_ARGS(mRtvHeap.GetAddressOf())))) {
		throw std::runtime_error("CreateDescriptorHeap Error");
	}

	for (UINT i = 0; i < SwapChainBufferCount; i++) {
		//スワップチェインバッファ取得
		if (FAILED(mSwapChain->GetBuffer(i, IID_PPV_ARGS(mRtBuffer[i].getResourceAddress())))) {
			throw std::runtime_error("GetSwapChainBuffer Error");
		}
		mRtBuffer[i].getResource()->SetName(Dx_Util::charToLPCWSTR("Rt"));
	}

	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.Format = mBackBufferFormat;
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(mRtvHeap->GetCPUDescriptorHandleForHeapStart());
	for (int i = 0; i < SwapChainBufferCount; i++) {
		device->getDevice()->CreateRenderTargetView(mRtBuffer[i].getResource(), &rtvDesc, rtvHeapHandle);
		mRtvHeapHandle[i] = rtvHeapHandle;
		rtvHeapHandle.ptr += mRtvDescriptorSize;
	}

	//深度ステンシルビュ-DescriptorHeapを作成
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	dsvHeapDesc.NodeMask = 0;
	if (FAILED(device->getDevice()->CreateDescriptorHeap(
		&dsvHeapDesc, IID_PPV_ARGS(mDsvHeap.GetAddressOf())))) {
		throw std::runtime_error("CreateDescriptorHeap Error");
	}

	D3D12_RESOURCE_DESC depthStencilDesc = {};
	depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	depthStencilDesc.Alignment = 0;
	depthStencilDesc.Width = mClientWidth;
	depthStencilDesc.Height = mClientHeight;
	depthStencilDesc.DepthOrArraySize = 1;
	depthStencilDesc.MipLevels = 1;
	depthStencilDesc.Format = mDepthStencilFormat;
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
		IID_PPV_ARGS(mDepthStencilBuffer.getResourceAddress())))) {
		throw std::runtime_error("CreateCommittedResource DepthStencil Error");
	}
	mDepthStencilBuffer.getResource()->SetName(Dx_Util::charToLPCWSTR("DepthStencil"));

	//深度ステンシルビューデスクリプターヒープの開始ハンドル取得
	D3D12_CPU_DESCRIPTOR_HANDLE mDsvHandle(mDsvHeap->GetCPUDescriptorHandleForHeapStart());
	//深度ステンシルビュー生成
	D3D12_DEPTH_STENCIL_VIEW_DESC depthdesc = {};
	depthdesc.Format = mDepthStencilFormat;
	depthdesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	depthdesc.Flags = D3D12_DSV_FLAG_NONE;
	device->getDevice()->CreateDepthStencilView(GetDepthBuffer()->getResource(), &depthdesc, mDsvHandle);
	mDsvHeapHandle = mDsvHandle;
	mDsvHandle.ptr += mDsvDescriptorSize;

	Dx_CommandListObj* cObj = comMa->getGraphicsComListObj(0);

	cObj->Bigin();

	//深度ステンシルバッファ,リソースバリア共有→深度書き込み
	GetDepthBuffer()->ResourceBarrier(0, D3D12_RESOURCE_STATE_DEPTH_WRITE);

	cObj->End();
	comMa->RunGpu();
	comMa->WaitFence();

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
}

void Dx_SwapChain::setPerspectiveFov(float ViewAngle, float nearPlane, float farPlane) {
	ViewY_theta = ViewAngle;
	NearPlane = nearPlane;
	FarPlane = farPlane;
	MatrixPerspectiveFovLH(&upd[0].mProj, ViewY_theta, aspect, NearPlane, FarPlane);
	MatrixPerspectiveFovLH(&upd[1].mProj, ViewY_theta, aspect, NearPlane, FarPlane);
}

void Dx_SwapChain::BiginDraw(int com_no, bool clearBackBuffer) {

	Dx_CommandListObj* cObj = Dx_CommandManager::GetInstance()->getGraphicsComListObj(com_no);

	cObj->getCommandList()->RSSetViewports(1, &mScreenViewport);
	cObj->getCommandList()->RSSetScissorRects(1, &mScissorRect);

	GetRtBuffer()->ResourceBarrier(com_no, D3D12_RESOURCE_STATE_RENDER_TARGET);

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = mRtvHeapHandle[mCurrBackBuffer];

	if (clearBackBuffer) {
		cObj->getCommandList()->ClearRenderTargetView(rtvHandle,
			DirectX::Colors::Black, 0, nullptr);
		cObj->getCommandList()->ClearDepthStencilView(mDsvHeapHandle,
			D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
	}

	cObj->getCommandList()->OMSetRenderTargets(1, &rtvHandle,
		false, &mDsvHeapHandle);
}

void Dx_SwapChain::EndDraw(int com_no) {

	Dx_CommandListObj* cObj = Dx_CommandManager::GetInstance()->getGraphicsComListObj(com_no);

	GetRtBuffer()->ResourceBarrier(com_no, D3D12_RESOURCE_STATE_PRESENT);
}

void Dx_SwapChain::DrawScreen() {
	// swap the back and front buffers
	mSwapChain->Present(0, 0);
	mCurrBackBuffer = mSwapChain->GetCurrentBackBufferIndex();
}

void Dx_SwapChain::Cameraset(CoordTf::VECTOR3 pos, CoordTf::VECTOR3 dir, CoordTf::VECTOR3 up) {
	//カメラの位置と方向を設定
	Dx_Device* dev = Dx_Device::GetInstance();
	Update& u = upd[dev->cBuffSwapUpdateIndex()];
	MatrixLookAtLH(&u.mView,
		pos, //カメラの位置
		dir, //カメラの方向を向ける点
		up); //カメラの上の方向(通常視点での上方向を1.0fにする)
	//シェーダー計算用座標登録(視点からの距離で使う)
	u.pos.as(pos.x, pos.y, pos.z);
	u.dir.as(dir.x, dir.y, dir.z);
	u.up.as(up.x, up.y, up.z);
}

float Dx_SwapChain::GetViewY_theta() {
	return ViewY_theta;
}

float Dx_SwapChain::Getaspect() {
	return aspect;
}

float Dx_SwapChain::GetNearPlane() {
	return NearPlane;
}

float Dx_SwapChain::GetFarPlane() {
	return FarPlane;
}

Dx_Resource* Dx_SwapChain::GetRtBuffer() {
	return &mRtBuffer[mCurrBackBuffer];
}

Dx_Resource* Dx_SwapChain::GetDepthBuffer() {
	return &mDepthStencilBuffer;
}
