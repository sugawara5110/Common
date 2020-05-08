//*****************************************************************************************//
//**                                                                                     **//
//**                   �@�@�@         MeshData�N���X                                     **//
//**                                                                                     **//
//*****************************************************************************************//

#ifndef Class_MeshData_Header
#define Class_MeshData_Header

#include "Dx12ProcessCore.h"

class MeshData {

protected:
	ID3DBlob* vs = nullptr;
	ID3DBlob* ps = nullptr;
	ID3DBlob* ps_NoMap = nullptr;
	ID3DBlob* hs = nullptr;
	ID3DBlob* ds = nullptr;
	ID3DBlob* gs = nullptr;
	ID3DBlob* gs_NoMap = nullptr;

	UINT** piFaceBuffer = nullptr;
	VertexM* pvVertexBuffer = nullptr;
	int FaceCount = 0;  //�|���S�����J�E���^�[
	char mFileName[255] = {};
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
	PolygonData mObj;

	bool LoadMaterialFromFile(char* FileName);
	void GetShaderByteCode(bool disp);

public:
	MeshData();
	~MeshData();
	void SetState(bool alpha, bool blend, bool disp, float diffuse = 0.0f, float specu = 0.0f, float ambi = 0.0f);
	bool GetBuffer(char* FileName);
	bool SetVertex();
	bool CreateMesh();
	ID3D12PipelineState* GetPipelineState(int index);
	//����Update
	void InstancedMap(float x, float y, float z, float thetaZ, float thetaY, float thetaX, float size);
	void InstanceUpdate(float r, float g, float b, float a, float disp, float shininess = 4.0f);
	//�P��Update
	void Update(float x, float y, float z, float r, float g, float b, float a, float thetaZ, float thetaY, float thetaX, float size, float disp, float shininess = 4.0f);
	//�`��
	void DrawOff();
	void Draw();

	void SetCommandList(int no);
	void CopyResource(ID3D12Resource* texture, D3D12_RESOURCE_STATES res, int index = 0);
	void TextureInit(int width, int height, int index = 0);
	HRESULT SetTextureMPixel(BYTE* frame, int index = 0);
};

#endif