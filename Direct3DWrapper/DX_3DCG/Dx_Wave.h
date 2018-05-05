//*****************************************************************************************//
//**                                                                                     **//
//**                   　　　         Waveクラス                                         **//
//**                                                                                     **//
//*****************************************************************************************//

#ifndef Class_Wave_Header
#define Class_Wave_Header

#include "Dx12ProcessCore.h"

class Wave :public Common {

protected:
	int                        com_no = 0;
	ID3DBlob                   *cs = nullptr;
	ID3DBlob                   *vs = nullptr;
	ID3DBlob                   *ps = nullptr;
	ID3DBlob                   *hs = nullptr;
	ID3DBlob                   *ds = nullptr;

	Microsoft::WRL::ComPtr<ID3D12RootSignature> mRootSignatureCom = nullptr;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> mRootSignatureDraw = nullptr;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mSrvHeap = nullptr;

	ConstantBuffer<CONSTANT_BUFFER> *mObjectCB = nullptr;
	ConstantBuffer<CONSTANT_BUFFER2> *mObjectCB1 = nullptr;
	ConstantBuffer<CONSTANT_BUFFER_WAVE> *mObjectCB_WAVE = nullptr;
	CONSTANT_BUFFER cb[2];
	CONSTANT_BUFFER2 sg;
	CONSTANT_BUFFER_WAVE cbw;
	int sw = 0;
	//UpLoadカウント
	int upCount = 0;
	//初回Up終了
	bool UpOn = FALSE;
	//DrawOn
	bool DrawOn = FALSE;

	std::unique_ptr<VertexView> Vview = nullptr;
	std::unique_ptr<IndexView> Iview = nullptr;

	//テクスチャ番号(通常テクスチャ用)
	int    t_no = -1;
	int    insNum = 0;
	int    texNum;//テクスチャー数

	int div;//分割数

	Vertex         *d3varray;  //頂点配列
	std::uint16_t  *d3varrayI;//頂点インデックス
	int            ver;      //頂点個数
	int            verI;    //頂点インデックス

	Microsoft::WRL::ComPtr<ID3D12PipelineState> mPSOCom = nullptr;//パイプラインOBJ
	Microsoft::WRL::ComPtr<ID3D12PipelineState> mPSODraw = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mInputBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mInputUploadBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mOutputBuffer = nullptr;

	static std::mutex mtx;
	static void Lock() { mtx.lock(); }
	static void Unlock() { mtx.unlock(); }

	void GetShaderByteCode(int texNum);
	void ComCreate();
	void DrawCreate(int texNo, int nortNo, bool blend, bool alpha);
	void CbSwap();
	void Compute();
	void DrawSub();

public:
	Wave();
	void SetCommandList(int no);
	~Wave();
	void GetVBarray(int v);
	void SetCol(float difR, float difG, float difB, float speR, float speG, float speB);
	void Create(int texNo, bool blend, bool alpha, float waveHeight, float divide);
	void Create(int texNo, int nortNo, bool blend, bool alpha, float waveHeight, float divide);
	//順番:左上左下右下右上
	void SetVertex(int i,
		float vx, float vy, float vz,
		float nx, float ny, float nz,
		float u, float v);
	void InstancedMap(float x, float y, float z, float theta);
	void InstancedMap(float x, float y, float z, float theta, float size);
	void InstancedMapSize3(float x, float y, float z, float theta, float sizeX, float sizeY, float sizeZ);
	void InstanceUpdate(float r, float g, float b, float a, float disp);
	void InstanceUpdate(float r, float g, float b, float a, float disp, float px, float py, float mx, float my);
	void Update(float x, float y, float z, float r, float g, float b, float a, float theta, float disp);
	void Update(float x, float y, float z, float r, float g, float b, float a, float theta, float disp, float size);
	void Update(float x, float y, float z, float r, float g, float b, float a, float theta, float disp, float size, float px, float py, float mx, float my);
	void DrawOff();
	void Draw();
};

#endif
