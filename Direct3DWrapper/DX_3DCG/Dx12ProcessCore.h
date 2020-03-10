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
#include <windows.h>
#include <wrl.h>//Microsoft::WRL
#include "DxStruct.h"
#include "DxEnum.h"
#include <dxgi1_4.h>
#include <d3d12.h>
#include "../MicroSoftLibrary/d3dx12.h"
#include <d3d10_1.h>
#include <D3Dcompiler.h>
#include <DirectXColors.h>
//#include <DirectXCollision.h>
#include "../MicroSoftLibrary/WICTextureLoader12.h"
//#include <memory>
#include <stdlib.h>
#include <string>
#include <vector>
#include <array>
//#include <unordered_map>
#include <assert.h>
#include <Process.h>
#include <mutex>
#include <new>

#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "dxgi.lib")

#define COM_NO 7

//前方宣言
template<class T>
class ConstantBuffer;
class Dx12Process;
class MeshData;
class PolygonData;
class PolygonData2D;
class ParticleData;
class SkinMesh;
class SkinMeshA;
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
//前方宣言

class Dx12Process_sub final {

private:
	friend Dx12Process;
	friend MeshData;
	friend PolygonData;
	friend PolygonData2D;
	friend ParticleData;
	friend SkinMesh;
	friend SkinMeshA;
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

	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> mCmdListAlloc[2];
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> mCommandList;
	int mAloc_Num = 0;
	volatile ComListState mComState;

	bool ListCreate();
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
	friend SkinMeshA;
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

	Microsoft::WRL::ComPtr<IDXGIFactory4> mdxgiFactory;
	Microsoft::WRL::ComPtr<ID3D12Device> md3dDevice;
	Microsoft::WRL::ComPtr<IDXGISwapChain> mSwapChain;
	Microsoft::WRL::ComPtr<ID3D12Fence> mFence;
	UINT64 mCurrentFence = 0;

	//MultiSampleレベルチェック
	bool m4xMsaaState = false;
	UINT m4xMsaaQuality = 0;

	Microsoft::WRL::ComPtr<ID3D12CommandQueue> mCommandQueue;
	Dx12Process_sub dx_sub[COM_NO];

	static const int SwapChainBufferCount = 2;
	int mCurrBackBuffer = 0;
	Microsoft::WRL::ComPtr<ID3D12Resource> mSwapChainBuffer[SwapChainBufferCount];
	Microsoft::WRL::ComPtr<ID3D12Resource> mDepthStencilBuffer;

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mRtvHeap;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mDsvHeap;

	//シェーダーバイトコード
	Microsoft::WRL::ComPtr<ID3DBlob> pGeometryShader_PSO = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> pGeometryShader_P = nullptr;

	Microsoft::WRL::ComPtr<ID3DBlob> pHullShader_Wave = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> pHullShaderTriangle = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> pHullShader_DISP = nullptr;

	Microsoft::WRL::ComPtr<ID3DBlob> pDomainShader_Wave = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> pDomainShaderTriangle = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> pDomainShader_DISP = nullptr;

	std::vector<D3D12_INPUT_ELEMENT_DESC> pVertexLayout_SKIN;
	std::vector<D3D12_SO_DECLARATION_ENTRY> pDeclaration_PSO;
	std::vector<D3D12_INPUT_ELEMENT_DESC> pVertexLayout_P;
	std::vector<D3D12_INPUT_ELEMENT_DESC> pVertexLayout_MESH;
	std::vector<D3D12_INPUT_ELEMENT_DESC> pVertexLayout_3D;
	std::vector<D3D12_INPUT_ELEMENT_DESC> pVertexLayout_3DBC;
	std::vector<D3D12_INPUT_ELEMENT_DESC> pVertexLayout_2D;

	Microsoft::WRL::ComPtr<ID3DBlob> pVertexShader_Wave = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> pVertexShader_SKIN = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> pVertexShader_SKIN_D = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> pVertexShader_PSO = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> pVertexShader_P = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> pVertexShader_MESH_D = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> pVertexShader_MESH = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> pVertexShader_DISP = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> pVertexShader_TC = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> pVertexShader_BC = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> pVertexShader_2D = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> pVertexShader_2DTC = nullptr;

