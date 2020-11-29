//*****************************************************************************************//
//**                                                                                     **//
//**                   　　　         MeshDataクラス                                     **//
//**                                                                                     **//
//*****************************************************************************************//

#ifndef Class_MeshData_Header
#define Class_MeshData_Header

#include "Dx12ProcessCore.h"

class MeshData {

protected:
	UINT** piFaceBuffer = nullptr;
	VertexM* pvVertexBuffer = nullptr;
	int FaceCount = 0;  //ポリゴン数カウンター
	char mFileName[255] = {};
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

	DivideArr divArr[16] = {};
	int numDiv = 0;
	PolygonData mObj;

	bool LoadMaterialFromFile(char* FileName, int numMaxInstance);
	void GetShaderByteCode(bool disp);

public:
	MeshData();
	~MeshData();
	void SetState(bool alpha, bool blend, bool disp, float diffuse = 0.0f, float specu = 0.0f, float ambi = 0.0f);
	bool GetBuffer(char* FileName, int numMaxInstance);
	bool SetVertex();
	bool CreateMesh();
	ID3D12PipelineState* GetPipelineState(int index);

	void Instancing(VECTOR3 pos, VECTOR3 angle, VECTOR3 size);

	void InstancingUpdate(VECTOR4 Color, float disp, float shininess = 4.0f);

	void Update(VECTOR3 pos, VECTOR4 Color, VECTOR3 angle, VECTOR3 size, float disp, float shininess = 4.0f);

	void DrawOff();
	void Draw(int com_no);
	void StreamOutput(int com_no);
	void Draw();
	void StreamOutput();
	void SetCommandList(int no);
	void CopyResource(ID3D12Resource* texture, D3D12_RESOURCE_STATES res, int index = 0);
	void TextureInit(int width, int height, int index = 0);
	HRESULT SetTextureMPixel(BYTE* frame, int index = 0);
	void setDivideArr(DivideArr* arr, int numdiv) {
		numDiv = numdiv;
		memcpy(divArr, arr, sizeof(DivideArr) * numDiv);
	}
	ParameterDXR* getParameter() { return mObj.getParameter(); }
	void UpdateDxrDivideBuffer() {
		mObj.UpdateDxrDivideBuffer();
	}
};

#endif