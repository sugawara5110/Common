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
	ID3DBlob* vs = nullptr;
	ID3DBlob* ps = nullptr;
	ID3DBlob* hs = nullptr;
	ID3DBlob* ds = nullptr;
	ID3DBlob* gs = nullptr;

	//�R���X�^���g�o�b�t�@OBJ
	ConstantBuffer<CONSTANT_BUFFER>* mObjectCB = nullptr;
	ConstantBuffer<CONSTANT_BUFFER2>* mObject_MESHCB = nullptr;//�}�e���A���n���p(1�񂵂��X�V���Ȃ�)
	//UpLoad�p
	CONSTANT_BUFFER cb[2];
	//UpLoad�J�E���g
	int upCount = 0;
	//����Up�I��
	bool UpOn = false;
	//DrawOn
	bool DrawOn = false;

	int* piFaceBuffer;
	VertexM* pvVertexBuffer;
	int FaceCount;  //�|���S�����J�E���^�[
	char mFileName[255];
	//�ꎞ�ۊ�
	VECTOR3* pvCoord;
	VECTOR3* pvNormal;
	VECTOR2* pvTexture;
	int ins_no = 0;
	int insNum[2] = {};

	bool alpha = false;
	bool blend = false;
	bool disp = false;//�e�Z���[�^�t���O
	float addDiffuse = 0.0f;
	float addSpecular = 0.0f;
	float addAmbient = 0.0f;

	struct meshMaterial {
		CHAR szName[255] = {};//�}�e���A����
		CHAR szTextureName[255] = {};//�e�N�X�`���[�t�@�C����
		CHAR norTextureName[255] = {};//�m�[�}���}�b�v��
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
	//����Update
	void InstancedMap(float x, float y, float z, float thetaZ, float thetaY, float thetaX, float size);
	void InstanceUpdate(float r, float g, float b, float a, float disp, float shininess = 4.0f);
	//�P��Update
	void Update(float x, float y, float z, float r, float g, float b, float a, float thetaZ, float thetaY, float thetaX, float size, float disp, float shininess = 4.0f);
	//�`��
	void DrawOff();
	void Draw();
};

#endif