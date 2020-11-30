//*****************************************************************************************//
//**                                                                                     **//
//**                   　　　       Dx12ProcessCoreクラス                                **//
//**                                                                                     **//
//*****************************************************************************************//

#ifndef Class_Dx12ProcessCore_Header
#define Class_Dx12ProcessCore_Header

#if defined(DEBUG) || defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#define _CRT_SECURE_NO_WARNINGS
#include "DxStruct.h"
#include "DxEnum.h"
#include <d3d12.h>
#include "../MicroSoftLibrary/d3dx12.h"
#include <d3d10_1.h>
#include <D3Dcompiler.h>
#include <DirectXColors.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include <array>
#include <assert.h>
#include <Process.h>
#include <mutex>
#include <new>
#include <typeinfo>

#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "dxgi.lib")

#define COM_NO 7
using Microsoft::WRL::ComPtr;

//前方宣言
template<class T>
class ConstantBuffer;
class Dx12Process;
class MeshData;
class PolygonData;
class PolygonData2D;
class ParticleData;
class SkinMesh;
class DxText;
class Wave;
class PostEffect;
class Common;
class DxNNCommon;
class DxNeuralNetwork;
class DxPooling;
class DxConvolution;
class SearchPixel;
class DxGradCAM;
class DxActivation;
class DxOptimizer;
class DXR_Basic;
class SkinnedCom;
struct StreamView;
void ErrorMessage(char* E_mes);
//前方宣言

class Dx12Process_sub final {

private:
	friend Dx12Process;
	friend MeshData;
	friend PolygonData;
	friend PolygonData2D;
	friend ParticleData;
	friend SkinMesh;
	friend Wave;
	friend PostEffect;
	friend Common;
	friend DxNNCommon;
	friend DxNeuralNetwork;
	friend DxPooling;
	friend DxConvolution;
	friend SearchPixel;
	friend DxGradCAM;
	friend DxActivation;
	friend DxOptimizer;
	friend DXR_Basic;
	friend SkinnedCom;
	friend StreamView;

	ComPtr<ID3D12CommandAllocator> mCmdListAlloc[2];
	ComPtr<ID3D12GraphicsCommandList4> mCommandList;
	int mAloc_Num = 0;
	volatile ComListState mComState = {};

	bool ListCreate(bool Compute);
	void ResourceBarrier(ID3D12Resource* res, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after);
	void CopyResourceGENERIC_READ(ID3D12Resource* dest, ID3D12Resource* src);
	void Bigin();
	void End();

	static int NumResourceBarrier;

	struct CopyList {
		ID3D12Resource* dest;
		ID3D12Resource* src;
	};

	std::unique_ptr<D3D12_RESOURCE_BARRIER[]> beforeBa = nullptr;
	int beforeCnt = 0;
	std::unique_ptr<CopyList[]> copy = nullptr;
	int copyCnt = 0;
	std::unique_ptr <D3D12_RESOURCE_BARRIER[]> afterBa = nullptr;
	int afterCnt = 0;
	std::unique_ptr <D3D12_RESOURCE_BARRIER[]> uavBa = nullptr;
	int uavCnt = 0;

	void createResourceBarrierList();
	void createUavResourceBarrierList();
	void delayResourceBarrierBefore(ID3D12Resource* res, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after);
	void delayCopyResource(ID3D12Resource* dest, ID3D12Resource* src);
	void delayResourceBarrierAfter(ID3D12Resource* res, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after);
	void delayUavResourceBarrier(ID3D12Resource* res);
	void RunDelayResourceBarrierBefore();
	void RunDelayCopyResource();
	void RunDelayResourceBarrierAfter();
	void RunDelayUavResourceBarrier();
};

class DxCommandQueue final {

private:
	friend Dx12Process;

	ComPtr<ID3D12CommandQueue> mCommandQueue;
	ComPtr<ID3D12Fence> mFence;
	UINT64 mCurrentFence = 0;

	bool Create(ID3D12Device5* dev, bool Compute);
	void setCommandList(UINT numList, ID3D12CommandList* const* cmdsLists);
	void waitFence();
};

class Dx12Process final {

private:
	template<class T>
	friend class ConstantBuffer;
	friend MeshData;
	friend PolygonData;
	friend PolygonData2D;
	friend ParticleData;
	friend SkinMesh;
	friend Dx12Process_sub;
	friend Wave;
	friend PostEffect;
	friend Common;
	friend DxNNCommon;
	friend DxNeuralNetwork;
	friend DxPooling;
	friend DxConvolution;
	friend SearchPixel;
	friend DxGradCAM;
	friend DxActivation;
	friend DxOptimizer;
	friend DXR_Basic;
	friend SkinnedCom;
	friend StreamView;

	ComPtr<IDXGIFactory4> mdxgiFactory;
	ComPtr<ID3D12Device5> md3dDevice;
	ComPtr<IDXGISwapChain3> mSwapChain;
	bool DXR_CreateResource = false;

	//MultiSampleレベルチェック
	bool m4xMsaaState = false;
	UINT m4xMsaaQuality = 0;

	DxCommandQueue graphicsQueue;
	DxCommandQueue computeQueue;
	Dx12Process_sub dx_sub[COM_NO];
	Dx12Process_sub dx_subCom[COM_NO];

	static const int SwapChainBufferCount = 2;
	int mCurrBackBuffer = 0;
	ComPtr<ID3D12Resource> mSwapChainBuffer[SwapChainBufferCount];
	ComPtr<ID3D12Resource> mDepthStencilBuffer;

