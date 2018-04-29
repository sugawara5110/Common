//*****************************************************************************************//
//**                                                                                     **//
//**                   �@�@�@         MeshData�N���X                                     **//
//**                                                                                     **//
//*****************************************************************************************//

#ifndef Class_MeshData_Header
#define Class_MeshData_Header

#include "Dx12ProcessCore.h"

class MeshData :public Common {

protected:
	int                        com_no = 0;
	ID3DBlob                   *vs = nullptr;
	ID3DBlob                   *vsB = nullptr;
	ID3DBlob                   *ps = nullptr;
	ID3DBlob                   *psB = nullptr;
	ID3DBlob                   *hs = nullptr;
	ID3DBlob                   *ds = nullptr;

	Microsoft::WRL::ComPtr<ID3D12RootSignature> mRootSignature = nullptr;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mSrvHeap = nullptr;
	int texNum;//�e�N�X�`���[��

			   //�R���X�^���g�o�b�t�@OBJ
	UploadBuffer<CONSTANT_BUFFER> *mObjectCB = nullptr;
	UploadBuffer<CONSTANT_BUFFER2> *mObject_MESHCB = nullptr;//�}�e���A���n���p(1�񂵂��X�V���Ȃ�)
															 //UpLoad�p
	CONSTANT_BUFFER cb[2];
	int sw = 0;//���؂�ւ�
			   //UpLoad�J�E���g
	int upCount = 0;
	//����Up�I��
	bool UpOn = FALSE;
	//DrawOn
	bool DrawOn = FALSE;

	//���_�o�b�t�@OBJ
	std::unique_ptr<VertexView> Vview = nullptr;
	std::unique_ptr<IndexView[]> Iview = nullptr;

	Microsoft::WRL::ComPtr<ID3D12PipelineState> mPSO = nullptr;//�p�C�v���C��OBJ
	Microsoft::WRL::ComPtr<ID3D12PipelineState> mPSO_B = nullptr;//�p�C�v���C��OBJ(�o���v�}�b�v)

	int              MaterialCount = 0;//�}�e���A����
	int              *piFaceBuffer;
	VertexM          *pvVertexBuffer;
	int              FaceCount;  //�|���S�����J�E���^�[
	char             mFileName[255];
	//�ꎞ�ۊ�
	VECTOR3 *pvCoord;
	VECTOR3 *pvNormal;
	VECTOR2 *pvTexture;
	int insNum = 0;

	MY_MATERIAL* pMaterial;

	bool alpha = FALSE;
	bool blend = FALSE;
	bool disp = FALSE;//�e�Z���[�^�t���O
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
	void SetCommandList(int no);
	void SetState(bool alpha, bool blend, bool disp);
	void SetState(bool alpha, bool blend, bool disp, float diffuse, float specu);
	void GetBuffer(char *FileName);
	void SetVertex();
	void CreateMesh();
	void GetTexture();
	ID3D12PipelineState *GetPipelineState();
	//��./dat/mesh/tree.obj
	//����Update
	void InstancedMap(float x, float y, float z, float thetaZ, float thetaY, float thetaX, float size);
	void InstanceUpdate(float r, float g, float b, float a, float disp);
	//�P��Update
	void Update(float x, float y, float z, float r, float g, float b, float a, float thetaZ, float thetaY, float thetaX, float size, float disp);
	//�`��
	void DrawOff();
	void Draw();
};

#endif