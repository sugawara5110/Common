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
	ID3DBlob* vs = nullptr;
	ID3DBlob* ps = nullptr;
	ID3DBlob* hs = nullptr;
	ID3DBlob* ds = nullptr;
	ID3DBlob* gs = nullptr;

	//コンスタントバッファOBJ
	ConstantBuffer<CONSTANT_BUFFER>* mObjectCB = nullptr;
	ConstantBuffer<CONSTANT_BUFFER2>* mObject_MESHCB = nullptr;//マテリアル渡し用(1回しか更新しない)
	//UpLoad用
	CONSTANT_BUFFER cb[2];
	//UpLoadカウント
	int upCount = 0;
	//初回Up終了
	bool UpOn = false;
	//DrawOn
	bool DrawOn = false;

	int* piFaceBuffer;
	VertexM* pvVertexBuffer;
	int FaceCount;  //ポリゴン数カウンター
	char mFileName[255];
	//一時保管
	VECTOR3* pvCoord;
	VECTOR3* pvNormal;
	VECTOR2* pvTexture;
	int ins_no = 0;
	int insNum[2] = {};

	bool alpha = false;
	bool blend = false;
	bool disp = false;//テセレータフラグ
	float addDiffuse = 0.0f;
	float addSpecular = 0.0f;
	float addAmbient = 0.0f;

	struct meshMaterial {
		CHAR szName[255] = {};//マテリアル名
		CHAR szTextureName[255] = {};//テクスチャーファイル名
		CHAR norTextureName[255] = {};//ノーマルマップ名
	};
	std::unique_ptr<meshMaterial[]>mMat = nullptr;

	PrimitiveType primType_create;
	DivideArr divArr[16] = {};
	int numDiv = 3;
	drawPara dpara = {};

	bool LoadMaterialFromFile(char* FileName);
	void GetShaderByteCode(bool disp);
	void CbSwap();

public:
	MeshData();
	~MeshData();
	void SetState(bool alpha, bool blend, bool disp, float diffuse = 0.0f, float specu = 0.0f, float ambi = 0.0f);
	bool GetBuffer(char* FileName);
	bool SetVertex();
	bool CreateMesh();
	bool GetTexture();
	ID3D12PipelineState* GetPipelineState();
	//複数Update
	void InstancedMap(float x, float y, float z, float thetaZ, float thetaY, float thetaX, float size);
	void InstanceUpdate(float r, float g, float b, float a, float disp, float shininess = 4.0f);
	//単体Update
	void Update(float x, float y, float z, float r, float g, float b, float a, float thetaZ, float thetaY, float thetaX, float size, float disp, float shininess = 4.0f);
	//描画
	void DrawOff();
	void Draw();
};

#endif