	ComPtr<ID3D12DescriptorHeap> mRtvHeap;
	ComPtr<ID3D12DescriptorHeap> mDsvHeap;

	//シェーダーバイトコード
	ComPtr<ID3DBlob> pGeometryShader_PSO = nullptr;
	ComPtr<ID3DBlob> pGeometryShader_P = nullptr;
	ComPtr<ID3DBlob> pGeometryShader_Before_ds = nullptr;
	ComPtr<ID3DBlob> pGeometryShader_Before_vs = nullptr;
	ComPtr<ID3DBlob> pGeometryShader_Before_ds_NoNormalMap = nullptr;
	ComPtr<ID3DBlob> pGeometryShader_Before_vs_NoNormalMap = nullptr;
	ComPtr<ID3DBlob> pGeometryShader_Before_vs_Output = nullptr;
	ComPtr<ID3DBlob> pGeometryShader_Before_ds_Output = nullptr;
	ComPtr<ID3DBlob> pGeometryShader_P_Output = nullptr;

	ComPtr<ID3DBlob> pHullShaderTriangle = nullptr;

	ComPtr<ID3DBlob> pDomainShader_Wave = nullptr;
	ComPtr<ID3DBlob> pDomainShaderTriangle = nullptr;

	std::vector<D3D12_INPUT_ELEMENT_DESC> pVertexLayout_SKIN;
	std::vector<D3D12_SO_DECLARATION_ENTRY> pDeclaration_PSO;
	std::vector<D3D12_SO_DECLARATION_ENTRY> pDeclaration_Output;
	std::vector<D3D12_INPUT_ELEMENT_DESC> pVertexLayout_P;
	std::vector<D3D12_INPUT_ELEMENT_DESC> pVertexLayout_MESH;
	std::vector<D3D12_INPUT_ELEMENT_DESC> pVertexLayout_3DBC;
	std::vector<D3D12_INPUT_ELEMENT_DESC> pVertexLayout_2D;

	ComPtr<ID3DBlob> pVertexShader_SKIN = nullptr;
	ComPtr<ID3DBlob> pVertexShader_SKIN_D = nullptr;
	ComPtr<ID3DBlob> pVertexShader_PSO = nullptr;
	ComPtr<ID3DBlob> pVertexShader_P = nullptr;
	ComPtr<ID3DBlob> pVertexShader_MESH_D = nullptr;
	ComPtr<ID3DBlob> pVertexShader_MESH = nullptr;
	ComPtr<ID3DBlob> pVertexShader_TC = nullptr;
	ComPtr<ID3DBlob> pVertexShader_BC = nullptr;
	ComPtr<ID3DBlob> pVertexShader_2D = nullptr;
	ComPtr<ID3DBlob> pVertexShader_2DTC = nullptr;

	ComPtr<ID3DBlob> pPixelShader_P = nullptr;
	ComPtr<ID3DBlob> pPixelShader_3D = nullptr;
	ComPtr<ID3DBlob> pPixelShader_3D_NoNormalMap = nullptr;
	ComPtr<ID3DBlob> pPixelShader_Emissive = nullptr;
	ComPtr<ID3DBlob> pPixelShader_BC = nullptr;
	ComPtr<ID3DBlob> pPixelShader_2D = nullptr;
	ComPtr<ID3DBlob> pPixelShader_2DTC = nullptr;

	ComPtr<ID3DBlob> pComputeShader_Wave = nullptr;
	ComPtr<ID3DBlob> pComputeShader_Post[2] = { nullptr };
	ComPtr<ID3DBlob> pVertexShader_SKIN_Com = nullptr;

	std::unique_ptr<char[]> ShaderNormalTangentCopy = nullptr;
	std::unique_ptr<char[]> ShaderCalculateLightingCopy = nullptr;

	bool CreateShaderByteCodeBool = true;

	D3D12_VIEWPORT mScreenViewport;
	D3D12_RECT mScissorRect;

	UINT mRtvDescriptorSize = 0;
	UINT mDsvDescriptorSize = 0;
	UINT mCbvSrvUavDescriptorSize = 0;

	DXGI_FORMAT mBackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	DXGI_FORMAT mDepthStencilFormat = DXGI_FORMAT_D32_FLOAT;
	int mClientWidth;
	int mClientHeight;

	//テクスチャ
	int texNum = 0;    //配列数
	InternalTexture* texture = nullptr;

	static Dx12Process* dx;//クラス内でオブジェクト生成し使いまわす
	static std::mutex mtx;
	static std::mutex mtxGraphicsQueue;
	static std::mutex mtxComputeQueue;

	struct Update {
		MATRIX mProj = {};
		MATRIX mView = {};

		VECTOR3 pos = {};
		VECTOR3 dir = {};
		VECTOR3 up = {};

		PointLight plight = {};
		int lightNum = 0;
		DirectionLight dlight = {};
	};
	Update upd[2] = {};//cBuffSwap

	MATRIX Vp;    //ビューポート行列(3D座標→2D座標変換時使用)
	Fog fog;

	//カメラ画角
	float ViewY_theta;
	//アスペクト比
	float aspect;
	//nearプレーン
	float NearPlane;
	//farプレーン
	float FarPlane;
	VECTOR4 GlobalAmbientLight = { 0.1f,0.1f,0.1f,0.0f };

	int cBuffSwap[2] = { 0,0 };
	int dxrBuffSwap[2] = { 0,0 };

	bool wireframe = false;

	Dx12Process() {}//外部からのオブジェクト生成禁止
	Dx12Process(const Dx12Process& obj) {}   // コピーコンストラクタ禁止
	void operator=(const Dx12Process& obj) {}// 代入演算子禁止
	~Dx12Process();