	Microsoft::WRL::ComPtr<ID3DBlob> pPixelShader_P = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> pPixelShader_Bump = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> pPixelShader_3D = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> pPixelShader_Emissive = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> pPixelShader_BC = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> pPixelShader_2D = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> pPixelShader_2DTC = nullptr;

	Microsoft::WRL::ComPtr<ID3DBlob> pComputeShader_Wave = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> pComputeShader_Post[2] = { nullptr };

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
	Texture* tex = nullptr;//外部からアドレスが渡される
	int texNum = 0;    //配列数
	ID3D12Resource** texture = nullptr;
	ID3D12Resource** textureUp = nullptr;

	static Dx12Process* dx;//クラス内でオブジェクト生成し使いまわす
	static std::mutex mtx;

	MATRIX mProj;
	MATRIX mView;
	MATRIX Vp;    //ビューポート行列(3D座標→2D座標変換時使用)
	float posX, posY, posZ;

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

	int  cBuffSwap[2] = { 0,0 };
	bool fenceMode = false;

	Dx12Process() {}//外部からのオブジェクト生成禁止
	Dx12Process(const Dx12Process& obj) {}   // コピーコンストラクタ禁止
	void operator=(const Dx12Process& obj) {}// 代入演算子禁止
	~Dx12Process();

	bool CreateShaderByteCode();
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateDefaultBuffer(ID3D12GraphicsCommandList* cmdList,
		const void* initData, UINT64 byteSize, Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer);

	Microsoft::WRL::ComPtr<ID3D12Resource> CreateStreamBuffer(UINT64 byteSize);

	Microsoft::WRL::ComPtr<ID3DBlob> CompileShader(LPSTR szFileName, size_t size, LPSTR szFuncName, LPSTR szProfileName);

	void InstancedMap(int& insNum, CONSTANT_BUFFER* cb, float x, float y, float z,
		float thetaZ, float thetaY, float thetaX, float sizeX, float sizeY = 0.0f, float sizeZ = 0.0f);

	void MatrixMap(CONSTANT_BUFFER* cb, float r, float g, float b, float a, float disp, float px, float py, float mx, float my);

	void FenceSetEvent();
	void WaitFence(bool mode);

	HRESULT textureInit(int width, int height,
		ID3D12Resource** up, ID3D12Resource** def, DXGI_FORMAT format,
		D3D12_RESOURCE_STATES firstState,
		D3D12_PLACED_SUBRESOURCE_FOOTPRINT& footprint,
		D3D12_TEXTURE_COPY_LOCATION& dest, D3D12_TEXTURE_COPY_LOCATION& src);

	HRESULT createTexture(int com_no, UCHAR* byteArr, DXGI_FORMAT format,
		ID3D12Resource** up, ID3D12Resource** def,
		int width, LONG_PTR RowPitch, int height);

	HRESULT createTextureArr(int com_no, int resourceIndex,
		UCHAR* byteArr, DXGI_FORMAT format,
		int width, LONG_PTR RowPitch, int height);

public:
	static void InstanceCreate();
	static Dx12Process* GetInstance();
	static void DeleteInstance();

	static void Lock() { mtx.lock(); }
	static void Unlock() { mtx.unlock(); }

