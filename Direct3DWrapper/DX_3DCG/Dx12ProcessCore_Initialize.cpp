//*****************************************************************************************//
//**                                                                                     **//
//**                   �@�@�@          Dx12ProcessCore�N���X                             **//
//**                                   Initialize�֐�                                    **//
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
		//�R�}���h�A���P�[�^����(�R�}���h���X�g�ɐςރo�b�t�@���m�ۂ���Obj)
		if (FAILED(Dx12Process::dx->md3dDevice->CreateCommandAllocator(
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			IID_PPV_ARGS(mCmdListAlloc[i].GetAddressOf())))) {
			ErrorMessage("CreateCommandAllocator Error");
			return false;
		}
	}

	//�R�}���h���X�g����
	if (FAILED(Dx12Process::dx->md3dDevice->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		mCmdListAlloc[0].Get(),
		nullptr,
		IID_PPV_ARGS(mCommandList.GetAddressOf())))) {
		ErrorMessage("CreateCommandList Error");
		return false;
	}

	//�ŏ��͕��������ǂ�
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
	//�R�}���h�N���[�Y
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
	//�����܂łŃR�}���h�L���[���I�����Ă��܂���
	//���̃C�x���g����������������Ȃ��\���L��ׁ�if�Ń`�F�b�N���Ă���
	//GetCompletedValue():Fence����UINT64�̃J�E���^�擾(�����l0)
	if (mFence->GetCompletedValue() < mCurrentFence) {
		HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);
		//����Fence�ɂ�����,mCurrentFence �̒l�ɂȂ�����C�x���g�𔭉΂�����
		mFence->SetEventOnCompletion(mCurrentFence, eventHandle);
		//�C�x���g�����΂���܂ő҂�(GPU�̏����҂�)����ɂ��GPU��̑S�R�}���h���s�I���܂ő҂�����
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}
}

void Dx12Process::WaitFence(bool mode) {
	fenceMode = mode;
	//�N���[�Y�ナ�X�g�ɉ�����
	for (int i = 0; i < COM_NO; i++) {
		if (dx_sub[i].mComState != CLOSE)continue;
		ID3D12CommandList* cmdsLists[] = { dx_sub[i].mCommandList.Get() };
		mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);
		dx_sub[i].mComState = USED;
	}

	//�C���N�������g���ꂽ���Ƃŕ`�抮���Ɣ��f
	mCurrentFence++;
	//GPU��Ŏ��s����Ă���R�}���h������,�����I�Ƀt�F���X��mCurrentFence����������
	//(mFence->GetCompletedValue()�œ�����l��mCurrentFence�Ɠ����ɂȂ�)
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

	//�|�X�g�G�t�F�N�g
	pComputeShader_Post[0] = CompileShader(ShaderPostEffect, strlen(ShaderPostEffect), "MosaicCS", "cs_5_0");
	pComputeShader_Post[1] = CompileShader(ShaderPostEffect, strlen(ShaderPostEffect), "BlurCS", "cs_5_0");

	//�X�L�����b�V��
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
	//�e�Z���[�^�[�L
	pVertexShader_SKIN_D = CompileShader(SkinD.str, SkinD.size, "VS", "vs_5_0");

	//�X�g���[���o�̓f�[�^��`(�p�[�e�B�N���p)
	pDeclaration_PSO =
	{
		{ 0, "POSITION", 0, 0, 3, 0 }, //�ux,y,z�v���X���b�g�u0�v�́uPOSITION�v�ɏo��
		{ 0, "POSITION", 1, 0, 3, 0 },
		{ 0, "POSITION", 2, 0, 3, 0 },
		{ 0, "COLOR", 0, 0, 4, 0 }
	};
	//�X�g���[���o��
	pVertexShader_PSO = CompileShader(ShaderParticle, strlen(ShaderParticle), "VS_SO", "vs_5_0");
	pGeometryShader_PSO = CompileShader(ShaderParticle, strlen(ShaderParticle), "GS_Point_SO", "gs_5_0");

	//�p�[�e�B�N�����_�C���v�b�g���C�A�E�g���`
	pVertexLayout_P =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "POSITION", 1, DXGI_FORMAT_R32G32B32_FLOAT, 0, 4 * 3, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "POSITION", 2, DXGI_FORMAT_R32G32B32_FLOAT, 0, 4 * 3 * 2, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 4 * 3 * 3, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};
	//�p�[�e�B�N��
	pVertexShader_P = CompileShader(ShaderParticle, strlen(ShaderParticle), "VS", "vs_5_0");
	pGeometryShader_P = CompileShader(ShaderParticle, strlen(ShaderParticle), "GS_Point", "gs_5_0");
	pPixelShader_P = CompileShader(ShaderParticle, strlen(ShaderParticle), "PS", "ps_5_0");

	//���b�V�����C�A�E�g
	pVertexLayout_MESH =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "GEO_NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 36, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};
	//���b�V��
	pVertexShader_MESH = CompileShader(Mesh.str, Mesh.size, "VSMesh", "vs_5_0");
	//�e�Z���[�^�[�L���b�V��
	pVertexShader_MESH_D = CompileShader(MeshD.str, MeshD.size, "VSMesh", "vs_5_0");

	//3D���C�A�E�gTexture�L��
	pVertexLayout_3D =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 4 * 3,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 4 * 3 * 2, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};
	//3D���C�A�E�g��{�F
	pVertexLayout_3DBC =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 4 * 3, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};
	//�e�N�X�`��3D
	pVertexShader_TC = CompileShader(D3.str, D3.size, "VSTextureColor", "vs_5_0");
	//��{�F3D
	pVertexShader_BC = CompileShader(D3.str, D3.size, "VSBaseColor", "vs_5_0");
	pPixelShader_BC = CompileShader(D3.str, D3.size, "PSBaseColor", "ps_5_0");
	//�e�Z���[�^
	pVertexShader_DISP = CompileShader(Disp.str, Disp.size, "VSDisp", "vs_5_0");
	pHullShader_DISP = CompileShader(Disp.str, Disp.size, "HSDisp", "hs_5_0");
	pDomainShader_DISP = CompileShader(Disp.str, Disp.size, "DSDisp", "ds_5_0");
	//Wave
	pComputeShader_Wave = CompileShader(ShaderWaveCom, strlen(ShaderWaveCom), "CS", "cs_5_0");
	pVertexShader_Wave = CompileShader(Wave.str, Wave.size, "VSWave", "vs_5_0");
	pHullShader_Wave = CompileShader(Wave.str, Wave.size, "HSWave", "hs_5_0");
	pDomainShader_Wave = CompileShader(Wave.str, Wave.size, "DSWave", "ds_5_0");

	//2D���C�A�E�g
	pVertexLayout_2D =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 4 * 3, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 4 * 3 + 4 * 4, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};
	//�e�N�X�`��2D
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
				sprintf(str, "�e�N�X�`����%d�ǂݍ��݃G���[", (i));
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
				sprintf(str, "�e�N�X�`����%d�ǂݍ��݃G���[", (i));
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
			sprintf(str, "texture[%d]�ǂݍ��݃G���[", (i));
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
			sprintf(str, "textureUp[%d]�ǂݍ��݃G���[", (i));
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
	//�f�o�b�O���̓f�o�b�O���C���[��L��������
	{
		ComPtr<ID3D12Debug> debugController;
		if (FAILED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
			ErrorMessage("D3D12GetDebugInterface Error");
			return false;
		}
		debugController->EnableDebugLayer();
	}