	bool CreateShaderByteCode();
	UINT64 getRequiredIntermediateSize(ID3D12Resource* res);
	HRESULT CopyResourcesToGPU(int com_no, ID3D12Resource* up, ID3D12Resource* def,
		const void* initData, LONG_PTR RowPitch);

	ComPtr<ID3D12Resource> CreateDefaultBuffer(int com_no,
		const void* initData, UINT64 byteSize, ComPtr<ID3D12Resource>& uploadBuffer, bool uav);

	ComPtr<ID3D12Resource> CreateStreamBuffer(UINT64 byteSize);

	ComPtr<ID3DBlob> CompileShader(LPSTR szFileName, size_t size, LPSTR szFuncName, LPSTR szProfileName);

	void Instancing(int& insNum, CONSTANT_BUFFER* cb, VECTOR3 pos, VECTOR3 angle, VECTOR3 size);

	void InstancingUpdate(CONSTANT_BUFFER* cb, VECTOR4 Color, float disp,
		float px, float py, float mx, float my, DivideArr* divArr, int numDiv, float shininess);

	HRESULT createDefaultResourceTEXTURE2D(ID3D12Resource** def, UINT64 width, UINT height,
		DXGI_FORMAT format, D3D12_RESOURCE_STATES firstState = D3D12_RESOURCE_STATE_COMMON);

	HRESULT createDefaultResourceTEXTURE2D_UNORDERED_ACCESS(ID3D12Resource** def, UINT64 width, UINT height,
		D3D12_RESOURCE_STATES firstState = D3D12_RESOURCE_STATE_COMMON,
		DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM);

	HRESULT createDefaultResourceBuffer(ID3D12Resource** def, UINT64 bufferSize,
		D3D12_RESOURCE_STATES firstState = D3D12_RESOURCE_STATE_COMMON);

	HRESULT createDefaultResourceBuffer_UNORDERED_ACCESS(ID3D12Resource** def, UINT64 bufferSize,
		D3D12_RESOURCE_STATES firstState = D3D12_RESOURCE_STATE_COMMON);

	HRESULT createUploadResource(ID3D12Resource** up, UINT64 uploadBufferSize);

	HRESULT createReadBackResource(ID3D12Resource** ba, UINT64 BufferSize);

	HRESULT textureInit(int width, int height,
		ID3D12Resource** up, ID3D12Resource** def, DXGI_FORMAT format,
		D3D12_RESOURCE_STATES firstState);

	HRESULT createTexture(int com_no, UCHAR* byteArr, DXGI_FORMAT format,
		ID3D12Resource** up, ID3D12Resource** def,
		int width, LONG_PTR RowPitch, int height);

	ComPtr<ID3D12RootSignature> CreateRsCommon(D3D12_ROOT_SIGNATURE_DESC* rootSigDesc);
	ComPtr<ID3D12DescriptorHeap> CreateDescHeap(int numDesc);
	ComPtr<ID3D12DescriptorHeap> CreateSamplerDescHeap(D3D12_SAMPLER_DESC& descSampler);

public:
	static void InstanceCreate();
	static Dx12Process* GetInstance();
	static void DeleteInstance();

	static void Lock() { mtx.lock(); }
	static void Unlock() { mtx.unlock(); }

	void setNumResourceBarrier(int num) { Dx12Process_sub::NumResourceBarrier = num; }

	void dxrCreateResource() { DXR_CreateResource = true; }
	bool Initialize(HWND hWnd, int width = 800, int height = 600);
	ID3D12Device5* getDevice() { return md3dDevice.Get(); }
	char* GetNameFromPass(char* pass);//パスからファイル名を抽出
	int GetTexNumber(CHAR* fileName);//リソースとして登録済みのテクスチャ配列番号をファイル名から取得

	void createTextureArr(int numTexArr, int resourceIndex, char* texName,
		UCHAR* byteArr, DXGI_FORMAT format,
		int width, LONG_PTR RowPitch, int height);

	void Bigin(int com_no);
	void BiginDraw(int com_no, bool clearBackBuffer = true);
	void EndDraw(int com_no);
	void End(int com_no);
	void setUpSwapIndex(int index) { cBuffSwap[0] = index; }
	void setDrawSwapIndex(int index) { cBuffSwap[1] = index; }
	void setStreamOutputSwapIndex(int index) { dxrBuffSwap[0] = index; }
	void setRaytraceSwapIndex(int index) { dxrBuffSwap[1] = index; }
	void WaitFenceNotLock();
	void WaitFence();
	void DrawScreen();
	void BiginCom(int com_no);
	void EndCom(int com_no);
	void WaitFenceNotLockCom();
	void WaitFenceCom();
	void Cameraset(VECTOR3 pos, VECTOR3 dir, VECTOR3 up = { 0.0f,0.0f,1.0f });

	void ResetPointLight();
	void setGlobalAmbientLight(float r, float g, float b) {
		GlobalAmbientLight.as(r, g, b, 0.0f);
	}

	bool PointLightPosSet(int Idx, VECTOR3 pos, VECTOR4 Color, bool on_off,
		float range, VECTOR3 atten = { 0.01f, 0.001f, 0.001f });

	void DirectionLight(float x, float y, float z, float r, float g, float b);
	void SetDirectionLight(bool onoff);
	void Fog(float r, float g, float b, float amount, float density, bool onoff);
	float GetViewY_theta();
	float Getaspect();
	float GetNearPlane();
	float GetFarPlane();
	InternalTexture* getInternalTexture(int index) { return &texture[index]; }
	void wireFrameTest(bool f) { wireframe = f; }
	void setPerspectiveFov(float ViewAngle, float NearPlane, float FarPlane);
};

