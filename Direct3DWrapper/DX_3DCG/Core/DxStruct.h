//*****************************************************************************************//
//**                                                                                     **//
//**                   　　　           DxStruct.h                                       **//
//**                                   Directx用構造体                                   **//
//*****************************************************************************************//

#ifndef Class_DxStruct_Header
#define Class_DxStruct_Header

#if defined(DEBUG) || defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#define NEW new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )
#else
#define NEW new
#endif

#include "../../../../CoordTf/CoordTf.h"
#include <windows.h>
#include <wrl.h>//Microsoft::WRL
#include <dxgi1_4.h>
#include <string.h>

#define LIGHT_PCS 256
#define RELEASE(p)    if(p){p->Release();  p=nullptr;}
#define S_DELETE(p)   if(p){delete p;      p=nullptr;}
#define ARR_DELETE(p) if(p){delete[] p;    p=nullptr;}
#define ARRAY_SIZE(a) (sizeof(a)/sizeof(a[0]))
#define ALIGNMENT_TO(alignment, val) ((val + alignment - 1) & ~(alignment - 1))

struct WVP_CB {
	CoordTf::MATRIX wvp;
	CoordTf::MATRIX world;
	CoordTf::VECTOR4 AddObjColor;//オブジェクトの色変化用
	CoordTf::VECTOR4 pXpYmXmY;
};

struct CONSTANT_BUFFER {

	CoordTf::VECTOR4 C_Pos;//視点位置

	//x:ディスプレイトメントマッピングの起伏量
	//y:divide配列数
	//z:shininess
	//w:Smooth範囲
	CoordTf::VECTOR4  DispAmount;

	//x:Smooth比率
	CoordTf::VECTOR4 SmoothRatio;

	//divide配列 x:distance, y:divide
	CoordTf::VECTOR4 Divide[16];
};

struct CONSTANT_BUFFER2 {
	CoordTf::VECTOR4 vDiffuse;//ディフューズ色
	CoordTf::VECTOR4 vSpeculer;//スぺキュラ色
	CoordTf::VECTOR4 vAmbient;//アンビエント
};

struct CONSTANT_BUFFER3 {
	//グローバルアンビエント
	CoordTf::VECTOR4 GlobalAmbientLight;

	//ポイントライト
	CoordTf::VECTOR4 pLightPos[LIGHT_PCS];//xyz:Pos, w:オンオフ
	CoordTf::VECTOR4 pLightColor[LIGHT_PCS];
	CoordTf::VECTOR4 pLightst[LIGHT_PCS];//レンジ, 減衰1, 減衰2, 減衰3
	CoordTf::VECTOR4 numLight;//x:ライト個数

	//ディレクショナルライト
	CoordTf::VECTOR4 dDirection;
	CoordTf::VECTOR4 dLightColor;
	CoordTf::VECTOR4 dLightst;//x:オンオフ

	//フォグ
	CoordTf::VECTOR4  FogAmo_Density; //フォグ量:x, フォグの密度:y, onoff:z
	CoordTf::VECTOR4  FogColor;   //フォグの色
};

struct cbInstanceID {
	CoordTf::VECTOR4 instanceID;//x:ID, y:1.0f on 0.0f off
};

//頂点3DTexture無し
struct VertexBC {
	CoordTf::VECTOR3 Pos;     //位置
	CoordTf::VECTOR4 color;   //色
};

//頂点3DTexture有り
struct Vertex {
	CoordTf::VECTOR3 Pos;    //位置
	CoordTf::VECTOR3 normal; //法線
	CoordTf::VECTOR2 tex;    //テクスチャ座標
};

//Mesh.obj
struct VertexM {
	CoordTf::VECTOR3 Pos;      //位置
	CoordTf::VECTOR3 normal;   //法線
	CoordTf::VECTOR3 tangent;  //接ベクトル
	CoordTf::VECTOR3 geoNormal;
	CoordTf::VECTOR2 tex;      //テクスチャ座標
};

class SameVertexList {

private:
	int list[50];
	unsigned int ind;

public:
	SameVertexList() {
		for (int i = 0; i < 50; i++) {
			list[i] = -1;
		}
		ind = 0;
	}

	void Push(int vInd) {
		list[ind] = vInd;
		ind++;
	}

	int Pop() {
		if (ind <= 0)return -1;
		ind--;
		return list[ind];
	}
};

class SameVertexListNormalization {

public:
	void Normalization(void* Vertices, int VerByteStride, BYTE byteOffset,
		UINT NumVertices, SameVertexList* svList) {

		using namespace CoordTf;
		float* ver = (float*)Vertices;
		int VerFloatStride = VerByteStride / sizeof(float);
		int floatOffset = byteOffset / sizeof(float);
		for (UINT i = 0; i < NumVertices; i++) {
			int indVB = 0;
			VECTOR3 geo[50] = {};
			int indVb[50] = {};
			int indGeo = 0;
			while (1) {
				indVB = svList[i].Pop();
				if (indVB == -1)break;
				indVb[indGeo] = indVB;
				float* v = &ver[indVB * VerFloatStride + floatOffset];
				geo[indGeo++].as(v[0], v[1], v[2]);
			}
			VECTOR3 sum = {};
			sum.as(0.0f, 0.0f, 0.0f);
			for (int i1 = 0; i1 < indGeo; i1++) {
				sum.x += geo[i1].x;
				sum.y += geo[i1].y;
				sum.z += geo[i1].z;
			}
			VECTOR3 ave = {};
			ave.x = sum.x / (float)indGeo;
			ave.y = sum.y / (float)indGeo;
			ave.z = sum.z / (float)indGeo;
			for (int i1 = 0; i1 < indGeo; i1++) {
				float* v = &ver[indVb[i1] * VerFloatStride + floatOffset];
				v[0] = ave.x;
				v[1] = ave.y;
				v[2] = ave.z;
			}
		}
	}
};

struct TextureNo {
	int diffuse;
	int normal;
	int specular;
};

struct MovieTexture {
	bool m_on = false;
	int width = 8;
	int height = 8;
	int resIndex = -1;
};

struct DivideArr {
	float distance;
	float divide;
};

struct VERTEX_DXR {
	CoordTf::VECTOR3 Pos = {};//頂点
	CoordTf::VECTOR3 Nor = {};//法線
	CoordTf::VECTOR3 Tangent = {};//接ベクトル
	CoordTf::VECTOR2 Tex[2] = {};//UV座標
};

#endif