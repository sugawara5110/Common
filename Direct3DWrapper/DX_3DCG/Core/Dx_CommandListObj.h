//*****************************************************************************************//
//**                                                                                     **//
//**                   Å@Å@Å@       Dx_CommandListObj                                    **//
//**                                                                                     **//
//*****************************************************************************************//

#ifndef Class_Dx_CommandListObj_Header
#define Class_Dx_CommandListObj_Header

#include "Dx_Device.h"
#include "DxEnum.h"
#include <iostream>

using Microsoft::WRL::ComPtr;

class Dx_CommandManager;

class Dx_CommandListObj final {

private:
	friend Dx_CommandManager;

	ComPtr<ID3D12CommandAllocator> mCmdListAlloc[2];
	ComPtr<ID3D12GraphicsCommandList4> mCommandList;
	int mAloc_Num = 0;
	enum ComListState {
		USED,
		OPEN,
		CLOSE
	};
	volatile ComListState mComState = {};

	bool ListCreate(bool Compute, ID3D12Device5* dev);

	static int NumResourceBarrier;

	struct CopyList {
		ID3D12Resource* dest;
		ID3D12Resource* src;
	};

	std::unique_ptr<D3D12_RESOURCE_BARRIER[]> beforeBa = nullptr;
	int beforeCnt = 0;
	std::unique_ptr<CopyList[]> copy = nullptr;
	int copyCnt = 0;
	std::unique_ptr <D3D12_RESOURCE_BARRIER[]> afterBa = nullptr;
	int afterCnt = 0;
	std::unique_ptr <D3D12_RESOURCE_BARRIER[]> uavBa = nullptr;
	int uavCnt = 0;

	void createResourceBarrierList();
	void createUavResourceBarrierList();

public:
	ID3D12GraphicsCommandList4* getCommandList();

	HRESULT CopyResourcesToGPU(ID3D12Resource* up, ID3D12Resource* def, const void* initData, LONG_PTR RowPitch);

	void Bigin();
	void End();

	void delayResourceBarrierBefore(ID3D12Resource* res, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after);
	void delayCopyResource(ID3D12Resource* dest, ID3D12Resource* src);
	void delayResourceBarrierAfter(ID3D12Resource* res, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after);
	void delayUavResourceBarrier(ID3D12Resource* res);

	void RunDelayResourceBarrierBefore();
	void RunDelayCopyResource();
	void RunDelayResourceBarrierAfter();
	void RunDelayUavResourceBarrier();

	void ResourceBarrier(ID3D12Resource* res, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after);
	void CopyResourceGENERIC_READ(ID3D12Resource* dest, ID3D12Resource* src);
};

#endif
