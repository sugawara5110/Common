//*****************************************************************************************//
//**                                                                                     **//
//**                   �@�@�@       Dx12ProcessCore�N���X                                **//
//**                                                                                     **//
//*****************************************************************************************//

#ifndef Class_Dx12ProcessCore_Header
#define Class_Dx12ProcessCore_Header

#if defined(DEBUG) || defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#define _CRT_SECURE_NO_WARNINGS
#include "Dx_ShaderHolder.h"
#include <DirectXColors.h>
#include <stdlib.h>
#include <string>
#include <array>
#include <assert.h>
#include <Process.h>
#include <mutex>
#include <new>
#include <typeinfo>

#pragma comment(lib,"d3dcompiler.lib")

using Microsoft::WRL::ComPtr;

class Dx12Process final {

private:
	template<class T>
	friend class ConstantBuffer;
	friend MeshData;
	friend PolygonData;
	friend PolygonData2D;
	friend ParticleData;
	friend SkinMesh;
	friend Dx_CommandListObj;
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
	friend TextObj;

	ComPtr<IDXGIFactory4> mdxgiFactory;
	ComPtr<IDXGISwapChain3> mSwapChain;
	std::unique_ptr<Dx_Device> device = nullptr;
	bool DXR_CreateResource = false;

	//MultiSample���x���`�F�b�N
	bool m4xMsaaState = false;
	UINT m4xMsaaQuality = 0;

	DxCommandQueue graphicsQueue;
	DxCommandQueue computeQueue;
	Dx_CommandListObj dx_sub[COM_NO];
	Dx_CommandListObj dx_subCom[COM_NO];

	static const int SwapChainBufferCount = 2;
	int mCurrBackBuffer = 0;
	ComPtr<ID3D12Resource> mSwapChainBuffer[SwapChainBufferCount];
	ComPtr<ID3D12Resource> mDepthStencilBuffer;

	ComPtr<ID3D12DescriptorHeap> mRtvHeap;
	ComPtr<ID3D12DescriptorHeap> mDsvHeap;

	std::unique_ptr<Dx_ShaderHolder> shaderH = nullptr;

	D3D12_VIEWPORT mScreenViewport;
	D3D12_RECT mScissorRect;

	UINT mRtvDescriptorSize = 0;
	UINT mDsvDescriptorSize = 0;
	UINT mCbvSrvUavDescriptorSize = 0;

	DXGI_FORMAT mBackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	DXGI_FORMAT mDepthStencilFormat = DXGI_FORMAT_D32_FLOAT;
	int mClientWidth;
	int mClientHeight;

	//�e�N�X�`��
	int texNum = 0;    //�z��
	InternalTexture* texture = nullptr;

	static Dx12Process* dx;//�N���X���ŃI�u�W�F�N�g�������g���܂킷
	static std::mutex mtx;
	static std::mutex mtxGraphicsQueue;
	static std::mutex mtxComputeQueue;

	struct Update {
		CoordTf::MATRIX mProj = {};
		CoordTf::MATRIX mView = {};

		CoordTf::VECTOR3 pos = {};
		CoordTf::VECTOR3 dir = {};
		CoordTf::VECTOR3 up = {};

		PointLight plight = {};
		int lightNum = 0;
		DirectionLight dlight = {};
	};
	Update upd[2] = {};//cBuffSwap

	CoordTf::MATRIX Vp;    //�r���[�|�[�g�s��(3D���W��2D���W�ϊ����g�p)
	Fog fog;

	//�J������p
	float ViewY_theta;
	//�A�X�y�N�g��
	float aspect;
	//near�v���[��
	float NearPlane;
	//far�v���[��
	float FarPlane;
	CoordTf::VECTOR4 GlobalAmbientLight = { 0.1f,0.1f,0.1f,0.0f };

	int cBuffSwap[2] = { 0,0 };
	int dxrBuffSwap[2] = { 0,0 };

	bool wireframe = false;

	Dx12Process() {}//�O������̃I�u�W�F�N�g�����֎~
	Dx12Process(const Dx12Process& obj) {}   // �R�s�[�R���X�g���N�^�֎~
	void operator=(const Dx12Process& obj) {}// ������Z�q�֎~
	~Dx12Process();

