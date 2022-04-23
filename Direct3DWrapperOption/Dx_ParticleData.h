//*****************************************************************************************//
//**                                                                                     **//
//**                   　　　       ParticleDataクラス                                   **//
//**                                                                                     **//
//*****************************************************************************************//

#ifndef Class_ParticleData_Header
#define Class_ParticleData_Header

#include "../Direct3DWrapper/DX_3DCG/Core/Dx12ProcessCore.h"

class ParticleData :public Common {

protected:
	//コンスタントバッファパーティクル用
	struct CONSTANT_BUFFER_P {
		CoordTf::MATRIX  WV;
		CoordTf::MATRIX  invRot;
		CoordTf::MATRIX  Proj;
		CoordTf::VECTOR4 size;//xパーティクル大きさ, yパーティクル初期化フラグ, zスピード
		CoordTf::VECTOR4 AddObjColor;//オブジェクトの色変化用
	};

	//パーティクル頂点
	struct PartPos {
		CoordTf::VECTOR3 CurrentPos; //描画に使う
		CoordTf::VECTOR3 PosSt;     //開始位置
		CoordTf::VECTOR3 PosEnd;   //終了位置
	};

	ID3DBlob* gsSO;
	ID3DBlob* vsSO;
	ID3DBlob* gs;
	ID3DBlob* vs;
	ID3DBlob* ps;
	int      ver;//頂点数

	PartPos* P_pos;//パーティクルデータ配列

	ComPtr<ID3D12RootSignature> mRootSignature_com = nullptr;
	ComPtr<ID3D12RootSignature> mRootSignature_draw = nullptr;
	ComPtr<ID3D12DescriptorHeap> mDescHeap = nullptr;

	//コンスタントバッファOBJ
	ConstantBuffer<CONSTANT_BUFFER_P>* mObjectCB = nullptr;
	CONSTANT_BUFFER_P cbP[2];
	bool firstCbSet[2];
	//DrawOn
	bool DrawOn = false;
	bool texpar_on = false;

	//頂点バッファOBJ
	std::unique_ptr<VertexView> Vview = nullptr;
	//ストリームバッファOBJ
	std::unique_ptr<StreamView[]> Sview = nullptr;
	int svInd = 0;
	bool firstDraw = false;
	int  streamInitcount = 0;
	bool parInit = false;
	ComPtr<ID3D12RootSignature> rootSignatureDXR = nullptr;
	ComPtr<ID3D12PipelineState> PSO_DXR = nullptr;
	ParameterDXR dxrPara = {};

	ComPtr<ID3D12PipelineState> mPSO_com = nullptr;
	ComPtr<ID3D12PipelineState> mPSO_draw = nullptr;

	void createShader();
	void GetShaderByteCode();
	void update(CONSTANT_BUFFER_P* cb_p, CoordTf::VECTOR3 pos,
		CoordTf::VECTOR4 color, float angle, float size, float speed);
	void update2(CONSTANT_BUFFER_P* cb_p, bool init);
	void GetVbColarray(int texture_no, float size, float density);
	void CreateVbObj(bool alpha, bool blend);
	bool CreatePartsCom();
	bool CreatePartsDraw(int texNo, bool alpha, bool blend);
	void DrawParts1(int com);
	void DrawParts2(int com);
	void DrawParts2StreamOutput(int com);
	void CbSwap(bool init);
	void createDxr(bool alpha, bool blend);
	CoordTf::MATRIX BillboardAngleCalculation(float angle);

public:
	ParticleData();
	~ParticleData();
	void GetBufferParticle(int texture_no, float size, float density);//テクスチャを元にパーティクルデータ生成, 全体サイズ倍率, 密度
	void GetBufferBill(int v);
	void SetVertex(int i, CoordTf::VECTOR3 pos);
	void setMaterialType(MaterialType type);

	void setPointLight(UINT VertexIndex, bool on_off,
		float range, CoordTf::VECTOR3 atten = { 0.01f, 0.001f, 0.001f });

	void setPointLightAll(bool on_off,
		float range, CoordTf::VECTOR3 atten = { 0.01f, 0.001f, 0.001f });

	bool CreateParticle(int texNo, bool alpha, bool blend);//パーティクル1個のテクスチャナンバー
	bool CreateBillboard(bool alpha, bool blend);//ver個の四角形を生成
	void Update(CoordTf::VECTOR3 pos, CoordTf::VECTOR4 color, float angle, float size, bool init, float speed);//sizeパーティクル1個のサイズ
	void DrawOff();
	void Draw(int com);
	void Draw();
	void Update(float size, CoordTf::VECTOR4 color);
	void DrawBillboard(int com);
	void DrawBillboard();
	void StreamOutput(int com);
	void StreamOutputBillboard(int com);
	void StreamOutput();
	void StreamOutputBillboard();
	ParameterDXR* getParameter();
	void setRefractiveIndex(float index) {
		dxrPara.updateDXR[dxrBuffSwapIndex()].RefractiveIndex = index;
	}
};

#endif