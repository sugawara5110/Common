//*****************************************************************************************//
//**                                                                                     **//
//**                   　　　         MeshDataクラス                                     **//
//**                                                                                     **//
//*****************************************************************************************//

#ifndef Class_MeshData_Header
#define Class_MeshData_Header

#include "Dx12ProcessCore.h"

class MeshData :public Common {

protected:
	ID3DBlob * vs = nullptr;
	ID3DBlob *vsB = nullptr;
	ID3DBlob *ps = nullptr;
	ID3DBlob *psB = nullptr;
	ID3DBlob *hs = nullptr;
	ID3DBlob *ds = nullptr;

	Microsoft::WRL::ComPtr<ID3D12RootSignature> mRootSignature = nullptr;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mSrvHeap = nullptr;
	int texNum;//テクスチャー数

			   //コンスタントバッファOBJ
	ConstantBuffer<CONSTANT_BUFFER> *mObjectCB = nullptr;
	ConstantBuffer<CONSTANT_BUFFER2> *mObject_MESHCB = nullptr;//マテリアル渡し用(1回しか更新しない)
															 //UpLoad用
	CONSTANT_BUFFER cb[2];
	int sw = 0;//↑切り替え
			   //UpLoadカウント
	int upCount = 0;
	//初回Up終了
	bool UpOn = false;
	//DrawOn
	bool DrawOn = false;

	//頂点バッファOBJ
	std::unique_ptr<VertexView> Vview = nullptr;
	std::unique_ptr<IndexView[]> Iview = nullptr;

	Microsoft::WRL::ComPtr<ID3D12PipelineState> mPSO = nullptr;//パイプラインOBJ
	Microsoft::WRL::ComPtr<ID3D12PipelineState> mPSO_B = nullptr;//パイプラインOBJ(バンプマップ)

	int        MaterialCount = 0;//マテリアル数
	int        *piFaceBuffer;
	VertexM    *pvVertexBuffer;
	int        FaceCount;  //ポリゴン数カウンター
	char       mFileName[255];
	//一時保管
	VECTOR3 *pvCoord;
	VECTOR3 *pvNormal;
	VECTOR2 *pvTexture;
	int insNum = 0;

	MY_MATERIAL* pMaterial;

	bool alpha = false;
	bool blend = false;
	bool disp = false;//テセレータフラグ
	float addDiffuse;
	float addSpecular;

	D3D_PRIMITIVE_TOPOLOGY primType_draw, primType_drawB;

	static std::mutex mtx;
	static void Lock() { mtx.lock(); }
	static void Unlock() { mtx.unlock(); }

	void LoadMaterialFromFile(char *FileName, MY_MATERIAL **ppMaterial);
	void GetShaderByteCode(bool disp);
	void CbSwap();

public:
	MeshData();
	~MeshData();
	void SetState(bool alpha, bool blend, bool disp, float diffuse = 0.0f, float specu = 0.0f);
	void GetBuffer(char *FileName);
	void SetVertex();
	void CreateMesh();
	void GetTexture();
	ID3D12PipelineState *GetPipelineState();
	//複数Update
	void InstancedMap(float x, float y, float z, float thetaZ, float thetaY, float thetaX, float size);
	void InstanceUpdate(float r, float g, float b, float a, float disp);
	//単体Update
	void Update(float x, float y, float z, float r, float g, float b, float a, float thetaZ, float thetaY, float thetaX, float size, float disp);
	//描画
	void DrawOff();
	void Draw();
};

#endif