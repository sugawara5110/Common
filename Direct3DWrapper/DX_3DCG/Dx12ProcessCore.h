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

	ComPtr<ID3D12CommandAllocator> mCmdListAlloc[2];
	ComPtr<ID3D12GraphicsCommandList4> mCommandList;
	int mAloc_Num = 0;
	volatile ComListState mComState;

	bool ListCreate();
	void ResourceBarrier(ID3D12Resource* res, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after);
	void Bigin();
	void End();
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

	ComPtr<IDXGIFactory4> mdxgiFactory;
	ComPtr<ID3D12Device5> md3dDevice;
	ComPtr<IDXGISwapChain3> mSwapChain;
	ComPtr<ID3D12Fence> mFence;
	UINT64 mCurrentFence = 0;
	bool DXR_CreateResource = false;

	//MultiSampleレベルチェック
	bool m4xMsaaState = false;
	UINT m4xMsaaQuality = 0;

	ComPtr<ID3D12CommandQueue> mCommandQueue;
	Dx12Process_sub dx_sub[COM_NO];

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

	ComPtr<ID3DBlob> pHullShader_Wave = nullptr;
	ComPtr<ID3DBlob> pHullShaderTriangle = nullptr;

	ComPtr<ID3DBlob> pDomainShader_Wave = nullptr;
	ComPtr<ID3DBlob> pDomainShaderTriangle = nullptr;

	std::vector<D3D12_INPUT_ELEMENT_DESC> pVertexLayout_SKIN;
	std::vector<D3D12_SO_DECLARATION_ENTRY> pDeclaration_PSO;
	std::vector<D3D12_SO_DECLARATION_ENTRY> pDeclaration_Output;
	std::vector<D3D12_INPUT_ELEMENT_DESC> pVertexLayout_P;
	std::vector<D3D12_INPUT_ELEMENT_DESC> pVertexLayout_MESH;
	std::vector<D3D12_INPUT_ELEMENT_DESC> pVertexLayout_3D;
	std::vector<D3D12_INPUT_ELEMENT_DESC> pVertexLayout_3DBC;
	std::vector<D3D12_INPUT_ELEMENT_DESC> pVertexLayout_2D;

	ComPtr<ID3DBlob> pVertexShader_Wave = nullptr;
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

	std::unique_ptr<char[]> ShaderNormalTangentCopy = nullptr;
	std::unique_ptr<char[]> ShaderCalculateLightingCopy = nullptr;

	bool CreateShaderByteCodeBool = true;

	D3D12_VIEWPORT mScreenViewport;
	D3D12_RECT mScissorRect;

	UINT mRtvDescriptorSize = 0;
	UINT mDsvDescriptorSize = 0;
	UINT mCbvSrvUavDescriptorSize = 0;

	DXGI_FORMAT mBackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	DXGI_FORMAT mDepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	int mClientWidth;
	int mClientHeight;

	//テクスチャ
	int texNum = 0;    //配列数
	InternalTexture* texture = nullptr;

	static Dx12Process* dx;//クラス内でオブジェクト生成し使いまわす
	static std::mutex mtx;

	MATRIX mProj;
	MATRIX mView;
	MATRIX Vp;    //ビューポート行列(3D座標→2D座標変換時使用)
	float posX, posY, posZ;
	float upX, upY, upZ;

	//カメラ画角
	float ViewY_theta;
	//アスペクト比
	float aspect;
	//nearプレーン
	float NearPlane;
	//farプレーン
	float FarPlane;

	PointLight plight;
	int lightNum = 0;
	DirectionLight dlight;
	Fog fog;
	VECTOR4 GlobalAmbientLight = { 0.1f,0.1f,0.1f,0.0f };

	int  cBuffSwap[2] = { 0,0 };
	bool fenceMode = false;

	Dx12Process() {}//外部からのオブジェクト生成禁止
	Dx12Process(const Dx12Process& obj) {}   // コピーコンストラクタ禁止
	void operator=(const Dx12Process& obj) {}// 代入演算子禁止
	~Dx12Process();

	bool CreateShaderByteCode();
	UINT64 getRequiredIntermediateSize(ID3D12Resource* res);
	HRESULT CopyResourcesToGPU(int com_no, ID3D12Resource* up, ID3D12Resource* def,
		const void* initData, LONG_PTR RowPitch);

	ComPtr<ID3D12Resource> CreateDefaultBuffer(int com_no,
		const void* initData, UINT64 byteSize, ComPtr<ID3D12Resource>& uploadBuffer);

	ComPtr<ID3D12Resource> CreateStreamBuffer(UINT64 byteSize);

	ComPtr<ID3DBlob> CompileShader(LPSTR szFileName, size_t size, LPSTR szFuncName, LPSTR szProfileName);

	void InstancedMap(int& insNum, CONSTANT_BUFFER* cb, float x, float y, float z,
		float thetaZ, float thetaY, float thetaX, float sizeX, float sizeY = 0.0f, float sizeZ = 0.0f);

	void MatrixMap(CONSTANT_BUFFER* cb, float r, float g, float b, float a,
		float disp, float px, float py, float mx, float my, DivideArr* divArr, int numDiv, float shininess);

	void FenceSetEvent();
	void WaitFence(bool mode);

	HRESULT createDefaultResourceTEXTURE2D(ID3D12Resource** def, UINT64 width, UINT height,
		DXGI_FORMAT format, D3D12_RESOURCE_STATES firstState = D3D12_RESOURCE_STATE_COMMON);

	HRESULT createDefaultResourceTEXTURE2D_UNORDERED_ACCESS(ID3D12Resource** def, UINT64 width, UINT height,
		D3D12_RESOURCE_STATES firstState = D3D12_RESOURCE_STATE_COMMON);

	HRESULT createDefaultResourceBuffer(ID3D12Resource** def, UINT64 bufferSize,
		D3D12_RESOURCE_STATES firstState = D3D12_RESOURCE_STATE_COMMON);

	HRESULT createDefaultResourceBuffer_UNORDERED_ACCESS(ID3D12Resource** def, UINT64 bufferSize,
		D3D12_RESOURCE_STATES firstState = D3D12_RESOURCE_STATE_COMMON);

	HRESULT createUploadResource(ID3D12Resource** up, UINT64 uploadBufferSize);

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

	void dxrCreateResource() { DXR_CreateResource = true; }
	bool Initialize(HWND hWnd, int width = 800, int height = 600);
	ID3D12Device5* getDevice() { return md3dDevice.Get(); }
	char* GetNameFromPass(char* pass);//パスからファイル名を抽出
	int GetTexNumber(CHAR* fileName);//リソースとして登録済みのテクスチャ配列番号をファイル名から取得

	void createTextureArr(int numTexArr, int resourceIndex, char* texName,
		UCHAR* byteArr, DXGI_FORMAT format,
		int width, LONG_PTR RowPitch, int height);

	void Sclear(int com_no);
	void Bigin(int com_no);
	void End(int com_no);
	void setUpSwapIndex(int index) { cBuffSwap[0] = index; }
	void setDrawSwapIndex(int index) { cBuffSwap[1] = index; }
	void WaitFenceCurrent();//GPU処理そのまま待つ
	void WaitFencePast();//前回GPU処理未完の場合待つ
	void DrawScreen();
	void Cameraset(float posX, float posY, float posZ,
		float dirX, float dirY, float dirZ,
		float upX = 0.0f, float upY = 0.0f, float upZ = 1.0f);

	void ResetPointLight();
	void setGlobalAmbientLight(float r, float g, float b) {
		GlobalAmbientLight.as(r, g, b, 0.0f);
	}

	bool PointLightPosSet(int Idx, float x, float y, float z,
		float r, float g, float b, float a, bool on_off,
		float range, float atten1 = 0.01f, float atten2 = 0.001f, float atten3 = 0.001f);

	void DirectionLight(float x, float y, float z, float r, float g, float b);
	void SetDirectionLight(bool onoff);
	void Fog(float r, float g, float b, float amount, float density, bool onoff);
	float GetViewY_theta();
	float Getaspect();
	float GetNearPlane();
	float GetFarPlane();
	InternalTexture* getInternalTexture(int index) { return &texture[index]; }
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

	//バッファのサイズ等
	UINT StreamByteStride = 0;
	UINT StreamBufferByteSize = 0;
	D3D12_GPU_VIRTUAL_ADDRESS BufferFilledSizeLocation;

	//ストリームバッファビュー
	D3D12_STREAM_OUTPUT_BUFFER_VIEW StreamBufferView()const
	{
		D3D12_STREAM_OUTPUT_BUFFER_VIEW sbv;
		sbv.BufferLocation = StreamBufferGPU->GetGPUVirtualAddress();
		sbv.SizeInBytes = StreamBufferByteSize;
		sbv.BufferFilledSizeLocation = BufferFilledSizeLocation;//以前のサイズ?
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

struct ParameterDXR {
	int NumMaterial = 0;
	UINT NumInstance = 1;
	std::unique_ptr<ID3D12Resource* []>difTex = nullptr;
	std::unique_ptr<ID3D12Resource* []>norTex = nullptr;
	std::unique_ptr<ID3D12Resource* []>speTex = nullptr;
	std::unique_ptr<VECTOR4[]> diffuse = nullptr;
	std::unique_ptr<VECTOR4[]> specular = nullptr;
	std::unique_ptr<VECTOR4[]> ambient = nullptr;
	std::unique_ptr<VertexView[]> VviewDXR = nullptr;
	std::unique_ptr<IndexView[]> IviewDXR = nullptr;
	std::unique_ptr<StreamView[]> SviewDXR = nullptr;
	std::unique_ptr<StreamView[]> SizeLocation = nullptr;
	MATRIX Transform[INSTANCE_PCS_3D] = {};
	float shininess;
	bool alphaTest;
};

//**********************************Commonクラス*************************************//
class Common {

protected:
	friend PolygonData;
	friend PolygonData2D;
	friend ParticleData;
	friend PostEffect;
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

	ComPtr<ID3D12RootSignature> CreateRootSignature(UINT numSrv, UINT numCbv);
	ComPtr<ID3D12RootSignature> CreateRs(int paramNum, D3D12_ROOT_PARAMETER* slotRootParameter);
	ComPtr<ID3D12RootSignature> CreateRsStreamOutput(int paramNum, D3D12_ROOT_PARAMETER* slotRootParameter);
	ComPtr<ID3D12RootSignature> CreateRsCompute(int paramNum, D3D12_ROOT_PARAMETER* slotRootParameter);
	ComPtr<ID3D12RootSignature> CreateRootSignatureStreamOutput(UINT numSrv, UINT numCbv);

	ComPtr<ID3D12PipelineState> CreatePSO(ID3DBlob* vs, ID3DBlob* hs,
		ID3DBlob* ds, ID3DBlob* ps, ID3DBlob* gs,
		ID3D12RootSignature* mRootSignature,
		std::vector<D3D12_INPUT_ELEMENT_DESC>* pVertexLayout,
		std::vector<D3D12_SO_DECLARATION_ENTRY>* pDeclaration,
		bool STREAM_OUTPUT, UINT StreamSize, bool alpha, bool blend,
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

	ComPtr<ID3D12PipelineState> CreatePsoStreamOutput(ID3DBlob* vs, ID3DBlob* gs,
		ID3D12RootSignature* mRootSignature,
		std::vector<D3D12_INPUT_ELEMENT_DESC>& pVertexLayout,
		std::vector<D3D12_SO_DECLARATION_ENTRY>& pDeclaration, UINT StreamSize, PrimitiveType type);

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
	//ポインタで受け取る
	ID3DBlob* vs = nullptr;
	ID3DBlob* ps = nullptr;
	ID3DBlob* ps_NoMap = nullptr;
	ID3DBlob* hs = nullptr;
	ID3DBlob* ds = nullptr;
	ID3DBlob* gs = nullptr;
	ID3DBlob* gs_NoMap = nullptr;

	//コンスタントバッファOBJ
	ConstantBuffer<CONSTANT_BUFFER>* mObjectCB = nullptr;
	ConstantBuffer<CONSTANT_BUFFER2>* mObjectCB1 = nullptr;
	CONSTANT_BUFFER cb[2];
	CONSTANT_BUFFER2 sg;
	//UpLoadカウント
	int upCount = 0;
	//初回Up終了
	bool UpOn = false;
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
	int numDiv = 3;
	drawPara dpara = {};
	ParameterDXR dxrPara = {};

	void GetShaderByteCode(bool light, int tNo);
	void CbSwap();
	void getBuffer(int numMaterial);
	void getVertexBuffer(UINT VertexByteStride, UINT numVertex);
	void getIndexBuffer(int materialIndex, UINT IndexBufferByteSize, UINT numIndex);

	template<typename T>
	void createDefaultBuffer(T* vertexArr, UINT** indexArr, bool verDelete_f) {
		dpara.Vview->VertexBufferGPU = dx->CreateDefaultBuffer(com_no, vertexArr,
			dpara.Vview->VertexBufferByteSize,
			dpara.Vview->VertexBufferUploader);
		if (verDelete_f)ARR_DELETE(vertexArr);//使わない場合解放

		for (int i = 0; i < dpara.NumMaterial; i++) {
			if (dpara.Iview[i].IndexCount <= 0)continue;
			dpara.Iview[i].IndexBufferGPU = dx->CreateDefaultBuffer(com_no, indexArr[i],
				dpara.Iview[i].IndexBufferByteSize,
				dpara.Iview[i].IndexBufferUploader);
			ARR_DELETE(indexArr[i]);
		}
		ARR_DELETE(indexArr);
	}

	void createBufferDXR(int numMaterial);

	void createParameterDXR(bool alpha);

	void setColorDXR(int materialIndex, CONSTANT_BUFFER2& sg);

	bool createPSO_DXR(std::vector<D3D12_INPUT_ELEMENT_DESC>& vertexLayout,
		const int numSrv, const int numCbv);

	void setTextureDXR();

	bool createPSO(std::vector<D3D12_INPUT_ELEMENT_DESC>& vertexLayout,
		const int numSrv, const int numCbv, bool blend, bool alpha);

	bool setDescHeap(const int numSrvTex,
		const int numSrvBuf, ID3D12Resource** buffer, UINT* StructureByteStride,
		const int numCbv, D3D12_GPU_VIRTUAL_ADDRESS ad3, UINT ad3Size);

	void instanceUpdate(float r, float g, float b, float a, DivideArr* divArr, int numDiv,
		float disp = 1.0f, float shininess = 4.0f);

	void update(float x, float y, float z, float r, float g, float b, float a,
		float thetaZ, float thetaY, float thetaX, float size,
		DivideArr* divArr, int numDiv,
		float disp = 1.0f, float shininess = 4.0f);

	void draw(int com_no, drawPara& para, ParameterDXR& dxr);
	void streamOutput(int com_no, drawPara& para, ParameterDXR& dxr);

	void ParameterDXR_Update();

public:
	PolygonData();
	~PolygonData();
	ID3D12PipelineState* GetPipelineState();
	void GetVBarray(PrimitiveType type);
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
	}

	void InstancedMap(float x, float y, float z, float thetaZ, float thetaY, float thetaX,
		float sizeX = 1.0f, float sizeY = 0.0f, float sizeZ = 0.0f);
	void InstanceUpdate(float r, float g, float b, float a, float disp, float shininess = 4.0f, float px = 1.0f, float py = 1.0f, float mx = 1.0f, float my = 1.0f);
	void Update(float x, float y, float z, float r, float g, float b, float a, float theta, float disp, float shininess = 4.0f,
		float size = 1.0f, float px = 1.0f, float py = 1.0f, float mx = 1.0f, float my = 1.0f);
	void DrawOff();
	void Draw(int com_no);
	void StreamOutput(int com_no);
	void Draw();
	void StreamOutput();
	ParameterDXR* getParameter();
};

//*********************************PolygonData2Dクラス*************************************//
class PolygonData2D :public Common {

protected:
	friend DxText;

	ID3DBlob* vs = nullptr;
	ID3DBlob* ps = nullptr;

	int      ver;//頂点数

	ComPtr<ID3D12RootSignature> mRootSignature = nullptr;
	ComPtr<ID3D12DescriptorHeap> mSrvHeap = nullptr;

	//コンスタントバッファOBJ
	ConstantBuffer<CONSTANT_BUFFER2D>* mObjectCB = nullptr;
	CONSTANT_BUFFER2D cb2[2];
	//UpLoadカウント
	int upCount = 0;
	//初回Up終了
	bool UpOn = false;
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
	void SetText();//DxText classでしか使わない
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

//エラーメッセージ
void ErrorMessage(char *E_mes);

#endif