struct VertexView {

	//各パラメーターを自分でコピーする
	ComPtr<ID3D12Resource> VertexBufferGPU = nullptr;
	ComPtr<ID3D12Resource> VertexBufferUploader = nullptr;

	//バッファのサイズ等
	UINT VertexByteStride = 0;
	UINT VertexBufferByteSize = 0;

	//頂点バッファビュー
	D3D12_VERTEX_BUFFER_VIEW VertexBufferView()const
	{
		D3D12_VERTEX_BUFFER_VIEW vbv;
		vbv.BufferLocation = VertexBufferGPU->GetGPUVirtualAddress();
		vbv.StrideInBytes = VertexByteStride;
		vbv.SizeInBytes = VertexBufferByteSize;

		return vbv;
	}
};

struct IndexView {

	//各パラメーターを自分でコピーする
	ComPtr<ID3D12Resource> IndexBufferGPU = nullptr;
	ComPtr<ID3D12Resource> IndexBufferUploader = nullptr;

	//バッファのサイズ等
	DXGI_FORMAT IndexFormat = DXGI_FORMAT_R32_UINT;
	UINT IndexBufferByteSize = 0;
	//DrawIndexedInstancedの引数で使用
	UINT IndexCount = 0;

	//インデックスバッファビュー
	D3D12_INDEX_BUFFER_VIEW IndexBufferView()const
	{
		D3D12_INDEX_BUFFER_VIEW ibv;
		ibv.BufferLocation = IndexBufferGPU->GetGPUVirtualAddress();
		ibv.Format = IndexFormat;
		ibv.SizeInBytes = IndexBufferByteSize;

		return ibv;
	}
};

struct StreamView {

	ComPtr<ID3D12Resource> StreamBufferGPU = nullptr;
	ComPtr<ID3D12Resource> BufferFilledSizeBufferGPU = nullptr;
	ComPtr<ID3D12Resource> ReadBuffer = nullptr;

	static ComPtr<ID3D12Resource> UpresetBuffer;
	static ComPtr<ID3D12Resource> resetBuffer;

	//バッファのサイズ等
	UINT StreamByteStride = 0;
	UINT StreamBufferByteSize = 0;
	UINT FilledSize = 0;

	static void createResetBuffer(int comNo) {
		Dx12Process* dx = Dx12Process::GetInstance();
		UINT64 zero[1];
		zero[0] = 0;
		resetBuffer = dx->CreateDefaultBuffer(comNo, zero,
			sizeof(UINT64),
			UpresetBuffer, false);
	}

	StreamView() {
		Dx12Process* dx = Dx12Process::GetInstance();
		BufferFilledSizeBufferGPU = dx->CreateStreamBuffer(sizeof(UINT64));
		dx->createReadBackResource(ReadBuffer.GetAddressOf(), sizeof(UINT64));
	}

	void ResetFilledSizeBuffer(int com) {
		Dx12Process* dx = Dx12Process::GetInstance();
		dx->dx_sub[com].delayResourceBarrierBefore(BufferFilledSizeBufferGPU.Get(),
			D3D12_RESOURCE_STATE_STREAM_OUT, D3D12_RESOURCE_STATE_COPY_DEST);

		dx->dx_sub[com].delayCopyResource(BufferFilledSizeBufferGPU.Get(),
			resetBuffer.Get());

		dx->dx_sub[com].delayResourceBarrierAfter(BufferFilledSizeBufferGPU.Get(),
			D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_STREAM_OUT);
	}

	void outputReadBack(int com) {
		Dx12Process* dx = Dx12Process::GetInstance();
		ID3D12GraphicsCommandList* mCList = dx->dx_sub[com].mCommandList.Get();
		dx->dx_sub[com].ResourceBarrier(BufferFilledSizeBufferGPU.Get(),
			D3D12_RESOURCE_STATE_STREAM_OUT, D3D12_RESOURCE_STATE_COPY_SOURCE);

		mCList->CopyResource(ReadBuffer.Get(), BufferFilledSizeBufferGPU.Get());

		dx->dx_sub[com].ResourceBarrier(BufferFilledSizeBufferGPU.Get(),
			D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_STREAM_OUT);
	}

	void outputFilledSize() {
		D3D12_RANGE range;
		range.Begin = 0;
		range.End = sizeof(UINT64);
		UINT* ba = nullptr;
		ReadBuffer.Get()->Map(0, &range, reinterpret_cast<void**>(&ba));
		FilledSize = ba[0];
		ReadBuffer.Get()->Unmap(0, nullptr);
	}

	//ストリームバッファビュー
	D3D12_STREAM_OUTPUT_BUFFER_VIEW StreamBufferView()const
	{
		D3D12_STREAM_OUTPUT_BUFFER_VIEW sbv;
		sbv.BufferLocation = StreamBufferGPU->GetGPUVirtualAddress();
		sbv.SizeInBytes = StreamBufferByteSize;
		sbv.BufferFilledSizeLocation = BufferFilledSizeBufferGPU.Get()->GetGPUVirtualAddress();
		return sbv;
	}
};

