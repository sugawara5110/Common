//*****************************************************************************************//
//**                                                                                     **//
//**                   Å@Å@Å@        DxSkinnedCom                                        **//
//**                                                                                     **//
//*****************************************************************************************//

#ifndef Class_DxSkinnedCom_Header
#define Class_DxSkinnedCom_Header

#include "Core/Dx12ProcessCore.h"

class SkinnedCom {

private:
	friend PolygonData;
	friend SkinMesh;
	ComPtr<ID3D12RootSignature> rootSignature = nullptr;
	ComPtr<ID3D12PipelineState> PSO = nullptr;
	ComPtr<ID3D12DescriptorHeap> descHeap = nullptr;
	std::unique_ptr<ComPtr<ID3D12Resource>[]> SkinnedVer = nullptr;
	PolygonData* pd = nullptr;
	int NumDesc = 0;
	const int numSrv = 1;
	const int numCbv = 1;
	const int numUav = 1;

	void getBuffer(PolygonData* pd, int numMaterial);
	bool createDescHeap(D3D12_GPU_VIRTUAL_ADDRESS ad3, UINT ad3Size);
	bool createPSO();
	bool createParameterDXR();
	void skinning(int comNo);
	void Skinning(int comNo);
};

#endif
