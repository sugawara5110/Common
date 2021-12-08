//*****************************************************************************************//
//**                                                                                     **//
//**                   　　　         Waveクラス                                         **//
//**                                                                                     **//
//*****************************************************************************************//

#ifndef Class_Wave_Header
#define Class_Wave_Header

#include "Core/Dx12ProcessCore.h"

class Wave {

protected:
	ComPtr<ID3D12RootSignature> mRootSignatureCom = nullptr;
	ConstantBuffer<CONSTANT_BUFFER_WAVE>* mObjectCB_WAVE = nullptr;
	CONSTANT_BUFFER_WAVE cbw;
	CONSTANT_BUFFER2 sg;

	int div;//分割数

	ComPtr<ID3D12PipelineState> mPSOCom = nullptr;
	ComPtr<ID3D12Resource> mInputBuffer = nullptr;
	ComPtr<ID3D12Resource> mInputUploadBuffer = nullptr;
	ComPtr<ID3D12Resource> mOutputBuffer = nullptr;
	DivideArr divArr[16] = {};
	int numDiv;

	VertexM* ver = nullptr;
	UINT* index = nullptr;
	BasicPolygon mObj;

	void GetShaderByteCode(bool smooth);
	bool ComCreate();
	bool DrawCreate(int texNo, int nortNo, bool blend, bool alpha, bool smooth, float divideBufferMagnification);
	void Compute(int com_no);

public:
	Wave();
	~Wave();
	void GetVBarray(int numMaxInstance);
	void SetCol(float difR, float difG, float difB, float speR, float speG, float speB,
		float amR = 0.0f, float amG = 0.0f, float amB = 0.0f);
	void setMaterialType(MaterialType type);
	bool Create(int texNo, bool blend, bool alpha, float waveHeight, float divide, bool smooth = true);
	bool Create(int texNo, int nortNo, bool blend, bool alpha, float waveHeight, float divide, bool smooth = true,
		float divideBufferMagnification = 1.0f);
	void SetVertex(Vertex* vertexArr, int numVer, UINT* ind, int numInd);

	void Instancing(float speed, CoordTf::VECTOR3 pos, CoordTf::VECTOR3 angle, CoordTf::VECTOR3 size, CoordTf::VECTOR4 Color);

	void InstancingUpdate(float disp, float shininess = 4.0f,
		float px = 1.0f, float py = 1.0f, float mx = 1.0f, float my = 1.0f);

	void Update(float speed, CoordTf::VECTOR3 pos, CoordTf::VECTOR4 Color,
		CoordTf::VECTOR3 angle, CoordTf::VECTOR3 size,
		float disp, float shininess = 4.0f,
		float px = 1.0f, float py = 1.0f, float mx = 1.0f, float my = 1.0f);

	void DrawOff();
	void Draw(int com_no);
	void StreamOutput(int com_no);
	void Draw();
	void StreamOutput();
	void SetCommandList(int no);
	void CopyResource(ID3D12Resource* texture, D3D12_RESOURCE_STATES res, int index = 0);
	void TextureInit(int width, int height, int index = 0);
	HRESULT SetTextureMPixel(BYTE* frame, int index = 0);
	void setDivideArr(DivideArr* arr, int numdiv) {
		numDiv = numdiv;
		memcpy(divArr, arr, sizeof(DivideArr) * numDiv);
	}
	ParameterDXR* getParameter() { return mObj.getParameter(); }
	void UpdateDxrDivideBuffer() {
		mObj.UpdateDxrDivideBuffer();
	}
	void setRefractiveIndex(float index) {
		mObj.setRefractiveIndex(index);
	}
	void SetName(char* name) { mObj.SetName(name); }
};

#endif