template<class T>
class ConstantBuffer {

private:
	ComPtr<ID3D12Resource> mUploadBuffer;
	BYTE* mMappedData = nullptr;
	UINT mElementByteSize = 0;
	Dx12Process* dx = nullptr;

public:
	ConstantBuffer(UINT elementCount) {

		dx = Dx12Process::GetInstance();
		//コンスタントバッファサイズは256バイトでアライメント
		mElementByteSize = ALIGNMENT_TO(256, sizeof(T));

		if (FAILED(dx->createUploadResource(&mUploadBuffer, (UINT64)mElementByteSize * elementCount))) {
			ErrorMessage("ConstantBufferエラー");
		}

		mUploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(&mMappedData));
	}

	ConstantBuffer(const ConstantBuffer& rhs) = delete;
	ConstantBuffer& operator=(const ConstantBuffer& rhs) = delete;

	~ConstantBuffer() {

		if (mUploadBuffer != nullptr)
			mUploadBuffer->Unmap(0, nullptr);

		mMappedData = nullptr;
	}

	ID3D12Resource* Resource()const {
		return mUploadBuffer.Get();
	}

	D3D12_GPU_VIRTUAL_ADDRESS getGPUVirtualAddress(UINT elementIndex) {
		return mUploadBuffer.Get()->GetGPUVirtualAddress() + 256 * elementIndex;
	}

	void CopyData(int elementIndex, const T& data) {
		memcpy(&mMappedData[elementIndex * mElementByteSize], &data, sizeof(T));
	}

	UINT getSizeInBytes() {
		return mElementByteSize;
	}

	UINT getElementByteSize() {
		return 256;
	}
};

struct drawPara {
	int NumMaterial = 0;
	int numDesc = 0;
	ComPtr<ID3D12DescriptorHeap> descHeap = nullptr;
	ComPtr<ID3D12RootSignature> rootSignature = nullptr;
	ComPtr<ID3D12RootSignature> rootSignatureDXR = nullptr;
	std::unique_ptr<VertexView> Vview = nullptr;
	std::unique_ptr<IndexView[]> Iview = nullptr;
	std::unique_ptr<MY_MATERIAL_S[]> material = nullptr;
	D3D_PRIMITIVE_TOPOLOGY TOPOLOGY;
	std::unique_ptr<ComPtr<ID3D12PipelineState>[]> PSO = nullptr;
	std::unique_ptr<ComPtr<ID3D12PipelineState>[]> PSO_DXR = nullptr;
	UINT insNum = 1;
};

struct UpdateDXR {
	UINT NumInstance = 1;
	std::unique_ptr<std::unique_ptr<VertexView[]>[]> VviewDXR = nullptr;
	std::unique_ptr<std::unique_ptr<UINT[]>[]> currentIndexCount = nullptr;
	MATRIX Transform[INSTANCE_PCS_3D] = {};
	MATRIX WVP[INSTANCE_PCS_3D] = {};
	VECTOR4 AddObjColor = {};//オブジェクトの色変化用
	float shininess;
	std::unique_ptr<UINT[]> InstanceID = nullptr;
	bool firstSet = false;//VviewDXRの最初のデータ更新完了フラグ
	bool createAS = false;//ASの最初の構築完了フラグ
	UINT InstanceMask = 0xFF;

	void InstanceMaskChange(bool DrawOn) {
		if (DrawOn)InstanceMask = 0xFF;
		else InstanceMask = 0x00;
	}

	void create(int numMaterial, int numMaxInstance) {
		VviewDXR = std::make_unique<std::unique_ptr<VertexView[]>[]>(numMaterial);
		currentIndexCount = std::make_unique<std::unique_ptr<UINT[]>[]>(numMaterial);
		for (int i = 0; i < numMaterial; i++) {
			VviewDXR[i] = std::make_unique<VertexView[]>(numMaxInstance);
			currentIndexCount[i] = std::make_unique<UINT[]>(numMaxInstance);
		}
	}
};

struct ParameterDXR {
	int NumMaterial = 0;
	UINT NumMaxInstance = 1;
	bool hs = false;
	std::unique_ptr<ID3D12Resource* []>difTex = nullptr;
	std::unique_ptr<ID3D12Resource* []>norTex = nullptr;
	std::unique_ptr<ID3D12Resource* []>speTex = nullptr;
	std::unique_ptr<VECTOR4[]> diffuse = nullptr;
	std::unique_ptr<VECTOR4[]> specular = nullptr;
	std::unique_ptr<VECTOR4[]> ambient = nullptr;
	std::unique_ptr<IndexView[]> IviewDXR = nullptr;
	std::unique_ptr<std::unique_ptr<StreamView[]>[]> SviewDXR = nullptr;
	UpdateDXR updateDXR[2] = {};
	bool updateF = false;//AS構築後のupdateの有無

	void create(int numMaterial, int numMaxInstance) {
		NumMaxInstance = numMaxInstance;
		IviewDXR = std::make_unique<IndexView[]>(numMaterial);
		difTex = std::make_unique<ID3D12Resource* []>(numMaterial);
		norTex = std::make_unique<ID3D12Resource* []>(numMaterial);
		speTex = std::make_unique<ID3D12Resource* []>(numMaterial);
		diffuse = std::make_unique<VECTOR4[]>(numMaterial);
		specular = std::make_unique<VECTOR4[]>(numMaterial);
		ambient = std::make_unique<VECTOR4[]>(numMaterial);
		SviewDXR = std::make_unique<std::unique_ptr<StreamView[]>[]>(numMaterial);
		for (int i = 0; i < numMaterial; i++) {
			SviewDXR[i] = std::make_unique<StreamView[]>(numMaxInstance);
		}
		updateDXR[0].create(numMaterial, numMaxInstance);
		updateDXR[1].create(numMaterial, numMaxInstance);
	}

	bool alphaTest;
	//ParticleData用
	bool useVertex = false;
	UINT numVertex = 1;
	std::unique_ptr<VECTOR3[]> v = nullptr;
};

