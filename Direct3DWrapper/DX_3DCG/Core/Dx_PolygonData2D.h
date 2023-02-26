//*****************************************************************************************//
//**                                                                                     **//
//**                   �@�@�@          PolygonData2D                                     **//
//**                                                                                     **//
//*****************************************************************************************//

#ifndef Class_PolygonData2D_Header
#define Class_PolygonData2D_Header

#include "Dx_Common.h"

#define INSTANCE_PCS_2D 256

struct CONSTANT_BUFFER2D {
	CoordTf::VECTOR4 Pos[INSTANCE_PCS_2D];
	CoordTf::VECTOR4 Color[INSTANCE_PCS_2D];
	CoordTf::VECTOR4 sizeXY[INSTANCE_PCS_2D];
	CoordTf::VECTOR4 WidHei;//�E�C���h�Ewh
};

class PolygonData2D :public DxCommon {

protected:
	ID3DBlob* vs = nullptr;
	ID3DBlob* ps = nullptr;

	int      ver;//���_��

	ComPtr<ID3D12RootSignature> mRootSignature = nullptr;
	ComPtr<ID3D12DescriptorHeap> mDescHeap = nullptr;

	//�R���X�^���g�o�b�t�@OBJ
	ConstantBuffer<CONSTANT_BUFFER2D>* mObjectCB = nullptr;
	CONSTANT_BUFFER2D cb2[2];
	bool firstCbSet[2];
	//DrawOn
	bool DrawOn = false;

	//���_�o�b�t�@OBJ
	std::unique_ptr<VertexView> Vview = nullptr;
	std::unique_ptr<IndexView> Iview = nullptr;
	ComPtr<ID3D12PipelineState> mPSO = nullptr;

	//�e�N�X�`���ێ�
	bool tex_on = false;

	int  ins_no = 0;
	int  insNum[2] = {};//Draw�œǂݍ��ݗp

	static float magnificationX;//�{��
	static float magnificationY;
	float magX = 1.0f;
	float magY = 1.0f;

	void GetShaderByteCode();
	void SetConstBf(CONSTANT_BUFFER2D* cb2, float x, float y, float z,
		float r, float g, float b, float a, float sizeX, float sizeY);
	void CbSwap();

public:
	MY_VERTEX2* d2varray;  //���_�z��
	std::uint16_t* d2varrayI;//���_�C���f�b�N�X

	static void Pos2DCompute(CoordTf::VECTOR3* p);//3D���W��2D���W�ϊ�(magnificationX, magnificationY�͖��������)
	static void SetMagnification(float x, float y);//�\���{��

	PolygonData2D();
	~PolygonData2D();
	void DisabledMagnification();
	ID3D12PipelineState* GetPipelineState();
	void GetVBarray2D(int pcs);
	void TexOn();
	bool CreateBox(int comIndex, float x, float y, float z,
		float sizex, float sizey,
		float r, float g, float b, float a,
		bool blend, bool alpha, int noTex = -1);
	bool Create(int comIndex, bool blend, bool alpha, int noTex = -1);
	void InstancedSetConstBf(float x, float y, float r, float g, float b, float a, float sizeX, float sizeY);
	void InstancedSetConstBf(float x, float y, float z, float r, float g, float b, float a, float sizeX, float sizeY);
	void InstanceUpdate();
	void Update(float x, float y, float r, float g, float b, float a, float sizeX, float sizeY);
	void Update(float x, float y, float z, float r, float g, float b, float a, float sizeX, float sizeY);
	void DrawOff();
	void Draw(int comIndex);
};

#endif
