//*****************************************************************************************//
//**                                                                                     **//
//**                                 DxSkinnedCom                                        **//
//**                                                                                     **//
//*****************************************************************************************//

#ifndef Class_DxSkinnedCom_Header
#define Class_DxSkinnedCom_Header

#include "Core/Dx12ProcessCore.h"

class SkinnedCom {

private:
	friend SkinMesh;
	ComPtr<ID3D12RootSignature> rootSignature = nullptr;
	ComPtr<ID3D12PipelineState> PSO = nullptr;
	ComPtr<ID3D12DescriptorHeap> descHeap = nullptr;
	ComPtr<ID3D12Resource> SkinnedVer = nullptr;
	BasicPolygon* pd = nullptr;
	const int numSrv = 1;
	const int numCbv = 1;
	const int numUav = 1;

	void getBuffer(BasicPolygon* pd);
	bool createDescHeap(D3D12_GPU_VIRTUAL_ADDRESS ad3, UINT ad3Size);
	bool createPSO();
	bool createParameterDXR(int comNo);
	void skinning(int comNo);
	void Skinning(int comNo);
};

#endif
