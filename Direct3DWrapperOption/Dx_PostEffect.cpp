//*****************************************************************************************//
//**                                                                                     **//
//**                   　　　       PostEffectクラス                                     **//
//**                                                                                     **//
//*****************************************************************************************//

#include "Dx_PostEffect.h"
#include "Shader/ShaderPostEffect.h"
#include <stdio.h>
#include <math.h>

namespace {

	ComPtr<ID3D12DescriptorHeap> mpSamplerHeap = nullptr;
	D3D12_GPU_DESCRIPTOR_HANDLE samplerHandle;
	bool createSamplerDone = false;

	void createSampler() {

		if (createSamplerDone)return;

		D3D12_SAMPLER_DESC samLinear = {};
		samLinear.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		samLinear.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		samLinear.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		samLinear.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		samLinear.MipLODBias = 0;
		samLinear.MaxAnisotropy = 16;
		samLinear.ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
		samLinear.MinLOD = 0.0f;
		samLinear.MaxLOD = D3D12_FLOAT32_MAX;

		mpSamplerHeap = Dx_Device::GetInstance()->CreateSamplerDescHeap(samLinear);
		samplerHandle = mpSamplerHeap.Get()->GetGPUDescriptorHandleForHeapStart();

		createSamplerDone = true;
	}

	ComPtr<ID3DBlob> pComputeShader_Post[3] = {};

	bool createShaderDone = false;

	float* gaussian(float sigma, int& numWid) {//sd:分散 数値が大きい程全体的に同等になる

		int wCnt = 0;
		while (expf(-(float)(wCnt * wCnt) / (sigma * sigma * 2.0f)) > 0.0001f) {
			if (wCnt++ > 1000)break;
		}

		numWid = wCnt;
		float total = 0.0f;
		float* gaArr = new float[numWid];

		for (int x = 0; x < numWid; x++)
		{
			total +=
				gaArr[x] = expf(-(float)(x * x) / (sigma * sigma * 2.0f));
		}

		for (int i = 0; i < numWid; i++)
		{
			gaArr[i] /= total;
		}

		return gaArr;
	}

	float* gaussian2D(float sigma, int& numWid) {

		int wCnt = 0;
		while (expf(-(float)(wCnt * wCnt) / (sigma * sigma * 2.0f)) > 0.0001f) {
			if (wCnt++ > 1000)break;
		}

		numWid = wCnt;
		float total = 0.0f;
		float* gaArr = new float[numWid * numWid];
		for (int y = 0; y < numWid; y++)
		{
			for (int x = 0; x < numWid; x++)
			{
				total +=
					gaArr[numWid * y + x] =
					expf(-(float)(x * x + y * y) / (sigma * sigma * 2.0f));
			}
		}

		for (int i = 0; i < numWid * numWid; i++)
		{
			gaArr[i] /= total;
		}

		return gaArr;
	}

	class Bloom :public Common {
	public:
		struct CONSTANT_BUFFER_Bloom {
			CoordTf::VECTOR4 GausSize;//x:GausSize, y:Bloom強さ, z:Luminance閾値, w:numGaus
		};

		ComPtr<ID3D12RootSignature> mRootSignatureCom = nullptr;
		ComPtr<ID3DBlob> pComputeShader_Post[5] = {};
		ComPtr<ID3D12PipelineState> mPSOCom[5] = {};
		ComPtr<ID3D12DescriptorHeap> mDescHeap = nullptr;

		D3D12_GPU_DESCRIPTOR_HANDLE mGaussianFilterHandleGPU = {};
		D3D12_GPU_DESCRIPTOR_HANDLE mInputHandleGPU = {};
		D3D12_GPU_DESCRIPTOR_HANDLE mOutputHandleGPU = {};

		ComPtr<ID3D12Resource> GaussianFilterBufferUp = nullptr;
		ComPtr<ID3D12Resource> GaussianFilterBuffer = nullptr;
		ComPtr<ID3D12Resource> mInputBuffer = nullptr;
		ComPtr<ID3D12Resource> mOutputBuffer = nullptr;

