//*****************************************************************************************//
//**                                                                                     **//
//**                   　　　          Dx12ProcessCoreクラス                             **//
//**                                   Initialize関数                                    **//
//*****************************************************************************************//

#define _CRT_SECURE_NO_WARNINGS
#include "Dx12ProcessCore.h"
#include <WindowsX.h>
#include "./ShaderCG/ShaderCommonParameters.h"
#include "./ShaderCG/ShaderNormalTangent.h"
#include "./ShaderCG/Shader2D.h"
#include "./ShaderCG/Shader3D.h"
#include "./ShaderCG/ShaderMesh.h"
#include "./ShaderCG/ShaderMesh_D.h"
#include "./ShaderCG/ShaderParticle.h"
#include "./ShaderCG/ShaderSkinMesh.h"
#include "./ShaderCG/ShaderSkinMesh_D.h"
#include "./ShaderCG/ShaderWaveCom.h"
#include "./ShaderCG/ShaderWaveDraw.h"
#include "./ShaderCG/ShaderCommonPS.h"
#include "./ShaderCG/ShaderCommonTriangleGS.h"
#include "./ShaderCG/ShaderCommonTriangleHSDS.h"
#include "./ShaderCG/ShaderPostEffect.h"
#include "./ShaderCG/ShaderCalculateLighting.h"
#include "./ShaderCG/ShaderGsOutput.h"
#include "./ShaderCG/ShaderSkinMeshCom.h"
#include <locale.h>

ComPtr<ID3D12Resource> StreamView::UpresetBuffer = nullptr;
ComPtr<ID3D12Resource> StreamView::resetBuffer = nullptr;

bool Dx12Process_sub::ListCreate(bool Compute) {

	D3D12_COMMAND_LIST_TYPE type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	if (Compute)type = D3D12_COMMAND_LIST_TYPE_COMPUTE;

	for (int i = 0; i < 2; i++) {
		//コマンドアロケータ生成(コマンドリストに積むバッファを確保するObj)
		if (FAILED(Dx12Process::dx->md3dDevice->CreateCommandAllocator(
			type,
			IID_PPV_ARGS(mCmdListAlloc[i].GetAddressOf())))) {
			ErrorMessage("CreateCommandAllocator Error");
			return false;
		}
	}

	//コマンドリスト生成
	if (FAILED(Dx12Process::dx->md3dDevice->CreateCommandList(
		0,
		type,
		mCmdListAlloc[0].Get(),
		nullptr,
		IID_PPV_ARGS(mCommandList.GetAddressOf())))) {
		ErrorMessage("CreateCommandList Error");
		return false;
	}

	//最初は閉じた方が良い
	mCommandList->Close();
	mCommandList->Reset(mCmdListAlloc[0].Get(), nullptr);
	mCommandList->Close();
	mComState = USED;

	return true;
}

void Dx12Process_sub::ResourceBarrier(ID3D12Resource* res, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after) {
	D3D12_RESOURCE_BARRIER BarrierDesc;
	BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	BarrierDesc.Transition.pResource = res;
	BarrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	BarrierDesc.Transition.StateBefore = before;
	BarrierDesc.Transition.StateAfter = after;
	mCommandList->ResourceBarrier(1, &BarrierDesc);
}

