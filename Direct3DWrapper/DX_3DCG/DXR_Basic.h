//*****************************************************************************************//
//**                                                                                     **//
//**                   �@�@�@          DXR_Basic.h                                       **//
//**                                                                                     **//
//*****************************************************************************************//

#ifndef Class_DXR_Basic_Header
#define Class_DXR_Basic_Header

#include "Dx12ProcessCore.h"
#include "../MicroSoftLibrary/DXCAPI/dxcapi.use.h"

struct AccelerationStructureBuffers
{
	ComPtr<ID3D12Resource> pScratch;
	ComPtr<ID3D12Resource> pResult;
	ComPtr<ID3D12Resource> pInstanceDesc;//top-level AS �����g��Ȃ�
	bool firstSet = false;
};

struct DxrConstantBuffer
{
	MATRIX projectionToWorld;
	VECTOR4 cameraUp;
	VECTOR4 cameraPosition;
	VECTOR4 emissivePosition[LIGHT_PCS];//xyz:Pos, w:�I���I�t
	VECTOR4 numEmissive;//.x�̂�
	VECTOR4 Lightst[LIGHT_PCS];//�����W, ����1, ����2, ����3
	VECTOR4 GlobalAmbientColor;
	VECTOR4 emissiveNo[LIGHT_PCS];//.x�̂�
	VECTOR4 dDirection;
	VECTOR4 dLightColor;
	VECTOR4 dLightst;//x:�I���I�t
	UINT maxRecursion;
};

struct DxrMaterialCB {
	VECTOR4 vDiffuse = { 0.8f,0.8f,0.8f,1.0f };//�f�B�t���[�Y�F
	VECTOR4 vSpeculer = { 0.6f,0.6f,0.6f,1.0f };//�X�؃L�����F
	VECTOR4 vAmbient = { 0.1f,0.1f,0.1f,0.0f };//�A���r�G���g
	VECTOR4 AddObjColor = {};//�I�u�W�F�N�g�̐F�ω��p
	float shininess = 4.0f;//�X�y�L��������
	float alphaTest = 0.0f;//1.0f:on, 0.0f:off 
	UINT materialNo = 0;//0:metallic, 1:emissive
};

enum MaterialType {
	METALLIC,
	NONREFLECTION,
	EMISSIVE,
	DIRECTIONLIGHT_METALLIC,
	DIRECTIONLIGHT_NONREFLECTION
};

struct ASobj {
	ComPtr<ID3D12Resource> mpTopLevelAS;
	std::unique_ptr<AccelerationStructureBuffers[]> bottomLevelBuffers;
	AccelerationStructureBuffers topLevelBuffers;
};

struct CBobj {
	DxrConstantBuffer cb = {};
	std::unique_ptr<DxrMaterialCB[]> matCb = nullptr;
};

class DXR_Basic {

private:
	ParameterDXR** PD;
	ASobj asObj[2] = {};
	CBobj cbObj[2] = {};
	int buffSwap[2] = { 0,0 };
	std::unique_ptr<ComPtr<ID3D12Resource>[]> VertexBufferGPU = nullptr;

	ConstantBuffer<DxrConstantBuffer>* sCB;
	ConstantBuffer<DxrMaterialCB>* material;

	ComPtr<ID3D12StateObject> mpPipelineState;
	ComPtr<ID3D12RootSignature> mpGlobalRootSig;
	ComPtr<ID3D12Resource> mpShaderTable;
	uint32_t mShaderTableEntrySize = 0;
	ComPtr<ID3D12Resource> mpOutputResource;
	ComPtr<ID3D12DescriptorHeap> mpSrvUavCbvHeap;
	uint32_t numSkipLocalHeap = 0;
	ComPtr<ID3D12DescriptorHeap> mpSamplerHeap;

	UINT numParameter = 0;//PD��
	UINT numMaterial = 0;//�S�}�e���A����
	UINT maxRecursion = 1;

	void createTriangleVB(UINT numMaterial);
	void createBottomLevelAS1(Dx12Process_sub* com, VertexView* vv,
		IndexView* iv, UINT currentIndexCount, UINT MaterialNo, bool update);
	void createBottomLevelAS(Dx12Process_sub* com);
	void createTopLevelAS(Dx12Process_sub* com);
	ComPtr<ID3D12RootSignature> createRootSignature(D3D12_ROOT_SIGNATURE_DESC& desc);
	void createAccelerationStructures();
	void createRtPipelineState();
	void createShaderResources();
	void createShaderTable();

	void updateMaterial(CBobj* cbObj);
	void updateCB(CBobj* cbObj, UINT numRecursion);
	void updateAS(Dx12Process_sub* com, UINT numRecursion);
	void setCB();
	void raytrace(Dx12Process_sub* com);

public:
	void initDXR(UINT numParameter, ParameterDXR** pd, MaterialType* type, UINT maxRecursion);
	void update_g(int comNo, UINT numRecursion);
	void update_c(int comNo, UINT numRecursion);
	void updateVertexBuffer(int comNo);
	void raytrace_g(int comNo);
	void raytrace_c(int comNo);
	void copyBackBuffer(int comNo);
	void setASswapIndex(int index) { buffSwap[0] = index; }
	void setRaytraceSwapIndex(int index) { buffSwap[1] = index; }
	~DXR_Basic() { S_DELETE(sCB); S_DELETE(material); }
};

#endif