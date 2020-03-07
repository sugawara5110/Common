//*****************************************************************************************//
//**                                                                                     **//
//**                   　　　          Dx12ProcessCoreクラス                             **//
//**                                   Initialize関数                                    **//
//*****************************************************************************************//

#define _CRT_SECURE_NO_WARNINGS
#include "Dx12ProcessCore.h"
#include <WindowsX.h>
#include "./ShaderCG/ShaderFunction.h"
#include "./ShaderCG/Shader2D.h"
#include "./ShaderCG/Shader3D.h"
#include "./ShaderCG/ShaderDisp.h"
#include "./ShaderCG/ShaderMesh.h"
#include "./ShaderCG/ShaderMesh_D.h"
#include "./ShaderCG/ShaderParticle.h"
#include "./ShaderCG/ShaderSkinMesh.h"
#include "./ShaderCG/ShaderSkinMesh_D.h"
#include "./ShaderCG/ShaderWaveCom.h"
#include "./ShaderCG/ShaderWaveDraw.h"
#include "./ShaderCG/ShaderCommonPS.h"
#include "./ShaderCG/ShaderCommonTriangleHSDS.h"
#include "./ShaderCG/ShaderPostEffect.h"
#include <locale.h>

using Microsoft::WRL::ComPtr;

bool Dx12Process_sub::ListCreate() {
	for (int i = 0; i < 2; i++) {
		//コマンドアロケータ生成(コマンドリストに積むバッファを確保するObj)
		if (FAILED(Dx12Process::dx->md3dDevice->CreateCommandAllocator(
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			IID_PPV_ARGS(mCmdListAlloc[i].GetAddressOf())))) {
			ErrorMessage("CreateCommandAllocator Error");
			return false;
		}
	}

	//コマンドリスト生成
	if (FAILED(Dx12Process::dx->md3dDevice->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
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

void Dx12Process_sub::Bigin() {
	mComState = OPEN;
	mAloc_Num = 1 - mAloc_Num;
	mCmdListAlloc[mAloc_Num]->Reset();
	mCommandList->Reset(mCmdListAlloc[mAloc_Num].Get(), nullptr);
}

void Dx12Process_sub::End() {
	//コマンドクローズ
	mCommandList->Close();
	mComState = CLOSE;
}

Dx12Process *Dx12Process::dx = nullptr;
std::mutex Dx12Process::mtx;

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

	WaitFenceCurrent();

	for (int i = 0; i < texNum; i++) {
		RELEASE(texture[i]);
		RELEASE(textureUp[i]);
	}
	ARR_DELETE(texture);
	ARR_DELETE(textureUp);
}

void Dx12Process::FenceSetEvent() {
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

void Dx12Process::WaitFence(bool mode) {
	fenceMode = mode;
	//クローズ後リストに加える
	for (int i = 0; i < COM_NO; i++) {
		if (dx_sub[i].mComState != CLOSE)continue;
		ID3D12CommandList* cmdsLists[] = { dx_sub[i].mCommandList.Get() };
		mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);
		dx_sub[i].mComState = USED;
	}

	//インクリメントされたことで描画完了と判断
	mCurrentFence++;
	//GPU上で実行されているコマンド完了後,自動的にフェンスにmCurrentFenceを書き込む
	//(mFence->GetCompletedValue()で得られる値がmCurrentFenceと同じになる)
	mCommandQueue->Signal(mFence.Get(), mCurrentFence);

	if (!mode)
		FenceSetEvent();
}

void Dx12Process::WaitFenceCurrent() {
	Lock();
	WaitFence(false);
	Unlock();
}

void Dx12Process::WaitFencePast() {
	Lock();
	WaitFence(true);
	Unlock();
}

class addShader {

public:
	char *str = nullptr;
	size_t size;

	void addStr(char *str1, size_t size1, char *str2, size_t size2) {
		size = size1 + size2 + 1;
		str = new char[size];
		memcpy(str, str1, size1 + 1);
		strncat(str, str2, size2 + 1);
	}

	~addShader() {
		S_DELETE(str);
	}
};

bool Dx12Process::CreateShaderByteCode() {

	addShader D3, Disp, Mesh, MeshD, Skin, SkinD, Wave, ComPS, ComHSDS;
	D3.addStr(ShaderFunction, strlen(ShaderFunction), Shader3D, strlen(Shader3D));
	Disp.addStr(ShaderFunction, strlen(ShaderFunction), ShaderDisp, strlen(ShaderDisp));
	Mesh.addStr(ShaderFunction, strlen(ShaderFunction), ShaderMesh, strlen(ShaderMesh));
	MeshD.addStr(ShaderFunction, strlen(ShaderFunction), ShaderMesh_D, strlen(ShaderMesh_D));
	Skin.addStr(ShaderFunction, strlen(ShaderFunction), ShaderSkinMesh, strlen(ShaderSkinMesh));
	SkinD.addStr(ShaderFunction, strlen(ShaderFunction), ShaderSkinMesh_D, strlen(ShaderSkinMesh_D));
	Wave.addStr(ShaderFunction, strlen(ShaderFunction), ShaderWaveDraw, strlen(ShaderWaveDraw));
	ComPS.addStr(ShaderFunction, strlen(ShaderFunction), ShaderCommonPS, strlen(ShaderCommonPS));
	ComHSDS.addStr(ShaderFunction, strlen(ShaderFunction), ShaderCommonTriangleHSDS, strlen(ShaderCommonTriangleHSDS));

	//CommonPS
	pPixelShader_Bump = CompileShader(ComPS.str, ComPS.size, "PS_LBump", "ps_5_0");
	pPixelShader_3D = CompileShader(ComPS.str, ComPS.size, "PS_L", "ps_5_0");
	pPixelShader_Emissive = CompileShader(ComPS.str, ComPS.size, "PS", "ps_5_0");
	//CommonHSDS(Triangle)
	pHullShaderTriangle = CompileShader(ComHSDS.str, ComHSDS.size, "HS", "hs_5_0");
	pDomainShaderTriangle = CompileShader(ComHSDS.str, ComHSDS.size, "DS", "ds_5_0");

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
		{ "BONE_INDEX", 0, DXGI_FORMAT_R32G32B32A32_UINT, 0, 44, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "BONE_WEIGHT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 60, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
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
		{ 0, "COLOR", 0, 0, 4, 0 }
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
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 4 * 3 * 3, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
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

	//3DレイアウトTexture有り
	pVertexLayout_3D =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 4 * 3,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 4 * 3 * 2, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};
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
	//テセレータ
	pVertexShader_DISP = CompileShader(Disp.str, Disp.size, "VSDisp", "vs_5_0");
	pHullShader_DISP = CompileShader(Disp.str, Disp.size, "HSDisp", "hs_5_0");
	pDomainShader_DISP = CompileShader(Disp.str, Disp.size, "DSDisp", "ds_5_0");
	//Wave
	pComputeShader_Wave = CompileShader(ShaderWaveCom, strlen(ShaderWaveCom), "CS", "cs_5_0");
	pVertexShader_Wave = CompileShader(Wave.str, Wave.size, "VSWave", "vs_5_0");
	pHullShader_Wave = CompileShader(Wave.str, Wave.size, "HSWave", "hs_5_0");
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

	return CreateShaderByteCodeBool;
}

void Dx12Process::SetTextureBinary(Texture *texture, int size) {
	texNum = size;
	tex = texture;
}

int Dx12Process::GetTexNumber(CHAR *fileName) {

	fileName = GetNameFromPass(fileName);

	for (int i = 0; i < texNum; i++) {
		if (tex[i].texName == '\0')continue;
		char str[50];
		char str1[50];
		strcpy(str, tex[i].texName);
		strcpy(str1, fileName);
		int i1 = -1;
		while (str[++i1] != '\0' && str[i1] != '.' && str[i1] == str1[i1]);
		if (str[i1] == '.' && str1[i1] == '.')return i;
	}

	return -1;
}

bool Dx12Process::GetTexture(int com_no) {

	texture = new ID3D12Resource*[texNum];
	textureUp = new ID3D12Resource*[texNum];

	std::unique_ptr<uint8_t[]> decodedData = nullptr;
	D3D12_SUBRESOURCE_DATA subresource;
	Microsoft::WRL::ComPtr<ID3D12Resource> t = nullptr;

	char str[50];

	for (int i = 0; i < texNum; i++) {
		if (tex[i].texName == nullptr)continue;

		if (tex[i].binary_ch != nullptr) {
			if (FAILED(DirectX::LoadWICTextureFromMemory(md3dDevice.Get(),
				(uint8_t*)tex[i].binary_ch, tex[i].binary_size, &t, decodedData, subresource))) {
				sprintf(str, "テクスチャ№%d読み込みエラー", (i));
				ErrorMessage(str);
				return false;
			}
		}
		else {
			wchar_t ws[200];
			setlocale(LC_CTYPE, "jpn");
			mbstowcs(ws, tex[i].texName, 200);
			tex[i].texName = GetNameFromPass(tex[i].texName);

			if (FAILED(DirectX::LoadWICTextureFromFile(md3dDevice.Get(),
				ws, &t, decodedData, subresource))) {
				sprintf(str, "テクスチャ№%d読み込みエラー", (i));
				ErrorMessage(str);
				return false;
			}
		}

		D3D12_RESOURCE_DESC texDesc;
		texDesc = t->GetDesc();

		D3D12_HEAP_PROPERTIES HeapProps;
		HeapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
		HeapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		HeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		HeapProps.CreationNodeMask = 1;
		HeapProps.VisibleNodeMask = 1;

		if (FAILED(md3dDevice->CreateCommittedResource(&HeapProps, D3D12_HEAP_FLAG_NONE, &texDesc,
			D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&texture[i])))) {
			sprintf(str, "texture[%d]読み込みエラー", (i));
			ErrorMessage(str);
			return false;
		}

		//upload
		UINT64 uploadBufferSize = GetRequiredIntermediateSize(texture[i], 0, 1);
		D3D12_HEAP_PROPERTIES HeapPropsUp;
		HeapPropsUp.Type = D3D12_HEAP_TYPE_UPLOAD;
		HeapPropsUp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		HeapPropsUp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		HeapPropsUp.CreationNodeMask = 1;
		HeapPropsUp.VisibleNodeMask = 1;

		D3D12_RESOURCE_DESC BufferDesc;
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

		if (FAILED(md3dDevice->CreateCommittedResource(&HeapPropsUp, D3D12_HEAP_FLAG_NONE,
			&BufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr, IID_PPV_ARGS(&textureUp[i])))) {
			sprintf(str, "textureUp[%d]読み込みエラー", (i));
			ErrorMessage(str);
			return false;
		}

		D3D12_RESOURCE_BARRIER BarrierDesc;
		BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		BarrierDesc.Transition.pResource = texture[i];
		BarrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
		BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
		dx_sub[com_no].mCommandList->ResourceBarrier(1, &BarrierDesc);

		UpdateSubresources(dx_sub[com_no].mCommandList.Get(), texture[i], textureUp[i], 0, 0, 1, &subresource);

		BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
		BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;
		dx_sub[com_no].mCommandList->ResourceBarrier(1, &BarrierDesc);
	}

	return true;
}

void Dx12Process::UpTextureRelease() {
	for (int i = 0; i < texNum; i++) {
		if (tex[i].binary_size == 0 || tex[i].UpKeep)continue;
		RELEASE(textureUp[i]);
	}
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

	//ハードウエア処理可能か,ハードウエア処理デバイス生成
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

	//フェンス生成
	//Command Queueに送信したCommand Listの完了を検知するために使用
	if (FAILED(md3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE,
		IID_PPV_ARGS(&mFence)))) {
		ErrorMessage("CreateFence Error");
		return false;
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

	//コマンドキューデスク
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT; //直接コマンドキュー
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE; //GPUタイムアウトが有効
	//コマンド待ち行列生成
	if (FAILED(md3dDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mCommandQueue)))) {
		ErrorMessage("CreateCommandQueue Error");
		return false;
	}

	for (int i = 0; i < COM_NO; i++) {
		//コマンドアロケータ,コマンドリスト生成
		if (!dx_sub[i].ListCreate())return false;
	}

	//初期化
	mSwapChain.Reset();

	//スワップチェイン生成
	DXGI_SWAP_CHAIN_DESC sd;
	sd.BufferDesc.Width = mClientWidth;
	sd.BufferDesc.Height = mClientHeight;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferDesc.Format = mBackBufferFormat;
	sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	sd.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	sd.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.BufferCount = SwapChainBufferCount;
	sd.OutputWindow = hWnd;
	sd.Windowed = true;
	sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	if (FAILED(mdxgiFactory->CreateSwapChain(
		mCommandQueue.Get(),
		&sd,
		mSwapChain.GetAddressOf()))) {
		ErrorMessage("CreateSwapChain Error");
		return false;
	}

	//Descriptor:何のバッファかを記述される
	//スワップチェインをRenderTargetとして使用するためのDescriptorHeapを作成(Descriptorの記録場所)
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
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
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc;
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
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(mRtvHeap->GetCPUDescriptorHandleForHeapStart());
	for (UINT i = 0; i < SwapChainBufferCount; i++) {
		//スワップチェインバッファ取得
		if (FAILED(mSwapChain->GetBuffer(i, IID_PPV_ARGS(&mSwapChainBuffer[i])))) {
			ErrorMessage("GetSwapChainBuffer Error");
			return false;
		}
		//レンダーターゲットビュー(Descriptor)生成,DescriptorHeapにDescriptorが記録される
		md3dDevice->CreateRenderTargetView(mSwapChainBuffer[i].Get(), nullptr, rtvHeapHandle);
		//ヒープ位置オフセット(2個あるので2個目記録位置にDescriptorSize分オフセット)
		rtvHeapHandle.Offset(1, mRtvDescriptorSize);
	}

	//デプスステンシルビューDESC
	D3D12_RESOURCE_DESC depthStencilDesc;
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

	D3D12_CLEAR_VALUE optClear;
	optClear.Format = mDepthStencilFormat;
	optClear.DepthStencil.Depth = 1.0f;
	optClear.DepthStencil.Stencil = 0;
	//深度ステンシルバッファ領域確保
	if (FAILED(md3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&depthStencilDesc,
		D3D12_RESOURCE_STATE_COMMON,
		&optClear,
		IID_PPV_ARGS(mDepthStencilBuffer.GetAddressOf())))) {
		ErrorMessage("CreateCommittedResource DepthStencil Error");
		return false;
	}

	//深度ステンシルビューデスクリプターヒープの開始ハンドル取得
	CD3DX12_CPU_DESCRIPTOR_HANDLE mDsvHeapHeapHandle(mDsvHeap->GetCPUDescriptorHandleForHeapStart());
	//深度ステンシルビュー生成
	md3dDevice->CreateDepthStencilView(mDepthStencilBuffer.Get(), nullptr, mDsvHeapHeapHandle);

	dx_sub[0].Bigin();

	//深度ステンシルバッファ,リソースバリア共有→深度書き込み
	dx_sub[0].mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mDepthStencilBuffer.Get(),
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE));

	dx_sub[0].End();
	WaitFenceCurrent();

	//ビューポート
	mScreenViewport.TopLeftX = 0;
	mScreenViewport.TopLeftY = 0;
	mScreenViewport.Width = static_cast<float>(mClientWidth);
	mScreenViewport.Height = static_cast<float>(mClientHeight);
	mScreenViewport.MinDepth = 0.0f;
	mScreenViewport.MaxDepth = 1.0f;

	mScissorRect = { 0, 0, mClientWidth, mClientHeight };

	MatrixIdentity(&mProj);
	MatrixIdentity(&mView);
	//カメラ画角
	ViewY_theta = 55.0f;
	// アスペクト比の計算
	aspect = (float)mScreenViewport.Width / (float)mScreenViewport.Height;
	//nearプレーン
	NearPlane = 1.0f;
	//farプレーン
	FarPlane = 10000.0f;
	MatrixPerspectiveFovLH(&mProj, ViewY_theta, aspect, NearPlane, FarPlane);

	//ビューポート行列作成(3D座標→2D座標変換に使用)
	MatrixViewPort(&Vp, mClientWidth, mClientHeight);

	//ポイントライト構造体初期化
	ResetPointLight();

	//平行光源初期化
	dlight.Direction.as(0.0f, 0.0f, 0.0f, 0.0f);
	dlight.LightColor.as(1.0f, 1.0f, 1.0f, 1.0f);
	dlight.Lightst.as(1.0f, 0.0f, 0.3f, 0.0f);

	//フォグ初期化
	fog.FogColor.as(1.0f, 1.0f, 1.0f, 1.0f);
	fog.Amount = 0.0f;
	fog.Density = 0.0f;
	fog.on_off = 0.0f;

	return CreateShaderByteCode();
}

