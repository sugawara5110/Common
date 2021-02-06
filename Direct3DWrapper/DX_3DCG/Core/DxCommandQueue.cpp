//*****************************************************************************************//
//**                                                                                     **//
//**                   　　　       DxCommandQueue                                       **//
//**                                                                                     **//
//*****************************************************************************************//

#include "DxCommandQueue.h"

bool DxCommandQueue::Create(ID3D12Device5* dev, bool Compute) {
	//フェンス生成
	//Command Queueに送信したCommand Listの完了を検知するために使用
	if (FAILED(dev->CreateFence(0, D3D12_FENCE_FLAG_NONE,
		IID_PPV_ARGS(&mFence)))) {
		ErrorMessage("CreateFence Error");
		return false;
	}
	//コマンドキュー生成
	D3D12_COMMAND_LIST_TYPE type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	if (Compute)type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = type;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE; //GPUタイムアウトが有効
	if (FAILED(dev->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mCommandQueue)))) {
		ErrorMessage("CreateCommandQueue Error");
		return false;
	}
	return true;
}

void DxCommandQueue::setCommandList(UINT numList, ID3D12CommandList* const* cmdsLists) {
	mCommandQueue->ExecuteCommandLists(numList, cmdsLists);
}

void DxCommandQueue::waitFence() {
	//インクリメントされたことで描画完了と判断
	mCurrentFence++;
	//GPU上で実行されているコマンド完了後,自動的にフェンスにmCurrentFenceを書き込む
	//(mFence->GetCompletedValue()で得られる値がmCurrentFenceと同じになる)
	mCommandQueue->Signal(mFence.Get(), mCurrentFence);//この命令以前のコマンドリストが待つ対象になる
	//ここまででコマンドキューが終了してしまうと
	//↓のイベントが正しく処理されない可能性有る為↓ifでチェックしている
	//GetCompletedValue():Fence内部UINT64のカウンタ取得(初期値0)
	if (mFence->GetCompletedValue() < mCurrentFence) {
		HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);
		//このFenceにおいて,mCurrentFence の値になったらイベントを発火させる
		mFence->SetEventOnCompletion(mCurrentFence, eventHandle);
		//イベントが発火するまで待つ(GPUの処理待ち)これによりGPU上の全コマンド実行終了まで待たせる
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}
}