//*****************************************************************************************//
//**                                                                                     **//
//**                   �@�@�@          Dx_SwapChain                                      **//
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

	//Descriptor �̃A�h���X�v�Z�Ŏg�p
	mRtvDescriptorSize = device->getDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	mDsvDescriptorSize = device->getDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

	//MultiSample���x���`�F�b�N
	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msQualityLevels;
	msQualityLevels.Format = mBackBufferFormat;
	msQualityLevels.SampleCount = 4;
	msQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
	msQualityLevels.NumQualityLevels = 0;
	if (FAILED(device->getDevice()->CheckFeatureSupport(
		D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
		&msQualityLevels,     //�@�\�̃T�|�[�g�������f�[�^���i�[
		sizeof(msQualityLevels)))) {
		throw std::runtime_error("CheckFeatureSupport Error");
	}

	m4xMsaaQuality = msQualityLevels.NumQualityLevels;
	if (m4xMsaaQuality <= 0) {
		throw std::runtime_error("Unexpected MSAA quality level.");
	}

	//������
	mSwapChain.Reset();

	//�X���b�v�`�F�C������
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

	//�X���b�v�`�F�C����RenderTarget�Ƃ��Ďg�p���邽�߂�DescriptorHeap���쐬(Descriptor�̋L�^�ꏊ)
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.NumDescriptors = SwapChainBufferCount;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;   //RenderTargetView
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE; //�V�F�[�_����A�N�Z�X���Ȃ��̂�NONE��OK
	rtvHeapDesc.NodeMask = 0;
	if (FAILED(device->getDevice()->CreateDescriptorHeap(
		&rtvHeapDesc, IID_PPV_ARGS(mRtvHeap.GetAddressOf())))) {
		throw std::runtime_error("CreateDescriptorHeap Error");
	}

	for (UINT i = 0; i < SwapChainBufferCount; i++) {
		//�X���b�v�`�F�C���o�b�t�@�擾
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

	//�[�x�X�e���V���r��-DescriptorHeap���쐬
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
	//�[�x�X�e���V���o�b�t�@�̈�m��
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

	//�[�x�X�e���V���r���[�f�X�N���v�^�[�q�[�v�̊J�n�n���h���擾
	D3D12_CPU_DESCRIPTOR_HANDLE mDsvHandle(mDsvHeap->GetCPUDescriptorHandleForHeapStart());
	//�[�x�X�e���V���r���[����
	D3D12_DEPTH_STENCIL_VIEW_DESC depthdesc = {};
	depthdesc.Format = mDepthStencilFormat;
	depthdesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	depthdesc.Flags = D3D12_DSV_FLAG_NONE;
	device->getDevice()->CreateDepthStencilView(GetDepthBuffer()->getResource(), &depthdesc, mDsvHandle);
	mDsvHeapHandle = mDsvHandle;
	mDsvHandle.ptr += mDsvDescriptorSize;

	Dx_CommandListObj* cObj = comMa->getGraphicsComListObj(0);

	cObj->Bigin();

	//�[�x�X�e���V���o�b�t�@,���\�[�X�o���A���L���[�x��������
	GetDepthBuffer()->ResourceBarrier(0, D3D12_RESOURCE_STATE_DEPTH_WRITE);

	cObj->End();
	comMa->RunGpu();
	comMa->WaitFence();

	//�r���[�|�[�g
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
	//�J������p
	ViewY_theta = 55.0f;
	// �A�X�y�N�g��̌v�Z
	aspect = (float)mScreenViewport.Width / (float)mScreenViewport.Height;
	//near�v���[��
	NearPlane = 1.0f;
	//far�v���[��
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
	//�J�����̈ʒu�ƕ�����ݒ�
	Dx_Device* dev = Dx_Device::GetInstance();
	Update& u = upd[dev->cBuffSwapUpdateIndex()];
	MatrixLookAtLH(&u.mView,
		pos, //�J�����̈ʒu
		dir, //�J�����̕�����������_
		up); //�J�����̏�̕���(�ʏ펋�_�ł̏������1.0f�ɂ���)
	//�V�F�[�_�[�v�Z�p���W�o�^(���_����̋����Ŏg��)
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
