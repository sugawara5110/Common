//*****************************************************************************************//
//**                                                                                     **//
//**                   　　　         MeshDataクラス                                     **//
//**                                                                                     **//
//*****************************************************************************************//

#ifndef Class_MeshData_Header
#define Class_MeshData_Header

#include "Core/Dx_BasicPolygon.h"

class MeshData {

protected:
	UINT** piFaceBuffer = nullptr;
	VertexM* pvVertexBuffer = nullptr;
	int FaceCount = 0;  //ポリゴン数カウンター
	char mFileName[255] = {};
	//一時保管
	CoordTf::VECTOR3* pvCoord;
	CoordTf::VECTOR3* pvNormal;
	CoordTf::VECTOR2* pvTexture;
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
	BasicPolygon mObj;

	bool LoadMaterialFromFile(char* FileName, int numMaxInstance);

public:
	MeshData();
	~MeshData();
	void SetState(bool alpha, bool blend, bool disp, float diffuse = 0.0f, float specu = 0.0f, float ambi = 0.0f);
	bool GetBuffer(char* FileName, int numMaxInstance);
	bool SetVertex();
	void setMaterialType(MaterialType type, int materialIndex = -1);

	void setPointLight(int materialIndex, int InstanceIndex, bool on_off,
		float range, CoordTf::VECTOR3 atten = { 0.01f, 0.001f, 0.001f });

	void setPointLightAll(bool on_off,
		float range, CoordTf::VECTOR3 atten = { 0.01f, 0.001f, 0.001f });

	bool CreateMesh(int comIndex, bool smooth = false, float divideBufferMagnification = 1.0f);
	ID3D12PipelineState* GetPipelineState(int index);

	void Instancing(CoordTf::VECTOR3 pos, CoordTf::VECTOR3 angle, CoordTf::VECTOR3 size, CoordTf::VECTOR4 Color);

	void InstancingUpdate(float disp, float SmoothRange = 0.1f, float SmoothRatio = 0.999f, float shininess = 4.0f);

	void Update(CoordTf::VECTOR3 pos, CoordTf::VECTOR4 Color,
		CoordTf::VECTOR3 angle, CoordTf::VECTOR3 size,
		float disp, float SmoothRange = 0.1f, float SmoothRatio = 0.999f, float shininess = 4.0f);

	void DrawOff();
	void Draw(int comIndex);
	void StreamOutput(int comIndex);
	void CopyResource(int comIndex, ID3D12Resource* texture, D3D12_RESOURCE_STATES res, int index = 0);
	void TextureInit(int width, int height, int index = 0);
	HRESULT SetTextureMPixel(int comIndex, BYTE* frame, int index = 0);
	void setDivideArr(DivideArr* arr, int numdiv) {
		numDiv = numdiv;
		memcpy(divArr, arr, sizeof(DivideArr) * numDiv);
	}
	ParameterDXR* getParameter() { return mObj.getParameter(); }
	void UpdateDxrDivideBuffer() {
		mObj.UpdateDxrDivideBuffer();
	}
	void setRefractiveIndex(float index) {
		mObj.setRefractiveIndex(index);
	}
	void setRoughness(float roug) {
		mObj.setRoughness(roug);
	}

	void SetName(char* name) { mObj.SetName(name); }
};

#endif