//**********************************Commonクラス*************************************//
class Common {

protected:
	friend PolygonData;
	friend PolygonData2D;
	friend ParticleData;
	friend PostEffect;
	friend SkinnedCom;
	Common() {};//外部からのオブジェクト生成禁止
	Common(const Common& obj) {}     // コピーコンストラクタ禁止
	void operator=(const Common& obj) {}// 代入演算子禁止

	Dx12Process* dx;
	ID3D12GraphicsCommandList* mCommandList;
	int com_no = 0;

	//テクスチャ
	ComPtr<ID3D12Resource> textureUp[256] = {};
	ComPtr<ID3D12Resource> texture[256] = {};
	MovieTexture movOn[256] = {};

	D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint;
	D3D12_TEXTURE_COPY_LOCATION dest, src;

	HRESULT createTextureResource(int resourceStartIndex, int MaterialNum, TextureNo* to);

	void CreateSrvTexture(ID3D12DescriptorHeap* heap, int offsetHeap, ID3D12Resource** texture, int texNum);

	void CreateSrvBuffer(ID3D12DescriptorHeap* heap, int offsetHeap, ID3D12Resource** buffer, int bufNum,
		UINT* StructureByteStride);

	void CreateCbv(ID3D12DescriptorHeap* heap, int offsetHeap,
		D3D12_GPU_VIRTUAL_ADDRESS* virtualAddress, UINT* sizeInBytes, int bufNum);

	void CreateUavBuffer(ID3D12DescriptorHeap* heap, int offsetHeap,
		ID3D12Resource** buffer, UINT* byteStride, UINT* bufferSize, int bufNum);

	ComPtr<ID3D12RootSignature> CreateRootSignature(UINT numSrv, UINT numCbv, UINT numUav, UINT numCbvPara, UINT RegisterStNoCbv);
	ComPtr<ID3D12RootSignature> CreateRs(int paramNum, D3D12_ROOT_PARAMETER* slotRootParameter);
	ComPtr<ID3D12RootSignature> CreateRsStreamOutput(int paramNum, D3D12_ROOT_PARAMETER* slotRootParameter);
	ComPtr<ID3D12RootSignature> CreateRsStreamOutputSampler(int paramNum, D3D12_ROOT_PARAMETER* slotRootParameter);
	ComPtr<ID3D12RootSignature> CreateRsCompute(int paramNum, D3D12_ROOT_PARAMETER* slotRootParameter);
	ComPtr<ID3D12RootSignature> CreateRootSignatureCompute(UINT numSrv, UINT numCbv, UINT numUav, UINT numCbvPara, UINT RegisterStNoCbv);
	ComPtr<ID3D12RootSignature> CreateRootSignatureStreamOutput(UINT numSrv, UINT numCbv, UINT numUav,
		bool sampler, UINT numCbvPara, UINT RegisterStNoCbv);

	ComPtr<ID3D12PipelineState> CreatePSO(ID3DBlob* vs, ID3DBlob* hs,
		ID3DBlob* ds, ID3DBlob* ps, ID3DBlob* gs,
		ID3D12RootSignature* mRootSignature,
		std::vector<D3D12_INPUT_ELEMENT_DESC>* pVertexLayout,
		bool STREAM_OUTPUT,
		std::vector<D3D12_SO_DECLARATION_ENTRY>* pDeclaration,
		UINT numDECLARATION,
		UINT* StreamSizeArr,
		UINT NumStrides,
		bool alpha, bool blend,
		PrimitiveType type);

	ComPtr<ID3D12PipelineState> CreatePsoVsPs(ID3DBlob* vs, ID3DBlob* ps,
		ID3D12RootSignature* mRootSignature,
		std::vector<D3D12_INPUT_ELEMENT_DESC>& pVertexLayout,
		bool alpha, bool blend,
		PrimitiveType type);

	ComPtr<ID3D12PipelineState> CreatePsoVsHsDsPs(ID3DBlob* vs, ID3DBlob* hs, ID3DBlob* ds, ID3DBlob* ps, ID3DBlob* gs,
		ID3D12RootSignature* mRootSignature,
		std::vector<D3D12_INPUT_ELEMENT_DESC>& pVertexLayout,
		bool alpha, bool blend,
		PrimitiveType type);

	ComPtr<ID3D12PipelineState> CreatePsoStreamOutput(ID3DBlob* vs, ID3DBlob* hs, ID3DBlob* ds, ID3DBlob* gs,
		ID3D12RootSignature* mRootSignature,
		std::vector<D3D12_INPUT_ELEMENT_DESC>& pVertexLayout,
		std::vector<D3D12_SO_DECLARATION_ENTRY>* pDeclaration,
		UINT numDECLARATION,
		UINT* StreamSizeArr,
		UINT NumStrides,
		PrimitiveType type);

	ComPtr<ID3D12PipelineState> CreatePsoParticle(ID3DBlob* vs, ID3DBlob* ps, ID3DBlob* gs,
		ID3D12RootSignature* mRootSignature,
		std::vector<D3D12_INPUT_ELEMENT_DESC>& pVertexLayout,
		bool alpha, bool blend);

	ComPtr<ID3D12PipelineState> CreatePsoCompute(ID3DBlob* cs,
		ID3D12RootSignature* mRootSignature);