void Dx12Process::Sclear(int com_no) {

	dx_sub[com_no].mCommandList->RSSetViewports(1, &mScreenViewport);
	dx_sub[com_no].mCommandList->RSSetScissorRects(1, &mScissorRect);

	dx_sub[com_no].mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mSwapChainBuffer[mCurrBackBuffer].Get(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	dx_sub[com_no].mCommandList->ClearRenderTargetView(CD3DX12_CPU_DESCRIPTOR_HANDLE(
		mRtvHeap->GetCPUDescriptorHandleForHeapStart(),
		mCurrBackBuffer,
		mRtvDescriptorSize), DirectX::Colors::Black, 0, nullptr);
	dx_sub[com_no].mCommandList->ClearDepthStencilView(mDsvHeap->GetCPUDescriptorHandleForHeapStart(),
		D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	dx_sub[com_no].mCommandList->OMSetRenderTargets(1, &CD3DX12_CPU_DESCRIPTOR_HANDLE(
		mRtvHeap->GetCPUDescriptorHandleForHeapStart(),
		mCurrBackBuffer,
		mRtvDescriptorSize), true, &mDsvHeap->GetCPUDescriptorHandleForHeapStart());

	dx_sub[com_no].mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mSwapChainBuffer[mCurrBackBuffer].Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
}

void Dx12Process::Bigin(int com_no) {
	if (fenceMode)FenceSetEvent();
	dx_sub[com_no].Bigin();
}

void Dx12Process::End(int com_no) {
	dx_sub[com_no].End();
}

void Dx12Process::DrawScreen() {
	// swap the back and front buffers
	mSwapChain->Present(0, 0);
	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;
}

void Dx12Process::Cameraset(float cx1, float cx2, float cy1, float cy2, float cz1, float cz2) {

	//カメラの位置と方向を設定
	MatrixLookAtLH(&mView,
		cx1, cy1, cz1,   //カメラの位置
		cx2, cy2, cz2,   //カメラの方向を向ける点
		0.0f, 0.0f, 1.0f); //カメラの上の方向(通常視点での上方向を1.0fにする)
	//シェーダー計算用座標登録(視点からの距離で使う)
	posX = cx1;
	posY = cy1;
	posZ = cz1;
}

void Dx12Process::ResetPointLight() {
	for (int i = 0; i < LIGHT_PCS; i++) {
		plight.LightPos[i].as(0.0f, 0.0f, 0.0f, 0.0f);
		plight.LightColor[i].as(0.0f, 0.0f, 0.0f, 0.0f);
		plight.Lightst[i].as(0.0f, 0.0f, 0.0f, 0.0f);
	}
	plight.ShadowLow_val = 0.0f;
	lightNum = 0;
}

void Dx12Process::P_ShadowBright(float val) {
	plight.ShadowLow_val = val;
}

bool Dx12Process::PointLightPosSet(int Idx, float x, float y, float z, float r, float g, float b, float a, float range,
	float brightness, float attenuation, bool on_off) {

	if (Idx > LIGHT_PCS - 1 || Idx < 0) {
		ErrorMessage("lightNumの値が範囲外です");
		return false;
	}

	if (Idx + 1 > lightNum && on_off)lightNum = Idx + 1;

	float onoff;
	if (on_off)onoff = 1.0f; else onoff = 0.0f;
	plight.LightPos[Idx].as(x, y, z, 1.0f);
	plight.LightColor[Idx].as(r, g, b, a);
	plight.Lightst[Idx].as(range, brightness, attenuation, onoff);
	plight.LightPcs = lightNum;

	return true;
}

void Dx12Process::DirectionLight(float x, float y, float z, float r, float g, float b, float bright, float ShadowBright) {
	dlight.Direction.as(x, y, z, 0.0f);
	dlight.LightColor.as(r, g, b, 0.0f);
	dlight.Lightst.x = bright;
	dlight.Lightst.z = ShadowBright;
	dlight.Lightst.w = 0.0f;
}

void Dx12Process::SetDirectionLight(bool onoff) {
	float f = 0.0f;
	if (onoff)f = 1.0f;
	dlight.Lightst.y = f;
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

Microsoft::WRL::ComPtr<ID3D12Resource> Dx12Process::CreateDefaultBuffer(
	ID3D12GraphicsCommandList* cmdList,
	const void* initData,
	UINT64 byteSize,
	Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer)
{
	ComPtr<ID3D12Resource> defaultBuffer;

	//デフォルトバッファ生成
	md3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(byteSize),
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(defaultBuffer.GetAddressOf()));

	//デフォルトバッファにCPUメモリデータをコピーするための中間バッファ生成
	md3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(byteSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(uploadBuffer.GetAddressOf()));

	//デフォルトバッファにコピーしたいデータの内容
	D3D12_SUBRESOURCE_DATA subResourceData = {};
	subResourceData.pData = initData;
	subResourceData.RowPitch = (LONG_PTR)byteSize;
	subResourceData.SlicePitch = subResourceData.RowPitch;

	//デフォルトバッファ,リソースバリア共有→コピーされる側
	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(),
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST));
	//UpdateSubresources:中間バッファにCPUメモリをコピーする
	//その後CommandListを使うと中間バッファの内容をアップロードされる
	UpdateSubresources<1>(cmdList, defaultBuffer.Get(), uploadBuffer.Get(), 0, 0, 1, &subResourceData);
	//デフォルトバッファ,リソースバリアコピーされる側→アップロード
	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(),
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ));

	return defaultBuffer;
}

