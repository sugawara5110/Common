//*****************************************************************************************//
//**                                                                                     **//
//**                   Å@Å@Å@       DxCommandQueue                                       **//
//**                                                                                     **//
//*****************************************************************************************//

#ifndef Class_DxCommandQueue_Header
#define Class_DxCommandQueue_Header

#include "Dx_CommandListObj.h"

using Microsoft::WRL::ComPtr;

class DxCommandQueue final {

private:
	friend Dx12Process;

	ComPtr<ID3D12CommandQueue> mCommandQueue;
	ComPtr<ID3D12Fence> mFence;
	UINT64 mCurrentFence = 0;

	bool Create(ID3D12Device5* dev, bool Compute);
	void setCommandList(UINT numList, ID3D12CommandList* const* cmdsLists);
	void waitFence();
};

#endif