	ID3D12Resource* GetSwapChainBuffer();
	ID3D12Resource* GetDepthStencilBuffer();
	D3D12_RESOURCE_STATES GetTextureStates();
	ComPtr<ID3DBlob> CompileShader(LPSTR szFileName, size_t size, LPSTR szFuncName, LPSTR szProfileName);

public:
	void SetCommandList(int no);
	void CopyResource(ID3D12Resource* texture, D3D12_RESOURCE_STATES res, int index = 0);
	void TextureInit(int width, int height, int index = 0);
	HRESULT SetTextureMPixel(BYTE* frame, int index = 0);
	InternalTexture* getInternalTexture(int index) { return &dx->texture[index]; }
	ID3D12Resource* getTextureResource(int index) { return texture[index].Get(); }
};

//*********************************PolygonDataクラス*************************************//
class PolygonData :public Common {

protected:
	friend SkinMesh;
	friend MeshData;
	friend Wave;
	friend DXR_Basic;
	friend SkinnedCom;
	//ポインタで受け取る
	ID3DBlob* vs = nullptr;
	ID3DBlob* ps = nullptr;
	ID3DBlob* ps_NoMap = nullptr;
	ID3DBlob* hs = nullptr;
	ID3DBlob* ds = nullptr;
	ID3DBlob* gs = nullptr;
	ID3DBlob* gs_NoMap = nullptr;
	ID3DBlob* cs = nullptr;

	//コンスタントバッファOBJ
	ConstantBuffer<CONSTANT_BUFFER>* mObjectCB = nullptr;
	ConstantBuffer<CONSTANT_BUFFER2>* mObjectCB1 = nullptr;
	ConstantBuffer<cbInstanceID>* mObjectCB_Ins = nullptr;
	CONSTANT_BUFFER cb[2];
	bool firstCbSet[2];
	CONSTANT_BUFFER2 sg;
	//DrawOn
	bool DrawOn = false;

	//テクスチャ番号(通常テクスチャ用)
	int ins_no = 0;
	int insNum[2] = {};

	void* ver = nullptr;  //頂点配列
	UINT** index = nullptr;//頂点インデックス
	int numIndex;    //頂点インデックス数

	PrimitiveType primType_create;
	DivideArr divArr[16] = {};
	int numDiv;
	drawPara dpara = {};
	ParameterDXR dxrPara = {};

	void GetShaderByteCode(bool light, int tNo);
	void CbSwap();
	void getBuffer(int numMaterial, int numMaxInstance, DivideArr* divArr = nullptr, int numDiv = 0);
	void getVertexBuffer(UINT VertexByteStride, UINT numVertex);
	void getIndexBuffer(int materialIndex, UINT IndexBufferByteSize, UINT numIndex);

	template<typename T>
	void createDefaultBuffer(T* vertexArr, UINT** indexArr, bool verDelete_f) {
		dpara.Vview->VertexBufferGPU = dx->CreateDefaultBuffer(com_no, vertexArr,
			dpara.Vview->VertexBufferByteSize,
			dpara.Vview->VertexBufferUploader, false);
		if (verDelete_f)ARR_DELETE(vertexArr);//使わない場合解放

		for (int i = 0; i < dpara.NumMaterial; i++) {
			if (dpara.Iview[i].IndexCount <= 0)continue;
			dpara.Iview[i].IndexBufferGPU = dx->CreateDefaultBuffer(com_no, indexArr[i],
				dpara.Iview[i].IndexBufferByteSize,
				dpara.Iview[i].IndexBufferUploader, false);
			ARR_DELETE(indexArr[i]);
		}
		ARR_DELETE(indexArr);
	}

	void createBufferDXR(int numMaterial, int numMaxInstance);

	void createParameterDXR(bool alpha);

	void setColorDXR(int materialIndex, CONSTANT_BUFFER2& sg);

	bool createPSO_DXR(std::vector<D3D12_INPUT_ELEMENT_DESC>& vertexLayout,
		const int numSrv, const int numCbv, const int numUav);

	void setTextureDXR();

	bool createPSO(std::vector<D3D12_INPUT_ELEMENT_DESC>& vertexLayout,
		const int numSrv, const int numCbv, const int numUav, bool blend, bool alpha);

	bool setDescHeap(const int numSrvTex,
		const int numSrvBuf, ID3D12Resource** buffer, UINT* StructureByteStride,
		const int numCbv, D3D12_GPU_VIRTUAL_ADDRESS ad3, UINT ad3Size);

	void draw(int com_no, drawPara& para);
	void streamOutput(int com_no, drawPara& para, ParameterDXR& dxr);

	void ParameterDXR_Update();

public:
	PolygonData();
	~PolygonData();
	ID3D12PipelineState* GetPipelineState();
	void GetVBarray(PrimitiveType type, int numMaxInstance);
	void SetCol(float difR, float difG, float difB, float speR, float speG, float speB,
		float amR = 0.0f, float amG = 0.0f, float amB = 0.0f);
	bool Create(bool light, int tNo, bool blend, bool alpha);
	bool Create(bool light, int tNo, int nortNo, int spetNo, bool blend, bool alpha);

