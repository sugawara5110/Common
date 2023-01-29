//*****************************************************************************************//
//**                                                                                     **//
//**                   　　　       Dx_CommandListObj                                    **//
//**                                                                                     **//
//*****************************************************************************************//

#include "Dx_CommandListObj.h"

bool Dx_CommandListObj::ListCreate(bool Compute, ID3D12Device5* dev) {

	D3D12_COMMAND_LIST_TYPE type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	if (Compute)type = D3D12_COMMAND_LIST_TYPE_COMPUTE;

	for (int i = 0; i < 2; i++) {
		//コマンドアロケータ生成(コマンドリストに積むバッファを確保するObj)
		if (FAILED(dev->CreateCommandAllocator(
			type,
			IID_PPV_ARGS(mCmdListAlloc[i].GetAddressOf())))) {
			Dx_Util::ErrorMessage("CreateCommandAllocator Error");
			return false;
		}
	}

	//コマンドリスト生成
	if (FAILED(dev->CreateCommandList(
		0,
		type,
		mCmdListAlloc[0].Get(),
		nullptr,
		IID_PPV_ARGS(mCommandList.GetAddressOf())))) {
		Dx_Util::ErrorMessage("CreateCommandList Error");
		return false;
	}

	//最初は閉じた方が良い
	mCommandList->Close();
	mCommandList->Reset(mCmdListAlloc[0].Get(), nullptr);
	mCommandList->Close();
	mComState = USED;

	return true;
}

static void getBarrierDesc(
	D3D12_RESOURCE_BARRIER& desc,
	ID3D12Resource* res,
	D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after) {

	desc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	desc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	desc.Transition.pResource = res;
	desc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	desc.Transition.StateBefore = before;
	desc.Transition.StateAfter = after;
}

void Dx_CommandListObj::ResourceBarrier(ID3D12Resource* res, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after) {
	D3D12_RESOURCE_BARRIER desc = {};
	getBarrierDesc(desc, res, before, after);
	mCommandList->ResourceBarrier(1, &desc);
}

void Dx_CommandListObj::CopyResourceGENERIC_READ(ID3D12Resource* dest, ID3D12Resource* src) {
	ResourceBarrier(dest,
		D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COPY_DEST);
	ResourceBarrier(src,
		D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COPY_SOURCE);

	mCommandList->CopyResource(dest, src);

	ResourceBarrier(dest,
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);
	ResourceBarrier(src,
		D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_GENERIC_READ);
}

void Dx_CommandListObj::Bigin() {
	mComState = OPEN;
	mAloc_Num = 1 - mAloc_Num;
	mCmdListAlloc[mAloc_Num]->Reset();
	mCommandList->Reset(mCmdListAlloc[mAloc_Num].Get(), nullptr);
}

void Dx_CommandListObj::End() {
	RunDelayResourceBarrierBefore();
	RunDelayCopyResource();
	RunDelayResourceBarrierAfter();
	//コマンドクローズ
	mCommandList->Close();
	mComState = CLOSE;
}

int Dx_CommandListObj::NumResourceBarrier = 1024;

void Dx_CommandListObj::createResourceBarrierList() {
	beforeBa = std::make_unique<D3D12_RESOURCE_BARRIER[]>(NumResourceBarrier);
	copy = std::make_unique<CopyList[]>(NumResourceBarrier);
	afterBa = std::make_unique<D3D12_RESOURCE_BARRIER[]>(NumResourceBarrier);
}

void Dx_CommandListObj::createUavResourceBarrierList() {
	uavBa = std::make_unique<D3D12_RESOURCE_BARRIER[]>(NumResourceBarrier);
}

void Dx_CommandListObj::delayResourceBarrierBefore(ID3D12Resource* res, D3D12_RESOURCE_STATES before,
	D3D12_RESOURCE_STATES after) {

	if (beforeCnt >= NumResourceBarrier) {
		Dx_Util::ErrorMessage("ResourceBarrier個数上限超えてます!");
	}
	getBarrierDesc(beforeBa[beforeCnt], res, before, after);
	beforeCnt++;
}

void Dx_CommandListObj::delayCopyResource(ID3D12Resource* dest, ID3D12Resource* src) {

	if (copyCnt >= NumResourceBarrier) {
		Dx_Util::ErrorMessage("ResourceBarrier個数上限超えてます!");
	}
	copy[copyCnt].dest = dest;
	copy[copyCnt].src = src;
	copyCnt++;
}

void Dx_CommandListObj::delayResourceBarrierAfter(ID3D12Resource* res, D3D12_RESOURCE_STATES before,
	D3D12_RESOURCE_STATES after) {

	if (afterCnt >= NumResourceBarrier) {
		Dx_Util::ErrorMessage("ResourceBarrier個数上限超えてます!");
	}
	getBarrierDesc(afterBa[afterCnt], res, before, after);
	afterCnt++;
}

void Dx_CommandListObj::delayUavResourceBarrier(ID3D12Resource* res) {

	if (uavCnt >= NumResourceBarrier) {
		Dx_Util::ErrorMessage("ResourceBarrier個数上限超えてます!");
	}
	D3D12_RESOURCE_BARRIER& ba = uavBa[uavCnt];
	//バリアしたリソースへのUAVアクセスに於いて次のUAVアクセス開始前に現在のUAVアクセスが終了する必要がある事を示す
	ba.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	ba.UAV.pResource = res;
	uavCnt++;
}

void Dx_CommandListObj::RunDelayResourceBarrierBefore() {
	if (beforeCnt <= 0)return;
	mCommandList->ResourceBarrier(beforeCnt, beforeBa.get());
	beforeCnt = 0;
}

void Dx_CommandListObj::RunDelayCopyResource() {
	for (int i = 0; i < copyCnt; i++)
		mCommandList->CopyResource(copy[i].dest, copy[i].src);

	copyCnt = 0;
}

void Dx_CommandListObj::RunDelayResourceBarrierAfter() {
	if (afterCnt <= 0)return;
	mCommandList->ResourceBarrier(afterCnt, afterBa.get());
	afterCnt = 0;
}

void Dx_CommandListObj::RunDelayUavResourceBarrier() {
	if (uavCnt <= 0)return;
	mCommandList->ResourceBarrier(uavCnt, uavBa.get());
	uavCnt = 0;
}

ID3D12GraphicsCommandList4* Dx_CommandListObj::getCommandList() {
	return mCommandList.Get();
}