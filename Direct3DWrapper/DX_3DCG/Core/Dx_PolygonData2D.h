//*****************************************************************************************//
//**                                                                                     **//
//**                   　　　          PolygonData2D                                     **//
//**                                                                                     **//
//*****************************************************************************************//

#ifndef Class_PolygonData2D_Header
#define Class_PolygonData2D_Header

#include "Dx_Common.h"

struct VERTEX2 {
	CoordTf::VECTOR3 Pos;
	CoordTf::VECTOR4 color;
	CoordTf::VECTOR2 tex;
};

class PolygonData2D :public DxCommon {

protected:
	struct WVP_CB2D {
		CoordTf::MATRIX world;
		CoordTf::VECTOR4 AddObjColor;
		CoordTf::VECTOR4 pXpYmXmY;
	};

	UINT NumMaxInstance = 1;

	ID3DBlob* vs = nullptr;
	ID3DBlob* ps = nullptr;

	ComPtr<ID3D12RootSignature> mRootSignature = nullptr;
	ComPtr<ID3D12DescriptorHeap> mDescHeap = nullptr;

	//コンスタントバッファOBJ
	ConstantBuffer<WVP_CB2D>* mObjectCB = nullptr;
	std::unique_ptr<WVP_CB2D[]> cb2[2];
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

	void GetShaderByteCode();

public:
	static void Pos2DCompute(CoordTf::VECTOR3* p);//3D座標→2D座標変換

	PolygonData2D();
	~PolygonData2D();
	void GetVBarray2D(UINT numMaxInstance);

	void TexOn();
	bool Create(int comIndex, bool blend, bool alpha, int Tex, VERTEX2* v2, int num_v2, UINT* index, int num_index);

	void Instancing(CoordTf::MATRIX world, CoordTf::VECTOR4 Color,
		float px = 1.0f, float py = 1.0f, float mx = 0.0f, float my = 0.0f);

	void Instancing(CoordTf::VECTOR3 pos, float angle, CoordTf::VECTOR2 size, CoordTf::VECTOR4 Color,
		float px = 1.0f, float py = 1.0f, float mx = 0.0f, float my = 0.0f);

	void InstancingUpdate();
	void Update(CoordTf::VECTOR3 pos, float angle, CoordTf::VECTOR2 size, CoordTf::VECTOR4 Color,
		float px = 1.0f, float py = 1.0f, float mx = 0.0f, float my = 0.0f);

	void DrawOff();
	void Draw(int comIndex);
};

#endif
