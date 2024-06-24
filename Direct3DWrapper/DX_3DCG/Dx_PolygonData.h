//*****************************************************************************************//
//**                                                                                     **//
//**                   　　　      PolygonData                                           **//
//**                                                                                     **//
//*****************************************************************************************//

#ifndef Class_PolygonData_Header
#define Class_PolygonData_Header

#include "Core/Dx_BasicPolygon.h"

class PolygonData :public BasicPolygon {

protected:
	void* ver = nullptr;  //頂点配列
	UINT** index = nullptr;//頂点インデックス
	int numIndex = 0;   //頂点インデックス数

public:
	PolygonData();
	~PolygonData();
	ID3D12PipelineState* GetPipelineState();
	void GetVBarray(PrimitiveType type, int numMaxInstance);

	void SetCol(float difR, float difG, float difB, float speR, float speG, float speB,
		float amR = 0.0f, float amG = 0.0f, float amB = 0.0f);

	void setMaterialType(MaterialType type);

	void setPointLight(int InstanceIndex, bool on_off,
		float range, CoordTf::VECTOR3 atten = { 0.01f, 0.001f, 0.001f },
		float plight_RandArea = 0.5f, int plight_numRandRay = 1);

	void setPointLightAll(bool on_off,
		float range, CoordTf::VECTOR3 atten = { 0.01f, 0.001f, 0.001f },
		float plight_RandArea = 0.5f, int plight_numRandRay = 1);

	bool Create(int comIndex, bool light, int tNo, bool blend, bool alpha);

	bool Create(int comIndex, bool light, int tNo, int nortNo, int spetNo, bool blend, bool alpha,
		bool smooth = false,
		float divideBufferMagnification = 1.0f);

	void InstancingUpdate(float disp, float SmoothRange = 0.1f, float SmoothRatio = 0.999f, float shininess = 4.0f,
		float px = 1.0f, float py = 1.0f, float mx = 1.0f, float my = 1.0f);

	void Update(CoordTf::VECTOR3 pos, CoordTf::VECTOR4 Color, CoordTf::VECTOR3 angle, CoordTf::VECTOR3 size,
		float disp, float SmoothRange = 0.1f, float SmoothRatio = 0.999f, float shininess = 4.0f,
		float px = 1.0f, float py = 1.0f, float mx = 1.0f, float my = 1.0f);

	template<typename T>
	void setVertex(T* vertexArr, int numVer, UINT* ind, int numInd) {
		if (typeid(Vertex) == typeid(T)) {
			ver = NEW VertexM[numVer];
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
			ver = NEW VertexBC[numVer];
			VertexBC* v = (VertexBC*)vertexArr;
			memcpy(ver, v, sizeof(VertexBC) * numVer);
			getVertexBuffer(sizeof(VertexBC), numVer);
		}
		if (typeid(VertexBC) != typeid(T) && typeid(Vertex) != typeid(T)) {
			Dx_Util::ErrorMessage("PolygonData::setVertex Error!!");
			return;
		}
		index = NEW UINT * [dpara.NumMaterial];
		index[0] = NEW UINT[numInd];
		memcpy(index[0], ind, sizeof(UINT) * numInd);
		numIndex = numInd;
		const UINT ibByteSize = numIndex * sizeof(UINT);
		getIndexBuffer(0, ibByteSize, numIndex);
	}
};

#endif