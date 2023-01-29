//*****************************************************************************************//
//**                                                                                     **//
//**                                Dx_Bloom                                             **//
//**                                                                                     **//
//*****************************************************************************************//

#ifndef Class_Dx_Bloom_Header
#define Class_Dx_Bloom_Header

#include "../Direct3DWrapper/DX_3DCG/Core/Dx12ProcessCore.h"

class Dx_Bloom :public Common {

private:
	struct CONSTANT_BUFFER_Bloom2 {
		int numInstance;
	};
	CONSTANT_BUFFER_Bloom2 cbb = {};
	ConstantBuffer<CONSTANT_BUFFER_Bloom2>* mObjectCB = nullptr;

	Dx_Resource mInputBuffer = {};
	Dx_Resource mOutputBuffer = {};

	ComPtr<ID3D12RootSignature> mRootSignatureCom = nullptr;
	ComPtr<ID3DBlob> pShader = {};
	ComPtr<ID3D12PipelineState> mPSO = {};
	ComPtr<ID3D12DescriptorHeap> mDescHeap = nullptr;

	bool createBuffer();
	bool createPipeline();

public:
	~Dx_Bloom();

	enum GaussianType {
		Type1D,
		Type2D
	};

	struct InstanceParam {
		uint32_t EmissiveInstanceId = 0;
		float thresholdLuminance = 0.0f;
		float bloomStrength = 1.0f;
	};

	bool Create(uint32_t numInstance, Dx_Resource* InstanceIdMapBuffer,
		std::vector<float>* sigma = nullptr,
		std::vector<std::vector<uint32_t>>* gausSizes = nullptr,
		std::vector<Dx_Bloom::GaussianType>* GaussianType = nullptr);

	void Compute(
		uint32_t comIndex,
		std::vector<InstanceParam> instanceParams,
		Dx_Resource* inout);
};

#endif