		struct gInOut {
			D3D12_GPU_DESCRIPTOR_HANDLE mLuminanceHandleGPU = {};
			D3D12_GPU_DESCRIPTOR_HANDLE mGaussInOutHandleGPU0 = {};
			D3D12_GPU_DESCRIPTOR_HANDLE mGaussInOutHandleGPU1 = {};

			ComPtr<ID3D12Resource> mLuminanceBuffer = nullptr;
			ComPtr<ID3D12Resource> mGaussInOutBuffer0 = nullptr;
			ComPtr<ID3D12Resource> mGaussInOutBuffer1 = nullptr;
		};

		struct GaussInOut {
		private:
			const UINT gaBaseSize[6] = {
				512,256,128,64,32,16
			};
		public:
			UINT numGauss = 1;
			const UINT MaxnumGauss = 10;
			std::unique_ptr<gInOut[]> gInout = nullptr;
			D3D12_GPU_DESCRIPTOR_HANDLE mGaussTexHandleGPU = {};

			std::unique_ptr<UINT[]> gaSizeArr = nullptr;

			void setPara(UINT num_gauss, UINT* GaussSizeArr = nullptr) {
				numGauss = num_gauss;
				if (!GaussSizeArr && num_gauss > _countof(gaBaseSize)) {
					numGauss = _countof(gaBaseSize);
				}
				if (num_gauss > MaxnumGauss)numGauss = MaxnumGauss;
				if (num_gauss <= 0)numGauss = 1;
				gInout = std::make_unique<gInOut[]>(numGauss);
				gaSizeArr = std::make_unique<UINT[]>(numGauss);
				for (UINT i = 0; i < numGauss; i++) {
					if (GaussSizeArr)gaSizeArr[i] = GaussSizeArr[i];
					else
						gaSizeArr[i] = gaBaseSize[i];
				}
			}
		};
		GaussInOut GaussInout = {};

		int GaussianWid = 0;
		float GaussianType = false;//false:2D
		float BloomStrength = 0.0f;
		float ThresholdLuminance = 0.0f;

		ConstantBuffer<CONSTANT_BUFFER_Bloom>* mObjectCB = nullptr;

		~Bloom() {
			S_DELETE(mObjectCB);
		}

		void createShader() {
			LPSTR csName[5] = {
				"BloomCS0",
				"BloomCS1",
				"BloomCS2",
				"BloomCS12",
				"BloomCS3"
			};

			for (int i = 0; i < _countof(csName); i++)
				pComputeShader_Post[i] = CompileShader(ShaderPostEffect2, strlen(ShaderPostEffect2), csName[i], "cs_5_1");
		}

		bool GaussianCreate(UINT num_gauss, UINT* GaussSizeArr) {
			float* gaArr = nullptr;
			int GaussianBufferSize = 0;

			if (GaussianType) {
				gaArr = gaussian(10, GaussianWid);
				GaussianBufferSize = GaussianWid;
			}
			else {
				gaArr = gaussian2D(4, GaussianWid);
				GaussianBufferSize = GaussianWid * GaussianWid;
			}

			GaussianFilterBuffer = dx->CreateDefaultBuffer(com_no, gaArr,
				GaussianBufferSize * sizeof(float), GaussianFilterBufferUp, false);
			if (!GaussianFilterBuffer) {
				Dx_Util::ErrorMessage("PostEffect::GaussianCreate Error!!");
				return false;
			}

			Dx_Device* device = Dx_Device::GetInstance();

			GaussInout.setPara(num_gauss, GaussSizeArr);

			for (UINT i = 0; i < GaussInout.numGauss; i++) {
				UINT s = GaussInout.gaSizeArr[i];
				if (FAILED(device->createDefaultResourceTEXTURE2D_UNORDERED_ACCESS(
					GaussInout.gInout[i].mGaussInOutBuffer0.GetAddressOf(),
					s, s))) {

					Dx_Util::ErrorMessage("PostEffect::GaussianCreate Error!!");
					return false;
				}
				if (FAILED(device->createDefaultResourceTEXTURE2D_UNORDERED_ACCESS(
					GaussInout.gInout[i].mGaussInOutBuffer1.GetAddressOf(),
					s, s))) {

					Dx_Util::ErrorMessage("PostEffect::GaussianCreate Error!!");
					return false;
				}

				if (FAILED(device->createDefaultResourceTEXTURE2D_UNORDERED_ACCESS(
					GaussInout.gInout[i].mLuminanceBuffer.GetAddressOf(),
					s, s))) {

					Dx_Util::ErrorMessage("PostEffect::GaussianCreate Error!!");
					return false;
				}
			}

			GaussianFilterBuffer.Get()->SetName(Dx_Util::charToLPCWSTR("GaussianFilterBuffer", objName));
			GaussianFilterBufferUp.Get()->SetName(Dx_Util::charToLPCWSTR("GaussianFilterBufferUp", objName));

			ARR_DELETE(gaArr);
			return true;
		}

