//*****************************************************************************************//
//**                                                                                     **//
//**                   �@�@�@        SkinMesh�N���X                                      **//
//**                                                                                     **//
//*****************************************************************************************//

#ifndef Class_SkinMesh_Header
#define Class_SkinMesh_Header

#include "Dx12ProcessCore.h"
#include <fbxsdk.h>

class SkinMesh_sub {

protected:
	friend SkinMesh;

	FbxScene *m_pmyScene = NULL;
	float end_frame, current_frame;

	bool centering, offset;
	float cx, cy, cz;
	MATRIX rotZYX;
	float connect_step;

	SkinMesh_sub();
	~SkinMesh_sub();
	bool Create(CHAR *szFileName);
};

class SkinMesh :public Common {

protected:
	friend SkinMesh_sub;
	friend Dx12Process;
	static FbxManager *m_pSdkManager;
	static FbxImporter *m_pImporter;
	//InitFBX�r�������p
	static volatile bool stInitFBX_ON, stSetNewPose_ON;
	static bool ManagerCreated;//�}�l�[�W���[�����t���O

	int                        com_no = 0;
	ID3DBlob                   *vs = nullptr;
	ID3DBlob                   *vsB = nullptr;
	ID3DBlob                   *hs = nullptr;
	ID3DBlob                   *ds = nullptr;
	ID3DBlob                   *ps = nullptr;
	ID3DBlob                   *psB = nullptr;
	bool alpha = FALSE;
	bool blend = FALSE;

	D3D_PRIMITIVE_TOPOLOGY primType_draw, primType_drawB;

	Microsoft::WRL::ComPtr<ID3D12RootSignature> mRootSignature = nullptr;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mSrvHeap = nullptr;
	int texNum;//�e�N�X�`���[��

    //�R���X�^���g�o�b�t�@OBJ
	UploadBuffer<CONSTANT_BUFFER> *mObjectCB0 = nullptr;
	UploadBuffer<CONSTANT_BUFFER2> *mObjectCB1 = nullptr;
	UploadBuffer<SHADER_GLOBAL_BONES> *mObject_BONES = nullptr;
	CONSTANT_BUFFER cb[2];
	SHADER_GLOBAL_BONES sgb[2];
	int sw = 0;
	//UpLoad�J�E���g
	int upCount = 0;
	//����Up�I��
	bool UpOn = FALSE;
	//DrawOn
	bool DrawOn = FALSE;

	std::unique_ptr<VertexView> Vview = nullptr;
	std::unique_ptr<IndexView[]> Iview = nullptr;

	Microsoft::WRL::ComPtr<ID3D12PipelineState> mPSO = nullptr;//�p�C�v���C��OBJ
	Microsoft::WRL::ComPtr<ID3D12PipelineState> mPSO_B = nullptr;//�p�C�v���C��OBJ(�o���v�}�b�v)

	//���b�V���֘A	
	DWORD *m_pdwNumVert;//���b�V�����̒��_��
	DWORD VerAllpcs;   //�S���_��
	MY_VERTEX_S *pvVB;//�g�p��ێ����邩�j�����邩�t���O�Ō��߂�,�ʏ�͔j��
	bool pvVB_delete_f;

	int MateAllpcs;  //�S�}�e���A����
	MY_MATERIAL_S *m_pMaterial;

	//�ꎞ�i�[�p
	DWORD *m_pMaterialCount;//���b�V�����̃}�e���A���J�E���g
	DWORD *pdwNumFace; //���b�V�����̃|���S����
	int *IndexCount34Me;  //4���_�|���S�������O�̃��b�V�����̃C���f�b�N�X��
	int IndexCount34MeAll;
	int *IndexCount3M;  //4���_�|���S��������̃}�e���A�����̃C���f�b�N�X��
	int **pIndex;      //���b�V�����̃C���f�b�N�X�z��(4���_�|���S��������)

	//�{�[��
	int m_iNumBone;
	BONE *m_BoneArray;

	//FBX
	SkinMesh_sub *fbx;
	FbxCluster **m_ppCluster;//�{�[�����
	char *m_pClusterName;
	FbxNode **m_ppNodeArray;//�eNode�ւ̃|�C���^�z��
	int NodeArraypcs;
	FbxNode **m_ppSubAnimationBone;//���̑��A�j���[�V�����{�[���|�C���^�z��
	MATRIX *m_pLastBoneMatrix;
	int AnimLastInd;
	float BoneConnect;

	static std::mutex mtx;
	static void Lock() { mtx.lock(); }
	static void Unlock() { mtx.unlock(); }

	void DestroyFBX();
	FbxScene* GetScene(int p);
	int SearchNodeCount(FbxNode *pnode, FbxNodeAttribute::EType SearchType);
	FbxNode *SearchNode(FbxNode *pnode, FbxNodeAttribute::EType SearchType, int Ind);
	HRESULT InitFBX(CHAR* szFileName, int p);
	void CreateIndexBuffer(int cnt, int *pIndex, int IviewInd);
	void CreateIndexBuffer2(int *pIndex, int IviewInd);
	HRESULT ReadSkinInfo(MY_VERTEX_S *pvVB);
	MATRIX GetCurrentPoseMatrix(int index);
	void MatrixMap_Bone(SHADER_GLOBAL_BONES *sbB);
	void GetTexture();
	bool SetNewPoseMatrices(float time, int ind);
	void CreateRotMatrix(float thetaZ, float thetaY, float thetaX, int ind);
	void CbSwap();

public:
	//���ʂŎg���}�l�[�W���[����(�O���Ő���������s��)
	static void CreateManager();
	static void DeleteManager();

	SkinMesh();
	~SkinMesh();

	void SetCommandList(int no);
	void SetState(bool alpha, bool blend);
	void ObjCentering(bool f, int ind);
	void ObjCentering(float x, float y, float z, float thetaZ, float thetaY, float thetaX, int ind);
	void ObjOffset(float x, float y, float z, float thetaZ, float thetaY, float thetaX, int ind);
	void SetConnectStep(int ind, float step);
	void Vertex_hold();
	HRESULT GetFbx(CHAR* szFileName);
	void GetBuffer(float end_frame);
	void SetVertex();
	void SetDiffuseTextureName(char *textureName, int materialIndex);
	void SetNormalTextureName(char *textureName, int materialIndex);
	void CreateFromFBX(bool disp);
	void CreateFromFBX();
	HRESULT GetFbxSub(CHAR* szFileName, int ind);
	HRESULT GetBuffer_Sub(int ind, float end_frame);
	void CreateFromFBX_SubAnimation(int ind);
	bool Update(float time, float x, float y, float z, float r, float g, float b, float a, float thetaZ, float thetaY, float thetaX, float size);
	bool Update(int ind, float time, float x, float y, float z, float r, float g, float b, float a, float thetaZ, float thetaY, float thetaX, float size, float disp = 1.0f);
	void DrawOff();
	void Draw();
	VECTOR3 GetVertexPosition(int verNum, float adjustZ, float adjustY, float adjustX, float thetaZ, float thetaY, float thetaX, float scale);
};

#endif