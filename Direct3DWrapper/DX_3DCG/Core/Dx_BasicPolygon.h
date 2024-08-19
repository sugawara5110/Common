//*****************************************************************************************//
//**                                                                                     **//
//**                   　　　          BasicPolygon                                      **//
//**                                                                                     **//
//*****************************************************************************************//

#ifndef Class_BasicPolygon_Header
#define Class_BasicPolygon_Header

#include "Dx_Common.h"
#include "ParameterDXR.h"
#include "Dx_Light.h"

class MeshData;
class SkinMesh;
class SkinnedCom;

struct MY_MATERIAL_S {
	CoordTf::VECTOR4 diffuse = {};
	CoordTf::VECTOR4 specular = {};
	CoordTf::VECTOR4 ambient = {};
	CHAR difUvName[255] = {};
	CHAR norUvName[255] = {};
	CHAR speUvName[255] = {};
	int diftex_no = -1;
	int nortex_no = -1;
	int spetex_no = -1;
};

class BasicPolygon :public DxCommon {

protected:
	friend MeshData;
	friend SkinMesh;
	friend SkinnedCom;
	//ポインタで受け取る
	ID3DBlob* vs = nullptr;
	ID3DBlob* ps = nullptr;
	ID3DBlob* ps_NoMap = nullptr;
	ID3DBlob* hs = nullptr;
	ID3DBlob* ds = nullptr;
	ID3DBlob* gs = nullptr;
	ID3DBlob* gs_NoMap = nullptr;

	PrimitiveType primType_create;

	//コンスタントバッファOBJ
	ConstantBuffer<CONSTANT_BUFFER>* mObjectCB = nullptr;
	ConstantBuffer<cbInstanceID>* mObjectCB_Ins = nullptr;
	ConstantBuffer<CONSTANT_BUFFER2>* mObjectCB2 = nullptr;
	ConstantBuffer<CONSTANT_BUFFER3>* mObjectCB3 = nullptr;
	ConstantBuffer<WVP_CB>* wvp = nullptr;

	CONSTANT_BUFFER cb[2] = {};
	CONSTANT_BUFFER3 cb3[2] = {};
	std::unique_ptr<WVP_CB[]> cbWVP[2] = {};
	bool firstCbSet[2];
	CONSTANT_BUFFER2 sg;
	//DrawOn
	bool DrawOn = false;

	//インスタンス数
	int ins_no = 0;
	int insNum[2] = {};

	struct drawPara {
		UINT NumMaxInstance = 1;
		int NumMaterial = 0;
		int numDesc = 0;
		ComPtr<ID3D12DescriptorHeap> descHeap = nullptr;
		ComPtr<ID3D12RootSignature> rootSignature = nullptr;
		ComPtr<ID3D12RootSignature> rootSignatureDXR = nullptr;
		std::unique_ptr<VertexView> Vview = nullptr;
		std::unique_ptr<IndexView[]> Iview = nullptr;
		std::unique_ptr<MY_MATERIAL_S[]> material = nullptr;
		D3D_PRIMITIVE_TOPOLOGY TOPOLOGY;
		std::unique_ptr<ComPtr<ID3D12PipelineState>[]> PSO = nullptr;
		std::unique_ptr<ComPtr<ID3D12PipelineState>[]> PSO_DXR = nullptr;
		UINT insNum = 1;
	};

	drawPara dpara = {};
	ParameterDXR dxrPara = {};

	DivideArr divArr[16] = {};
	int numDiv;

	void createBufferDXR(int numMaterial, int numMaxInstance);
	void setTextureDXR();
	void CbSwap();
	void draw(int comIndex, drawPara& para);
	void ParameterDXR_Update();
	void streamOutput(int comIndex, drawPara& para, ParameterDXR& dxr);

	void getBuffer(int numMaterial, int numMaxInstance, DivideArr* divArr = nullptr, int numDiv = 0);
	void getVertexBuffer(UINT VertexByteStride, UINT numVertex);
	void getIndexBuffer(int materialIndex, UINT IndexBufferByteSize, UINT numIndex);
	void setColorDXR(int materialIndex, CONSTANT_BUFFER2& sg);
	void createDefaultBuffer(int comIndex, void* vertexArr, UINT** indexArr);

	bool createPSO(std::vector<D3D12_INPUT_ELEMENT_DESC>& vertexLayout,
		const int numSrv, const int numCbv, const int numUav, bool blend, bool alpha);

	bool createTexResource(int comIndex);

	bool setDescHeap(const int numSrvTex, const int numSrvBuf,
		ID3D12Resource** buffer, UINT* StructureByteStride,
		const int numCbv, D3D12_GPU_VIRTUAL_ADDRESS ad3, UINT ad3Size);

	void setParameterDXR(bool alpha);

	bool createStreamOutputResource(int comIndex, float divideBufferMagnification);

	bool createPSO_DXR(std::vector<D3D12_INPUT_ELEMENT_DESC>& vertexLayout,
		const int numSrv, const int numCbv, const int numUav, bool smooth);

	void GetShaderByteCode(PrimitiveType type, bool light, bool smooth, bool BC_On,
		ID3DBlob* changeVs, ID3DBlob* changeDs);

public:
	BasicPolygon();
	~BasicPolygon();

	void Instancing(CoordTf::VECTOR3 pos, CoordTf::VECTOR3 angle, CoordTf::VECTOR3 size, CoordTf::VECTOR4 Color);

	void InstancingUpdate(float disp, float SmoothRange, float SmoothRatio, float shininess = 2.0f,
		float px = 1.0f, float py = 1.0f, float mx = 1.0f, float my = 1.0f);

	void Update(CoordTf::VECTOR3 pos, CoordTf::VECTOR4 Color, CoordTf::VECTOR3 angle, CoordTf::VECTOR3 size,
		float disp, float SmoothRange, float SmoothRatio, float shininess = 4.0f,
		float px = 1.0f, float py = 1.0f, float mx = 1.0f, float my = 1.0f);

	void Draw(int comIndex);
	void StreamOutput(int comIndex);
	void DrawOff();
	ParameterDXR* getParameter();

	void setDivideArr(DivideArr* arr, int numdiv) {
		numDiv = numdiv;
		memcpy(divArr, arr, sizeof(DivideArr) * numDiv);
	}

	void UpdateDxrDivideBuffer();

	void setRefractiveIndex(float index) {
		Dx_Device* dev = Dx_Device::GetInstance();
		dxrPara.updateDXR[dev->dxrBuffSwapIndex()].RefractiveIndex = index;
	}

	void setRoughness(float roug) {
		Dx_Device* dev = Dx_Device::GetInstance();
		dxrPara.updateDXR[dev->dxrBuffSwapIndex()].roughness = roug;
	}
};

#endif