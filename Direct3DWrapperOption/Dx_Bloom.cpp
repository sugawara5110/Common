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
		Dx_Resource mOutputBuffer = {};
		Dx_Resource* mpInputBuffer = nullptr;
		Dx_Resource* mpInstanceIdMapBuffer = nullptr;

		struct gInOut {
			D3D12_GPU_DESCRIPTOR_HANDLE mLuminanceHandleGPU = {};
			D3D12_GPU_DESCRIPTOR_HANDLE mGaussInOutHandleGPU0 = {};
			D3D12_GPU_DESCRIPTOR_HANDLE mGaussInOutHandleGPU1 = {};

			Dx_Resource mLuminanceBuffer = {};
			Dx_Resource mGaussInOutBuffer0 = {};
			Dx_Resource mGaussInOutBuffer1 = {};
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

		bool GaussianCreate(int comIndex, std::vector<uint32_t>gausSize) {
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

			Dx_CommandManager& ma = *Dx_CommandManager::GetInstance();
			GaussianFilterBuffer = ma.CreateDefaultBuffer(comIndex, gaArr,
				GaussianBufferSize * sizeof(float), GaussianFilterBufferUp, false);
			if (!GaussianFilterBuffer) {
				Dx_Util::ErrorMessage("Bloom::GaussianCreate Error!!");
				return false;
			}

			Dx_Device* device = Dx_Device::GetInstance();

			GaussInout.setPara(gausSize);

			for (UINT i = 0; i < GaussInout.numGauss; i++) {
				UINT s = GaussInout.gaSizeArr[i];
				if (FAILED(GaussInout.gInout[i].mGaussInOutBuffer0.createDefaultResourceTEXTURE2D_UNORDERED_ACCESS(s, s))) {
					Dx_Util::ErrorMessage("Bloom::GaussianCreate Error!!");
					return false;
				}
				if (FAILED(GaussInout.gInout[i].mGaussInOutBuffer1.createDefaultResourceTEXTURE2D_UNORDERED_ACCESS(s, s))) {
					Dx_Util::ErrorMessage("Bloom::GaussianCreate Error!!");
					return false;
				}

				if (FAILED(GaussInout.gInout[i].mLuminanceBuffer.createDefaultResourceTEXTURE2D_UNORDERED_ACCESS(s, s))) {
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
			int comIndex,
			Dx_Resource* InputBuffer,
			Dx_Resource* InstanceIdMapBuffer,
			std::vector<UINT>gaBaseSize = { 256,128,64,32,16 }) {

			Dx_Device* device = Dx_Device::GetInstance();
			createSampler();
			createShader();
			if (!createDescHeap())return false;
			mObjectCB = new ConstantBuffer<CONSTANT_BUFFER_Bloom>(1);

			Dx12Process* dx = Dx12Process::GetInstance();

			if (FAILED(mOutputBuffer.createDefaultResourceTEXTURE2D_UNORDERED_ACCESS(dx->getClientWidth(), dx->getClientHeight()))) {
				Dx_Util::ErrorMessage("Bloom::ComCreate Error!!"); return false;
			}
			mpInputBuffer = InputBuffer;
			mpInstanceIdMapBuffer = InstanceIdMapBuffer;
			mOutputBuffer.getResource()->SetName(Dx_Util::charToLPCWSTR("mOutputBuffer1", objName));

			if (!GaussianCreate(comIndex, gaBaseSize))return false;

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
			mpInputBuffer->CreateSrvTexture(hDescriptor);
			mpInstanceIdMapBuffer->CreateSrvTexture(hDescriptor);

			mOutputBuffer.CreateUavTexture(hDescriptor);
			for (UINT i = 0; i < GaussInout.numGauss; i++) {
				GaussInout.gInout[i].mGaussInOutBuffer0.CreateUavTexture(hDescriptor);
				GaussInout.gInout[i].mGaussInOutBuffer1.CreateUavTexture(hDescriptor);
				GaussInout.gInout[i].mLuminanceBuffer.CreateUavTexture(hDescriptor);
			}
			for (UINT i = 0; i < GaussInout.numGauss; i++)
				GaussInout.gInout[i].mGaussInOutBuffer1.CreateSrvTexture(hDescriptor);
			//↑シェーダーで配列として使うので連続で並べる必要がある

			mOutputBuffer.ResourceBarrier(comIndex, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

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

			Dx_CommandListObj* cObj = Dx_CommandManager::GetInstance()->getGraphicsComListObj(comIndex);

			ID3D12GraphicsCommandList* mCList = cObj->getCommandList();

			Dx12Process* dx = Dx12Process::GetInstance();

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
				UINT gauX = (UINT)GaussInout.gInout[i].mGaussInOutBuffer0.Width / 8;
				UINT gauY = GaussInout.gInout[i].mGaussInOutBuffer0.Height / 8;

				mCList->SetComputeRootDescriptorTable(6, GaussInout.gInout[i].mGaussInOutHandleGPU0);
				mCList->SetComputeRootDescriptorTable(7, GaussInout.gInout[i].mGaussInOutHandleGPU1);
				mCList->SetComputeRootDescriptorTable(8, GaussInout.gInout[i].mLuminanceHandleGPU);

				mCList->SetPipelineState(mPSOCom[0].Get());
				mCList->Dispatch(gauX, gauY, 1);//CS内8, 8, 1
				ba.UAV.pResource = GaussInout.gInout[i].mLuminanceBuffer.getResource();
				mCList->ResourceBarrier(1, &ba);

				if (gaussianType == Dx_Bloom::GaussianType::Type1D) {
					mCList->SetPipelineState(mPSOCom[1].Get());
					mCList->Dispatch(gauX, gauY, 1);
					ba.UAV.pResource = GaussInout.gInout[i].mGaussInOutBuffer0.getResource();
					mCList->ResourceBarrier(1, &ba);

					mCList->SetPipelineState(mPSOCom[2].Get());
					mCList->Dispatch(gauX, gauY, 1);
					ba.UAV.pResource = GaussInout.gInout[i].mGaussInOutBuffer1.getResource();
					mCList->ResourceBarrier(1, &ba);
				}
				else {
					mCList->SetPipelineState(mPSOCom[3].Get());
					mCList->Dispatch(gauX, gauY, 1);
					ba.UAV.pResource = GaussInout.gInout[i].mGaussInOutBuffer1.getResource();
					mCList->ResourceBarrier(1, &ba);
				}
			}

			mCList->SetComputeRootDescriptorTable(3, GaussInout.mGaussTexHandleGPU);
			mCList->SetPipelineState(mPSOCom[4].Get());
			mCList->Dispatch(disX, disY, 1);
			ba.UAV.pResource = mOutputBuffer.getResource();
			mCList->ResourceBarrier(1, &ba);
		}

		Dx_Resource* getOutputBuffer() {
			return &mOutputBuffer;
		}
	};
}

namespace {
	std::unique_ptr<Bloom[]> bloom = {};
}

Dx_Bloom::~Dx_Bloom() {
	S_DELETE(mObjectCB);
}

bool Dx_Bloom::createBuffer(int comIndex) {

	Dx_Device* device = Dx_Device::GetInstance();
	Dx12Process* dx = Dx12Process::GetInstance();

	mObjectCB = new ConstantBuffer<CONSTANT_BUFFER_Bloom2>(1);

	if (FAILED(mOutputBuffer.createDefaultResourceTEXTURE2D_UNORDERED_ACCESS(dx->getClientWidth(), dx->getClientHeight()))) {
		Dx_Util::ErrorMessage("Bloom::ComCreate Error!!"); return false;
	}

	mOutputBuffer.getResource()->SetName(Dx_Util::charToLPCWSTR("mOutputBuffer", objName));

	if (FAILED(mInputBuffer.createDefaultResourceTEXTURE2D_UNORDERED_ACCESS(dx->getClientWidth(), dx->getClientHeight()))) {
		Dx_Util::ErrorMessage("Bloom::ComCreate Error!!"); return false;
	}

	mInputBuffer.getResource()->SetName(Dx_Util::charToLPCWSTR("mInputBuffer", objName));

	mInputBuffer.ResourceBarrier(comIndex, D3D12_RESOURCE_STATE_GENERIC_READ);

	mOutputBuffer.ResourceBarrier(comIndex, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	return true;
}

bool Dx_Bloom::createPipeline() {

	Dx_Device* device = Dx_Device::GetInstance();

	mDescHeap = device->CreateDescHeap(3 + cbb.numInstance);
	D3D12_CPU_DESCRIPTOR_HANDLE  hDescriptor = mDescHeap->GetCPUDescriptorHandleForHeapStart();

	D3D12_GPU_VIRTUAL_ADDRESS Address[1] = { mObjectCB->getGPUVirtualAddress(0) };
	UINT size[1] = { mObjectCB->getSizeInBytes() };
	device->CreateCbv(hDescriptor, Address, size, 1);
	mInputBuffer.CreateSrvTexture(hDescriptor);

	for (int i = 0; i < cbb.numInstance; i++) {
		Dx_Resource* res = bloom[i].getOutputBuffer();
		res->CreateSrvTexture(hDescriptor);
	}

	mOutputBuffer.CreateUavTexture(hDescriptor);

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

bool Dx_Bloom::Create(int comIndex,
	uint32_t numInstance, Dx_Resource* InstanceIdMapBuffer,
	std::vector<float>* sigma,
	std::vector<std::vector<uint32_t>>* gausSizes,
	std::vector<Dx_Bloom::GaussianType>* GaussianType) {

	Dx_CommandManager* cMa = Dx_CommandManager::GetInstance();
	Dx_CommandListObj& d = *cMa->getGraphicsComListObj(comIndex);

	d.Bigin();

	if (!createBuffer(comIndex))return false;

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
			bloom[i].ComCreate(comIndex, &mInputBuffer, InstanceIdMapBuffer, (*gausSizes)[i]);
		}
		else {
			bloom[i].ComCreate(comIndex, &mInputBuffer, InstanceIdMapBuffer);
		}
	}

	if (!createPipeline())return false;

	d.End();
	cMa->RunGpu();
	cMa->WaitFence();

	return true;
}

void Dx_Bloom::Compute(
	uint32_t comIndex,
	std::vector<InstanceParam> instanceParams,
	Dx_Resource* inout) {

	Dx_CommandManager* cMa = Dx_CommandManager::GetInstance();
	Dx_CommandListObj& d = *cMa->getGraphicsComListObj(comIndex);
	ID3D12GraphicsCommandList* mCList = d.getCommandList();
	Dx12Process* dx = Dx12Process::GetInstance();

	d.Bigin();

	mInputBuffer.CopyResource(comIndex, inout);

	d.End();
	cMa->RunGpu();
	cMa->WaitFence();

	for (int i = 0; i < cbb.numInstance; i++) {
		d.Bigin();
		bloom[i].Compute(
			comIndex,
			instanceParams[i].EmissiveInstanceId,
			instanceParams[i].thresholdLuminance,
			instanceParams[i].bloomStrength
		);
		d.End();
		cMa->RunGpu();
		cMa->WaitFence();
	}

	d.Bigin();

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
	ba.UAV.pResource = mOutputBuffer.getResource();
	mCList->ResourceBarrier(1, &ba);

	inout->CopyResource(comIndex, &mOutputBuffer);

	d.End();
	cMa->RunGpu();
	cMa->WaitFence();
}