void Dx12Process_sub::CopyResourceGENERIC_READ(ID3D12Resource* dest, ID3D12Resource* src) {
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

void Dx12Process_sub::Bigin() {
	mComState = OPEN;
	mAloc_Num = 1 - mAloc_Num;
	mCmdListAlloc[mAloc_Num]->Reset();
	mCommandList->Reset(mCmdListAlloc[mAloc_Num].Get(), nullptr);
}

void Dx12Process_sub::End() {
	RunDelayResourceBarrierBefore();
	RunDelayCopyResource();
	RunDelayResourceBarrierAfter();
	//コマンドクローズ
	mCommandList->Close();
	mComState = CLOSE;
}

int Dx12Process_sub::NumResourceBarrier = 512;

void Dx12Process_sub::createResourceBarrierList() {
	beforeBa = std::make_unique<D3D12_RESOURCE_BARRIER[]>(NumResourceBarrier);
	copy = std::make_unique<CopyList[]>(NumResourceBarrier);
	afterBa = std::make_unique<D3D12_RESOURCE_BARRIER[]>(NumResourceBarrier);
}

void Dx12Process_sub::createUavResourceBarrierList() {
	uavBa = std::make_unique<D3D12_RESOURCE_BARRIER[]>(NumResourceBarrier);
}

void Dx12Process_sub::delayResourceBarrierBefore(ID3D12Resource* res, D3D12_RESOURCE_STATES before,
	D3D12_RESOURCE_STATES after) {

	if (beforeCnt >= NumResourceBarrier) {
		ErrorMessage("ResourceBarrier個数上限超えてます!");
	}
	D3D12_RESOURCE_BARRIER& ba = beforeBa[beforeCnt];
	ba.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	ba.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	ba.Transition.pResource = res;
	ba.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	ba.Transition.StateBefore = before;
	ba.Transition.StateAfter = after;
	beforeCnt++;
}

void Dx12Process_sub::delayCopyResource(ID3D12Resource* dest, ID3D12Resource* src) {

	if (copyCnt >= NumResourceBarrier) {
		ErrorMessage("ResourceBarrier個数上限超えてます!");
	}
	copy[copyCnt].dest = dest;
	copy[copyCnt].src = src;
	copyCnt++;
}

void Dx12Process_sub::delayResourceBarrierAfter(ID3D12Resource* res, D3D12_RESOURCE_STATES before,
	D3D12_RESOURCE_STATES after) {

	if (afterCnt >= NumResourceBarrier) {
		ErrorMessage("ResourceBarrier個数上限超えてます!");
	}
	D3D12_RESOURCE_BARRIER& ba = afterBa[afterCnt];
	ba.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	ba.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	ba.Transition.pResource = res;
	ba.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	ba.Transition.StateBefore = before;
	ba.Transition.StateAfter = after;
	afterCnt++;
}

void Dx12Process_sub::delayUavResourceBarrier(ID3D12Resource* res) {

	if (uavCnt >= NumResourceBarrier) {
		ErrorMessage("ResourceBarrier個数上限超えてます!");
	}
	D3D12_RESOURCE_BARRIER& ba = uavBa[uavCnt];
	//バリアしたリソースへのUAVアクセスに於いて次のUAVアクセス開始前に現在のUAVアクセスが終了する必要がある事を示す
	ba.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	ba.UAV.pResource = res;
	uavCnt++;
}

void Dx12Process_sub::RunDelayResourceBarrierBefore() {
	if (beforeCnt <= 0)return;
	mCommandList->ResourceBarrier(beforeCnt, beforeBa.get());
	beforeCnt = 0;
}

void Dx12Process_sub::RunDelayCopyResource() {
	for (int i = 0; i < copyCnt; i++)
		mCommandList->CopyResource(copy[i].dest, copy[i].src);

	copyCnt = 0;
}

void Dx12Process_sub::RunDelayResourceBarrierAfter() {
	if (afterCnt <= 0)return;
	mCommandList->ResourceBarrier(afterCnt, afterBa.get());
	afterCnt = 0;
}

void Dx12Process_sub::RunDelayUavResourceBarrier() {
	if (uavCnt <= 0)return;
	mCommandList->ResourceBarrier(uavCnt, uavBa.get());
	uavCnt = 0;
}

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

Dx12Process *Dx12Process::dx = nullptr;
std::mutex Dx12Process::mtx;
std::mutex Dx12Process::mtxGraphicsQueue;
std::mutex Dx12Process::mtxComputeQueue;

void Dx12Process::InstanceCreate() {

	if (dx == nullptr)dx = new Dx12Process();
}

Dx12Process *Dx12Process::GetInstance() {

	if (dx != nullptr)return dx;
	return nullptr;
}

void Dx12Process::DeleteInstance() {

	if (dx != nullptr) {
		delete dx;
		dx = nullptr;
	}
}

Dx12Process::~Dx12Process() {
	ARR_DELETE(texture);
}

void Dx12Process::WaitFenceNotLock() {
	//クローズ後リストに加える
	static ID3D12CommandList* cmdsLists[COM_NO] = {};
	UINT cnt = 0;
	for (int i = 0; i < COM_NO; i++) {
		if (dx_sub[i].mComState != CLOSE)continue;
		cmdsLists[cnt++] = dx_sub[i].mCommandList.Get();
		dx_sub[i].mComState = USED;
	}

	graphicsQueue.setCommandList(cnt, cmdsLists);
	graphicsQueue.waitFence();
}

void Dx12Process::WaitFence() {
	mtxGraphicsQueue.lock();
	WaitFenceNotLock();
	mtxGraphicsQueue.unlock();
}

void Dx12Process::BiginCom(int com_no) {
	dx_subCom[com_no].Bigin();
}

void Dx12Process::EndCom(int com_no) {
	dx_subCom[com_no].End();
}

void Dx12Process::WaitFenceNotLockCom() {
	static ID3D12CommandList* cmdsLists[COM_NO] = {};
	UINT cnt = 0;
	for (int i = 0; i < COM_NO; i++) {
		if (dx_subCom[i].mComState != CLOSE)continue;
		cmdsLists[cnt++] = dx_subCom[i].mCommandList.Get();
		dx_subCom[i].mComState = USED;
	}

	computeQueue.setCommandList(cnt, cmdsLists);
	computeQueue.waitFence();
}

void Dx12Process::WaitFenceCom() {
	mtxComputeQueue.lock();
	WaitFenceNotLockCom();
	mtxComputeQueue.unlock();
}

bool Dx12Process::CreateShaderByteCode() {

	size_t norS_size = strlen(ShaderNormalTangent) + 1;
	size_t norL_size = strlen(ShaderCalculateLighting) + 1;
	ShaderNormalTangentCopy = std::make_unique<char[]>(norS_size);
	ShaderCalculateLightingCopy = std::make_unique<char[]>(norL_size);
	memcpy(ShaderNormalTangentCopy.get(), ShaderNormalTangent, norS_size);
	memcpy(ShaderCalculateLightingCopy.get(), ShaderCalculateLighting, norL_size);

	//各Shader結合
	addChar D3, Mesh, MeshD, Skin, SkinD, Wave, ComPS, ComHSDS, ComGS, ParaNor, Lighting, GsOut;
	char* com = ShaderCommonParameters;
	ParaNor.addStr(com, ShaderNormalTangent);
	Lighting.addStr(ParaNor.str, ShaderCalculateLighting);
	D3.addStr(com, Shader3D);
	Mesh.addStr(com, ShaderMesh);
	MeshD.addStr(com, ShaderMesh_D);
	Skin.addStr(com, ShaderSkinMesh);
	SkinD.addStr(com, ShaderSkinMesh_D);
	Wave.addStr(com, ShaderWaveDraw);
	ComPS.addStr(Lighting.str, ShaderCommonPS);
	ComHSDS.addStr(com, ShaderCommonTriangleHSDS);
	ComGS.addStr(ParaNor.str, ShaderCommonTriangleGS);
	GsOut.addStr(com, ShaderGsOutput);

	//CommonPS
	pPixelShader_3D = CompileShader(ComPS.str, ComPS.size, "PS_L", "ps_5_0");
	pPixelShader_3D_NoNormalMap = CompileShader(ComPS.str, ComPS.size, "PS_L_NoNormalMap", "ps_5_0");
	pPixelShader_Emissive = CompileShader(ComPS.str, ComPS.size, "PS", "ps_5_0");
	//CommonHSDS(Triangle)
	pHullShaderTriangle = CompileShader(ComHSDS.str, ComHSDS.size, "HS", "hs_5_0");
	pDomainShaderTriangle = CompileShader(ComHSDS.str, ComHSDS.size, "DS", "ds_5_0");
	//CommonGS
	pGeometryShader_Before_ds = CompileShader(ComGS.str, ComGS.size, "GS_Before_ds", "gs_5_0");
	pGeometryShader_Before_vs = CompileShader(ComGS.str, ComGS.size, "GS_Before_vs", "gs_5_0");
	pGeometryShader_Before_ds_NoNormalMap = CompileShader(ComGS.str, ComGS.size, "GS_Before_ds_NoNormalMap", "gs_5_0");
	pGeometryShader_Before_vs_NoNormalMap = CompileShader(ComGS.str, ComGS.size, "GS_Before_vs_NoNormalMap", "gs_5_0");

	//ポストエフェクト
	pComputeShader_Post[0] = CompileShader(ShaderPostEffect, strlen(ShaderPostEffect), "MosaicCS", "cs_5_0");
	pComputeShader_Post[1] = CompileShader(ShaderPostEffect, strlen(ShaderPostEffect), "BlurCS", "cs_5_0");

	//スキンメッシュ
	pVertexLayout_SKIN =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "GEO_NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 36, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 1, DXGI_FORMAT_R32G32_FLOAT, 0, 44, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "BONE_INDEX", 0, DXGI_FORMAT_R32G32B32A32_UINT, 0, 52, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "BONE_WEIGHT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 68, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};
	pVertexShader_SKIN = CompileShader(Skin.str, Skin.size, "VSSkin", "vs_5_0");
	//テセレーター有
	pVertexShader_SKIN_D = CompileShader(SkinD.str, SkinD.size, "VS", "vs_5_0");

	//ストリーム出力データ定義(パーティクル用)
	pDeclaration_PSO =
	{
		{ 0, "POSITION", 0, 0, 3, 0 }, //「x,y,z」をスロット「0」の「POSITION」に出力
		{ 0, "POSITION", 1, 0, 3, 0 },
		{ 0, "POSITION", 2, 0, 3, 0 },
		{ 0, "NORMAL", 0, 0, 3, 0 }
	};
	//ストリーム出力
	pVertexShader_PSO = CompileShader(ShaderParticle, strlen(ShaderParticle), "VS_SO", "vs_5_0");
	pGeometryShader_PSO = CompileShader(ShaderParticle, strlen(ShaderParticle), "GS_Point_SO", "gs_5_0");

	//パーティクル頂点インプットレイアウトを定義
	pVertexLayout_P =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "POSITION", 1, DXGI_FORMAT_R32G32B32_FLOAT, 0, 4 * 3, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "POSITION", 2, DXGI_FORMAT_R32G32B32_FLOAT, 0, 4 * 3 * 2, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 4 * 3 * 3, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};
	//パーティクル
	pVertexShader_P = CompileShader(ShaderParticle, strlen(ShaderParticle), "VS", "vs_5_0");
	pGeometryShader_P = CompileShader(ShaderParticle, strlen(ShaderParticle), "GS_Point", "gs_5_0");
	pPixelShader_P = CompileShader(ShaderParticle, strlen(ShaderParticle), "PS", "ps_5_0");

	//メッシュレイアウト
	pVertexLayout_MESH =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "GEO_NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 36, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};
	//メッシュ
	pVertexShader_MESH = CompileShader(Mesh.str, Mesh.size, "VSMesh", "vs_5_0");
	//テセレーター有メッシュ
	pVertexShader_MESH_D = CompileShader(MeshD.str, MeshD.size, "VSMesh", "vs_5_0");

	//3Dレイアウト基本色
	pVertexLayout_3DBC =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 4 * 3, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};
	//テクスチャ3D
	pVertexShader_TC = CompileShader(D3.str, D3.size, "VSTextureColor", "vs_5_0");
	//基本色3D
	pVertexShader_BC = CompileShader(D3.str, D3.size, "VSBaseColor", "vs_5_0");
	pPixelShader_BC = CompileShader(D3.str, D3.size, "PSBaseColor", "ps_5_0");
	//Wave
	pComputeShader_Wave = CompileShader(ShaderWaveCom, strlen(ShaderWaveCom), "CS", "cs_5_0");
	pDomainShader_Wave = CompileShader(Wave.str, Wave.size, "DSWave", "ds_5_0");

	//2Dレイアウト
	pVertexLayout_2D =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 4 * 3, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 4 * 3 + 4 * 4, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};
	//テクスチャ2D
	pVertexShader_2DTC = CompileShader(Shader2D, strlen(Shader2D), "VSTextureColor", "vs_5_0");
	pPixelShader_2DTC = CompileShader(Shader2D, strlen(Shader2D), "PSTextureColor", "ps_5_0");
	//2D
	pVertexShader_2D = CompileShader(Shader2D, strlen(Shader2D), "VSBaseColor", "vs_5_0");
	pPixelShader_2D = CompileShader(Shader2D, strlen(Shader2D), "PSBaseColor", "ps_5_0");

	//DXRへのOutput
	pGeometryShader_Before_vs_Output = CompileShader(GsOut.str, GsOut.size, "GS_Before_vs", "gs_5_0");
	pGeometryShader_Before_ds_Output = CompileShader(GsOut.str, GsOut.size, "GS_Before_ds", "gs_5_0");
	pDeclaration_Output =
	{
		{ 0, "POSITION", 0, 0, 3, 0 },
		{ 0, "NORMAL",   0, 0, 3, 0 },
		{ 0, "TEXCOORD", 0, 0, 2, 0 },
		{ 0, "TEXCOORD", 1, 0, 2, 0 }
	};

	pGeometryShader_P_Output = CompileShader(ShaderParticle, strlen(ShaderParticle), "GS_PointDxr", "gs_5_0");

	pVertexShader_SKIN_Com = CompileShader(ShaderSkinMeshCom, strlen(ShaderSkinMeshCom), "VSSkinCS", "cs_5_0");

	return CreateShaderByteCodeBool;
}

