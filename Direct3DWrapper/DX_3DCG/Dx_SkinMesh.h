//*****************************************************************************************//
//**                                                                                     **//
//**                   　　　        SkinMeshクラス(FbxLoader)                           **//
//**                                                                                     **//
//*****************************************************************************************//

#ifndef Class_SkinMesh_Header
#define Class_SkinMesh_Header

#include "Dx12ProcessCore.h"
#include "../../../FbxLoader/FbxLoader.h"

class SkinMesh_sub {

protected:
	friend SkinMesh;
	FbxLoader* fbxL = nullptr;
	float end_frame = 0.0f;
	float current_frame = 0.0f;
	bool centering = false;
	bool offset = false;
	float cx = 0.0f;
	float cy = 0.0f;
	float cz = 0.0f;
	float connect_step = 3000.0f;
	MATRIX rotZYX = {};

	SkinMesh_sub();
	~SkinMesh_sub();
	bool Create(CHAR* szFileName);
};

class SkinMesh {

protected:
	friend SkinMesh_sub;
	friend Dx12Process;
	ID3DBlob* vs = nullptr;
	ID3DBlob* hs = nullptr;
	ID3DBlob* ds = nullptr;
	ID3DBlob* ps = nullptr;
	ID3DBlob* ps_NoMap = nullptr;
	ID3DBlob* gs = nullptr;
	ID3DBlob* gs_NoMap = nullptr;

	bool alpha = false;
	bool blend = false;
	float addDiffuse = 0.0f;
	float addSpecular = 0.0f;
	float addAmbient = 0.0f;

	PrimitiveType primType_create;
	DivideArr divArr[16] = {};
	int numDiv = 3;

	//コンスタントバッファOBJ
	ConstantBuffer<SHADER_GLOBAL_BONES>* mObject_BONES = nullptr;
	SHADER_GLOBAL_BONES sgb[2] = {};

	MY_VERTEX_S** pvVB = nullptr;//使用後保持するか破棄するかフラグで決める,通常は破棄
	UINT*** newIndex = nullptr;
	bool pvVB_delete_f = true;

	//ボーン
	int numBone = 0;
	BONE* m_BoneArray = nullptr;
	char* boneName = nullptr;
	int InternalAnimationIndex = 0;

	//FBX
	int numMesh = 0;
	SkinMesh_sub* fbx = nullptr;
	Deformer** m_ppSubAnimationBone = nullptr;//その他アニメーションボーンポインタ配列
	MATRIX* m_pLastBoneMatrix = nullptr;
	int AnimLastInd = -1;
	float BoneConnect;
	PolygonData* mObj = nullptr;
	int com_no = 0;

	void DestroyFBX();
	HRESULT InitFBX(CHAR* szFileName, int p);
	void ReadSkinInfo(FbxMeshNode* mesh, MY_VERTEX_S* pvVB);
	MATRIX GetCurrentPoseMatrix(int index);
	void MatrixMap_Bone(SHADER_GLOBAL_BONES* sbB);
	bool SetNewPoseMatrices(float time, int ind);
	void CreateRotMatrix(float thetaZ, float thetaY, float thetaX, int ind);
	void GetShaderByteCode(bool disp);

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
	void GetBuffer(float end_frame);
	void SetVertex(bool lclOn = false);
	void SetDiffuseTextureName(char* textureName, int materialIndex, int meshIndex);
	void SetNormalTextureName(char* textureName, int materialIndex, int meshIndex);
	void SetSpeculerTextureName(char* textureName, int materialIndex, int meshIndex);
	bool CreateFromFBX(bool disp);
	bool CreateFromFBX();
	HRESULT GetFbxSub(CHAR* szFileName, int ind);
	HRESULT GetBuffer_Sub(int ind, float end_frame);
	void CreateFromFBX_SubAnimation(int ind);
	void setInternalAnimationIndex(int index) { InternalAnimationIndex = index; }
	bool Update(float time, float x, float y, float z, float r, float g, float b, float a,
		float thetaZ, float thetaY, float thetaX, float size);
	bool Update(int ind, float time, float x, float y, float z, float r, float g, float b, float a,
		float thetaZ, float thetaY, float thetaX, float size, float disp = 1.0f, float shininess = 4.0f);
	void DrawOff();
	void Draw(int com_no);
	void StreamOutput(int com_no);
	void Draw();
	void StreamOutput();
	VECTOR3 GetVertexPosition(int meshIndex, int verNum, float adjustZ, float adjustY, float adjustX,
		float thetaZ, float thetaY, float thetaX, float scale);

	void SetCommandList(int no);
	void CopyResource(ID3D12Resource* texture, D3D12_RESOURCE_STATES res, int texIndex = 0, int meshIndex = 0);
	void TextureInit(int width, int height, int texIndex = 0, int meshIndex = 0);
	HRESULT SetTextureMPixel(BYTE* frame, int texIndex = 0, int meshIndex = 0);
	int getNumMesh() { return numMesh; }
	ParameterDXR* getParameter(int meshIndex) { return mObj[meshIndex].getParameter(); }
};

#endif