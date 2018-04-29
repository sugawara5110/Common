//*****************************************************************************************//
//**                                                                                     **//
//**                   �@�@�@       ParticleData�N���X                                   **//
//**                                                                                     **//
//*****************************************************************************************//

#ifndef Class_ParticleData_Header
#define Class_ParticleData_Header

#include "Dx12ProcessCore.h"

class ParticleData :public Common {

protected:
	int                        com_no = 0;
	ID3DBlob                   *gsSO;
	ID3DBlob                   *vsSO;
	ID3DBlob                   *gs;
	ID3DBlob                   *vs;
	ID3DBlob                   *ps;
	int                        ver;//���_��

	PartPos                    *P_pos;//�p�[�e�B�N���f�[�^�z��

	Microsoft::WRL::ComPtr<ID3D12RootSignature> mRootSignature_com = nullptr;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> mRootSignature_draw = nullptr;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mSrvHeap = nullptr;

	//�R���X�^���g�o�b�t�@OBJ
	UploadBuffer<CONSTANT_BUFFER_P> *mObjectCB = nullptr;
	CONSTANT_BUFFER_P cbP[2];
	int sw = 0;
	//UpLoad�J�E���g
	int upCount = 0;
	//����Up�I��
	bool UpOn = FALSE;
	//DrawOn
	bool DrawOn = FALSE;
	bool texpar_on = FALSE;

	//���_�o�b�t�@OBJ
	std::unique_ptr<VertexView> Vview = nullptr;
	//�X�g���[���o�b�t�@OBJ
	std::unique_ptr<StreamView[]> Sview1 = nullptr;
	std::unique_ptr<StreamView[]> Sview2 = nullptr;//BufferFilledSizeLocation�̑����
	int svInd = 0;
	bool firstDraw = FALSE;
	int  streamInitcount = 0;
	bool parInit = FALSE;

	Microsoft::WRL::ComPtr<ID3D12PipelineState> mPSO_com = nullptr;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> mPSO_draw = nullptr;

	static std::mutex mtx;
	static void Lock() { mtx.lock(); }
	static void Unlock() { mtx.unlock(); }

	void GetShaderByteCode();
	void MatrixMap(CONSTANT_BUFFER_P *cb_p, float x, float y, float z, float theta, float size, float speed, bool tex);
	void MatrixMap2(CONSTANT_BUFFER_P *cb_p, bool init);
	void GetVbColarray(int texture_no, float size, float density);
	void CreateVbObj();
	void CreatePartsCom();
	void CreatePartsDraw(int texpar);
	void DrawParts0();
	void DrawParts1();
	void DrawParts2();
	void CbSwap(bool init);

public:
	ParticleData();
	~ParticleData();
	void SetCommandList(int no);
	void GetBufferParticle(int texture_no, float size, float density);//�e�N�X�`�������Ƀp�[�e�B�N���f�[�^����, �S�̃T�C�Y�{��, ���x
	void GetBufferBill(int v);
	void SetVertex(int i,
		float vx, float vy, float vz,
		float r, float g, float b, float a);
	void CreateParticle(int texpar);//�p�[�e�B�N��1�̃e�N�X�`���i���o�[
	void CreateBillboard();//ver�̎l�p�`�𐶐�
	void Update(float x, float y, float z, float theta, float size, bool init, float speed);//size�p�[�e�B�N��1�̃T�C�Y
	void DrawOff();
	void Draw();
	void Update(float size);
	void DrawBillboard();
};

#endif