int Dx12Process::GetTexNumber(CHAR* fileName) {

	fileName = GetNameFromPass(fileName);

	for (int i = 0; i < texNum; i++) {
		if (texture[i].texName == '\0')continue;
		char str[50];
		char str1[50];
		strcpy(str, texture[i].texName);
		strcpy(str1, fileName);
		int i1 = -1;
		while (str[++i1] != '\0' && str[i1] != '.' && str[i1] == str1[i1]);
		if (str[i1] == '.' && str1[i1] == '.')return i;
	}

	return -1;
}

static HRESULT createDefaultResourceCommon(ID3D12Device* dev, ID3D12Resource** def,
	D3D12_RESOURCE_DIMENSION dimension, UINT64 width, UINT height,
	DXGI_FORMAT format, D3D12_RESOURCE_FLAGS flags,
	D3D12_RESOURCE_STATES firstState) {

	D3D12_RESOURCE_DESC desc = {};
	desc.Dimension = dimension;
	desc.Width = width;
	desc.Height = height;
	desc.DepthOrArraySize = 1;
	desc.MipLevels = 1;
	desc.Format = format;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	if (dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
		desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	desc.Flags = flags;

	D3D12_HEAP_PROPERTIES HeapProps = {};
	HeapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
	HeapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	HeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	HeapProps.CreationNodeMask = 1;
	HeapProps.VisibleNodeMask = 1;

	//デフォルトバッファ生成
	return dev->CreateCommittedResource(
		&HeapProps,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		firstState,
		nullptr,
		IID_PPV_ARGS(def));
}

HRESULT Dx12Process::createDefaultResourceTEXTURE2D(ID3D12Resource** def, UINT64 width, UINT height,
	DXGI_FORMAT format, D3D12_RESOURCE_STATES firstState) {

	return createDefaultResourceCommon(md3dDevice.Get(), def,
		D3D12_RESOURCE_DIMENSION_TEXTURE2D, width, height,
		format, D3D12_RESOURCE_FLAG_NONE, firstState);
}

HRESULT Dx12Process::createDefaultResourceTEXTURE2D_UNORDERED_ACCESS(ID3D12Resource** def, UINT64 width, UINT height,
	D3D12_RESOURCE_STATES firstState,
	DXGI_FORMAT format) {

	return createDefaultResourceCommon(md3dDevice.Get(), def,
		D3D12_RESOURCE_DIMENSION_TEXTURE2D, width, height,
		format, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
		firstState);
}

HRESULT Dx12Process::createDefaultResourceBuffer(ID3D12Resource** def, UINT64 bufferSize,
	D3D12_RESOURCE_STATES firstState) {

	return createDefaultResourceCommon(md3dDevice.Get(), def,
		D3D12_RESOURCE_DIMENSION_BUFFER, bufferSize, 1,
		DXGI_FORMAT_UNKNOWN, D3D12_RESOURCE_FLAG_NONE,
		firstState);
}

HRESULT Dx12Process::createDefaultResourceBuffer_UNORDERED_ACCESS(ID3D12Resource** def, UINT64 bufferSize,
	D3D12_RESOURCE_STATES firstState) {

	return createDefaultResourceCommon(md3dDevice.Get(), def,
		D3D12_RESOURCE_DIMENSION_BUFFER, bufferSize, 1,
		DXGI_FORMAT_UNKNOWN, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
		firstState);
}

HRESULT Dx12Process::createUploadResource(ID3D12Resource** up, UINT64 uploadBufferSize) {
	D3D12_HEAP_PROPERTIES HeapPropsUp = {};
	HeapPropsUp.Type = D3D12_HEAP_TYPE_UPLOAD;
	HeapPropsUp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	HeapPropsUp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	HeapPropsUp.CreationNodeMask = 1;
	HeapPropsUp.VisibleNodeMask = 1;

	D3D12_RESOURCE_DESC BufferDesc = {};
	BufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	BufferDesc.Alignment = 0;
	BufferDesc.Width = uploadBufferSize;
	BufferDesc.Height = 1;
	BufferDesc.DepthOrArraySize = 1;
	BufferDesc.MipLevels = 1;
	BufferDesc.Format = DXGI_FORMAT_UNKNOWN;
	BufferDesc.SampleDesc.Count = 1;
	BufferDesc.SampleDesc.Quality = 0;
	BufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	BufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	return md3dDevice->CreateCommittedResource(&HeapPropsUp, D3D12_HEAP_FLAG_NONE,
		&BufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(up));
}

HRESULT Dx12Process::createReadBackResource(ID3D12Resource** ba, UINT64 BufferSize) {
	D3D12_HEAP_PROPERTIES heap = {};
	heap.Type = D3D12_HEAP_TYPE_READBACK;
	heap.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heap.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heap.CreationNodeMask = 1;
	heap.VisibleNodeMask = 1;

	D3D12_RESOURCE_DESC desc = {};
	desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	desc.Alignment = 0;
	desc.Width = BufferSize;
	desc.Height = 1;
	desc.DepthOrArraySize = 1;
	desc.MipLevels = 1;
	desc.Format = DXGI_FORMAT_UNKNOWN;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	desc.Flags = D3D12_RESOURCE_FLAG_NONE;

	return md3dDevice->CreateCommittedResource(
		&heap,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(ba));
}

HRESULT Dx12Process::textureInit(int width, int height,
	ID3D12Resource** up, ID3D12Resource** def, DXGI_FORMAT format,
	D3D12_RESOURCE_STATES firstState) {

	HRESULT hr = createDefaultResourceTEXTURE2D(def, width, height, format, firstState);
	if (FAILED(hr)) {
		return hr;
	}

	//upload
	UINT64 uploadBufferSize = getRequiredIntermediateSize(*def);
	hr = createUploadResource(up, uploadBufferSize);
	if (FAILED(hr)) {
		return hr;
	}
	return S_OK;
}

ComPtr<ID3D12RootSignature> Dx12Process::CreateRsCommon(D3D12_ROOT_SIGNATURE_DESC* rootSigDesc)
{
	ComPtr<ID3D12RootSignature>rs;

	ComPtr<ID3DBlob> serializedRootSig = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;

	HRESULT hr = D3D12SerializeRootSignature(rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
		serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

	if (FAILED(hr)) {
		ErrorMessage("Dx12Process::CreateRsCommon Error!!");
		ErrorMessage((char*)errorBlob.Get()->GetBufferPointer());
		return nullptr;
	}

	//RootSignature生成
	hr = md3dDevice.Get()->CreateRootSignature(
		0,
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(rs.GetAddressOf()));

	if (FAILED(hr)) {
		ErrorMessage("Dx12Process::CreateRsCommon Error!!"); return nullptr;
	}

	return rs;
}

ComPtr <ID3D12DescriptorHeap> Dx12Process::CreateDescHeap(int numDesc) {
	ComPtr <ID3D12DescriptorHeap>heap;

	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.NumDescriptors = numDesc;
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	HRESULT hr;
	hr = md3dDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&heap));
	if (FAILED(hr)) {
		ErrorMessage("Dx12Process::CreateDescHeap Error!!"); return nullptr;
	}
	return heap;
}