	bool Initialize(HWND hWnd, int width = 800, int height = 600);
	char* GetNameFromPass(char* pass);//パスからファイル名を抽出
	void SetTextureBinary(Texture* byte, int size);//外部で生成したデコード済みバイナリ配列のポインタと配列数をセットする,解放は外部で
	int GetTexNumber(CHAR* fileName);//リソースとして登録済みのテクスチャ配列番号をファイル名から取得
	bool GetTexture(int com_no);//デコード済みのバイナリからリソースの生成
	bool GetTexture2(int com_no);//テスト中
	void UpTextureRelease();
	void Sclear(int com_no);
	void Bigin(int com_no);
	void End(int com_no);
	void setUpSwapIndex(int index) { cBuffSwap[0] = index; }
	void setDrawSwapIndex(int index) { cBuffSwap[1] = index; }
	void WaitFenceCurrent();//GPU処理そのまま待つ
	void WaitFencePast();//前回GPU処理未完の場合待つ
	void DrawScreen();
	void Cameraset(float cx1, float cx2, float cy1, float cy2, float cz1, float cz2);
	void ResetPointLight();
	void P_ShadowBright(float val);
	bool PointLightPosSet(int Idx, float x, float y, float z, float r, float g, float b, float a, float range,
		float brightness, float attenuation, bool on_off);
	void DirectionLight(float x, float y, float z, float r, float g, float b, float bright, float ShadowBright);
	void SetDirectionLight(bool onoff);
	void Fog(float r, float g, float b, float amount, float density, bool onoff);
	float GetViewY_theta();
	float Getaspect();
	float GetNearPlane();
	float GetFarPlane();
};

struct VertexView {

	//各パラメーターを自分でコピーする
	Microsoft::WRL::ComPtr<ID3D12Resource> VertexBufferGPU = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> VertexBufferUploader = nullptr;

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
	Microsoft::WRL::ComPtr<ID3D12Resource> IndexBufferGPU = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> IndexBufferUploader = nullptr;

	//バッファのサイズ等
	DXGI_FORMAT IndexFormat = DXGI_FORMAT_R16_UINT;
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

	Microsoft::WRL::ComPtr<ID3D12Resource> StreamBufferGPU = nullptr;

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
	Microsoft::WRL::ComPtr<ID3D12Resource> mUploadBuffer;
	BYTE *mMappedData = nullptr;
	UINT mElementByteSize = 0;
	Dx12Process *dx = nullptr;

public:
	ConstantBuffer(UINT elementCount) {

		dx = Dx12Process::GetInstance();
		//コンスタントバッファサイズは256バイト単位にしておく(アライメント)
		mElementByteSize = (sizeof(T) + 255) & ~255;//255を足して255の補数の論理積を取る

		if (FAILED(dx->md3dDevice->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(mElementByteSize * elementCount),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&mUploadBuffer)))) {
			char *str = "ConstantBufferエラー";
			throw str;
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

	ID3D12Resource *Resource()const {
		return mUploadBuffer.Get();
	}

	void CopyData(int elementIndex, const T& data) {
		memcpy(&mMappedData[elementIndex*mElementByteSize], &data, sizeof(T));
	}
};

//**********************************Commonクラス*************************************//

class Common {

protected:
	friend MeshData;
	friend PolygonData;
	friend PolygonData2D;
	friend ParticleData;
	friend SkinMesh;
	friend SkinMeshA;
	friend Wave;
	friend PostEffect;
	Common() {}//外部からのオブジェクト生成禁止
	Common(const Common& obj) {}     // コピーコンストラクタ禁止
	void operator=(const Common& obj) {}// 代入演算子禁止

	Dx12Process* dx;
	ID3D12GraphicsCommandList* mCommandList;
	int com_no = 0;

	//テクスチャ保持(SetTextureMPixel用)
	ID3D12Resource* texture = nullptr;
	ID3D12Resource* textureUp = nullptr;
	//movie_on
	bool m_on = false;
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint;
	D3D12_TEXTURE_COPY_LOCATION dest, src;

	Microsoft::WRL::ComPtr <ID3D12DescriptorHeap> CreateSrvHeap(int MaterialNum, int texNum,
		TextureNo* to, ID3D12Resource* movietex = nullptr);

