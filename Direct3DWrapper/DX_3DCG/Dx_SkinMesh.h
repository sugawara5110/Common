//*****************************************************************************************//
//**                                                                                     **//
//**                                 SkinMeshクラス(FbxLoader)                           **//
//**                                                                                     **//
//*****************************************************************************************//

#ifndef Class_SkinMesh_Header
#define Class_SkinMesh_Header

#include "Core/Dx12ProcessCore.h"
#include "DxSkinnedCom.h"
#include "../../../FbxLoader/FbxLoader.h"

class SkinMesh {

protected:
	class SkinMesh_sub {
	public:
		FbxLoader* fbxL = nullptr;
		std::unique_ptr<float[]> end_frame = nullptr;
		float current_frame = 0.0f;
		bool centering = false;
		bool offset = false;
		float cx = 0.0f;
		float cy = 0.0f;
		float cz = 0.0f;
		float connect_step;
		CoordTf::MATRIX rotZYX = {};
		int InternalLastAnimationIndex = 0;

		SkinMesh_sub();
		~SkinMesh_sub();
		bool Create(CHAR* szFileName);
	};

	bool alpha = false;
	bool blend = false;
	float addDiffuse = 0.0f;
	float addSpecular = 0.0f;
	float addAmbient = 0.0f;

	DivideArr divArr[16] = {};
	int numDiv = 0;

	//コンスタントバッファOBJ
	ConstantBuffer<SHADER_GLOBAL_BONES>* mObject_BONES = nullptr;
	SHADER_GLOBAL_BONES sgb[2] = {};

	MY_VERTEX_S** pvVB = nullptr;//使用後保持するか破棄するかフラグで決める,通常は破棄
	VertexM** pvVBM = nullptr;
	UINT*** newIndex = nullptr;
	bool pvVB_delete_f = true;

	//ボーン
	int* numBone = nullptr;
	int maxNumBone = 0;
	int maxNumBoneMeshIndex = 0;
	BONE* m_BoneArray = nullptr;
	char* boneName = nullptr;

	struct meshCenterPos {
		CoordTf::VECTOR3 pos = {};
		UINT bBoneIndex = {};
		float bBoneWeight = {};
	};
	std::unique_ptr<meshCenterPos[]> centerPos = nullptr;

	//FBX
	int numMesh = 0;
	SkinMesh_sub* fbx = nullptr;
	Deformer** m_ppSubAnimationBone = nullptr;//その他アニメーションボーンポインタ配列
	CoordTf::MATRIX* m_pLastBoneMatrix = nullptr;
	int AnimLastInd;
	float BoneConnect;
	BasicPolygon* mObj = nullptr;
	SkinnedCom* sk = nullptr;
	CoordTf::MATRIX Axis = {};
	std::unique_ptr<bool[]> noUseMesh = nullptr;

	void DestroyFBX();
	HRESULT InitFBX(CHAR* szFileName, int p);
	void ReadSkinInfo(FbxMeshNode* mesh, MY_VERTEX_S* pvVB, meshCenterPos* centerPos);
	CoordTf::MATRIX GetCurrentPoseMatrix(int index);
	void MatrixMap_Bone(SHADER_GLOBAL_BONES* sbB);
	bool SetNewPoseMatrices(float time, int ind, int InternalAnimationIndex);
	void CreateRotMatrix(float thetaZ, float thetaY, float thetaX, int ind);
	void GetShaderByteCode(bool disp, bool smooth);
	void GetMeshCenterPos();
	void createMaterial(int meshInd, UINT numMaterial, FbxMeshNode* mesh, char* uv0Name, char* uv1Name, int* uvSw);
	void swapTex(MY_VERTEX_S* vb, FbxMeshNode* mesh, int* uvSw);
	void splitIndex(UINT numMaterial, FbxMeshNode* mesh, int meshIndex);
	void normalRecalculation(bool lclOn, double** nor, FbxMeshNode* mesh);
	void createAxis();
	void LclTransformation(FbxMeshNode* mesh, CoordTf::VECTOR3* vec);

public:
	SkinMesh();
	~SkinMesh();

	void SetState(bool alpha, bool blend, float diffuse = 0.0f, float specu = 0.0f, float ambi = 0.0f);
	void ObjCentering(bool f, int ind);
	void ObjCentering(float x, float y, float z, float thetaZ, float thetaY, float thetaX, int ind);
	void ObjOffset(float x, float y, float z, float thetaZ, float thetaY, float thetaX, int ind);
	void SetConnectStep(int ind, float step);
	void Vertex_hold();
	HRESULT GetFbx(CHAR* szFileName);
	void GetBuffer(int numMaxInstance, int num_end_frame, float* end_frame, bool singleMesh = false, bool deformer = true);
	void GetBuffer(int numMaxInstance, float end_frame, bool singleMesh = false, bool deformer = true);
	void noUseMeshIndex(int meshIndex);
	void SetVertex(bool lclOn = false, bool axisOn = false, bool VerCentering = false);
	void SetDiffuseTextureName(char* textureName, int materialIndex, int meshIndex);
	void SetNormalTextureName(char* textureName, int materialIndex, int meshIndex);
	void SetSpeculerTextureName(char* textureName, int materialIndex, int meshIndex);

	void setMaterialType(MaterialType type, int materialIndex = -1, int meshIndex = -1);
	void setPointLight(int meshIndex, int materialIndex, int InstanceIndex, bool on_off,
		float range, CoordTf::VECTOR3 atten = { 0.01f, 0.001f, 0.001f });
	void setPointLightAll(bool on_off,
		float range, CoordTf::VECTOR3 atten = { 0.01f, 0.001f, 0.001f });

	bool CreateFromFBX(int comIndex, bool disp = false, bool smooth = false, float divideBufferMagnification = 1.0f);
	HRESULT GetFbxSub(CHAR* szFileName, int ind);
	HRESULT GetBuffer_Sub(int ind, int num_end_frame, float* end_frame);
	HRESULT GetBuffer_Sub(int ind, float end_frame);
	void CreateFromFBX_SubAnimation(int ind);

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
	int getNumMesh() { return numMesh; }
	ParameterDXR* getParameter(int meshIndex) { return mObj[meshIndex].getParameter(); }
	void setDivideArr(DivideArr* arr, int numdiv) {
		numDiv = numdiv;
		memcpy(divArr, arr, sizeof(DivideArr) * numDiv);
	}
	void UpdateDxrDivideBuffer() {
		for (int i = 0; i < numMesh; i++)
			mObj[i].UpdateDxrDivideBuffer();
	}
	void setRefractiveIndex(int meshIndex, float RefractiveIndex) {
		mObj[meshIndex].setRefractiveIndex(RefractiveIndex);
	}
	void setAllRefractiveIndex(float Index) {
		for (int i = 0; i < numMesh; i++)
			mObj[i].setRefractiveIndex(Index);
	}
	void SetName(char* name) {
		for (int i = 0; i < numMesh; i++)
			mObj[i].SetName(name);
	}
};

#endif