	HRESULT CopyResourcesToGPU(int com_no, ID3D12Resource* up, ID3D12Resource* def,
		const void* initData, LONG_PTR RowPitch);

	ComPtr<ID3D12Resource> CreateDefaultBuffer(int com_no,
		const void* initData, UINT64 byteSize, ComPtr<ID3D12Resource>& uploadBuffer, bool uav);

	void Instancing(int& insNum, CONSTANT_BUFFER* cb, CoordTf::VECTOR3 pos, CoordTf::VECTOR3 angle, CoordTf::VECTOR3 size);

	void InstancingUpdate(CONSTANT_BUFFER* cb, CoordTf::VECTOR4 Color, float disp,
		float px, float py, float mx, float my, DivideArr* divArr, int numDiv, float shininess);

	HRESULT createTexture(int com_no, UCHAR* byteArr, DXGI_FORMAT format,
		ID3D12Resource** up, ID3D12Resource** def,
		int width, LONG_PTR RowPitch, int height);

public:
	static void InstanceCreate();
	static Dx12Process* GetInstance();
	static void DeleteInstance();

	static void Lock() { mtx.lock(); }
	static void Unlock() { mtx.unlock(); }

	void setNumResourceBarrier(int num) { Dx_CommandListObj::NumResourceBarrier = num; }

	void dxrCreateResource() { DXR_CreateResource = true; }
	bool Initialize(HWND hWnd, int width = 800, int height = 600);
	ID3D12Device5* getDevice() { return device->getDevice(); }
	char* GetNameFromPass(char* pass);//�p�X����t�@�C�����𒊏o
	int GetTexNumber(CHAR* fileName);//���\�[�X�Ƃ��ēo�^�ς݂̃e�N�X�`���z��ԍ����t�@�C��������擾

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
	void RunGpuNotLock();
	void RunGpu();
	void WaitFenceNotLock();
	void WaitFence();
	void DrawScreen();
	void BiginCom(int com_no);
	void EndCom(int com_no);
	void RunGpuNotLockCom();
	void RunGpuCom();
	void WaitFenceNotLockCom();
	void WaitFenceCom();
	void Cameraset(CoordTf::VECTOR3 pos, CoordTf::VECTOR3 dir, CoordTf::VECTOR3 up = { 0.0f,0.0f,1.0f });

	void ResetPointLight();
	void setGlobalAmbientLight(float r, float g, float b) {
		GlobalAmbientLight.as(r, g, b, 0.0f);
	}

	bool PointLightPosSet(int Idx, CoordTf::VECTOR3 pos, CoordTf::VECTOR4 Color, bool on_off,
		float range, CoordTf::VECTOR3 atten = { 0.01f, 0.001f, 0.001f });

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

	//�e�p�����[�^�[�������ŃR�s�[����
	ComPtr<ID3D12Resource> VertexBufferGPU = nullptr;
	ComPtr<ID3D12Resource> VertexBufferUploader = nullptr;

	//�o�b�t�@�̃T�C�Y��
	UINT VertexByteStride = 0;
	UINT VertexBufferByteSize = 0;

	//���_�o�b�t�@�r���[
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

	//�e�p�����[�^�[�������ŃR�s�[����
	ComPtr<ID3D12Resource> IndexBufferGPU = nullptr;
	ComPtr<ID3D12Resource> IndexBufferUploader = nullptr;

	//�o�b�t�@�̃T�C�Y��
	DXGI_FORMAT IndexFormat = DXGI_FORMAT_R32_UINT;
	UINT IndexBufferByteSize = 0;
	//DrawIndexedInstanced�̈����Ŏg�p
	UINT IndexCount = 0;

	//�C���f�b�N�X�o�b�t�@�r���[
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

	//�o�b�t�@�̃T�C�Y��
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
		BufferFilledSizeBufferGPU = dx->device->CreateStreamBuffer(sizeof(UINT64));
		dx->device->createReadBackResource(ReadBuffer.GetAddressOf(), sizeof(UINT64));
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

	//�X�g���[���o�b�t�@�r���[
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
		//�R���X�^���g�o�b�t�@�T�C�Y��256�o�C�g�ŃA���C�����g
		mElementByteSize = ALIGNMENT_TO(256, sizeof(T));

