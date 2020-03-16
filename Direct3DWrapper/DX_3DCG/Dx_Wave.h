//*****************************************************************************************//
//**                                                                                     **//
//**                   �@�@�@         Wave�N���X                                         **//
//**                                                                                     **//
//*****************************************************************************************//

#ifndef Class_Wave_Header
#define Class_Wave_Header

#include "Dx12ProcessCore.h"

class Wave :public Common {

protected:
	ID3DBlob* cs = nullptr;
	ID3DBlob* vs = nullptr;
	ID3DBlob* ps = nullptr;
	ID3DBlob* hs = nullptr;
	ID3DBlob* ds = nullptr;

	ComPtr<ID3D12RootSignature> mRootSignatureCom = nullptr;
	ComPtr<ID3D12RootSignature> mRootSignatureDraw = nullptr;
	ComPtr<ID3D12DescriptorHeap> mSrvHeap = nullptr;

	ConstantBuffer<CONSTANT_BUFFER>* mObjectCB = nullptr;
	ConstantBuffer<CONSTANT_BUFFER2>* mObjectCB1 = nullptr;
	ConstantBuffer<CONSTANT_BUFFER_WAVE>* mObjectCB_WAVE = nullptr;
	CONSTANT_BUFFER cb[2];
	CONSTANT_BUFFER2 sg;
	CONSTANT_BUFFER_WAVE cbw;
	//UpLoad�J�E���g
	int upCount = 0;
	//����Up�I��
	bool UpOn = false;
	//DrawOn
	bool DrawOn = false;

	std::unique_ptr<VertexView> Vview = nullptr;
	std::unique_ptr<IndexView[]> Iview = nullptr;

	//�e�N�X�`���ԍ�(�ʏ�e�N�X�`���p)
	MY_MATERIAL_S material[1];
	int ins_no = 0;
	int insNum[2] = {};

	int div;//������

	Vertex* d3varray;  //���_�z��
	std::uint16_t* d3varrayI;//���_�C���f�b�N�X
	int ver;      //���_��
	int verI;    //���_�C���f�b�N�X

	ComPtr<ID3D12PipelineState> mPSOCom = nullptr;//�p�C�v���C��OBJ
	ComPtr<ID3D12PipelineState> mPSODraw = nullptr;
	ComPtr<ID3D12Resource> mInputBuffer = nullptr;
	ComPtr<ID3D12Resource> mInputUploadBuffer = nullptr;
	ComPtr<ID3D12Resource> mOutputBuffer = nullptr;

	void GetShaderByteCode(int texNum);
	bool ComCreate();
	bool DrawCreate(int texNo, int nortNo, bool blend, bool alpha);
	void CbSwap();
	void Compute();
	void DrawSub();

public:
	Wave();
	~Wave();
	void GetVBarray(int v);
	void SetCol(float difR, float difG, float difB, float speR, float speG, float speB,
		float amR = 0.0f, float amG = 0.0f, float amB = 0.0f);
	bool Create(int texNo, bool blend, bool alpha, float waveHeight, float divide);
	bool Create(int texNo, int nortNo, bool blend, bool alpha, float waveHeight, float divide);
	//����:���㍶���E���E��
	void SetVertex(int i,
		float vx, float vy, float vz,
		float nx, float ny, float nz,
		float u, float v);
	void InstancedMap(float x, float y, float z, float theta, float sizeX = 1.0f, float sizeY = 0.0f, float sizeZ = 0.0f);
	void InstanceUpdate(float r, float g, float b, float a, float disp, float px = 1.0f, float py = 1.0f, float mx = 1.0f, float my = 1.0f);
	void Update(float x, float y, float z, float r, float g, float b, float a, float theta, float disp,
		float size = 1.0f, float px = 1.0f, float py = 1.0f, float mx = 1.0f, float my = 1.0f);
	void DrawOff();
	void Draw();
};

#endif
