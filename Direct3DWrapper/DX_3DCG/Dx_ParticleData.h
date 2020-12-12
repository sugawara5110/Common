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
	ID3DBlob* gsSO;
	ID3DBlob* vsSO;
	ID3DBlob* gs;
	ID3DBlob* vs;
	ID3DBlob* ps;
	int      ver;//���_��

	PartPos* P_pos;//�p�[�e�B�N���f�[�^�z��

	ComPtr<ID3D12RootSignature> mRootSignature_com = nullptr;
	ComPtr<ID3D12RootSignature> mRootSignature_draw = nullptr;
	ComPtr<ID3D12DescriptorHeap> mDescHeap = nullptr;

	//�R���X�^���g�o�b�t�@OBJ
	ConstantBuffer<CONSTANT_BUFFER_P>* mObjectCB = nullptr;
	CONSTANT_BUFFER_P cbP[2];
	bool firstCbSet[2];
	//DrawOn
	bool DrawOn = false;
	bool texpar_on = false;

	//���_�o�b�t�@OBJ
	std::unique_ptr<VertexView> Vview = nullptr;
	//�X�g���[���o�b�t�@OBJ
	std::unique_ptr<StreamView[]> Sview = nullptr;
	int svInd = 0;
	bool firstDraw = false;
	int  streamInitcount = 0;
	bool parInit = false;
	ComPtr<ID3D12RootSignature> rootSignatureDXR = nullptr;
	ComPtr<ID3D12PipelineState> PSO_DXR = nullptr;
	ParameterDXR dxrPara = {};

	ComPtr<ID3D12PipelineState> mPSO_com = nullptr;
	ComPtr<ID3D12PipelineState> mPSO_draw = nullptr;

	void GetShaderByteCode();
	void update(CONSTANT_BUFFER_P* cb_p, VECTOR3 pos, VECTOR4 color, float angle, float size, float speed);
	void update2(CONSTANT_BUFFER_P* cb_p, bool init);
	void GetVbColarray(int texture_no, float size, float density);
	void CreateVbObj();
	bool CreatePartsCom();
	bool CreatePartsDraw(int texpar);
	void DrawParts1(int com);
	void DrawParts2(int com);
	void DrawParts2StreamOutput(int com);
	void CbSwap(bool init);
	void createDxr();
	MATRIX BillboardAngleCalculation(float angle);

public:
	ParticleData();
	~ParticleData();
	void GetBufferParticle(int texture_no, float size, float density);//�e�N�X�`�������Ƀp�[�e�B�N���f�[�^����, �S�̃T�C�Y�{��, ���x
	void GetBufferBill(int v);
	void SetVertex(int i, VECTOR3 pos, VECTOR3 nor);
	bool CreateParticle(int texpar);//�p�[�e�B�N��1�̃e�N�X�`���i���o�[
	bool CreateBillboard();//ver�̎l�p�`�𐶐�
	void Update(VECTOR3 pos, VECTOR4 color, float angle, float size, bool init, float speed);//size�p�[�e�B�N��1�̃T�C�Y
	void DrawOff();
	void Draw(int com);
	void Draw();
	void Update(float size, VECTOR4 color);
	void DrawBillboard(int com);
	void DrawBillboard();
	void StreamOutput(int com);
	void StreamOutputBillboard(int com);
	void StreamOutput();
	void StreamOutputBillboard();
	ParameterDXR* getParameter();
	void setRefractiveIndex(float index) {
		dxrPara.updateDXR[dx->dxrBuffSwap[0]].RefractiveIndex = index;
	}
};

#endif