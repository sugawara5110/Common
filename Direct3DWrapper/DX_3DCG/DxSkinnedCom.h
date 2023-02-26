//*****************************************************************************************//
//**                                                                                     **//
//**                                 DxSkinnedCom                                        **//
//**                                                                                     **//
//*****************************************************************************************//

#ifndef Class_DxSkinnedCom_Header
#define Class_DxSkinnedCom_Header

#include "Core/Dx_BasicPolygon.h"
#define MAX_BONES 256

struct MY_VERTEX_S {
	CoordTf::VECTOR3 vPos = {};//���_
	CoordTf::VECTOR3 vNorm = {};//�@��
	CoordTf::VECTOR3 vTangent;  //�ڃx�N�g��
	CoordTf::VECTOR3 vGeoNorm = {};//�W�I���g���@��
	CoordTf::VECTOR2 vTex0 = {};//UV���W0
	CoordTf::VECTOR2 vTex1 = {};//UV���W1
	UINT bBoneIndex[4] = {};//�{�[���@�ԍ�
	float bBoneWeight[4] = {};//�{�[���@�d��
};

struct BONE {
	CoordTf::MATRIX mBindPose;//�����|�[�Y
	CoordTf::MATRIX mNewPose;//���݂̃|�[�Y

	BONE()
	{
		ZeroMemory(this, sizeof(BONE));
	}
};

struct SHADER_GLOBAL_BONES {
	CoordTf::MATRIX mBone[MAX_BONES];
	SHADER_GLOBAL_BONES()
	{
		for (int i = 0; i < MAX_BONES; i++)
		{
			MatrixIdentity(&mBone[i]);
		}
	}
};

class SkinnedCom {

private:
	friend SkinMesh;
	ComPtr<ID3D12RootSignature> rootSignature = nullptr;
	ComPtr<ID3D12PipelineState> PSO = nullptr;
	ComPtr<ID3D12DescriptorHeap> descHeap = nullptr;
	Dx_Resource SkinnedVer = {};
	BasicPolygon* pd = nullptr;
	const int numSrv = 1;
	const int numCbv = 1;
	const int numUav = 1;

	void getBuffer(BasicPolygon* pd);
	bool createDescHeap(D3D12_GPU_VIRTUAL_ADDRESS ad3, UINT ad3Size);
	bool createPSO();
	bool createParameterDXR(int comIndex);
	void skinning(int comIndex);
	void Skinning(int comIndex);
};

#endif
