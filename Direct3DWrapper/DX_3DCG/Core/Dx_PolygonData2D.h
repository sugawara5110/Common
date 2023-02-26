//*****************************************************************************************//
//**                                                                                     **//
//**                   　　　          PolygonData2D                                     **//
//**                                                                                     **//
//*****************************************************************************************//

#ifndef Class_PolygonData2D_Header
#define Class_PolygonData2D_Header

#include "Dx_Common.h"

#define INSTANCE_PCS_2D 256

struct CONSTANT_BUFFER2D {
	CoordTf::VECTOR4 Pos[INSTANCE_PCS_2D];
	CoordTf::VECTOR4 Color[INSTANCE_PCS_2D];
	CoordTf::VECTOR4 sizeXY[INSTANCE_PCS_2D];
	CoordTf::VECTOR4 WidHei;//ウインドウwh
};

class PolygonData2D :public DxCommon {

protected:
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

	//テクスチャ保持
	bool tex_on = false;

	int  ins_no = 0;
	int  insNum[2] = {};//Drawで読み込み用

	static float magnificationX;//倍率
	static float magnificationY;
	float magX = 1.0f;
	float magY = 1.0f;

	void GetShaderByteCode();
	void SetConstBf(CONSTANT_BUFFER2D* cb2, float x, float y, float z,
		float r, float g, float b, float a, float sizeX, float sizeY);
	void CbSwap();

public:
	MY_VERTEX2* d2varray;  //頂点配列
	std::uint16_t* d2varrayI;//頂点インデックス

	static void Pos2DCompute(CoordTf::VECTOR3* p);//3D座標→2D座標変換(magnificationX, magnificationYは無視される)
	static void SetMagnification(float x, float y);//表示倍率

	PolygonData2D();
	~PolygonData2D();
	void DisabledMagnification();
	ID3D12PipelineState* GetPipelineState();
	void GetVBarray2D(int pcs);
	void TexOn();
	bool CreateBox(int comIndex, float x, float y, float z,
		float sizex, float sizey,
		float r, float g, float b, float a,
		bool blend, bool alpha, int noTex = -1);
	bool Create(int comIndex, bool blend, bool alpha, int noTex = -1);
	void InstancedSetConstBf(float x, float y, float r, float g, float b, float a, float sizeX, float sizeY);
	void InstancedSetConstBf(float x, float y, float z, float r, float g, float b, float a, float sizeX, float sizeY);
	void InstanceUpdate();
	void Update(float x, float y, float r, float g, float b, float a, float sizeX, float sizeY);
	void Update(float x, float y, float z, float r, float g, float b, float a, float sizeX, float sizeY);
	void DrawOff();
	void Draw(int comIndex);
};

#endif