	template<typename T>
	void setVertex(T* vertexArr, int numVer, UINT* ind, int numInd) {
		if (typeid(Vertex) == typeid(T)) {
			ver = new VertexM[numVer];
			VertexM* verM = (VertexM*)ver;
			Vertex* v = (Vertex*)vertexArr;
			for (int i = 0; i < numVer; i++) {
				verM[i].Pos.as(v[i].Pos.x, v[i].Pos.y, v[i].Pos.z);
				verM[i].normal.as(v[i].normal.x, v[i].normal.y, v[i].normal.z);
				verM[i].geoNormal.as(v[i].normal.x, v[i].normal.y, v[i].normal.z);
				verM[i].tex.as(v[i].tex.x, v[i].tex.y);
			}
			getVertexBuffer(sizeof(VertexM), numVer);
		}
		if (typeid(VertexBC) == typeid(T)) {
			ver = new VertexBC[numVer];
			VertexBC* v = (VertexBC*)vertexArr;
			memcpy(ver, v, sizeof(VertexBC) * numVer);
			getVertexBuffer(sizeof(VertexBC), numVer);
		}
		if (typeid(VertexBC) != typeid(T) && typeid(Vertex) != typeid(T)) {
			ErrorMessage("PolygonData::setVertex Error!!");
			return;
		}
		index = new UINT * [dpara.NumMaterial];
		index[0] = new UINT[numInd];
		memcpy(index[0], ind, sizeof(UINT) * numInd);
		numIndex = numInd;
		const UINT ibByteSize = numIndex * sizeof(UINT);
		getIndexBuffer(0, ibByteSize, numIndex);
	}

	void Instancing(VECTOR3 pos, VECTOR3 angle, VECTOR3 size);

	void InstancingUpdate(VECTOR4 Color, float disp, float shininess = 4.0f,
		float px = 1.0f, float py = 1.0f, float mx = 1.0f, float my = 1.0f);

	void Update(VECTOR3 pos, VECTOR4 Color, VECTOR3 angle, VECTOR3 size, float disp, float shininess = 4.0f,
		float px = 1.0f, float py = 1.0f, float mx = 1.0f, float my = 1.0f);

	void DrawOff();
	void Draw(int com_no);
	void StreamOutput(int com_no);
	void Draw();
	void StreamOutput();
	ParameterDXR* getParameter();
	void setDivideArr(DivideArr* arr, int numdiv) {
		numDiv = numdiv;
		memcpy(divArr, arr, sizeof(DivideArr) * numDiv);
	}
	void UpdateDxrDivideBuffer();
};

//*********************************PolygonData2Dクラス*************************************//
class PolygonData2D :public Common {

protected:
	friend DxText;

	ID3DBlob* vs = nullptr;
	ID3DBlob* ps = nullptr;

	int      ver;//頂点数

	ComPtr<ID3D12RootSignature> mRootSignature = nullptr;
	ComPtr<ID3D12DescriptorHeap> mDescHeap = nullptr;

	//コンスタントバッファOBJ
	ConstantBuffer<CONSTANT_BUFFER2D>* mObjectCB = nullptr;
	CONSTANT_BUFFER2D cb2[2];
	bool firstCbSet[2];
	//DrawOn
	bool DrawOn = false;

	//頂点バッファOBJ
	std::unique_ptr<VertexView> Vview = nullptr;
	std::unique_ptr<IndexView> Iview = nullptr;

	ComPtr<ID3D12PipelineState> mPSO = nullptr;

	//テクスチャ保持,DxText classでしか使わない
	bool tex_on = false;
	//SetTextParameter
	int Twidth;
	int Theight;
	int Tcount;
	TEXTMETRIC* Tm;
	GLYPHMETRICS* Gm;
	BYTE* Ptr;
	DWORD* Allsize;
	bool CreateTextOn = false;

	int  ins_no = 0;
	int  insNum[2] = {};//Drawで読み込み用

	static float magnificationX;//倍率
	static float magnificationY;
	float magX = 1.0f;
	float magY = 1.0f;

	void GetShaderByteCode();
	void SetConstBf(CONSTANT_BUFFER2D* cb2, float x, float y, float z, float r, float g, float b, float a, float sizeX, float sizeY);
	void SetTextParameter(int width, int height, int textCount, TEXTMETRIC** TM, GLYPHMETRICS** GM, BYTE** ptr, DWORD** allsize);
	void SetText(int com_no);//DxText classでしか使わない
	void CbSwap();

public:
	MY_VERTEX2* d2varray;  //頂点配列
	std::uint16_t* d2varrayI;//頂点インデックス

	static void Pos2DCompute(VECTOR3* p);//3D座標→2D座標変換(magnificationX, magnificationYは無視される)
	static void SetMagnification(float x, float y);//表示倍率

	PolygonData2D();
	~PolygonData2D();
	void DisabledMagnification();
	ID3D12PipelineState* GetPipelineState();
	void GetVBarray2D(int pcs);
	void TexOn();
	bool CreateBox(float x, float y, float z, float sizex, float sizey, float r, float g, float b, float a, bool blend, bool alpha);
	bool Create(bool blend, bool alpha);
	void InstancedSetConstBf(float x, float y, float r, float g, float b, float a, float sizeX, float sizeY);
	void InstancedSetConstBf(float x, float y, float z, float r, float g, float b, float a, float sizeX, float sizeY);
	void InstanceUpdate();
	void Update(float x, float y, float r, float g, float b, float a, float sizeX, float sizeY);
	void Update(float x, float y, float z, float r, float g, float b, float a, float sizeX, float sizeY);
	void DrawOff();
	void Draw(int com_no);
	void Draw();
};

//************************************addCharクラス****************************************//
class addChar {

public:
	char* str = nullptr;
	size_t size;

	void addStr(char* str1, char* str2);

	~addChar() {
		S_DELETE(str);
	}
};

class T_float {

protected:
	static DWORD f[2], time[2];
	static DWORD time_fps[2];//FPS計測用
	static int frame[2];    //FPS計測使用
	static int up;
	static char str[50];//ウインドウ枠文字表示使用
	static float adj;

public:
	static void GetTime(HWND hWnd);//常に実行
	static void GetTimeUp(HWND hWnd);//常に実行
	static void AddAdjust(float ad);//1.0fが標準
	static int GetUps();
	float Add(float f);
};

#endif
