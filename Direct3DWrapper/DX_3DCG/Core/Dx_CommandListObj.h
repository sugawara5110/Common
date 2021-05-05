//*****************************************************************************************//
//**                                                                                     **//
//**                   　　　       Dx_CommandListObj                                    **//
//**                                                                                     **//
//*****************************************************************************************//

#ifndef Class_Dx_CommandListObj_Header
#define Class_Dx_CommandListObj_Header

#include "Dx_Device.h"
#include "DxEnum.h"
#include <iostream>

#define COM_NO 32
using Microsoft::WRL::ComPtr;

//前方宣言
template<class T>
class ConstantBuffer;
class Dx12Process;
class MeshData;
class PolygonData;
class PolygonData2D;
class ParticleData;
class SkinMesh;
class DxText;
class Wave;
class PostEffect;
class Common;
class DxNNCommon;
class DxNeuralNetwork;
class DxPooling;
class DxConvolution;
class SearchPixel;
class DxGradCAM;
class DxActivation;
class DxOptimizer;
class DXR_Basic;
class SkinnedCom;
class TextObj;
struct StreamView;
//前方宣言

class Dx_CommandListObj final {

private:
	friend Dx12Process;
	friend MeshData;
	friend PolygonData;
	friend PolygonData2D;
	friend ParticleData;
	friend SkinMesh;
	friend Wave;
	friend PostEffect;
	friend Common;
	friend DxNNCommon;
	friend DxNeuralNetwork;
	friend DxPooling;
	friend DxConvolution;
	friend SearchPixel;
	friend DxGradCAM;
	friend DxActivation;
	friend DxOptimizer;
	friend DXR_Basic;
	friend SkinnedCom;
	friend StreamView;

	ComPtr<ID3D12CommandAllocator> mCmdListAlloc[2];
	ComPtr<ID3D12GraphicsCommandList4> mCommandList;
	int mAloc_Num = 0;
	volatile ComListState mComState = {};

	bool ListCreate(bool Compute, ID3D12Device5* dev);
	void ResourceBarrier(ID3D12Resource* res, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after);
	void CopyResourceGENERIC_READ(ID3D12Resource* dest, ID3D12Resource* src);
	void Bigin();
	void End();

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
	void delayResourceBarrierBefore(ID3D12Resource* res, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after);
	void delayCopyResource(ID3D12Resource* dest, ID3D12Resource* src);
	void delayResourceBarrierAfter(ID3D12Resource* res, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after);
	void delayUavResourceBarrier(ID3D12Resource* res);
	void RunDelayResourceBarrierBefore();
	void RunDelayCopyResource();
	void RunDelayResourceBarrierAfter();
	void RunDelayUavResourceBarrier();
};

#endif