		if (FAILED(dx->device->createUploadResource(&mUploadBuffer, (UINT64)mElementByteSize * elementCount))) {
			ErrorMessage("ConstantBuffer�G���[");
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
	CoordTf::MATRIX Transform[INSTANCE_PCS_3D] = {};
	CoordTf::MATRIX WVP[INSTANCE_PCS_3D] = {};
	CoordTf::VECTOR4 AddObjColor = {};//�I�u�W�F�N�g�̐F�ω��p
	float shininess;
	std::unique_ptr<UINT[]> InstanceID = nullptr;
	bool firstSet = false;//VviewDXR�̍ŏ��̃f�[�^�X�V�����t���O
	bool createAS = false;//AS�̍ŏ��̍\�z�����t���O
	UINT InstanceMask = 0xFF;
	float RefractiveIndex = 0.0f;//���ܗ�

	//ParticleData, SkinMesh�p
	bool useVertex = false;
	UINT numVertex = 1;
	std::unique_ptr<CoordTf::VECTOR3[]> v = nullptr;

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
	std::unique_ptr<CoordTf::VECTOR4[]> diffuse = nullptr;
	std::unique_ptr<CoordTf::VECTOR4[]> specular = nullptr;
	std::unique_ptr<CoordTf::VECTOR4[]> ambient = nullptr;
	std::unique_ptr<IndexView[]> IviewDXR = nullptr;
	std::unique_ptr<std::unique_ptr<StreamView[]>[]> SviewDXR = nullptr;
	UpdateDXR updateDXR[2] = {};
	bool updateF = false;//AS�\�z���update�̗L��

	void create(int numMaterial, int numMaxInstance) {
		NumMaxInstance = numMaxInstance;
		IviewDXR = std::make_unique<IndexView[]>(numMaterial);
		difTex = std::make_unique<ID3D12Resource* []>(numMaterial);
		norTex = std::make_unique<ID3D12Resource* []>(numMaterial);
		speTex = std::make_unique<ID3D12Resource* []>(numMaterial);
		diffuse = std::make_unique<CoordTf::VECTOR4[]>(numMaterial);
		specular = std::make_unique<CoordTf::VECTOR4[]>(numMaterial);
		ambient = std::make_unique<CoordTf::VECTOR4[]>(numMaterial);
		SviewDXR = std::make_unique<std::unique_ptr<StreamView[]>[]>(numMaterial);
		for (int i = 0; i < numMaterial; i++) {
			SviewDXR[i] = std::make_unique<StreamView[]>(numMaxInstance);
		}
		updateDXR[0].create(numMaterial, numMaxInstance);
		updateDXR[1].create(numMaterial, numMaxInstance);
	}

	void resetCreateAS() {
		updateDXR[0].createAS = false;
		updateDXR[1].createAS = false;
	}

	bool alphaBlend = false;
	bool alphaTest = false;
};

//**********************************Common�N���X*************************************//
class Common {

protected:
	friend PolygonData;
	friend PolygonData2D;
	friend ParticleData;
	friend PostEffect;
	friend SkinnedCom;
	Common() {};//�O������̃I�u�W�F�N�g�����֎~
	Common(const Common& obj) {}     // �R�s�[�R���X�g���N�^�֎~
	void operator=(const Common& obj) {}// ������Z�q�֎~

	Dx12Process* dx;
	ID3D12GraphicsCommandList* mCommandList;
	int com_no = 0;

	//�e�N�X�`��
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

//*********************************PolygonData�N���X*************************************//
class PolygonData :public Common {

protected:
	friend SkinMesh;
	friend MeshData;
	friend Wave;
	friend DXR_Basic;
	friend SkinnedCom;
	//�|�C���^�Ŏ󂯎��
	ID3DBlob* vs = nullptr;
	ID3DBlob* ps = nullptr;
	ID3DBlob* ps_NoMap = nullptr;
	ID3DBlob* hs = nullptr;
	ID3DBlob* ds = nullptr;
	ID3DBlob* gs = nullptr;
	ID3DBlob* gs_NoMap = nullptr;
	ID3DBlob* cs = nullptr;

