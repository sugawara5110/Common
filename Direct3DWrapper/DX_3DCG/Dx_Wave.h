//*****************************************************************************************//
//**                                                                                     **//
//**                   　　　         Waveクラス                                         **//
//**                                                                                     **//
//*****************************************************************************************//

#ifndef Class_Wave_Header
#define Class_Wave_Header

#include "Dx12ProcessCore.h"

class Wave {

protected:
	ID3DBlob* cs = nullptr;
	ID3DBlob* vs = nullptr;
	ID3DBlob* ps = nullptr;
	ID3DBlob* ps_NoMap = nullptr;
	ID3DBlob* hs = nullptr;
	ID3DBlob* ds = nullptr;
	ID3DBlob* gs = nullptr;
	ID3DBlob* gs_NoMap = nullptr;

	ComPtr<ID3D12RootSignature> mRootSignatureCom = nullptr;
	ConstantBuffer<CONSTANT_BUFFER_WAVE>* mObjectCB_WAVE = nullptr;
	CONSTANT_BUFFER_WAVE cbw;
	CONSTANT_BUFFER2 sg;

	int div;//分割数
	Vertex* ver = nullptr;  //頂点配列
	UINT** index = nullptr;//頂点インデックス
	int numVer;      //頂点個数
	int numIndex;    //頂点インデックス数

	ComPtr<ID3D12PipelineState> mPSOCom = nullptr;
	ComPtr<ID3D12Resource> mInputBuffer = nullptr;
	ComPtr<ID3D12Resource> mInputUploadBuffer = nullptr;
	ComPtr<ID3D12Resource> mOutputBuffer = nullptr;
	PolygonData mObj;

	void GetShaderByteCode();
	bool ComCreate();
	bool DrawCreate(int texNo, int nortNo, bool blend, bool alpha);
	void Compute();
	void DrawSub();

public:
	Wave();
	~Wave();
	void GetVBarray(int v);
	void SetCol(float difR, float difG, float difB, float speR, float speG, float speB,
		float amR = 0.0f, float amG = 0.0f, float amB = 0.0f);
	bool Create(int texNo, bool blend, bool alpha, float waveHeight, float divide);
	bool Create(int texNo, int nortNo, bool blend, bool alpha, float waveHeight, float divide);
	//順番:左上左下右下右上
	void SetVertex(int i,
		float vx, float vy, float vz,
		float nx, float ny, float nz,
		float u, float v);
	void InstancedMap(float x, float y, float z, float theta, float sizeX = 1.0f, float sizeY = 0.0f, float sizeZ = 0.0f);
	void InstanceUpdate(float r, float g, float b, float a, float disp, float shininess = 4.0f,
		float px = 1.0f, float py = 1.0f, float mx = 1.0f, float my = 1.0f);
	void Update(float x, float y, float z, float r, float g, float b, float a, float theta, float disp, float shininess = 4.0f,
		float size = 1.0f, float px = 1.0f, float py = 1.0f, float mx = 1.0f, float my = 1.0f);
	void DrawOff();
	void Draw();

	void SetCommandList(int no);
	void CopyResource(ID3D12Resource* texture, D3D12_RESOURCE_STATES res, int index = 0);
	void TextureInit(int width, int height, int index = 0);
	HRESULT SetTextureMPixel(BYTE* frame, int index = 0);
};

#endif
