//*****************************************************************************************//
//**                                                                                     **//
//**                   　　　       Dx_Bloom                                             **//
//**                                                                                     **//
//*****************************************************************************************//

#include "Dx_Bloom.h"
#include "Shader/ShaderBloom.h"

namespace {

	char* changeStr(char* srcStr, char* target, char* replacement) {
		uint32_t ln = (uint32_t)strlen(srcStr);
		uint32_t taln = (uint32_t)strlen(target);
		uint32_t reln = (uint32_t)strlen(replacement);
		uint32_t retln = ln - taln + reln;

		char* ret = new char[retln + 1];
		int retCnt = 0;
		for (uint32_t i = 0; i < ln; i++) {
			if (!strncmp(&srcStr[i], target, sizeof(char) * taln)) {
				memcpy(&ret[retCnt], replacement, sizeof(char) * reln);
				retCnt += reln;
				i += (taln - 1);
			}
			else {
				ret[retCnt] = srcStr[i];
				retCnt++;
			}
		}
		ret[retln] = '\0';
		return ret;
	}

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
		mpSamplerHeap.Get()->SetName(L"mpSamplerHeap");
		samplerHandle = mpSamplerHeap.Get()->GetGPUDescriptorHandleForHeapStart();

		createSamplerDone = true;
	}

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

	private:
		ComPtr<ID3D12DescriptorHeap> mDescHeap = nullptr;
		static const UINT MaxnumGauss = 10;
		int numDescSrv = 3 + MaxnumGauss;
		int numDescUav = 1 + MaxnumGauss * 3;
		int numDesc = numDescSrv + numDescUav;

		UINT64 gpuHandle;
		D3D12_CPU_DESCRIPTOR_HANDLE hDescriptor;

		bool createDescHeap() {
			Dx_Device* device = Dx_Device::GetInstance();
			mDescHeap = device->CreateDescHeap(numDesc);
			mDescHeap.Get()->SetName(L"mDescHeap");
			if (!mDescHeap) {
				Dx_Util::ErrorMessage("Dx_Bloom::ComCreate Error!!"); return false;
			}

			gpuHandle = mDescHeap->GetGPUDescriptorHandleForHeapStart().ptr;
			hDescriptor = mDescHeap->GetCPUDescriptorHandleForHeapStart();

			return true;
		}

		void setDescriptorHeaps(ID3D12GraphicsCommandList* mCList) {
			ID3D12DescriptorHeap* descriptorHeaps[] = { mDescHeap.Get() ,mpSamplerHeap.Get() };
			mCList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
		}

		struct CONSTANT_BUFFER_Bloom {
			float GaussianWid;//ガウス幅
			float bloomStrength;//ブルーム強さ
			float thresholdLuminance;//輝度閾値
			float numGaussFilter;//ガウスフィルター数
			int   InstanceID;//処理対象ID
		};

		ComPtr<ID3D12RootSignature> mRootSignatureCom = nullptr;
		ComPtr<ID3DBlob> pComputeShader_Post[5] = {};
		ComPtr<ID3D12PipelineState> mPSOCom[5] = {};

		D3D12_GPU_DESCRIPTOR_HANDLE mGaussianFilterHandleGPU = {};
		D3D12_GPU_DESCRIPTOR_HANDLE mInputHandleGPU = {};
		D3D12_GPU_DESCRIPTOR_HANDLE mOutputHandleGPU = {};
		D3D12_GPU_DESCRIPTOR_HANDLE mInstanceIdMapHandleGPU = {};

		ComPtr<ID3D12Resource> GaussianFilterBufferUp = nullptr;
		ComPtr<ID3D12Resource> GaussianFilterBuffer = nullptr;
		ComPtr<ID3D12Resource> mOutputBuffer = nullptr;
		ID3D12Resource* mpInputBuffer = nullptr;
		ID3D12Resource* mpInstanceIdMapBuffer = nullptr;

		struct gInOut {
			D3D12_GPU_DESCRIPTOR_HANDLE mLuminanceHandleGPU = {};
			D3D12_GPU_DESCRIPTOR_HANDLE mGaussInOutHandleGPU0 = {};
			D3D12_GPU_DESCRIPTOR_HANDLE mGaussInOutHandleGPU1 = {};

			ComPtr<ID3D12Resource> mLuminanceBuffer = nullptr;
			ComPtr<ID3D12Resource> mGaussInOutBuffer0 = nullptr;
			ComPtr<ID3D12Resource> mGaussInOutBuffer1 = nullptr;
		};

		struct GaussInOut {
		public:
			UINT numGauss = 1;
			std::unique_ptr<gInOut[]> gInout = nullptr;
			D3D12_GPU_DESCRIPTOR_HANDLE mGaussTexHandleGPU = {};

			std::vector<uint32_t> gaSizeArr;

			void setPara(std::vector<uint32_t>gausSize) {
				gaSizeArr = gausSize;
				numGauss = (UINT)gaSizeArr.size();
				if (numGauss > MaxnumGauss)numGauss = MaxnumGauss;
				gInout = std::make_unique<gInOut[]>(numGauss);
			}
		};
		GaussInOut GaussInout = {};

		int GaussianWid = 0;
		Dx_Bloom::GaussianType gaussianType = Dx_Bloom::GaussianType::Type1D;
		float Sigma = 1.0f;

		ConstantBuffer<CONSTANT_BUFFER_Bloom>* mObjectCB = nullptr;

		void createShader() {
			LPSTR csName[5] = {
				"BloomCS0",
				"BloomCS1",
				"BloomCS2",
				"BloomCS12",
				"BloomCS3"
			};

			for (int i = 0; i < _countof(csName); i++)
				pComputeShader_Post[i] = CompileShader(ShaderBloom, strlen(ShaderBloom), csName[i], "cs_5_1");
		}

		bool GaussianCreate(std::vector<uint32_t>gausSize) {
			float* gaArr = nullptr;
			int GaussianBufferSize = 0;

			if (gaussianType == Dx_Bloom::GaussianType::Type1D) {
				gaArr = gaussian(Sigma, GaussianWid);
				GaussianBufferSize = GaussianWid;
			}
			else {
				gaArr = gaussian2D(Sigma, GaussianWid);
				GaussianBufferSize = GaussianWid * GaussianWid;
			}

			GaussianFilterBuffer = dx->CreateDefaultBuffer(com_no, gaArr,
				GaussianBufferSize * sizeof(float), GaussianFilterBufferUp, false);
			if (!GaussianFilterBuffer) {
				Dx_Util::ErrorMessage("Bloom::GaussianCreate Error!!");
				return false;
			}

			Dx_Device* device = Dx_Device::GetInstance();

			GaussInout.setPara(gausSize);

			for (UINT i = 0; i < GaussInout.numGauss; i++) {
				UINT s = GaussInout.gaSizeArr[i];
				if (FAILED(device->createDefaultResourceTEXTURE2D_UNORDERED_ACCESS(
					GaussInout.gInout[i].mGaussInOutBuffer0.GetAddressOf(),
					s, s))) {

					Dx_Util::ErrorMessage("Bloom::GaussianCreate Error!!");
					return false;
				}
				if (FAILED(device->createDefaultResourceTEXTURE2D_UNORDERED_ACCESS(
					GaussInout.gInout[i].mGaussInOutBuffer1.GetAddressOf(),
					s, s))) {

					Dx_Util::ErrorMessage("Bloom::GaussianCreate Error!!");
					return false;
				}

				if (FAILED(device->createDefaultResourceTEXTURE2D_UNORDERED_ACCESS(
					GaussInout.gInout[i].mLuminanceBuffer.GetAddressOf(),
					s, s))) {

					Dx_Util::ErrorMessage("Bloom::GaussianCreate Error!!");
					return false;
				}
			}

			GaussianFilterBuffer.Get()->SetName(Dx_Util::charToLPCWSTR("GaussianFilterBuffer", objName));
			GaussianFilterBufferUp.Get()->SetName(Dx_Util::charToLPCWSTR("GaussianFilterBufferUp", objName));

			ARR_DELETE(gaArr);
			return true;
		}

	public:
		~Bloom() {
			S_DELETE(mObjectCB);
		}

		void setGaussianType(Dx_Bloom::GaussianType GaussianType = Dx_Bloom::GaussianType::Type1D) {
			gaussianType = GaussianType;
		}

		void setSigma(float sigma = 10.0f) {
			Sigma = sigma;
		}

		bool ComCreate(
			ID3D12Resource* InputBuffer,
			ID3D12Resource* InstanceIdMapBuffer,
			std::vector<UINT>gaBaseSize = { 256,128,64,32,16 }) {

			Dx_Device* device = Dx_Device::GetInstance();
			createSampler();
			createShader();
			if (!createDescHeap())return false;
			mObjectCB = new ConstantBuffer<CONSTANT_BUFFER_Bloom>(1);

			if (FAILED(device->createDefaultResourceTEXTURE2D_UNORDERED_ACCESS(mOutputBuffer.GetAddressOf(),
				dx->getClientWidth(), dx->getClientHeight()))) {

				Dx_Util::ErrorMessage("Bloom::ComCreate Error!!"); return false;
			}
			mpInputBuffer = InputBuffer;
			mpInstanceIdMapBuffer = InstanceIdMapBuffer;
			mOutputBuffer.Get()->SetName(Dx_Util::charToLPCWSTR("mOutputBuffer1", objName));

			if (!GaussianCreate(gaBaseSize))return false;

			mGaussianFilterHandleGPU.ptr = gpuHandle;
			mInputHandleGPU.ptr = (gpuHandle += dx->getCbvSrvUavDescriptorSize());
			mInstanceIdMapHandleGPU.ptr = (gpuHandle += dx->getCbvSrvUavDescriptorSize());
			mOutputHandleGPU.ptr = (gpuHandle += dx->getCbvSrvUavDescriptorSize());

			for (UINT i = 0; i < GaussInout.numGauss; i++) {
				GaussInout.gInout[i].mGaussInOutHandleGPU0.ptr = (gpuHandle += dx->getCbvSrvUavDescriptorSize());
				GaussInout.gInout[i].mGaussInOutHandleGPU1.ptr = (gpuHandle += dx->getCbvSrvUavDescriptorSize());
				GaussInout.gInout[i].mLuminanceHandleGPU.ptr = (gpuHandle += dx->getCbvSrvUavDescriptorSize());
			}
			GaussInout.mGaussTexHandleGPU.ptr = (gpuHandle += dx->getCbvSrvUavDescriptorSize());
			gpuHandle += dx->getCbvSrvUavDescriptorSize();
			//↑配列用

			UINT StructureByteStride[1] = { sizeof(float) };
			device->CreateSrvBuffer(hDescriptor, GaussianFilterBuffer.GetAddressOf(), 1, StructureByteStride);
			device->CreateSrvTexture(hDescriptor, &mpInputBuffer, 1);
			device->CreateSrvTexture(hDescriptor, &mpInstanceIdMapBuffer, 1);

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

			comObj->ResourceBarrier(mOutputBuffer.Get(),
				D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

			//ルートシグネチャ
			const int numSrv = 4;
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
				s.NumDescriptors = (i == 2) ? MaxnumGauss : 1;
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

		void Compute(uint32_t comIndex,
			uint32_t EmissiveInstanceId,
			float thresholdLuminance,
			float bloomStrength) {

			SetCommandList(comIndex);
			ID3D12GraphicsCommandList* mCList = mCommandList;
			Dx_CommandListObj& d = *comObj;

			CONSTANT_BUFFER_Bloom cb = {};
			cb.GaussianWid = (float)GaussianWid;
			cb.bloomStrength = bloomStrength;
			cb.thresholdLuminance = thresholdLuminance;
			cb.numGaussFilter = (float)GaussInout.numGauss;
			cb.InstanceID = EmissiveInstanceId;
			mObjectCB->CopyData(0, cb);

			setDescriptorHeaps(mCList);
			mCList->SetComputeRootSignature(mRootSignatureCom.Get());
			mCList->SetComputeRootConstantBufferView(0, mObjectCB->Resource()->GetGPUVirtualAddress());
			mCList->SetComputeRootDescriptorTable(1, mGaussianFilterHandleGPU);
			mCList->SetComputeRootDescriptorTable(2, mInputHandleGPU);
			mCList->SetComputeRootDescriptorTable(4, mInstanceIdMapHandleGPU);
			mCList->SetComputeRootDescriptorTable(5, mOutputHandleGPU);
			mCList->SetComputeRootDescriptorTable(9, samplerHandle);

			D3D12_RESOURCE_BARRIER ba = {};
			ba.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;

			UINT disX = dx->getClientWidth() / 32;
			UINT disY = dx->getClientHeight() / 8;

			for (UINT i = 0; i < GaussInout.numGauss; i++) {
				UINT gauX = (UINT)GaussInout.gInout[i].mGaussInOutBuffer0.Get()->GetDesc().Width / 8;
				UINT gauY = GaussInout.gInout[i].mGaussInOutBuffer0.Get()->GetDesc().Height / 8;

				mCList->SetComputeRootDescriptorTable(6, GaussInout.gInout[i].mGaussInOutHandleGPU0);
				mCList->SetComputeRootDescriptorTable(7, GaussInout.gInout[i].mGaussInOutHandleGPU1);
				mCList->SetComputeRootDescriptorTable(8, GaussInout.gInout[i].mLuminanceHandleGPU);

				mCList->SetPipelineState(mPSOCom[0].Get());
				mCList->Dispatch(gauX, gauY, 1);//CS内8, 8, 1
				ba.UAV.pResource = GaussInout.gInout[i].mLuminanceBuffer.Get();
				mCList->ResourceBarrier(1, &ba);

				if (gaussianType == Dx_Bloom::GaussianType::Type1D) {
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
		}

		ID3D12Resource* getOutputBuffer() {
			return mOutputBuffer.Get();
		}
	};
}

namespace {
	std::unique_ptr<Bloom[]> bloom = {};
}

Dx_Bloom::~Dx_Bloom() {
	S_DELETE(mObjectCB);
}

bool Dx_Bloom::createBuffer() {

	Dx_Device* device = Dx_Device::GetInstance();

	mObjectCB = new ConstantBuffer<CONSTANT_BUFFER_Bloom2>(1);

	if (FAILED(device->createDefaultResourceTEXTURE2D_UNORDERED_ACCESS(mOutputBuffer.GetAddressOf(),
		dx->getClientWidth(), dx->getClientHeight()))) {

		Dx_Util::ErrorMessage("Bloom::ComCreate Error!!"); return false;
	}

	mOutputBuffer.Get()->SetName(Dx_Util::charToLPCWSTR("mOutputBuffer", objName));

	if (FAILED(device->createDefaultResourceTEXTURE2D_UNORDERED_ACCESS(mInputBuffer.GetAddressOf(),
		dx->getClientWidth(), dx->getClientHeight()))) {

		Dx_Util::ErrorMessage("Bloom::ComCreate Error!!"); return false;
	}

	mInputBuffer.Get()->SetName(Dx_Util::charToLPCWSTR("mInputBuffer", objName));

	comObj->ResourceBarrier(mInputBuffer.Get(),
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_GENERIC_READ);

	comObj->ResourceBarrier(mOutputBuffer.Get(),
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	return true;
}

bool Dx_Bloom::createPipeline() {

	Dx_Device* device = Dx_Device::GetInstance();

	mDescHeap = device->CreateDescHeap(3 + cbb.numInstance);
	D3D12_CPU_DESCRIPTOR_HANDLE  hDescriptor = mDescHeap->GetCPUDescriptorHandleForHeapStart();

	D3D12_GPU_VIRTUAL_ADDRESS Address[1] = { mObjectCB->getGPUVirtualAddress(0) };
	UINT size[1] = { mObjectCB->getSizeInBytes() };
	device->CreateCbv(hDescriptor, Address, size, 1);
	device->CreateSrvTexture(hDescriptor, mInputBuffer.GetAddressOf(), 1);

	for (int i = 0; i < cbb.numInstance; i++) {
		ID3D12Resource* res = bloom[i].getOutputBuffer();
		device->CreateSrvTexture(hDescriptor, &res, 1);
	}

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	uavDesc.Texture2D.MipSlice = 0;

	device->getDevice()->CreateUnorderedAccessView(mOutputBuffer.Get(), nullptr, &uavDesc, hDescriptor);

	char replace[255] = {};
	snprintf(replace, sizeof(replace), "%d", cbb.numInstance);

	char* ShaderBloom2A = changeStr(ShaderBloom2, "replace_NUM_Bloom", replace);

	pShader = CompileShader(ShaderBloom2A, strlen(ShaderBloom2A), "BloomCS4", "cs_5_1");

	ARR_DELETE(ShaderBloom2A);

	UINT rCnt = 0;

	D3D12_DESCRIPTOR_RANGE cb = {};
	cb.BaseShaderRegister = 0;
	cb.NumDescriptors = 1;
	cb.RegisterSpace = 0;
	cb.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	cb.OffsetInDescriptorsFromTableStart = rCnt++;
	D3D12_DESCRIPTOR_RANGE in = {};
	in.BaseShaderRegister = 0;
	in.NumDescriptors = 1;
	in.RegisterSpace = 0;
	in.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	in.OffsetInDescriptorsFromTableStart = rCnt++;
	D3D12_DESCRIPTOR_RANGE bl = {};
	bl.BaseShaderRegister = 1;
	bl.NumDescriptors = cbb.numInstance;
	bl.RegisterSpace = 1;
	bl.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	bl.OffsetInDescriptorsFromTableStart = rCnt;
	rCnt += cbb.numInstance;
	D3D12_DESCRIPTOR_RANGE out = {};
	out.BaseShaderRegister = 0;
	out.NumDescriptors = 1;
	out.RegisterSpace = 0;
	out.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
	out.OffsetInDescriptorsFromTableStart = rCnt++;

	std::vector<D3D12_DESCRIPTOR_RANGE> range = { cb,in,bl,out };

	D3D12_ROOT_PARAMETER para = {};
	para.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	para.DescriptorTable.NumDescriptorRanges = (UINT)range.size();
	para.DescriptorTable.pDescriptorRanges = range.data();
	para.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	D3D12_DESCRIPTOR_RANGE Sampler = {};
	Sampler.BaseShaderRegister = 0;
	Sampler.NumDescriptors = 1;
	Sampler.RegisterSpace = 0;
	Sampler.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
	Sampler.OffsetInDescriptorsFromTableStart = 0;

	D3D12_ROOT_PARAMETER sam = {};
	sam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	sam.DescriptorTable.NumDescriptorRanges = 1;
	sam.DescriptorTable.pDescriptorRanges = &Sampler;
	sam.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	std::vector<D3D12_ROOT_PARAMETER> root = { para,sam };

	mRootSignatureCom = CreateRsCompute((int)root.size(), root.data());
	if (mRootSignatureCom == nullptr)return false;

	mPSO = CreatePsoCompute(pShader.Get(), mRootSignatureCom.Get());

	return true;
}

bool Dx_Bloom::Create(uint32_t numInstance, ID3D12Resource* InstanceIdMapBuffer,
	std::vector<float>* sigma,
	std::vector<std::vector<uint32_t>>* gausSizes,
	std::vector<Dx_Bloom::GaussianType>* GaussianType) {

	Dx12Process* dx = Dx12Process::GetInstance();

	dx->Bigin(0);

	if (!createBuffer())return false;

	cbb.numInstance = numInstance;
	mObjectCB->CopyData(0, cbb);
	bloom = std::make_unique<Bloom[]>(numInstance);
	for (int i = 0; i < cbb.numInstance; i++) {

		if (sigma) {
			bloom[i].setSigma((*sigma)[i]);
		}
		else {
			bloom[i].setSigma();
		}

		if (GaussianType) {
			bloom[i].setGaussianType((*GaussianType)[i]);
		}
		else {
			bloom[i].setGaussianType();
		}

		if (gausSizes) {
			bloom[i].ComCreate(mInputBuffer.Get(), InstanceIdMapBuffer, (*gausSizes)[i]);
		}
		else {
			bloom[i].ComCreate(mInputBuffer.Get(), InstanceIdMapBuffer);
		}
	}

	if (!createPipeline())return false;

	dx->End(0);
	dx->RunGpu();
	dx->WaitFence();

	return true;
}

void Dx_Bloom::Compute(
	uint32_t comIndex,
	std::vector<InstanceParam> instanceParams,
	ID3D12Resource* inout, D3D12_RESOURCE_STATES firstState) {

	SetCommandList(comIndex);
	ID3D12GraphicsCommandList* mCList = mCommandList;
	Dx_CommandListObj& d = *comObj;

	dx->Bigin(comIndex);

	//バックバッファをコピー元にする
	d.ResourceBarrier(inout,
		firstState, D3D12_RESOURCE_STATE_COPY_SOURCE);
	d.ResourceBarrier(mInputBuffer.Get(),
		D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COPY_DEST);
	//現在のバックバッファをインプット用バッファにコピーする
	mCList->CopyResource(mInputBuffer.Get(), inout);

	d.ResourceBarrier(mInputBuffer.Get(),
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);

	d.ResourceBarrier(inout,
		D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_COPY_DEST);

	dx->End(comIndex);
	dx->RunGpu();
	dx->WaitFence();

	for (int i = 0; i < cbb.numInstance; i++) {
		dx->Bigin(comIndex);
		bloom[i].Compute(
			comIndex,
			instanceParams[i].EmissiveInstanceId,
			instanceParams[i].thresholdLuminance,
			instanceParams[i].bloomStrength
		);
		dx->End(comIndex);
		dx->RunGpu();
		dx->WaitFence();
	}

	dx->Bigin(comIndex);

	mCList->SetComputeRootSignature(mRootSignatureCom.Get());

	ID3D12DescriptorHeap* descriptorHeaps[] = { mDescHeap.Get() ,mpSamplerHeap.Get() };
	mCList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	D3D12_GPU_DESCRIPTOR_HANDLE srvHandle = mDescHeap->GetGPUDescriptorHandleForHeapStart();

	mCList->SetComputeRootDescriptorTable(0, srvHandle);
	mCList->SetComputeRootDescriptorTable(1, samplerHandle);

	D3D12_RESOURCE_BARRIER ba = {};
	ba.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;

	UINT disX = dx->getClientWidth() / 32;
	UINT disY = dx->getClientHeight() / 8;

	mCList->SetPipelineState(mPSO.Get());
	mCList->Dispatch(disX, disY, 1);
	ba.UAV.pResource = mOutputBuffer.Get();
	mCList->ResourceBarrier(1, &ba);

	d.ResourceBarrier(mOutputBuffer.Get(),
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_GENERIC_READ);

	//計算後バックバッファにコピー
	mCList->CopyResource(inout, mOutputBuffer.Get());

	d.ResourceBarrier(mOutputBuffer.Get(),
		D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	d.ResourceBarrier(inout,
		D3D12_RESOURCE_STATE_COPY_DEST, firstState);

	dx->End(comIndex);
	dx->RunGpu();
	dx->WaitFence();
}