	//�R���X�^���g�o�b�t�@OBJ
	ConstantBuffer<CONSTANT_BUFFER>* mObjectCB = nullptr;
	ConstantBuffer<CONSTANT_BUFFER2>* mObjectCB1 = nullptr;
	ConstantBuffer<cbInstanceID>* mObjectCB_Ins = nullptr;
	CONSTANT_BUFFER cb[2];
	bool firstCbSet[2];
	CONSTANT_BUFFER2 sg;
	//DrawOn
	bool DrawOn = false;

	//�e�N�X�`���ԍ�(�ʏ�e�N�X�`���p)
	int ins_no = 0;
	int insNum[2] = {};

	void* ver = nullptr;  //���_�z��
	UINT** index = nullptr;//���_�C���f�b�N�X
	int numIndex;    //���_�C���f�b�N�X��

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
		if (verDelete_f)ARR_DELETE(vertexArr);//�g��Ȃ��ꍇ���

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

	void createParameterDXR(bool alpha, bool blend, float divideBufferMagnification);

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
	bool Create(bool light, int tNo, int nortNo, int spetNo, bool blend, bool alpha,
		float divideBufferMagnification = 1.0f);

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

	void Instancing(CoordTf::VECTOR3 pos, CoordTf::VECTOR3 angle, CoordTf::VECTOR3 size);

	void InstancingUpdate(CoordTf::VECTOR4 Color, float disp, float shininess = 4.0f,
		float px = 1.0f, float py = 1.0f, float mx = 1.0f, float my = 1.0f);

	void Update(CoordTf::VECTOR3 pos, CoordTf::VECTOR4 Color, CoordTf::VECTOR3 angle, CoordTf::VECTOR3 size, float disp, float shininess = 4.0f,
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
	void setRefractiveIndex(float index) {
		dxrPara.updateDXR[dx->dxrBuffSwap[0]].RefractiveIndex = index;
	}
};

//*********************************PolygonData2D�N���X*************************************//
class PolygonData2D :public Common {

protected:
	friend DxText;

	ID3DBlob* vs = nullptr;
	ID3DBlob* ps = nullptr;

	int      ver;//���_��

	ComPtr<ID3D12RootSignature> mRootSignature = nullptr;
	ComPtr<ID3D12DescriptorHeap> mDescHeap = nullptr;

	//�R���X�^���g�o�b�t�@OBJ
	ConstantBuffer<CONSTANT_BUFFER2D>* mObjectCB = nullptr;
	CONSTANT_BUFFER2D cb2[2];
	bool firstCbSet[2];
	//DrawOn
	bool DrawOn = false;

	//���_�o�b�t�@OBJ
	std::unique_ptr<VertexView> Vview = nullptr;
	std::unique_ptr<IndexView> Iview = nullptr;
	ComPtr<ID3D12PipelineState> mPSO = nullptr;

	//�e�N�X�`���ێ�
	bool tex_on = false;

	int  ins_no = 0;
	int  insNum[2] = {};//Draw�œǂݍ��ݗp

	static float magnificationX;//�{��
	static float magnificationY;
	float magX = 1.0f;
	float magY = 1.0f;

	void GetShaderByteCode();
	void SetConstBf(CONSTANT_BUFFER2D* cb2, float x, float y, float z,
		float r, float g, float b, float a, float sizeX, float sizeY);
	void CbSwap();

public:
	MY_VERTEX2* d2varray;  //���_�z��
	std::uint16_t* d2varrayI;//���_�C���f�b�N�X

	static void Pos2DCompute(CoordTf::VECTOR3* p);//3D���W��2D���W�ϊ�(magnificationX, magnificationY�͖��������)
	static void SetMagnification(float x, float y);//�\���{��

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

class T_float {

protected:
	static DWORD f[2], time[2];
	static DWORD time_fps[2];//FPS�v���p
	static int frame[2];    //FPS�v���g�p
	static int up;
	static char str[50];//�E�C���h�E�g�����\���g�p
	static float adj;

public:
	static void GetTime(HWND hWnd);//��Ɏ��s
	static void GetTimeUp(HWND hWnd);//��Ɏ��s
	static void AddAdjust(float ad);//1.0f���W��
	static int GetUps();
	float Add(float f);
};

#endif