		bool ComCreate(UINT num_gauss, UINT* GaussSizeArr) {
			Dx_Device* device = Dx_Device::GetInstance();
			createSampler();
			createShader();
			mObjectCB = new ConstantBuffer<CONSTANT_BUFFER_Bloom>(1);

			if (FAILED(device->createDefaultResourceTEXTURE2D_UNORDERED_ACCESS(mOutputBuffer.GetAddressOf(), dx->getClientWidth(), dx->getClientHeight()))) {
				Dx_Util::ErrorMessage("PostEffect::ComCreate Error!!"); return false;
			}
			if (FAILED(device->createDefaultResourceTEXTURE2D(mInputBuffer.GetAddressOf(),
				dx->getClientWidth(), dx->getClientHeight(), DXGI_FORMAT_R8G8B8A8_UNORM))) {

				Dx_Util::ErrorMessage("PostEffect::ComCreate Error!!"); return false;
			}
			mOutputBuffer.Get()->SetName(Dx_Util::charToLPCWSTR("mOutputBuffer", objName));
			mInputBuffer.Get()->SetName(Dx_Util::charToLPCWSTR("mInputBuffer", objName));

			if (!GaussianCreate(num_gauss, GaussSizeArr))return false;

			int numDescSrv = 2 + GaussInout.numGauss;
			int numDescUav = 1 + GaussInout.numGauss * 3;
			int numDesc = numDescSrv + numDescUav;
			mDescHeap = device->CreateDescHeap(numDesc);
			if (!mDescHeap) {
				Dx_Util::ErrorMessage("PostEffect::ComCreate Error!!"); return false;
			}
			UINT64 ptr = mDescHeap->GetGPUDescriptorHandleForHeapStart().ptr;
			mGaussianFilterHandleGPU.ptr = ptr;
			mInputHandleGPU.ptr = (ptr += dx->getCbvSrvUavDescriptorSize());
			mOutputHandleGPU.ptr = (ptr += dx->getCbvSrvUavDescriptorSize());
			for (UINT i = 0; i < GaussInout.numGauss; i++) {
				GaussInout.gInout[i].mGaussInOutHandleGPU0.ptr = (ptr += dx->getCbvSrvUavDescriptorSize());
				GaussInout.gInout[i].mGaussInOutHandleGPU1.ptr = (ptr += dx->getCbvSrvUavDescriptorSize());
				GaussInout.gInout[i].mLuminanceHandleGPU.ptr = (ptr += dx->getCbvSrvUavDescriptorSize());
			}
			GaussInout.mGaussTexHandleGPU.ptr = (ptr += dx->getCbvSrvUavDescriptorSize());
			//↑配列用

			D3D12_CPU_DESCRIPTOR_HANDLE hDescriptor = mDescHeap->GetCPUDescriptorHandleForHeapStart();

			UINT StructureByteStride[1] = { sizeof(float) };
			device->CreateSrvBuffer(hDescriptor, GaussianFilterBuffer.GetAddressOf(), 1, StructureByteStride);
			device->CreateSrvTexture(hDescriptor, mInputBuffer.GetAddressOf(), 1);

			D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
			uavDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
			uavDesc.Texture2D.MipSlice = 0;

			device->getDevice()->CreateUnorderedAccessView(mOutputBuffer.Get(), nullptr, &uavDesc, hDescriptor);
			hDescriptor.ptr += dx->getCbvSrvUavDescriptorSize();
			for (UINT i = 0; i < GaussInout.numGauss; i++) {
				device->getDevice()->CreateUnorderedAccessView(GaussInout.gInout[i].mGaussInOutBuffer0.Get(), nullptr, &uavDesc, hDescriptor);
				hDescriptor.ptr += dx->getCbvSrvUavDescriptorSize();
				device->getDevice()->CreateUnorderedAccessView(GaussInout.gInout[i].mGaussInOutBuffer1.Get(), nullptr, &uavDesc, hDescriptor);
				hDescriptor.ptr += dx->getCbvSrvUavDescriptorSize();
				device->getDevice()->CreateUnorderedAccessView(GaussInout.gInout[i].mLuminanceBuffer.Get(), nullptr, &uavDesc, hDescriptor);
				hDescriptor.ptr += dx->getCbvSrvUavDescriptorSize();
			}
			for (UINT i = 0; i < GaussInout.numGauss; i++)
				device->CreateSrvTexture(hDescriptor, GaussInout.gInout[i].mGaussInOutBuffer1.GetAddressOf(), 1);
			//↑シェーダーで配列として使うので連続で並べる必要がある

			comObj->ResourceBarrier(mInputBuffer.Get(),
				D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_GENERIC_READ);

			comObj->ResourceBarrier(mOutputBuffer.Get(),
				D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

			//ルートシグネチャ
			const int numSrv = 3;
			const int numUav = 4;
			const int numCbv = 1;
			const int numSampler = 1;
			const int numPara = numSrv + numUav + numCbv + numSampler;
			D3D12_ROOT_PARAMETER rootParameter[numPara] = {};
			D3D12_ROOT_PARAMETER& r = rootParameter[0];
			r.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
			r.Descriptor.ShaderRegister = 0;
			r.Descriptor.RegisterSpace = 0;
			r.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

			D3D12_DESCRIPTOR_RANGE sRange[numSrv] = {};
			for (int i = 0; i < numSrv; i++) {
				D3D12_DESCRIPTOR_RANGE& s = sRange[i];
				s.BaseShaderRegister = i;
				s.NumDescriptors = (i == 2) ? GaussInout.MaxnumGauss : 1;
				s.RegisterSpace = (i == 2) ? 1 : 0;
				s.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
				s.OffsetInDescriptorsFromTableStart = 0;
				D3D12_ROOT_PARAMETER& r = rootParameter[i + numCbv];
				r.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
				r.DescriptorTable.NumDescriptorRanges = 1;
				r.DescriptorTable.pDescriptorRanges = &s;
				r.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
			}

			D3D12_DESCRIPTOR_RANGE uRange[numUav] = {};
			for (int i = 0; i < numUav; i++) {
				D3D12_DESCRIPTOR_RANGE& u = uRange[i];
				u.BaseShaderRegister = i;
				u.NumDescriptors = 1;
				u.RegisterSpace = 0;
				u.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
				u.OffsetInDescriptorsFromTableStart = 0;
				D3D12_ROOT_PARAMETER& r = rootParameter[i + numCbv + numSrv];
				r.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
				r.DescriptorTable.NumDescriptorRanges = 1;
				r.DescriptorTable.pDescriptorRanges = &u;
				r.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
			}
			D3D12_DESCRIPTOR_RANGE Sampler = {};
			Sampler.BaseShaderRegister = 0;
			Sampler.NumDescriptors = 1;
			Sampler.RegisterSpace = 0;
			Sampler.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
			Sampler.OffsetInDescriptorsFromTableStart = 0;
			D3D12_ROOT_PARAMETER& sam = rootParameter[numUav + numCbv + numSrv];
			sam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			sam.DescriptorTable.NumDescriptorRanges = 1;
			sam.DescriptorTable.pDescriptorRanges = &Sampler;
			sam.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

			mRootSignatureCom = CreateRsCompute(numPara, rootParameter);
			if (mRootSignatureCom == nullptr)return false;

			createShader();

			for (int i = 0; i < _countof(pComputeShader_Post); i++) {
				ID3DBlob* cs = pComputeShader_Post[i].Get();
				mPSOCom[i] = CreatePsoCompute(cs, mRootSignatureCom.Get());
				if (mPSOCom[i] == nullptr)return false;
			}

			return true;
		}

		void Compute(int com, bool On) {
			if (!On)return;

			SetCommandList(com);
			ID3D12GraphicsCommandList* mCList = mCommandList;
			Dx_CommandListObj& d = *comObj;

			CONSTANT_BUFFER_Bloom cb = {};
			cb.GausSize.x = (float)GaussianWid;
			cb.GausSize.y = BloomStrength;
			cb.GausSize.z = ThresholdLuminance;
			cb.GausSize.w = (float)GaussInout.numGauss;
			mObjectCB->CopyData(0, cb);

			ID3D12DescriptorHeap* descriptorHeaps[] = { mDescHeap.Get() ,mpSamplerHeap.Get() };
			mCList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
			mCList->SetComputeRootSignature(mRootSignatureCom.Get());
			mCList->SetComputeRootConstantBufferView(0, mObjectCB->Resource()->GetGPUVirtualAddress());

			//バックバッファをコピー元にする
			d.delayResourceBarrierBefore(GetSwapChainBuffer(),
				D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE);
			d.delayResourceBarrierBefore(mInputBuffer.Get(),
				D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COPY_DEST);
			d.RunDelayResourceBarrierBefore();
			//現在のバックバッファをインプット用バッファにコピーする
			mCList->CopyResource(mInputBuffer.Get(), GetSwapChainBuffer());

			d.ResourceBarrier(mInputBuffer.Get(),
				D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);

			mCList->SetComputeRootDescriptorTable(1, mGaussianFilterHandleGPU);
			mCList->SetComputeRootDescriptorTable(2, mInputHandleGPU);
			mCList->SetComputeRootDescriptorTable(4, mOutputHandleGPU);
			mCList->SetComputeRootDescriptorTable(8, samplerHandle);

			D3D12_RESOURCE_BARRIER ba = {};
			ba.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;

			UINT disX = dx->getClientWidth() / 32;
			UINT disY = dx->getClientHeight() / 8;

			for (UINT i = 0; i < GaussInout.numGauss; i++) {
				UINT gauX = (UINT)GaussInout.gInout[i].mGaussInOutBuffer0.Get()->GetDesc().Width / 8;
				UINT gauY = GaussInout.gInout[i].mGaussInOutBuffer0.Get()->GetDesc().Height / 8;

				mCList->SetComputeRootDescriptorTable(5, GaussInout.gInout[i].mGaussInOutHandleGPU0);
				mCList->SetComputeRootDescriptorTable(6, GaussInout.gInout[i].mGaussInOutHandleGPU1);
				mCList->SetComputeRootDescriptorTable(7, GaussInout.gInout[i].mLuminanceHandleGPU);

				mCList->SetPipelineState(mPSOCom[0].Get());
				mCList->Dispatch(gauX, gauY, 1);//CS内8, 8, 1
				ba.UAV.pResource = GaussInout.gInout[i].mLuminanceBuffer.Get();
				mCList->ResourceBarrier(1, &ba);

				if (GaussianType) {
					mCList->SetPipelineState(mPSOCom[1].Get());
					mCList->Dispatch(gauX, gauY, 1);
					ba.UAV.pResource = GaussInout.gInout[i].mGaussInOutBuffer0.Get();
					mCList->ResourceBarrier(1, &ba);

					mCList->SetPipelineState(mPSOCom[2].Get());
					mCList->Dispatch(gauX, gauY, 1);
					ba.UAV.pResource = GaussInout.gInout[i].mGaussInOutBuffer1.Get();
					mCList->ResourceBarrier(1, &ba);
				}
				else {
					mCList->SetPipelineState(mPSOCom[3].Get());
					mCList->Dispatch(gauX, gauY, 1);
					ba.UAV.pResource = GaussInout.gInout[i].mGaussInOutBuffer1.Get();
					mCList->ResourceBarrier(1, &ba);
				}
			}

			mCList->SetComputeRootDescriptorTable(3, GaussInout.mGaussTexHandleGPU);
			mCList->SetPipelineState(mPSOCom[4].Get());
			mCList->Dispatch(disX, disY, 1);
			ba.UAV.pResource = mOutputBuffer.Get();
			mCList->ResourceBarrier(1, &ba);

			d.delayResourceBarrierBefore(GetSwapChainBuffer(),
				D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_COPY_DEST);
			d.delayResourceBarrierBefore(mOutputBuffer.Get(),
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_GENERIC_READ);
			d.RunDelayResourceBarrierBefore();
			//計算後バックバッファにコピー
			mCList->CopyResource(GetSwapChainBuffer(), mOutputBuffer.Get());

			d.delayResourceBarrierBefore(mOutputBuffer.Get(),
				D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			d.delayResourceBarrierBefore(GetSwapChainBuffer(),
				D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_RENDER_TARGET);
			d.RunDelayResourceBarrierBefore();
		}
	};
	std::unique_ptr<Bloom> bloom = nullptr;
}

void PostEffect::createShader() {

	if (createShaderDone)return;

	LPSTR csName[3] = {
		"MosaicCS",
		"BlurCS",
		"DepthOfFieldCS"
	};

	for (int i = 0; i < _countof(pComputeShader_Post); i++) {
		pComputeShader_Post[i] = CompileShader(ShaderPostEffect, strlen(ShaderPostEffect), csName[i], "cs_5_1");
	}

	createShaderDone = true;
}

PostEffect::PostEffect() {
	mObjectCB = new ConstantBuffer<CONSTANT_BUFFER_PostMosaic>(1);
}

PostEffect::~PostEffect() {
	S_DELETE(mObjectCB);
}

bool PostEffect::ComCreateMosaic() {
	return ComCreate(0);
}

bool PostEffect::ComCreateBlur() {
	return ComCreate(1);
}

bool PostEffect::ComCreateDepthOfField() {
	return ComCreate(2);
}

bool PostEffect::ComCreateBloom(float gaussianType, UINT num_gauss, UINT* GaussSizeArr) {
	bloom = std::make_unique<Bloom>();
	bloom->GaussianType = gaussianType;
	return bloom->ComCreate(num_gauss, GaussSizeArr);
}

bool PostEffect::ComCreate(int no) {

	Dx_Device* device = Dx_Device::GetInstance();
	const int numSrv = 2;
	const int numUav = 1;
	const int numDesc = numSrv + numUav;
	mDescHeap = device->CreateDescHeap(numDesc);
	if (!mDescHeap) {
		Dx_Util::ErrorMessage("PostEffect::ComCreate Error!!"); return false;
	}
	if (FAILED(device->createDefaultResourceTEXTURE2D_UNORDERED_ACCESS(mOutputBuffer.GetAddressOf(), dx->getClientWidth(), dx->getClientHeight()))) {
		Dx_Util::ErrorMessage("PostEffect::ComCreate Error!!"); return false;
	}
	if (FAILED(device->createDefaultResourceTEXTURE2D(mInputBuffer.GetAddressOf(),
		dx->getClientWidth(), dx->getClientHeight(), DXGI_FORMAT_R8G8B8A8_UNORM))) {

		Dx_Util::ErrorMessage("PostEffect::ComCreate Error!!"); return false;
	}
	mOutputBuffer.Get()->SetName(Dx_Util::charToLPCWSTR("mOutputBuffer", objName));
	mInputBuffer.Get()->SetName(Dx_Util::charToLPCWSTR("mInputBuffer", objName));

	mDepthHandleGPU = mDescHeap->GetGPUDescriptorHandleForHeapStart();
	mInputHandleGPU = mDepthHandleGPU;
	mOutputHandleGPU = mDepthHandleGPU;
	mInputHandleGPU.ptr += dx->getCbvSrvUavDescriptorSize() * 1;
	mOutputHandleGPU.ptr += dx->getCbvSrvUavDescriptorSize() * 2;

	D3D12_CPU_DESCRIPTOR_HANDLE hDescriptor = mDescHeap->GetCPUDescriptorHandleForHeapStart();
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	srvDesc.Format = dx->getDepthStencilSrvFormat();
	srvDesc.Texture2D.MipLevels = GetDepthStencilBuffer()->GetDesc().MipLevels;

	device->getDevice()->CreateShaderResourceView(GetDepthStencilBuffer(), &srvDesc, hDescriptor);
	hDescriptor.ptr += dx->getCbvSrvUavDescriptorSize();

	device->CreateSrvTexture(hDescriptor, mInputBuffer.GetAddressOf(), 1);

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	uavDesc.Texture2D.MipSlice = 0;

	device->getDevice()->CreateUnorderedAccessView(mOutputBuffer.Get(), nullptr, &uavDesc, hDescriptor);
	hDescriptor.ptr += dx->getCbvSrvUavDescriptorSize();

	comObj->ResourceBarrier(mInputBuffer.Get(),
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_GENERIC_READ);

	comObj->ResourceBarrier(mOutputBuffer.Get(),
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	//ルートシグネチャ
	const int numCbv = 1;
	const int numPara = numDesc + numCbv;
	D3D12_ROOT_PARAMETER rootParameter[numPara] = {};
	D3D12_ROOT_PARAMETER& r = rootParameter[0];
	r.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	r.Descriptor.ShaderRegister = 0;
	r.Descriptor.RegisterSpace = 0;
	r.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	D3D12_DESCRIPTOR_RANGE sRange[numSrv] = {};
	for (int i = 0; i < numSrv; i++) {
		D3D12_DESCRIPTOR_RANGE& s = sRange[i];
		s.BaseShaderRegister = i;
		s.NumDescriptors = 1;
		s.RegisterSpace = 0;
		s.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		s.OffsetInDescriptorsFromTableStart = 0;
		D3D12_ROOT_PARAMETER& r = rootParameter[i + numCbv];
		r.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		r.DescriptorTable.NumDescriptorRanges = 1;
		r.DescriptorTable.pDescriptorRanges = &s;
		r.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	}

	D3D12_DESCRIPTOR_RANGE uRange[numUav] = {};
	for (int i = 0; i < numUav; i++) {
		D3D12_DESCRIPTOR_RANGE& u = uRange[i];
		u.BaseShaderRegister = i;
		u.NumDescriptors = 1;
		u.RegisterSpace = 0;
		u.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
		u.OffsetInDescriptorsFromTableStart = 0;
		D3D12_ROOT_PARAMETER& r = rootParameter[i + numCbv + numSrv];
		r.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		r.DescriptorTable.NumDescriptorRanges = 1;
		r.DescriptorTable.pDescriptorRanges = &u;
		r.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	}

	mRootSignatureCom = CreateRsCompute(numPara, rootParameter);
	if (mRootSignatureCom == nullptr)return false;

	createShader();
	cs = pComputeShader_Post[no].Get();
	mPSOCom[0] = CreatePsoCompute(cs, mRootSignatureCom.Get());
	if (mPSOCom[0] == nullptr)return false;

	return true;
}

void PostEffect::ComputeMosaic(int com, bool On, int size) {
	Compute(com, On, size, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
}

void PostEffect::ComputeBlur(int com, bool On, float blurX, float blurY, float blurLevel) {
	Compute(com, On, 0, blurX, blurY, blurLevel, 0.0f, 0.0f);
}

void PostEffect::ComputeDepthOfField(int com, bool On, float blurLevel, float focusDepth, float focusRange) {
	Compute(com, On, 0, 0.0f, 0.0f, blurLevel, focusDepth, focusRange);
}

void PostEffect::ComputeMosaic(bool On, int size) {
	ComputeMosaic(com_no, On, size);
}

void PostEffect::ComputeBlur(bool On, float blurX, float blurY, float blurLevel) {
	ComputeBlur(com_no, On, blurX, blurY, blurLevel);
}

void PostEffect::ComputeDepthOfField(bool On, float blurLevel, float focusDepth, float focusRange) {
	ComputeDepthOfField(com_no, On, blurLevel, focusDepth, focusRange);
}

void PostEffect::ComputeBloom(int com, bool On, float bloomStrength, float thresholdLuminance) {
	bloom->BloomStrength = bloomStrength;
	bloom->ThresholdLuminance = thresholdLuminance;
	bloom->Compute(com, On);
}

void PostEffect::Compute(int com, bool On, int size,
	float blurX, float blurY, float blurLevel,
	float focusDepth, float focusRange) {

	if (!On)return;

	SetCommandList(com);
	ID3D12GraphicsCommandList* mCList = mCommandList;
	Dx_CommandListObj& d = *comObj;

	CONSTANT_BUFFER_PostMosaic cb = {};
	cb.mosaicSize.x = (float)size;
	cb.blur.x = blurX;
	cb.blur.y = blurY;
	cb.blur.z = blurLevel;
	cb.blur.w = focusDepth;
	cb.focusRange = focusRange;
	mObjectCB->CopyData(0, cb);

	mCList->SetPipelineState(mPSOCom[0].Get());

	ID3D12DescriptorHeap* descriptorHeaps[] = { mDescHeap.Get() };
	mCList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
	mCList->SetComputeRootSignature(mRootSignatureCom.Get());
	mCList->SetComputeRootConstantBufferView(0, mObjectCB->Resource()->GetGPUVirtualAddress());

	//深度バッファ
	d.delayResourceBarrierBefore(GetDepthStencilBuffer(),
		D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_DEPTH_READ);
	//バックバッファをコピー元にする
	d.delayResourceBarrierBefore(GetSwapChainBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE);
	d.delayResourceBarrierBefore(mInputBuffer.Get(),
		D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COPY_DEST);
	d.RunDelayResourceBarrierBefore();
	//現在のバックバッファをインプット用バッファにコピーする
	mCList->CopyResource(mInputBuffer.Get(), GetSwapChainBuffer());

	d.ResourceBarrier(mInputBuffer.Get(),
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);

	mCList->SetComputeRootDescriptorTable(1, mDepthHandleGPU);
	mCList->SetComputeRootDescriptorTable(2, mInputHandleGPU);
	mCList->SetComputeRootDescriptorTable(3, mOutputHandleGPU);

	D3D12_RESOURCE_BARRIER ba = {};
	ba.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;

	UINT disX = dx->getClientWidth() / 32;
	UINT disY = dx->getClientHeight() / 8;
	mCList->Dispatch(disX, disY, 1);//CS内32, 8, 1
	ba.UAV.pResource = mOutputBuffer.Get();
	mCList->ResourceBarrier(1, &ba);

	//深度バッファ
	d.delayResourceBarrierBefore(GetDepthStencilBuffer(),
		D3D12_RESOURCE_STATE_DEPTH_READ, D3D12_RESOURCE_STATE_DEPTH_WRITE);

	d.delayResourceBarrierBefore(GetSwapChainBuffer(),
		D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_COPY_DEST);
	d.delayResourceBarrierBefore(mOutputBuffer.Get(),
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_GENERIC_READ);
	d.RunDelayResourceBarrierBefore();
	//計算後バックバッファにコピー
	mCList->CopyResource(GetSwapChainBuffer(), mOutputBuffer.Get());

	d.delayResourceBarrierBefore(mOutputBuffer.Get(),
		D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	d.delayResourceBarrierBefore(GetSwapChainBuffer(),
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_RENDER_TARGET);
	d.RunDelayResourceBarrierBefore();
}
