//*****************************************************************************************//
//**                                                                                     **//
//**                   　　　          Dx_SwapChain                                      **//
//**                                                                                     **//
//*****************************************************************************************//

#ifndef Class_Dx_SwapChain_Header
#define Class_Dx_SwapChain_Header

#include "Dx_TextureHolder.h"
#include <DirectXColors.h>

class Dx_SwapChain;

class CameraData {

public:
	CoordTf::MATRIX View{};
	CoordTf::MATRIX Proj{};
	CoordTf::MATRIX CurrentVP{};
	CoordTf::MATRIX PreviousVP{};
	CoordTf::VECTOR3 Position{};
	CoordTf::VECTOR3 Right{};
	CoordTf::VECTOR3 Up{};
	CoordTf::VECTOR3 Forward{};
	float Near = 0;
	float Far = 0;
	float Fov = 0;
	uint32_t Width = 0;
	uint32_t Height = 0;

	Dx_Util::Jitter jitter{};

	bool getFrameIndexReset()const {
		if (FrameIndex <= 0)return true;
		return false;
	}

private:
	friend Dx_SwapChain;
	uint32_t FrameIndex = 0;
	bool Jitter_F = false;

	void countUpFrameIndex() {
		if (INT_MAX <= FrameIndex++) {
			FrameIndex = 0;
		}
	}
};

class Dx_SwapChain {

public:
	struct Update {
		CoordTf::MATRIX mProj = {};
		CoordTf::MATRIX mView = {};

		CoordTf::VECTOR3 pos = {};
		CoordTf::VECTOR3 dir = {};
		CoordTf::VECTOR3 up = {};

		CoordTf::MATRIX currViewProjection = {};
		CoordTf::MATRIX prevViewProjection = {};
	};

private:
	ComPtr<IDXGISwapChain3> mSwapChain;

	//MultiSampleレベルチェック
	bool m4xMsaaState = false;
	UINT m4xMsaaQuality = 0;

	static const int SwapChainBufferCount = 2;
	int mCurrBackBuffer = 0;

	D3D12_CPU_DESCRIPTOR_HANDLE mRtvHeapHandle[SwapChainBufferCount] = {};
	Dx_Resource mRtBuffer[SwapChainBufferCount] = {};
	D3D12_CPU_DESCRIPTOR_HANDLE mDsvHeapHandle = {};
	Dx_Resource mDepthStencilBuffer = {};

	ComPtr<ID3D12DescriptorHeap> mRtvHeap;
	ComPtr<ID3D12DescriptorHeap> mDsvHeap;

	D3D12_VIEWPORT mScreenViewport;
	D3D12_RECT mScissorRect;

	UINT mRtvDescriptorSize = 0;
	UINT mDsvDescriptorSize = 0;

	DXGI_FORMAT mBackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	DXGI_FORMAT mDepthStencilResFormat = DXGI_FORMAT_R32_TYPELESS;
	DXGI_FORMAT mDepthStencilFormat = DXGI_FORMAT_D32_FLOAT;
	DXGI_FORMAT mDepthStencilSrvFormat = DXGI_FORMAT_R32_FLOAT;
	uint32_t mClientWidth;
	uint32_t mClientHeight;

	Update upd[2] = {};//cBuffSwap
	CameraData cam = {};

	//カメラ画角
	float ViewY_theta = 0.0f;
	//アスペクト比
	float aspect = 0.0f;
	//nearプレーン
	float NearPlane = 0.0f;
	//farプレーン
	float FarPlane = 0.0f;

	static Dx_SwapChain* sw;
	Dx_SwapChain() {}//外部からのオブジェクト生成禁止
	Dx_SwapChain(const Dx_SwapChain& obj) = delete;   // コピーコンストラクタ禁止
	void operator=(const Dx_SwapChain& obj) = delete;// 代入演算子禁止

public:
	static void InstanceCreate();
	static Dx_SwapChain* GetInstance();
	static void DeleteInstance();

	void Initialize(HWND hWnd, uint32_t width = 800, uint32_t height = 600);

	void setPerspectiveFov(float ViewAngle, float NearPlane, float FarPlane);

	void BiginDraw(int com_no, bool clearBackBuffer = true);
	void EndDraw(int com_no);
	void DrawScreen();

	void Jitter_SW(bool sw);
	void Cameraset(CoordTf::VECTOR3 pos, CoordTf::VECTOR3 dir, CoordTf::VECTOR3 up = { 0.0f,0.0f,1.0f });

	float GetViewY_theta();
	float Getaspect();
	float GetNearPlane();
	float GetFarPlane();

	Update getUpdate(int index) { return upd[index]; }

	DXGI_FORMAT getDepthStencilSrvFormat() { return mDepthStencilSrvFormat; }

	uint32_t getClientWidth()const { return mClientWidth; }
	uint32_t getClientHeight()const { return mClientHeight; }
	Dx_Resource* GetRtBuffer();
	Dx_Resource* GetDepthBuffer();

	bool get_m4xMsaaState() { return m4xMsaaState; }
	UINT get_m4xMsaaQuality() { return m4xMsaaQuality; }

	IDXGISwapChain3* getSwapChain();

	CameraData getCameraData();
};

#endif