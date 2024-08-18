//*****************************************************************************************//
//**                                                                                     **//
//**                                 SkinMeshクラス(FbxLoader)                           **//
//**                                                                                     **//
//*****************************************************************************************//

#ifndef Class_SkinMesh_Header
#define Class_SkinMesh_Header

#include "DxSkinnedCom.h"

class SkinMesh :public SkinMeshHelper {

protected:
	bool alpha = false;
	bool blend = false;
	float addDiffuse = 0.0f;
	float addSpecular = 0.0f;
	float addAmbient = 0.0f;

	DivideArr divArr[16] = {};
	int numDiv = 0;

	//コンスタントバッファOBJ
	ConstantBuffer<SHADER_GLOBAL_BONES>* mObject_BONES = nullptr;

	Skin_VERTEX** pvVB = nullptr;//使用後保持するか破棄するかフラグで決める,通常は破棄
	VertexM** pvVBM = nullptr;
	bool pvVB_delete_f = true;
	uint32_t*** newIndex = nullptr;
	uint32_t** NumNewIndex = nullptr;

	BasicPolygon* mObj = nullptr;
	SkinnedCom* sk = nullptr;

	void GetShaderByteCode(bool disp, bool smooth);
	void GetMeshCenterPos();
	void createMaterial(int meshInd, UINT numMaterial, FbxMeshNode* mesh, char* uv0Name, char* uv1Name, int* uvSw);
	void swapTex(Skin_VERTEX* vb, FbxMeshNode* mesh, int* uvSw);

public:
	SkinMesh();
	~SkinMesh();

	void SetState(bool alpha, bool blend, float diffuse = 0.0f, float specu = 0.0f, float ambi = 0.0f);
	void GetBuffer(int numMaxInstance, int num_end_frame, float* end_frame, bool singleMesh = false, bool deformer = true);
	void GetBuffer(int numMaxInstance, float end_frame, bool singleMesh = false, bool deformer = true);
	void SetVertex(bool lclOn = false, bool axisOn = false, bool VerCentering = false);
	void Vertex_hold();
	void SetDiffuseTextureName(char* textureName, int materialIndex, int meshIndex);
	void SetNormalTextureName(char* textureName, int materialIndex, int meshIndex);
	void SetSpeculerTextureName(char* textureName, int materialIndex, int meshIndex);

	void setMaterialType(MaterialType type, int materialIndex = -1, int meshIndex = -1);

	void setPointLight(int meshIndex, int materialIndex, int InstanceIndex, bool on_off,
		float range, CoordTf::VECTOR3 atten = { 0.01f, 0.001f, 0.001f });

	void setPointLightAll(bool on_off,
		float range, CoordTf::VECTOR3 atten = { 0.01f, 0.001f, 0.001f });

	bool CreateFromFBX(int comIndex, bool disp = false, bool smooth = false, float divideBufferMagnification = 1.0f);

	void Instancing(CoordTf::VECTOR3 pos, CoordTf::VECTOR4 Color,
		CoordTf::VECTOR3 angle, CoordTf::VECTOR3 size);

	bool InstancingUpdate(int ind, float time, int InternalAnimationIndex = 0,
		float disp = 1.0f,
		float SmoothRange = 0.1f, float SmoothRatio = 0.999f, float shininess = 4.0f);

	bool Update(int ind, float time,
		CoordTf::VECTOR3 pos, CoordTf::VECTOR4 Color,
		CoordTf::VECTOR3 angle, CoordTf::VECTOR3 size,
		int InternalAnimationIndex = 0,
		float disp = 1.0f,
		float SmoothRange = 0.1f, float SmoothRatio = 0.999f, float shininess = 4.0f);

	void DrawOff();
	void Draw(int comIndex);
	void StreamOutput(int comIndex);

	CoordTf::VECTOR3 GetVertexPosition(int meshIndex, int verNum, float adjustZ, float adjustY, float adjustX,
		float thetaZ, float thetaY, float thetaX, float scale);

	void CopyResource(int comIndex, ID3D12Resource* texture, D3D12_RESOURCE_STATES res, int texIndex = 0, int meshIndex = 0);
	void TextureInit(int width, int height, int texIndex = 0, int meshIndex = 0);
	HRESULT SetTextureMPixel(int com_no, BYTE* frame, int texIndex = 0, int meshIndex = 0);
	ParameterDXR* getParameter(int meshIndex) { return mObj[meshIndex].getParameter(); }
	void setDivideArr(DivideArr* arr, int numdiv) {
		numDiv = numdiv;
		memcpy(divArr, arr, sizeof(DivideArr) * numDiv);
	}
	void UpdateDxrDivideBuffer() {
		for (int i = 0; i < getNumMesh(); i++)
			mObj[i].UpdateDxrDivideBuffer();
	}
	void setRefractiveIndex(int meshIndex, float RefractiveIndex) {
		mObj[meshIndex].setRefractiveIndex(RefractiveIndex);
	}
	void setAllRefractiveIndex(float Index) {
		for (int i = 0; i < getNumMesh(); i++)
			mObj[i].setRefractiveIndex(Index);
	}
	void SetName(char* name) {
		for (int i = 0; i < getNumMesh(); i++)
			mObj[i].SetName(name);
	}
	void setRoughness(int meshIndex, float roug) {
		mObj[meshIndex].setRoughness(roug);
	}
	void setAllRoughness(float roug) {
		for (int i = 0; i < getNumMesh(); i++)
			mObj[i].setRoughness(roug);
	}
};

#endif