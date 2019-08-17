//*****************************************************************************************//
//**                                                                                     **//
//**                   　　　           DxStruct.h                                       **//
//**                                   Directx用構造体                                   **//
//*****************************************************************************************//

#ifndef Class_DxStruct_Header
#define Class_DxStruct_Header

#include "DxFunction.h"

#define LIGHT_PCS 150
#define INSTANCE_PCS_3D 150
#define INSTANCE_PCS_2D 80
#define MAX_BONES 150
#define RELEASE(p)    if(p){p->Release();  p=nullptr;}
#define S_DELETE(p)   if(p){delete p;      p=nullptr;}
#define ARR_DELETE(p) if(p){delete[] p;    p=nullptr;}

//シェーダー受け渡し用バッファ3D用
struct CONSTANT_BUFFER {

	MATRIX World[INSTANCE_PCS_3D];
	MATRIX WVP[INSTANCE_PCS_3D];
	VECTOR4 C_Pos;       //視点位置
	VECTOR4 AddObjColor;//オブジェクトの色変化用

    //ポイントライト
	VECTOR4 pLightPos[LIGHT_PCS];
	VECTOR4 pLightColor[LIGHT_PCS];
	VECTOR4 pLightst[LIGHT_PCS];
	VECTOR4 pShadowLow_Lpcs;//影の下限値, ライト個数, ライト有無

    //ディレクショナルライト
	VECTOR4 dDirection;
	VECTOR4 dLightColor;
	VECTOR4 dLightst;

	//フォグ
	VECTOR4  FogAmo_Density; //フォグ量x, フォグの密度y, onoffz
	VECTOR4  FogColor;   //フォグの色

    //ディスプレイトメントマッピングの起伏量x(0入力の場合デフォルト値3になる)
	VECTOR4  DispAmount;

	//UV座標移動用
	VECTOR4 pXpYmXmY;
};

struct CONSTANT_BUFFER2 {
	VECTOR4 vDiffuse;//ディフューズ色
	VECTOR4 vSpeculer;//スぺキュラ色
};

//コンスタントバッファ2D用
struct CONSTANT_BUFFER2D {
	VECTOR4 Pos[INSTANCE_PCS_2D];
	VECTOR4 Color[INSTANCE_PCS_2D];
	VECTOR4 sizeXY[INSTANCE_PCS_2D];
	VECTOR4 WidHei;//ウインドウwh
};

//コンスタントバッファパーティクル用
struct CONSTANT_BUFFER_P {
	MATRIX  WV;
	MATRIX  Proj;
	VECTOR4 size;//xパーティクル大きさ, yパーティクル初期化フラグ, zスピード
};

//ポイントライト
struct PointLight {
	VECTOR4 LightPos[LIGHT_PCS];   //光源
	VECTOR4 LightColor[LIGHT_PCS];//色
	VECTOR4 Lightst[LIGHT_PCS];  //レンジ,明るさ,減衰の大きさ,オンオフ
	float   ShadowLow_val; //影の下限値
	int     LightPcs;     //ライト個数
};

//平行光源
struct DirectionLight {
	VECTOR4 Direction;  //方向
	VECTOR4 LightColor;//色
	VECTOR4 Lightst;  //明るさx,オンオフy,影の下限値
};

//フォグ
struct Fog {
	VECTOR4  FogColor;//フォグの色
	float    Amount;  //フォグ量
	float    Density;//密度
	float    on_off;
};

//頂点3DTexture有り
struct Vertex {
	VECTOR3 Pos;       //位置
	VECTOR3 normal;   //法線
	VECTOR2 tex;    //テクスチャ座標
};

//頂点3DTexture無し
struct VertexBC {
	VECTOR3 Pos;       //位置
	VECTOR4 color;   //色
};

//頂点2D
struct MY_VERTEX2 {
	VECTOR3 Pos;       
	VECTOR4 color;
	VECTOR2 tex;    
};

//パーティクル頂点
struct PartPos {
	VECTOR3 CurrentPos; //描画に使う
	VECTOR3 PosSt;     //開始位置
	VECTOR3 PosEnd;   //終了位置
	VECTOR4 Col;
};

//Mesh.obj
struct VertexM {
	VECTOR3 Pos;       //位置
	VECTOR3 normal;   //法線
	VECTOR3 geoNormal;
	VECTOR2 tex;    //テクスチャ座標
};

//以下スキンメッシュ
struct MY_VERTEX_S{
	VECTOR3 vPos;//頂点
	VECTOR3 vNorm;//法線
	VECTOR3 vGeoNorm;//ジオメトリ法線
	VECTOR2 vTex;//UV座標
	UINT bBoneIndex[4];//ボーン　番号
	float bBoneWeight[4];//ボーン　重み
	MY_VERTEX_S()
	{
		ZeroMemory(this, sizeof(MY_VERTEX_S));
	}
};

struct MY_MATERIAL_S {
	CHAR szName[255];
	VECTOR4 Kd;//ディフューズ
	VECTOR4 Ks;//スペキュラー
	CHAR szTextureName[255];//テクスチャーファイル名
	CHAR norTextureName[255];//ノーマルマップ
	DWORD dwNumFace;//マテリアル毎のポリゴン数
	int tex_no;
	int nortex_no;
	MY_MATERIAL_S()
	{
		ZeroMemory(this, sizeof(MY_MATERIAL_S));
		tex_no = -1;
		nortex_no = -1;
	}
	~MY_MATERIAL_S()
	{

	}
};

struct BONE{
	MATRIX mBindPose;//初期ポーズ
	MATRIX mNewPose;//現在のポーズ

	BONE()
	{
		ZeroMemory(this, sizeof(BONE));
	}
};

struct SHADER_GLOBAL_BONES {
	MATRIX mBone[MAX_BONES];
	SHADER_GLOBAL_BONES()
	{
		for (int i = 0; i < MAX_BONES; i++)
		{
			MatrixIdentity(&mBone[i]);
		}
	}
};

//テクスチャ管理
struct Texture {
	char  *binary_ch = nullptr; //デコード後バイナリ
	int   binary_size = 0;  //バイナリサイズ
	char  *texName = nullptr; //ファイル名
	bool  UpKeep = false; //Upload後Up用バッファを保持するか
};

//Wave
struct WaveData
{
	float sinWave;
	float theta;
};

struct CONSTANT_BUFFER_WAVE {
	VECTOR4 wHei_divide;//x:waveHeight, y:分割数
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

struct TextureNo {
	int diffuse;
	int normal;
	bool movie;
};

//ポストエフェクト
struct CONSTANT_BUFFER_PostMosaic {
	VECTOR4 mosaicSize;//x
	VECTOR4 blur;//xy:座標, z:強さ
};

#endif