	Microsoft::WRL::ComPtr <ID3D12RootSignature> CreateRsCommon(CD3DX12_ROOT_SIGNATURE_DESC* rootSigDesc);//直接使用禁止
	Microsoft::WRL::ComPtr <ID3D12RootSignature> CreateRs(int paramNum, CD3DX12_ROOT_PARAMETER* slotRootParameter);
	Microsoft::WRL::ComPtr <ID3D12RootSignature> CreateRsStreamOutput(int paramNum, CD3DX12_ROOT_PARAMETER* slotRootParameter);
	Microsoft::WRL::ComPtr <ID3D12RootSignature> CreateRsCompute(int paramNum, CD3DX12_ROOT_PARAMETER* slotRootParameter);

	Microsoft::WRL::ComPtr <ID3D12PipelineState> CreatePSO(ID3DBlob* vs, ID3DBlob* hs,
		ID3DBlob* ds, ID3DBlob* ps, ID3DBlob* gs,
		ID3D12RootSignature* mRootSignature,
		std::vector<D3D12_INPUT_ELEMENT_DESC>* pVertexLayout,
		std::vector<D3D12_SO_DECLARATION_ENTRY>* pDeclaration,
		bool STREAM_OUTPUT, UINT StreamSize, bool alpha, bool blend,
		PrimitiveType type);

	Microsoft::WRL::ComPtr <ID3D12PipelineState> CreatePsoVsPs(ID3DBlob* vs, ID3DBlob* ps,
		ID3D12RootSignature* mRootSignature,
		std::vector<D3D12_INPUT_ELEMENT_DESC>& pVertexLayout,
		bool alpha, bool blend,
		PrimitiveType type = NUL);

	Microsoft::WRL::ComPtr <ID3D12PipelineState> CreatePsoVsHsDsPs(ID3DBlob* vs, ID3DBlob* hs, ID3DBlob* ds, ID3DBlob* ps,
		ID3D12RootSignature* mRootSignature,
		std::vector<D3D12_INPUT_ELEMENT_DESC>& pVertexLayout,
		bool alpha, bool blend,
		PrimitiveType type = NUL);

	Microsoft::WRL::ComPtr <ID3D12PipelineState> CreatePsoStreamOutput(ID3DBlob* vs, ID3DBlob* gs,
		ID3D12RootSignature* mRootSignature,
		std::vector<D3D12_INPUT_ELEMENT_DESC>& pVertexLayout,
		std::vector<D3D12_SO_DECLARATION_ENTRY>& pDeclaration, UINT StreamSize);

	Microsoft::WRL::ComPtr <ID3D12PipelineState> CreatePsoParticle(ID3DBlob* vs, ID3DBlob* ps, ID3DBlob* gs,
		ID3D12RootSignature* mRootSignature,
		std::vector<D3D12_INPUT_ELEMENT_DESC>& pVertexLayout,
		bool alpha, bool blend);

	Microsoft::WRL::ComPtr <ID3D12PipelineState> CreatePsoCompute(ID3DBlob* cs,
		ID3D12RootSignature* mRootSignature);

	ID3D12Resource* GetSwapChainBuffer();
	ID3D12Resource* GetDepthStencilBuffer();
	ID3D12Resource* GetTexture(int Num);
	D3D12_RESOURCE_STATES GetTextureStates();
	ID3D12Resource* GetTextureUp(int Num);
	Microsoft::WRL::ComPtr<ID3DBlob> CompileShader(LPSTR szFileName, size_t size, LPSTR szFuncName, LPSTR szProfileName);

	struct drawPara {
		UINT NumMaterial = 0;
		ID3D12DescriptorHeap* srv = nullptr;
		ID3D12RootSignature* rootSignature = nullptr;
		VertexView* Vview = nullptr;
		IndexView* Iview = nullptr;
		MY_MATERIAL_S* material = nullptr;
		D3D_PRIMITIVE_TOPOLOGY haveNortexTOPOLOGY;
		D3D_PRIMITIVE_TOPOLOGY notHaveNortexTOPOLOGY;
		ID3D12PipelineState* haveNortexPSO = nullptr;
		ID3D12PipelineState* notHaveNortexPSO = nullptr;
		ID3D12Resource* cbRes0 = nullptr;
		ID3D12Resource* cbRes1 = nullptr;
		ID3D12Resource* cbRes2 = nullptr;
		ID3D12Resource* sRes0 = nullptr;
		ID3D12Resource* sRes1 = nullptr;
		UINT insNum = 1;
	};
	void drawsub(drawPara para);

public:
	void SetCommandList(int no);
	void CopyResource(ID3D12Resource* texture, D3D12_RESOURCE_STATES res);
	HRESULT TextureInit(int width, int height);
	HRESULT SetTextureMPixel(UINT** m_pix, BYTE r, BYTE g, BYTE b, BYTE a, BYTE Threshold = 50);
};

