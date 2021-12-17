//*****************************************************************************************//
//**                                                                                     **//
//**                   　　　      PolygonDataクラス                                     **//
//**                                                                                     **//
//*****************************************************************************************//

#ifndef Class_PolygonData_Header
#define Class_PolygonData_Header

#include "Core/Dx12ProcessCore.h"

class PolygonData :public BasicPolygon {

protected:
	void* ver = nullptr;  //頂点配列
	UINT** index = nullptr;//頂点インデックス
	int numIndex = 0;   //頂点インデックス数

	void GetShaderByteCode(bool light, int tNo, bool smooth);

public:
	PolygonData();
	~PolygonData();
	ID3D12PipelineState* GetPipelineState();
	void GetVBarray(PrimitiveType type, int numMaxInstance);

	void SetCol(float difR, float difG, float difB, float speR, float speG, float speB,
		float amR = 0.0f, float amG = 0.0f, float amB = 0.0f);

	void setMaterialType(MaterialType type);
	bool Create(bool light, int tNo, bool blend, bool alpha);

	bool Create(bool light, int tNo, int nortNo, int spetNo, bool blend, bool alpha,
		bool smooth = false,
		float divideBufferMagnification = 1.0f);

	template<typename T>
	void setVertex(T* vertexArr, int numVer, UINT* ind, int numInd) {
		if (typeid(Vertex) == typeid(T)) {
			ver = new VertexM[numVer];
			VertexM* verM = (VertexM*)ver;
			Vertex* v = (Vertex*)vertexArr;
			for (int i = 0; i < numVer; i++) {
				verM[i].Pos.as(v[i].Pos.x, v[i].Pos.y, v[i].Pos.z);
				verM[i].normal.as(v[i].normal.x, v[i].normal.y, v[i].normal.z);
				verM[i].geoNormal.as(v[i].normal.x, v[i].normal.y, v[i].normal.z);
				verM[i].tex.as(v[i].tex.x, v[i].tex.y);
			}
			getVertexBuffer(sizeof(VertexM), numVer);
		}
		if (typeid(VertexBC) == typeid(T)) {
			ver = new VertexBC[numVer];
			VertexBC* v = (VertexBC*)vertexArr;
			memcpy(ver, v, sizeof(VertexBC) * numVer);
			getVertexBuffer(sizeof(VertexBC), numVer);
		}
		if (typeid(VertexBC) != typeid(T) && typeid(Vertex) != typeid(T)) {
			Dx_Util::ErrorMessage("PolygonData::setVertex Error!!");
			return;
		}
		index = new UINT * [dpara.NumMaterial];
		index[0] = new UINT[numInd];
		memcpy(index[0], ind, sizeof(UINT) * numInd);
		numIndex = numInd;
		const UINT ibByteSize = numIndex * sizeof(UINT);
		getIndexBuffer(0, ibByteSize, numIndex);
	}
};

#endif