ComPtr<ID3D12DescriptorHeap> Dx12Process::CreateSamplerDescHeap(D3D12_SAMPLER_DESC& descSampler) {
	ComPtr <ID3D12DescriptorHeap>heap;

	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.NumDescriptors = 1;
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	HRESULT hr;
	hr = md3dDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&heap));
	if (FAILED(hr)) {
		ErrorMessage("Dx12Process::CreateSamplerDescHeap Error!!"); return nullptr;
	}

	D3D12_CPU_DESCRIPTOR_HANDLE handle = heap.Get()->GetCPUDescriptorHandleForHeapStart();
	md3dDevice->CreateSampler(&descSampler, handle);
	return heap;
}

HRESULT Dx12Process::createTexture(int com_no, UCHAR* byteArr, DXGI_FORMAT format,
	ID3D12Resource** up, ID3D12Resource** def,
	int width, LONG_PTR RowPitch, int height) {

	HRESULT hr = textureInit(width, height, up, def, format,
		D3D12_RESOURCE_STATE_COMMON);
	if (FAILED(hr)) {
		return hr;
	}

	dx_sub[com_no].ResourceBarrier(*def, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
	hr = CopyResourcesToGPU(com_no, *up, *def, byteArr, RowPitch);
	dx_sub[com_no].ResourceBarrier(*def, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);

	return hr;
}

void Dx12Process::createTextureArr(int numTexArr, int resourceIndex, char* texName,
	UCHAR* byteArr, DXGI_FORMAT format,
	int width, LONG_PTR RowPitch, int height) {

	if (!texture) {
		texNum = numTexArr + 2;//dummyNor,dummyDifSpe分
		texture = new InternalTexture[texNum];
		UCHAR dm[8 * 4 * 8] = {};
		UCHAR ndm[4] = { 128,128,255,0 };
		for (int i = 0; i < 8 * 4 * 8; i += 4)
			memcpy(&dm[i], ndm, sizeof(UCHAR) * 4);
		texture[0].setParameter(DXGI_FORMAT_R8G8B8A8_UNORM, 8, 8 * 4, 8);
		texture[0].setName("dummyNor.");
		texture[0].setData(dm);

		UCHAR sdm[4] = { 255,255,255,255 };
		for (int i = 0; i < 8 * 4 * 8; i += 4)
			memcpy(&dm[i], sdm, sizeof(UCHAR) * 4);
		texture[1].setParameter(DXGI_FORMAT_R8G8B8A8_UNORM, 8, 8 * 4, 8);
		texture[1].setName("dummyDifSpe.");
		texture[1].setData(dm);
	}

	InternalTexture* tex = &texture[resourceIndex + 2];
	tex->setParameter(format, width, RowPitch, height);
	tex->setName(texName);
	tex->setData(byteArr);
}

bool Dx12Process::Initialize(HWND hWnd, int width, int height) {

	mClientWidth = width;
	mClientHeight = height;

#if defined(DEBUG) || defined(_DEBUG) 
	//デバッグ中はデバッグレイヤーを有効化する
	{
		ComPtr<ID3D12Debug> debugController;
		if (FAILED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
			ErrorMessage("D3D12GetDebugInterface Error");
			return false;
		}
		debugController->EnableDebugLayer();
	}
#endif

	//ファクトリ生成
	//アダプターの列挙、スワップ チェーンの作成、
	//および全画面表示モードとの間の切り替えに使用される Alt + 
	//Enter キー シーケンスとのウィンドウの関連付けを行うオブジェクトを生成するために使用
	if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&mdxgiFactory)))) {
		ErrorMessage("CreateDXGIFactory1 Error");
		return false;
	}

	//デバイス生成
	HRESULT hardwareResult = D3D12CreateDevice(
		nullptr,             // default adapter
		D3D_FEATURE_LEVEL_11_0,
		IID_PPV_ARGS(&md3dDevice));

	//↑ハードウエア処理不可の場合ソフトウエア処理にする,ソフトウエア処理デバイス生成
	if (FAILED(hardwareResult))
	{
		ComPtr<IDXGIAdapter> pWarpAdapter;
		if (FAILED(mdxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&pWarpAdapter)))) {
			ErrorMessage("EnumWarpAdapter Error");
			return false;
		}

		if (FAILED(D3D12CreateDevice(
			pWarpAdapter.Get(),
			D3D_FEATURE_LEVEL_11_0,
			IID_PPV_ARGS(&md3dDevice)))) {
			ErrorMessage("D3D12CreateDevice Error");
			return false;
		}
	}
	else if (DXR_CreateResource) {
		D3D12_FEATURE_DATA_D3D12_OPTIONS5 features5 = {};
		HRESULT hr = md3dDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &features5, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS5));
		if (FAILED(hr) || features5.RaytracingTier == D3D12_RAYTRACING_TIER_NOT_SUPPORTED)
		{
			ErrorMessage("DXR not supported");
			DXR_CreateResource = false;
			return false;
		}
	}

	//Descriptor のアドレス計算で使用
	mRtvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	mDsvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	mCbvSrvUavDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	//MultiSampleレベルチェック
	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msQualityLevels;
	msQualityLevels.Format = mBackBufferFormat;
	msQualityLevels.SampleCount = 4;
	msQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
	msQualityLevels.NumQualityLevels = 0;
	if (FAILED(md3dDevice->CheckFeatureSupport(
		D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
		&msQualityLevels,     //機能のサポートを示すデータが格納
		sizeof(msQualityLevels)))) {
		ErrorMessage("CheckFeatureSupport Error");
		return false;
	}

	m4xMsaaQuality = msQualityLevels.NumQualityLevels;
	assert(m4xMsaaQuality > 0 && "Unexpected MSAA quality level.");

	//コマンドキュー生成
	if (!graphicsQueue.Create(md3dDevice.Get(), false))return false;
	if (!computeQueue.Create(md3dDevice.Get(), true))return false;

	for (int i = 0; i < COM_NO; i++) {
		//コマンドアロケータ,コマンドリスト生成
		if (!dx_sub[i].ListCreate(false))return false;
		dx_sub[i].createResourceBarrierList();
		dx_sub[i].createUavResourceBarrierList();
		if (!dx_subCom[i].ListCreate(true))return false;
		dx_subCom[i].createUavResourceBarrierList();
	}

	//初期化
	mSwapChain.Reset();

	//スワップチェイン生成
	DXGI_SWAP_CHAIN_DESC1 sd = {};
	sd.Width = mClientWidth;
	sd.Height = mClientHeight;
	sd.Format = mBackBufferFormat;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.BufferCount = SwapChainBufferCount;
	sd.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	sd.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	sd.Scaling = DXGI_SCALING_STRETCH;
	sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	sd.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	ComPtr<IDXGISwapChain1> swapChain;
	if (FAILED(mdxgiFactory->CreateSwapChainForHwnd(
		graphicsQueue.mCommandQueue.Get(),
		hWnd,
		&sd,
		nullptr,
		nullptr,
		swapChain.GetAddressOf()))) {
		ErrorMessage("CreateSwapChain Error");
		return false;
	}
	swapChain->QueryInterface(IID_PPV_ARGS(mSwapChain.GetAddressOf()));

	//Descriptor:何のバッファかを記述される
	//スワップチェインをRenderTargetとして使用するためのDescriptorHeapを作成(Descriptorの記録場所)
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.NumDescriptors = SwapChainBufferCount;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;   //RenderTargetView
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE; //シェーダからアクセスしないのでNONEでOK
	rtvHeapDesc.NodeMask = 0;
	if (FAILED(md3dDevice->CreateDescriptorHeap(
		&rtvHeapDesc, IID_PPV_ARGS(mRtvHeap.GetAddressOf())))) {
		ErrorMessage("CreateDescriptorHeap Error");
		return false;
	}

	//深度ステンシルビュ-DescriptorHeapを作成
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	dsvHeapDesc.NodeMask = 0;
	if (FAILED(md3dDevice->CreateDescriptorHeap(
		&dsvHeapDesc, IID_PPV_ARGS(mDsvHeap.GetAddressOf())))) {
		ErrorMessage("CreateDescriptorHeap Error");
		return false;
	}

	//レンダーターゲットビューデスクリプターヒープの開始ハンドル取得
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(mRtvHeap->GetCPUDescriptorHandleForHeapStart());
	for (UINT i = 0; i < SwapChainBufferCount; i++) {
		//スワップチェインバッファ取得
		if (FAILED(mSwapChain->GetBuffer(i, IID_PPV_ARGS(&mSwapChainBuffer[i])))) {
			ErrorMessage("GetSwapChainBuffer Error");
			return false;
		}
		//レンダーターゲットビュー(Descriptor)生成,DescriptorHeapにDescriptorが記録される
		md3dDevice->CreateRenderTargetView(mSwapChainBuffer[i].Get(), nullptr, rtvHeapHandle);
		//ヒープ位置オフセット(2個あるので2個目記録位置にDescriptorSize分オフセット)
		rtvHeapHandle.ptr += mRtvDescriptorSize;
	}

	//デプスステンシルビューDESC
	D3D12_RESOURCE_DESC depthStencilDesc = {};
	depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	depthStencilDesc.Alignment = 0;
	depthStencilDesc.Width = mClientWidth;
	depthStencilDesc.Height = mClientHeight;
	depthStencilDesc.DepthOrArraySize = 1;
	depthStencilDesc.MipLevels = 1;
	depthStencilDesc.Format = mDepthStencilFormat;
	depthStencilDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	depthStencilDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_HEAP_PROPERTIES depthStencilHeapProps = {};
	depthStencilHeapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
	depthStencilHeapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	depthStencilHeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	depthStencilHeapProps.CreationNodeMask = 1;
	depthStencilHeapProps.VisibleNodeMask = 1;

	D3D12_CLEAR_VALUE optClear;
	optClear.Format = mDepthStencilFormat;
	optClear.DepthStencil.Depth = 1.0f;
	optClear.DepthStencil.Stencil = 0;
	//深度ステンシルバッファ領域確保
	if (FAILED(md3dDevice->CreateCommittedResource(
		&depthStencilHeapProps,
		D3D12_HEAP_FLAG_NONE,
		&depthStencilDesc,
		D3D12_RESOURCE_STATE_COMMON,
		&optClear,
		IID_PPV_ARGS(mDepthStencilBuffer.GetAddressOf())))) {
		ErrorMessage("CreateCommittedResource DepthStencil Error");
		return false;
	}

	//深度ステンシルビューデスクリプターヒープの開始ハンドル取得
	D3D12_CPU_DESCRIPTOR_HANDLE mDsvHeapHeapHandle(mDsvHeap->GetCPUDescriptorHandleForHeapStart());
	//深度ステンシルビュー生成
	md3dDevice->CreateDepthStencilView(mDepthStencilBuffer.Get(), nullptr, mDsvHeapHeapHandle);

	dx_sub[0].Bigin();

	//深度ステンシルバッファ,リソースバリア共有→深度書き込み
	dx_sub[0].ResourceBarrier(mDepthStencilBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE);

	StreamView::createResetBuffer(0);

	dx_sub[0].End();
	WaitFence();

	//ビューポート
	mScreenViewport.TopLeftX = 0;
	mScreenViewport.TopLeftY = 0;
	mScreenViewport.Width = static_cast<float>(mClientWidth);
	mScreenViewport.Height = static_cast<float>(mClientHeight);
	mScreenViewport.MinDepth = 0.0f;
	mScreenViewport.MaxDepth = 1.0f;

	mScissorRect = { 0, 0, mClientWidth, mClientHeight };

	MatrixIdentity(&upd[0].mProj);
	MatrixIdentity(&upd[1].mProj);
	MatrixIdentity(&upd[0].mView);
	MatrixIdentity(&upd[1].mView);
	//カメラ画角
	ViewY_theta = 55.0f;
	// アスペクト比の計算
	aspect = (float)mScreenViewport.Width / (float)mScreenViewport.Height;
	//nearプレーン
	NearPlane = 1.0f;
	//farプレーン
	FarPlane = 10000.0f;
	MatrixPerspectiveFovLH(&upd[0].mProj, ViewY_theta, aspect, NearPlane, FarPlane);
	MatrixPerspectiveFovLH(&upd[1].mProj, ViewY_theta, aspect, NearPlane, FarPlane);

	//ビューポート行列作成(3D座標→2D座標変換に使用)
	MatrixViewPort(&Vp, mClientWidth, mClientHeight);

	//ポイントライト構造体初期化
	ResetPointLight();

	//平行光源初期化
	upd[0].dlight.Direction.as(0.0f, 0.0f, 0.0f, 0.0f);
	upd[0].dlight.LightColor.as(1.0f, 1.0f, 1.0f, 1.0f);
	upd[0].dlight.onoff = 0.0f;
	upd[1].dlight.Direction.as(0.0f, 0.0f, 0.0f, 0.0f);
	upd[1].dlight.LightColor.as(1.0f, 1.0f, 1.0f, 1.0f);
	upd[1].dlight.onoff = 0.0f;

	//フォグ初期化
	fog.FogColor.as(1.0f, 1.0f, 1.0f, 1.0f);
	fog.Amount = 0.0f;
	fog.Density = 0.0f;
	fog.on_off = 0.0f;

	return CreateShaderByteCode();
}

