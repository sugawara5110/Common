//*****************************************************************************************//
//**                                                                                     **//
//**                   　　　       ParticleDataクラス                                   **//
//**                                                                                     **//
//*****************************************************************************************//

#ifndef Class_ParticleData_Header
#define Class_ParticleData_Header

#include "Dx12ProcessCore.h"

class ParticleData :public Common {

protected:
	int                        com_no = 0;
	ID3DBlob                   *gsSO;
	ID3DBlob                   *vsSO;
	ID3DBlob                   *gs;
	ID3DBlob                   *vs;
	ID3DBlob                   *ps;
	int                        ver;//頂点数

	PartPos                    *P_pos;//パーティクルデータ配列

	Microsoft::WRL::ComPtr<ID3D12RootSignature> mRootSignature_com = nullptr;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> mRootSignature_draw = nullptr;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mSrvHeap = nullptr;

	//コンスタントバッファOBJ
	UploadBuffer<CONSTANT_BUFFER_P> *mObjectCB = nullptr;
	CONSTANT_BUFFER_P cbP[2];
	int sw = 0;
	//UpLoadカウント
	int upCount = 0;
	//初回Up終了
	bool UpOn = FALSE;
	//DrawOn
	bool DrawOn = FALSE;
	bool texpar_on = FALSE;

	//頂点バッファOBJ
	std::unique_ptr<VertexView> Vview = nullptr;
	//ストリームバッファOBJ
	std::unique_ptr<StreamView[]> Sview1 = nullptr;
	std::unique_ptr<StreamView[]> Sview2 = nullptr;//BufferFilledSizeLocationの送り先
	int svInd = 0;
	bool firstDraw = FALSE;
	int  streamInitcount = 0;
	bool parInit = FALSE;

	Microsoft::WRL::ComPtr<ID3D12PipelineState> mPSO_com = nullptr;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> mPSO_draw = nullptr;

	static std::mutex mtx;
	static void Lock() { mtx.lock(); }
	static void Unlock() { mtx.unlock(); }

	void GetShaderByteCode();
	void MatrixMap(CONSTANT_BUFFER_P *cb_p, float x, float y, float z, float theta, float size, float speed, bool tex);
	void MatrixMap2(CONSTANT_BUFFER_P *cb_p, bool init);
	void GetVbColarray(int texture_no, float size, float density);
	void CreateVbObj();
	void CreatePartsCom();
	void CreatePartsDraw(int texpar);
	void DrawParts0();
	void DrawParts1();
	void DrawParts2();
	void CbSwap(bool init);

public:
	ParticleData();
	~ParticleData();
	void SetCommandList(int no);
	void GetBufferParticle(int texture_no, float size, float density);//テクスチャを元にパーティクルデータ生成, 全体サイズ倍率, 密度
	void GetBufferBill(int v);
	void SetVertex(int i,
		float vx, float vy, float vz,
		float r, float g, float b, float a);
	void CreateParticle(int texpar);//パーティクル1個のテクスチャナンバー
	void CreateBillboard();//ver個の四角形を生成
	void Update(float x, float y, float z, float theta, float size, bool init, float speed);//sizeパーティクル1個のサイズ
	void DrawOff();
	void Draw();
	void Update(float size);
	void DrawBillboard();
};

#endif