#endif

	//�t�@�N�g������
	//�A�_�v�^�[�̗񋓁A�X���b�v �`�F�[���̍쐬�A
	//����ёS��ʕ\�����[�h�Ƃ̊Ԃ̐؂�ւ��Ɏg�p����� Alt + 
	//Enter �L�[ �V�[�P���X�Ƃ̃E�B���h�E�̊֘A�t�����s���I�u�W�F�N�g�𐶐����邽�߂Ɏg�p
	if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&mdxgiFactory)))) {
		ErrorMessage("CreateDXGIFactory1 Error");
		return false;
	}

	//�n�[�h�E�G�A�����\��,�n�[�h�E�G�A�����f�o�C�X����
	HRESULT hardwareResult = D3D12CreateDevice(
		nullptr,             // default adapter
		D3D_FEATURE_LEVEL_11_0,
		IID_PPV_ARGS(&md3dDevice));

	//���n�[�h�E�G�A�����s�̏ꍇ�\�t�g�E�G�A�����ɂ���,�\�t�g�E�G�A�����f�o�C�X����
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

	//�t�F���X����
	//Command Queue�ɑ��M����Command List�̊��������m���邽�߂Ɏg�p
	if (FAILED(md3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE,
		IID_PPV_ARGS(&mFence)))) {
		ErrorMessage("CreateFence Error");
		return false;
	}

	//Descriptor �̃A�h���X�v�Z�Ŏg�p
	mRtvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	mDsvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	mCbvSrvUavDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	//MultiSample���x���`�F�b�N
	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msQualityLevels;
	msQualityLevels.Format = mBackBufferFormat;
	msQualityLevels.SampleCount = 4;
	msQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
	msQualityLevels.NumQualityLevels = 0;
	if (FAILED(md3dDevice->CheckFeatureSupport(
		D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
		&msQualityLevels,     //�@�\�̃T�|�[�g�������f�[�^���i�[
		sizeof(msQualityLevels)))) {
		ErrorMessage("CheckFeatureSupport Error");
		return false;
	}

	m4xMsaaQuality = msQualityLevels.NumQualityLevels;
	assert(m4xMsaaQuality > 0 && "Unexpected MSAA quality level.");

	//�R�}���h�L���[�f�X�N
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT; //���ڃR�}���h�L���[
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE; //GPU�^�C���A�E�g���L��
	//�R�}���h�҂��s�񐶐�
	if (FAILED(md3dDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mCommandQueue)))) {
		ErrorMessage("CreateCommandQueue Error");
		return false;
	}

	for (int i = 0; i < COM_NO; i++) {
		//�R�}���h�A���P�[�^,�R�}���h���X�g����
		if (!dx_sub[i].ListCreate())return false;
	}

	//������
	mSwapChain.Reset();

	//�X���b�v�`�F�C������
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

	//Descriptor:���̃o�b�t�@�����L�q�����
	//�X���b�v�`�F�C����RenderTarget�Ƃ��Ďg�p���邽�߂�DescriptorHeap���쐬(Descriptor�̋L�^�ꏊ)
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
	rtvHeapDesc.NumDescriptors = SwapChainBufferCount;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;   //RenderTargetView
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE; //�V�F�[�_����A�N�Z�X���Ȃ��̂�NONE��OK
	rtvHeapDesc.NodeMask = 0;
	if (FAILED(md3dDevice->CreateDescriptorHeap(
		&rtvHeapDesc, IID_PPV_ARGS(mRtvHeap.GetAddressOf())))) {
		ErrorMessage("CreateDescriptorHeap Error");
		return false;
	}

	//�[�x�X�e���V���r��-DescriptorHeap���쐬
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

	//�����_�[�^�[�Q�b�g�r���[�f�X�N���v�^�[�q�[�v�̊J�n�n���h���擾
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(mRtvHeap->GetCPUDescriptorHandleForHeapStart());
	for (UINT i = 0; i < SwapChainBufferCount; i++) {
		//�X���b�v�`�F�C���o�b�t�@�擾
		if (FAILED(mSwapChain->GetBuffer(i, IID_PPV_ARGS(&mSwapChainBuffer[i])))) {
			ErrorMessage("GetSwapChainBuffer Error");
			return false;
		}
		//�����_�[�^�[�Q�b�g�r���[(Descriptor)����,DescriptorHeap��Descriptor���L�^�����
		md3dDevice->CreateRenderTargetView(mSwapChainBuffer[i].Get(), nullptr, rtvHeapHandle);
		//�q�[�v�ʒu�I�t�Z�b�g(2����̂�2�ڋL�^�ʒu��DescriptorSize���I�t�Z�b�g)
		rtvHeapHandle.Offset(1, mRtvDescriptorSize);
	}

	//�f�v�X�X�e���V���r���[DESC
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
	//�[�x�X�e���V���o�b�t�@�̈�m��
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

	//�[�x�X�e���V���r���[�f�X�N���v�^�[�q�[�v�̊J�n�n���h���擾
	CD3DX12_CPU_DESCRIPTOR_HANDLE mDsvHeapHeapHandle(mDsvHeap->GetCPUDescriptorHandleForHeapStart());
	//�[�x�X�e���V���r���[����
	md3dDevice->CreateDepthStencilView(mDepthStencilBuffer.Get(), nullptr, mDsvHeapHeapHandle);

	dx_sub[0].Bigin();

	//�[�x�X�e���V���o�b�t�@,���\�[�X�o���A���L���[�x��������
	dx_sub[0].mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mDepthStencilBuffer.Get(),
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE));

	dx_sub[0].End();
	WaitFenceCurrent();

	//�r���[�|�[�g
	mScreenViewport.TopLeftX = 0;
	mScreenViewport.TopLeftY = 0;
	mScreenViewport.Width = static_cast<float>(mClientWidth);
	mScreenViewport.Height = static_cast<float>(mClientHeight);
	mScreenViewport.MinDepth = 0.0f;
	mScreenViewport.MaxDepth = 1.0f;

	mScissorRect = { 0, 0, mClientWidth, mClientHeight };

	MatrixIdentity(&mProj);
	MatrixIdentity(&mView);
	//�J������p
	ViewY_theta = 55.0f;
	// �A�X�y�N�g��̌v�Z
	aspect = (float)mScreenViewport.Width / (float)mScreenViewport.Height;
	//near�v���[��
	NearPlane = 1.0f;
	//far�v���[��
	FarPlane = 10000.0f;
	MatrixPerspectiveFovLH(&mProj, ViewY_theta, aspect, NearPlane, FarPlane);

	//�r���[�|�[�g�s��쐬(3D���W��2D���W�ϊ��Ɏg�p)
	MatrixViewPort(&Vp, mClientWidth, mClientHeight);

	//�|�C���g���C�g�\���̏�����
	ResetPointLight();

	//���s����������
	dlight.Direction.as(0.0f, 0.0f, 0.0f, 0.0f);
	dlight.LightColor.as(1.0f, 1.0f, 1.0f, 1.0f);
	dlight.Lightst.as(1.0f, 0.0f, 0.3f, 0.0f);

	//�t�H�O������
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

	//�J�����̈ʒu�ƕ�����ݒ�
	MatrixLookAtLH(&mView,
		cx1, cy1, cz1,   //�J�����̈ʒu
		cx2, cy2, cz2,   //�J�����̕�����������_
		0.0f, 0.0f, 1.0f); //�J�����̏�̕���(�ʏ펋�_�ł̏������1.0f�ɂ���)
	//�V�F�[�_�[�v�Z�p���W�o�^(���_����̋����Ŏg��)
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
		ErrorMessage("lightNum�̒l���͈͊O�ł�");
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

	//�f�t�H���g�o�b�t�@����
	md3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(byteSize),
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(defaultBuffer.GetAddressOf()));

	//�f�t�H���g�o�b�t�@��CPU�������f�[�^���R�s�[���邽�߂̒��ԃo�b�t�@����
	md3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(byteSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(uploadBuffer.GetAddressOf()));

	//�f�t�H���g�o�b�t�@�ɃR�s�[�������f�[�^�̓��e
	D3D12_SUBRESOURCE_DATA subResourceData = {};
	subResourceData.pData = initData;
	subResourceData.RowPitch = (LONG_PTR)byteSize;
	subResourceData.SlicePitch = subResourceData.RowPitch;

	//�f�t�H���g�o�b�t�@,���\�[�X�o���A���L���R�s�[����鑤
	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(),
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST));
	//UpdateSubresources:���ԃo�b�t�@��CPU���������R�s�[����
	//���̌�CommandList���g���ƒ��ԃo�b�t�@�̓��e���A�b�v���[�h�����
	UpdateSubresources<1>(cmdList, defaultBuffer.Get(), uploadBuffer.Get(), 0, 0, 1, &subResourceData);
	//�f�t�H���g�o�b�t�@,���\�[�X�o���A�R�s�[����鑤���A�b�v���[�h
	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(),
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ));

	return defaultBuffer;
}