//*********************************PolygonDataクラス*************************************//

class PolygonData :public Common {

protected:
	//ポインタで受け取る
	ID3DBlob* vs = nullptr;
	ID3DBlob* ps = nullptr;
	ID3DBlob* hs = nullptr;
	ID3DBlob* ds = nullptr;

	Microsoft::WRL::ComPtr<ID3D12RootSignature> mRootSignature = nullptr;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mSrvHeap = nullptr;

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

	//頂点バッファOBJ
	std::unique_ptr<VertexView> Vview = nullptr;
	std::unique_ptr<IndexView[]> Iview = nullptr;

	//テクスチャ番号(通常テクスチャ用)
	MY_MATERIAL_S material[1];
	int ins_no = 0;
	int insNum[2] = {};
	int texNum;//テクスチャ個数

	//パイプラインOBJ
	Microsoft::WRL::ComPtr<ID3D12PipelineState> mPSO = nullptr;

	Vertex* d3varray;  //頂点配列
	VertexBC* d3varrayBC;//頂点配列基本色
	std::uint16_t* d3varrayI;//頂点インデックス
	int ver;      //頂点個数
	int verI;    //頂点インデックス

	PrimitiveType          primType_create;
	D3D_PRIMITIVE_TOPOLOGY primType_draw;

	void GetShaderByteCode(bool light, int tNo, int nortNo);
	void CbSwap();

public:
	PolygonData();
	~PolygonData();
	ID3D12PipelineState* GetPipelineState();
	void GetVBarray(PrimitiveType type, int v);
	void SetCol(float difR, float difG, float difB, float speR, float speG, float speB,
		float amR = 0.0f, float amG = 0.0f, float amB = 0.0f);
	bool Create(bool light, int tNo, bool blend, bool alpha);
	bool Create(bool light, int tNo, int nortNo, bool blend, bool alpha);
	void SetVertex(int I1, int I2, int i,
		float vx, float vy, float vz,
		float nx, float ny, float nz,
		float u, float v);
	void SetVertex(int I1, int i,
		float vx, float vy, float vz,
		float nx, float ny, float nz,
		float u, float v);
	void SetVertexBC(int I1, int I2, int i,
		float vx, float vy, float vz,
		float r, float g, float b, float a);
	void SetVertexBC(int I1, int i,
		float vx, float vy, float vz,
		float r, float g, float b, float a);
	void InstancedMap(float x, float y, float z, float theta, float sizeX = 1.0f, float sizeY = 0.0f, float sizeZ = 0.0f);
	void InstanceUpdate(float r, float g, float b, float a, float disp, float px = 1.0f, float py = 1.0f, float mx = 1.0f, float my = 1.0f);
	void Update(float x, float y, float z, float r, float g, float b, float a, float theta, float disp,
		float size = 1.0f, float px = 1.0f, float py = 1.0f, float mx = 1.0f, float my = 1.0f);
	void DrawOff();
	void Draw();
};

//*********************************PolygonData2Dクラス*************************************//

class PolygonData2D :public Common {

protected:
	friend DxText;

	ID3DBlob* vs = nullptr;
	ID3DBlob* ps = nullptr;

	int      ver;//頂点数

	Microsoft::WRL::ComPtr<ID3D12RootSignature> mRootSignature = nullptr;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mSrvHeap = nullptr;

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

	Microsoft::WRL::ComPtr<ID3D12PipelineState> mPSO = nullptr;

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
