//*****************************************************************************************//
//**                                                                                     **//
//**                   　　　           DxStruct.h                                       **//
//**                                   Directx用構造体                                   **//
//*****************************************************************************************//

#ifndef Class_DxStruct_Header
#define Class_DxStruct_Header

#include "../../../../CoordTf/CoordTf.h"
#include <windows.h>
#include <wrl.h>//Microsoft::WRL
#include <dxgi1_4.h>
#include <string.h>

#define LIGHT_PCS 256
#define INSTANCE_PCS_2D 256
#define MAX_BONES 256
#define RELEASE(p)    if(p){p->Release();  p=nullptr;}
#define S_DELETE(p)   if(p){delete p;      p=nullptr;}
#define ARR_DELETE(p) if(p){delete[] p;    p=nullptr;}
#define ARRAY_SIZE(a) (sizeof(a)/sizeof(a[0]))
#define ALIGNMENT_TO(alignment, val) ((val + alignment - 1) & ~(alignment - 1))

//シェーダー受け渡し用バッファ3D用
struct WVP_CB {
	CoordTf::MATRIX wvp;
	CoordTf::MATRIX world;
	CoordTf::VECTOR4 AddObjColor;//オブジェクトの色変化用
};

struct CONSTANT_BUFFER {

	CoordTf::VECTOR4 C_Pos;//視点位置
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

	//x:ディスプレイトメントマッピングの起伏量
	//y:divide配列数
	//z:shininess
	//w:Smooth範囲
	CoordTf::VECTOR4  DispAmount;

	//x:Smooth比率
	CoordTf::VECTOR4 SmoothRatio;

	//divide配列 x:distance, y:divide
	CoordTf::VECTOR4 Divide[16];

	//UV座標移動用
	CoordTf::VECTOR4 pXpYmXmY;
};

struct CONSTANT_BUFFER2 {
	CoordTf::VECTOR4 vDiffuse;//ディフューズ色
	CoordTf::VECTOR4 vSpeculer;//スぺキュラ色
	CoordTf::VECTOR4 vAmbient;//アンビエント
};

struct cbInstanceID {
	CoordTf::VECTOR4 instanceID;//x:ID, y:1.0f on 0.0f off
};

//コンスタントバッファ2D用
struct CONSTANT_BUFFER2D {
	CoordTf::VECTOR4 Pos[INSTANCE_PCS_2D];
	CoordTf::VECTOR4 Color[INSTANCE_PCS_2D];
	CoordTf::VECTOR4 sizeXY[INSTANCE_PCS_2D];
	CoordTf::VECTOR4 WidHei;//ウインドウwh
};

//ポイントライト
struct PointLight {
	CoordTf::VECTOR4 LightPos[LIGHT_PCS];   //xyz:Pos, w:オンオフ
	CoordTf::VECTOR4 LightColor[LIGHT_PCS];//色
	CoordTf::VECTOR4 Lightst[LIGHT_PCS];  //レンジ, 減衰1, 減衰2, 減衰3
	int     LightPcs;     //ライト個数
};

//平行光源
struct DirectionLight {
	CoordTf::VECTOR4 Direction;  //方向
	CoordTf::VECTOR4 LightColor;//色
	float onoff;
};

//フォグ
struct Fog {
	CoordTf::VECTOR4  FogColor;//フォグの色
	float    Amount;  //フォグ量
	float    Density;//密度
	float    on_off;
};

//頂点3DTexture無し
struct VertexBC {
	CoordTf::VECTOR3 Pos;     //位置
	CoordTf::VECTOR4 color;   //色
};

//頂点2D
struct MY_VERTEX2 {
	CoordTf::VECTOR3 Pos;
	CoordTf::VECTOR4 color;
	CoordTf::VECTOR2 tex;
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

//以下スキンメッシュ
struct MY_VERTEX_S {
	CoordTf::VECTOR3 vPos = {};//頂点
	CoordTf::VECTOR3 vNorm = {};//法線
	CoordTf::VECTOR3 vTangent;  //接ベクトル
	CoordTf::VECTOR3 vGeoNorm = {};//ジオメトリ法線
	CoordTf::VECTOR2 vTex0 = {};//UV座標0
	CoordTf::VECTOR2 vTex1 = {};//UV座標1
	UINT bBoneIndex[4] = {};//ボーン　番号
	float bBoneWeight[4] = {};//ボーン　重み
};

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

struct BONE {
	CoordTf::MATRIX mBindPose;//初期ポーズ
	CoordTf::MATRIX mNewPose;//現在のポーズ

	BONE()
	{
		ZeroMemory(this, sizeof(BONE));
	}
};

struct SHADER_GLOBAL_BONES {
	CoordTf::MATRIX mBone[MAX_BONES];
	SHADER_GLOBAL_BONES()
	{
		for (int i = 0; i < MAX_BONES; i++)
		{
			MatrixIdentity(&mBone[i]);
		}
	}
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