Microsoft::WRL::ComPtr<ID3D12Resource> Dx12Process::CreateStreamBuffer(UINT64 byteSize)
{
	ComPtr<ID3D12Resource> defaultBuffer;

	//ストリームバッファ生成
	md3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(byteSize),
		D3D12_RESOURCE_STATE_STREAM_OUT,
		nullptr,
		IID_PPV_ARGS(defaultBuffer.GetAddressOf()));

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
		OutputDebugStringA((char*)errors->GetBufferPointer());

	return byteCode;
}

void Dx12Process::InstancedMap(int& insNum, CONSTANT_BUFFER* cb, float x, float y, float z,
	float thetaZ, float thetaY, float thetaX, float sizeX, float sizeY, float sizeZ) {

	if (sizeY <= 0.0f || sizeZ <= 0.0f) {
		sizeY = sizeX;
		sizeZ = sizeX;
	}

	if (insNum > INSTANCE_PCS_3D - 1)insNum--;
	MATRIX mov;
	MATRIX rotZ, rotY, rotX, rotZY, rotZYX;
	MATRIX scale;
	MATRIX scro;
	MATRIX world;
	MATRIX WV;

	//拡大縮小
	MatrixScaling(&scale, sizeX, sizeY, sizeZ);
	//表示位置
	MatrixRotationZ(&rotZ, thetaZ);
	MatrixRotationY(&rotY, thetaY);
	MatrixRotationX(&rotX, thetaX);
	MatrixMultiply(&rotZY, &rotZ, &rotY);
	MatrixMultiply(&rotZYX, &rotZY, &rotX);
	MatrixTranslation(&mov, x, y, z);
	MatrixMultiply(&scro, &rotZYX, &scale);
	MatrixMultiply(&world, &scro, &mov);

	//ワールド、カメラ、射影行列、等
	cb->World[insNum] = world;
	MatrixMultiply(&WV, &world, &mView);
	MatrixMultiply(&cb->WVP[insNum], &WV, &mProj);
	MatrixTranspose(&cb->World[insNum]);
	MatrixTranspose(&cb->WVP[insNum]);
	insNum++;
}

