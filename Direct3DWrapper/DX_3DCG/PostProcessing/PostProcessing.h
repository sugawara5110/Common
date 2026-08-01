//*****************************************************************************************//
//**                                                                                     **//
//**                   Å@Å@Å@         PostProcessing                                     **//
//**                                                                                     **//
//*****************************************************************************************//

#ifndef Class_PostProcessing_Header
#define Class_PostProcessing_Header

#include "../Core/Dx_Common.h"

class PostProcessing :public DxCommon {

private:
	ComPtr<ID3D12RootSignature> mRootSignature = nullptr;
	ComPtr<ID3D12DescriptorHeap> mDescHeap = nullptr;
	ComPtr<ID3D12PipelineState> mPSO = nullptr;

	struct ToneMapCB {
		float Exposure;
	};
	ToneMapCB cb = {};
	ConstantBuffer<ToneMapCB>* mObjectCB = nullptr;

	ComPtr <ID3D12PipelineState> CreatePsoVsPs(
		ID3DBlob* vs, ID3DBlob* ps,
		ID3D12RootSignature* mRootSignature,
		bool alpha, bool blend);

public:
	~PostProcessing();

	bool Initialize(std::vector<Dx_Resource*>& po);

	void setExposure(float exposure);

	void Execute(uint32_t comIndex, uint32_t resourceIndex);
};

#endif