void Dx12Process::setPerspectiveFov(float ViewAngle, float nearPlane, float farPlane) {
	ViewY_theta = ViewAngle;
	NearPlane = nearPlane;
	FarPlane = farPlane;
	MatrixPerspectiveFovLH(&upd[0].mProj, ViewY_theta, aspect, NearPlane, FarPlane);
	MatrixPerspectiveFovLH(&upd[1].mProj, ViewY_theta, aspect, NearPlane, FarPlane);
}

void Dx12Process::BiginDraw(int com_no, bool clearBackBuffer) {
	dx_sub[com_no].mCommandList->RSSetViewports(1, &mScreenViewport);
	dx_sub[com_no].mCommandList->RSSetScissorRects(1, &mScissorRect);

	dx_sub[com_no].ResourceBarrier(mSwapChainBuffer[mCurrBackBuffer].Get(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = mRtvHeap->GetCPUDescriptorHandleForHeapStart();
	rtvHandle.ptr += mCurrBackBuffer * mRtvDescriptorSize;

	if (clearBackBuffer) {
		dx_sub[com_no].mCommandList->ClearRenderTargetView(rtvHandle,
			DirectX::Colors::Black, 0, nullptr);

		dx_sub[com_no].mCommandList->ClearDepthStencilView(mDsvHeap->GetCPUDescriptorHandleForHeapStart(),
			D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
	}

	dx_sub[com_no].mCommandList->OMSetRenderTargets(1, &rtvHandle,
		true, &mDsvHeap->GetCPUDescriptorHandleForHeapStart());
}

void Dx12Process::EndDraw(int com_no) {
	dx_sub[com_no].ResourceBarrier(mSwapChainBuffer[mCurrBackBuffer].Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
}

void Dx12Process::Bigin(int com_no) {
	dx_sub[com_no].Bigin();
}

void Dx12Process::End(int com_no) {
	dx_sub[com_no].End();
}

void Dx12Process::DrawScreen() {
	// swap the back and front buffers
	mSwapChain->Present(0, 0);
	mCurrBackBuffer = mSwapChain->GetCurrentBackBufferIndex();
}

void Dx12Process::Cameraset(VECTOR3 pos, VECTOR3 dir, VECTOR3 up) {
	//カメラの位置と方向を設定
	MatrixLookAtLH(&upd[cBuffSwap[0]].mView,
		pos, //カメラの位置
		dir, //カメラの方向を向ける点
		up); //カメラの上の方向(通常視点での上方向を1.0fにする)
	//シェーダー計算用座標登録(視点からの距離で使う)
	upd[cBuffSwap[0]].pos.as(pos.x, pos.y, pos.z);
	upd[cBuffSwap[0]].dir.as(dir.x, dir.y, dir.z);
	upd[cBuffSwap[0]].up.as(up.x, up.y, up.z);
}

void Dx12Process::ResetPointLight() {
	for (int i = 0; i < LIGHT_PCS; i++) {
		upd[0].plight.LightPos[i].as(0.0f, 0.0f, 0.0f, 0.0f);
		upd[0].plight.LightColor[i].as(0.0f, 0.0f, 0.0f, 0.0f);
		upd[0].plight.Lightst[i].as(0.0f, 1.0f, 0.001f, 0.001f);
		upd[1].plight.LightPos[i].as(0.0f, 0.0f, 0.0f, 0.0f);
		upd[1].plight.LightColor[i].as(0.0f, 0.0f, 0.0f, 0.0f);
		upd[1].plight.Lightst[i].as(0.0f, 1.0f, 0.001f, 0.001f);
	}
	upd[0].lightNum = 0;
	upd[1].lightNum = 0;
}

bool Dx12Process::PointLightPosSet(int Idx, VECTOR3 pos, VECTOR4 Color, bool on_off,
	float range, VECTOR3 atten) {

	if (Idx > LIGHT_PCS - 1 || Idx < 0) {
		ErrorMessage("lightNumの値が範囲外です");
		return false;
	}

	if (Idx + 1 > upd[cBuffSwap[0]].lightNum && on_off)upd[cBuffSwap[0]].lightNum = Idx + 1;

	float onoff;
	if (on_off)onoff = 1.0f; else onoff = 0.0f;
	upd[cBuffSwap[0]].plight.LightPos[Idx].as(pos.x, pos.y, pos.z, onoff);
	upd[cBuffSwap[0]].plight.LightColor[Idx].as(Color.x, Color.y, Color.z, Color.w);
	upd[cBuffSwap[0]].plight.Lightst[Idx].as(range, atten.x, atten.y, atten.z);
	upd[cBuffSwap[0]].plight.LightPcs = upd[cBuffSwap[0]].lightNum;

	return true;
}

void Dx12Process::DirectionLight(float x, float y, float z, float r, float g, float b) {
	upd[cBuffSwap[0]].dlight.Direction.as(x, y, z, 0.0f);
	upd[cBuffSwap[0]].dlight.LightColor.as(r, g, b, 0.0f);
}

void Dx12Process::SetDirectionLight(bool onoff) {
	float f = 0.0f;
	if (onoff)f = 1.0f;
	upd[0].dlight.onoff = f;
	upd[1].dlight.onoff = f;
}

void Dx12Process::Fog(float r, float g, float b, float amount, float density, bool onoff) {

	if (!onoff) {
		fog.on_off = 0.0f;
		return;
	}
	fog.on_off = 1.0f;
	fog.FogColor.as(r, g, b, 1.0f);
	fog.Amount = amount;
	fog.Density = density;
}

UINT64 Dx12Process::getRequiredIntermediateSize(ID3D12Resource* res) {
	D3D12_RESOURCE_DESC desc = res->GetDesc();
	UINT64  total_bytes = 0;
	dx->md3dDevice->GetCopyableFootprints(&desc, 0, 1, 0, nullptr, nullptr, nullptr, &total_bytes);
	return total_bytes;
}

HRESULT Dx12Process::CopyResourcesToGPU(int com_no, ID3D12Resource* up, ID3D12Resource* def,
	const void* initData, LONG_PTR RowPitch) {

	bool copyTexture = false;
	D3D12_RESOURCE_DESC desc = def->GetDesc();
	if (desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE1D ||
		desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D ||
		desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D) {
		copyTexture = true;
	}

	D3D12_SUBRESOURCE_DATA subResourceData = {};
	subResourceData.pData = initData;

	D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint;
	D3D12_TEXTURE_COPY_LOCATION dest, src;

	if (copyTexture) {
		UINT64  total_bytes = 0;
		dx->md3dDevice->GetCopyableFootprints(&desc, 0, 1, 0, &footprint, nullptr, nullptr, &total_bytes);

		memset(&dest, 0, sizeof(dest));
		dest.pResource = def;
		dest.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		dest.SubresourceIndex = 0;

		memset(&src, 0, sizeof(src));
		src.pResource = up;
		src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
		src.PlacedFootprint = footprint;

		subResourceData.RowPitch = footprint.Footprint.RowPitch;
	}
	else {
		subResourceData.RowPitch = RowPitch;
	}

	D3D12_SUBRESOURCE_DATA mapResource;
	HRESULT hr = up->Map(0, nullptr, reinterpret_cast<void**>(&mapResource));
	if (FAILED(hr)) {
		return hr;
	}

	UCHAR* destResource = (UCHAR*)mapResource.pData;
	mapResource.RowPitch = subResourceData.RowPitch;
	UCHAR* srcResource = (UCHAR*)subResourceData.pData;

	UINT copyDestWsize = (UINT)mapResource.RowPitch;
	UINT copySrcWsize = (UINT)RowPitch;

	if (copyDestWsize == copySrcWsize) {
		memcpy(destResource, srcResource, sizeof(UCHAR) * copySrcWsize * desc.Height);
	}
	else {
		for (UINT s = 0; s < desc.Height; s++) {
			memcpy(&destResource[copyDestWsize * s], &srcResource[copySrcWsize * s], sizeof(UCHAR) * copySrcWsize);
		}
	}

	up->Unmap(0, nullptr);

	if (copyTexture) {
		dx_sub[com_no].mCommandList.Get()->CopyTextureRegion(&dest, 0, 0, 0, &src, nullptr);
	}
	else {
		dx_sub[com_no].mCommandList.Get()->CopyBufferRegion(def, 0, up, 0, copySrcWsize);
	}

	return S_OK;
}

ComPtr<ID3D12Resource> Dx12Process::CreateDefaultBuffer(
	int com_no,
	const void* initData,
	UINT64 byteSize,
	ComPtr<ID3D12Resource>& uploadBuffer, bool uav)
{
	ComPtr<ID3D12Resource> defaultBuffer;
	HRESULT hr;
	D3D12_RESOURCE_STATES after = D3D12_RESOURCE_STATE_GENERIC_READ;

	if (uav) {
		hr = createDefaultResourceBuffer_UNORDERED_ACCESS(defaultBuffer.GetAddressOf(), byteSize);
		after = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	}
	else {
		hr = createDefaultResourceBuffer(defaultBuffer.GetAddressOf(), byteSize);
	}
	if (FAILED(hr)) {
		return nullptr;
	}

	UINT64 uploadBufferSize = getRequiredIntermediateSize(defaultBuffer.Get());
	hr = createUploadResource(uploadBuffer.GetAddressOf(), uploadBufferSize);
	if (FAILED(hr)) {
		return nullptr;
	}

	dx_sub[com_no].ResourceBarrier(defaultBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
	hr = CopyResourcesToGPU(com_no, uploadBuffer.Get(), defaultBuffer.Get(), initData, byteSize);
	if (FAILED(hr)) {
		return nullptr;
	}
	dx_sub[com_no].ResourceBarrier(defaultBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, after);

	return defaultBuffer;
}

ComPtr<ID3D12Resource> Dx12Process::CreateStreamBuffer(UINT64 byteSize)
{
	ComPtr<ID3D12Resource> defaultBuffer;

	createDefaultResourceCommon(md3dDevice.Get(), defaultBuffer.GetAddressOf(),
		D3D12_RESOURCE_DIMENSION_BUFFER, byteSize, 1,
		DXGI_FORMAT_UNKNOWN, D3D12_RESOURCE_FLAG_NONE,
		D3D12_RESOURCE_STATE_STREAM_OUT);

	return defaultBuffer;
}

ComPtr<ID3DBlob> Dx12Process::CompileShader(LPSTR szFileName, size_t size, LPSTR szFuncName, LPSTR szProfileName) {

	HRESULT hr;
	ComPtr<ID3DBlob> byteCode = nullptr;
	ComPtr<ID3DBlob> errors = nullptr;
	hr = D3DCompile(szFileName, size, nullptr, nullptr, nullptr, szFuncName, szProfileName,
		D3D10_SHADER_DEBUG | D3D10_SHADER_SKIP_OPTIMIZATION, 0, &byteCode, &errors);
	if (FAILED(hr)) {
		CreateShaderByteCodeBool = false;
	}

	if (errors != nullptr)
		ErrorMessage((char*)errors->GetBufferPointer());

	return byteCode;
}

void Dx12Process::Instancing(int& insNum, CONSTANT_BUFFER* cb, VECTOR3 pos, VECTOR3 angle, VECTOR3 size) {
	if (insNum > INSTANCE_PCS_3D - 1)insNum--;
	MATRIX mov;
	MATRIX rotZ, rotY, rotX, rotZY, rotZYX;
	MATRIX scale;
	MATRIX scro;
	MATRIX world;
	MATRIX WV;

	//拡大縮小
	MatrixScaling(&scale, size.x, size.y, size.z);
	//表示位置
	MatrixRotationZ(&rotZ, angle.z);
	MatrixRotationY(&rotY, angle.y);
	MatrixRotationX(&rotX, angle.x);
	MatrixMultiply(&rotZY, &rotZ, &rotY);
	MatrixMultiply(&rotZYX, &rotZY, &rotX);
	MatrixTranslation(&mov, pos.x, pos.y, pos.z);
	MatrixMultiply(&scro, &rotZYX, &scale);
	MatrixMultiply(&world, &scro, &mov);

	//ワールド、カメラ、射影行列、等
	cb->World[insNum] = world;
	MatrixMultiply(&WV, &world, &upd[cBuffSwap[0]].mView);
	MatrixMultiply(&cb->WVP[insNum], &WV, &upd[cBuffSwap[0]].mProj);
	MatrixTranspose(&cb->World[insNum]);
	MatrixTranspose(&cb->WVP[insNum]);
	insNum++;
}

void Dx12Process::InstancingUpdate(CONSTANT_BUFFER* cb, VECTOR4 Color, float disp,
	float px, float py, float mx, float my, DivideArr* divArr, int numDiv, float shininess) {

	cb->C_Pos.as(upd[cBuffSwap[0]].pos.x,
		upd[cBuffSwap[0]].pos.y,
		upd[cBuffSwap[0]].pos.z, 0.0f);
	cb->viewUp.as(upd[cBuffSwap[0]].up.x,
		upd[cBuffSwap[0]].up.y,
		upd[cBuffSwap[0]].up.z, 0.0f);
	cb->AddObjColor.as(Color.x, Color.y, Color.z, Color.w);
	memcpy(&cb->GlobalAmbientLight, &GlobalAmbientLight, sizeof(VECTOR4));
	cb->numLight.as((float)upd[cBuffSwap[0]].plight.LightPcs, 0.0f, 0.0f, 0.0f);
	memcpy(cb->pLightPos, upd[cBuffSwap[0]].plight.LightPos, sizeof(VECTOR4) * LIGHT_PCS);
	memcpy(cb->pLightColor, upd[cBuffSwap[0]].plight.LightColor, sizeof(VECTOR4) * LIGHT_PCS);
	memcpy(cb->pLightst, upd[cBuffSwap[0]].plight.Lightst, sizeof(VECTOR4) * LIGHT_PCS);
	cb->dDirection = upd[cBuffSwap[0]].dlight.Direction;
	cb->dLightColor = upd[cBuffSwap[0]].dlight.LightColor;
	cb->dLightst.x = upd[cBuffSwap[0]].dlight.onoff;
	cb->FogAmo_Density.as(fog.Amount, fog.Density, fog.on_off, 0.0f);
	cb->FogColor = fog.FogColor;
	cb->DispAmount.as(disp, float(numDiv), shininess, 0.0f);
	cb->pXpYmXmY.as(px, py, mx, my);
	for (int i = 0; i < numDiv; i++) {
		cb->Divide[i].as(divArr[i].distance, divArr[i].divide, 0.0f, 0.0f);
	}
}

float Dx12Process::GetViewY_theta() {
	return ViewY_theta;
}

float Dx12Process::Getaspect() {
	return aspect;
}

float Dx12Process::GetNearPlane() {
	return NearPlane;
}

float Dx12Process::GetFarPlane() {
	return FarPlane;
}

char* Dx12Process::GetNameFromPass(char* pass) {

	CHAR temp[255];
	strcpy_s(temp, pass);

	bool f = false;

	for (int i = 0; temp[i] != '\0' && i < 255; i++) {
		if (temp[i] == '\\' || temp[i] == '/') { f = true; break; }
	}

	if (f) {
		//ファイル名のみでは無い場合の処理
		while (*pass != '\0') pass++;//終端文字までポインタを進める
		while (*pass != '\\' && *pass != '/')pass--;//ファイル名先頭の'\'か'/'までポインタを進める
		pass++;//'\'または'/'の次(ファイル名先頭文字)までポインタを進める
	}

	return pass;//ポインタ操作してるので返り値を使用させる
}

void addChar::addStr(char* str1, char* str2) {
	size_t size1 = strlen(str1);
	size_t size2 = strlen(str2);
	size = size1 + size2 + 1;
	str = new char[size];
	memcpy(str, str1, size1 + 1);
	strncat(str, str2, size2 + 1);
}

//移動量一定化
DWORD T_float::f[2] = { timeGetTime() };
DWORD T_float::time[2] = { 0 };
DWORD T_float::time_fps[2] = { 0 };//FPS計測用
int T_float::frame[2] = { 0 };     //FPS計測使用
int T_float::up = 0;
char T_float::str[50];     //ウインドウ枠文字表示使用
float T_float::adj = 1.0f;

void T_float::GetTime(HWND hWnd) {
	time[0] = timeGetTime() - f[0];
	f[0] = timeGetTime();

	//FPS計測
	frame[0]++;
	sprintf(str, "     Ctrl:決定  Delete:キャンセル  fps=%d  ups=%d", frame[0], up);
	if (timeGetTime() - time_fps[0] > 1000)
	{
		time_fps[0] = timeGetTime();
		frame[0] = 0;
		char Name[100] = { 0 };
		GetClassNameA(hWnd, Name, sizeof(Name));
		strcat(Name, str);
		SetWindowTextA(hWnd, Name);
	}
}

void T_float::GetTimeUp(HWND hWnd) {
	time[1] = timeGetTime() - f[1];
	f[1] = timeGetTime();

	//UPS計測
	frame[1]++;
	if (timeGetTime() - time_fps[1] > 1000)
	{
		time_fps[1] = timeGetTime();
		up = frame[1];
		frame[1] = 0;
	}
}

void T_float::AddAdjust(float ad) {
	adj = ad;
}

int T_float::GetUps() {
	return up;
}

float T_float::Add(float f) {
	float r = ((float)time[1] * f) / 2.0f * adj;
	if (r <= 0.0f)return 0.01f;
	if (r >= 1000000.0f)return 1000000.0f;
	return r;
}

//エラーメッセージ
void ErrorMessage(char *E_mes) {
	MessageBoxA(0, E_mes, 0, MB_OK);
}