void Dx12Process::MatrixMap(CONSTANT_BUFFER *cb, float r, float g, float b, float a, float disp, float px, float py, float mx, float my) {

	cb->C_Pos.as(posX, posY, posZ, 0.0f);
	cb->AddObjColor.as(r, g, b, a);
	cb->pShadowLow_Lpcs.as(plight.ShadowLow_val, (float)plight.LightPcs, 0.0f, 0.0f);
	memcpy(cb->pLightPos, plight.LightPos, sizeof(VECTOR4) * LIGHT_PCS);
	memcpy(cb->pLightColor, plight.LightColor, sizeof(VECTOR4) * LIGHT_PCS);
	memcpy(cb->pLightst, plight.Lightst, sizeof(VECTOR4) * LIGHT_PCS);
	cb->dDirection = dlight.Direction;
	cb->dLightColor = dlight.LightColor;
	cb->dLightst = dlight.Lightst;
	cb->FogAmo_Density.as(fog.Amount, fog.Density, fog.on_off, 0.0f);
	cb->FogColor = fog.FogColor;
	if (disp == 0.0f)disp = 3.0f;
	cb->DispAmount.as(disp, 0.0f, 0.0f, 0.0f);
	cb->pXpYmXmY.as(px, py, mx, my);
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

char *Dx12Process::GetNameFromPass(char *pass) {

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