Microsoft::WRL::ComPtr<ID3D12Resource> Dx12Process::CreateStreamBuffer(UINT64 byteSize)
{
	ComPtr<ID3D12Resource> defaultBuffer;

	//�X�g���[���o�b�t�@����
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

	//�g��k��
	MatrixScaling(&scale, sizeX, sizeY, sizeZ);
	//�\���ʒu
	MatrixRotationZ(&rotZ, thetaZ);
	MatrixRotationY(&rotY, thetaY);
	MatrixRotationX(&rotX, thetaX);
	MatrixMultiply(&rotZY, &rotZ, &rotY);
	MatrixMultiply(&rotZYX, &rotZY, &rotX);
	MatrixTranslation(&mov, x, y, z);
	MatrixMultiply(&scro, &rotZYX, &scale);
	MatrixMultiply(&world, &scro, &mov);

	//���[���h�A�J�����A�ˉe�s��A��
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
		//�t�@�C�����݂̂ł͖����ꍇ�̏���
		while (*pass != '\0') pass++;//�I�[�����܂Ń|�C���^��i�߂�
		while (*pass != '\\' && *pass != '/')pass--;//�t�@�C�����擪��'\'��'/'�܂Ń|�C���^��i�߂�
		pass++;//'\'�܂���'/'�̎�(�t�@�C�����擪����)�܂Ń|�C���^��i�߂�
	}

	return pass;//�|�C���^���삵�Ă�̂ŕԂ�l���g�p������
}

//�ړ��ʈ�艻
DWORD T_float::f[2] = { timeGetTime() };
DWORD T_float::time[2] = { 0 };
DWORD T_float::time_fps[2] = { 0 };//FPS�v���p
int T_float::frame[2] = { 0 };     //FPS�v���g�p
int T_float::up = 0;
char T_float::str[50];     //�E�C���h�E�g�����\���g�p
float T_float::adj = 1.0f;

void T_float::GetTime(HWND hWnd) {
	time[0] = timeGetTime() - f[0];
	f[0] = timeGetTime();

	//FPS�v��
	frame[0]++;
	sprintf(str, "     Ctrl:����  Delete:�L�����Z��  fps=%d  ups=%d", frame[0], up);
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

	//UPS�v��
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

//�G���[���b�Z�[�W
void ErrorMessage(char *E_mes) {
	MessageBoxA(0, E_mes, 0, MB_OK);
}