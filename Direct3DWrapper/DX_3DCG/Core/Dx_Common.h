//*****************************************************************************************//
//**                                                                                     **//
//**                   　　　       DxCommon                                             **//
//**                                                                                     **//
//*****************************************************************************************//

#ifndef Class_DxCommon_Header
#define Class_DxCommon_Header

#include "Dx_SwapChain.h"

class DxCommon {

protected:
	DxCommon();//外部からのオブジェクト生成禁止
	DxCommon(const DxCommon& obj) {}     // コピーコンストラクタ禁止
	void operator=(const DxCommon& obj) {}// 代入演算子禁止

	char objName[256] = {};

	//テクスチャ
	std::unique_ptr<bool[]> createRes = nullptr;
	std::unique_ptr<ID3D12Resource* []> textureUp = nullptr;
	std::unique_ptr<ID3D12Resource* []> texture = nullptr;
	int numTexRes = 0;
	MovieTexture* movOn = nullptr;
	int movOnSize = 0;

	D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint = {};
	D3D12_TEXTURE_COPY_LOCATION dest = {};
	D3D12_TEXTURE_COPY_LOCATION src = {};

	HRESULT createTex(int comIndex, int tNo, int& resCnt, char* upName, char* defName, char* ObjName);
	HRESULT createTextureResource(int comIndex, int resourceStartIndex, int MaterialNum, TextureNo* to, char* ObjName);

	ComPtr<ID3D12RootSignature> CreateRootSignature(UINT numSrv, UINT numCbv, UINT numUav,
		UINT numCbvPara, UINT RegisterStNoCbv, UINT numArrCbv, UINT* numDescriptors);

	ComPtr<ID3D12RootSignature> CreateRs(int paramNum, D3D12_ROOT_PARAMETER* slotRootParameter);
	ComPtr<ID3D12RootSignature> CreateRsStreamOutput(int paramNum, D3D12_ROOT_PARAMETER* slotRootParameter);
	ComPtr<ID3D12RootSignature> CreateRsStreamOutputSampler(int paramNum, D3D12_ROOT_PARAMETER* slotRootParameter);
	ComPtr<ID3D12RootSignature> CreateRsCompute(int paramNum, D3D12_ROOT_PARAMETER* slotRootParameter);

	ComPtr<ID3D12RootSignature> CreateRootSignatureCompute(UINT numSrv, UINT numCbv, UINT numUav,
		UINT numCbvPara, UINT RegisterStNoCbv, UINT numArrCbv, UINT* numDescriptors);

	ComPtr<ID3D12RootSignature> CreateRootSignatureStreamOutput(UINT numSrv, UINT numCbv, UINT numUav,
		bool sampler, UINT numCbvPara, UINT RegisterStNoCbv, UINT numArrCbv, UINT* numDescriptors);

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

public:
	~DxCommon();
	void SetName(char* name);
	void CopyResource(int comIndex, ID3D12Resource* texture, D3D12_RESOURCE_STATES res, int texIndex = 0);
	void TextureInit(int width, int height, int texIndex = 0);
	HRESULT SetTextureMPixel(int comIndex, BYTE* frame, int index = 0);
	ID3D12Resource* getTextureResource(int index) { return texture[index]; }
};

#endif