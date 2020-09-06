//*****************************************************************************************//
//**                                                                                     **//
//**                   �@�@�@           DxStruct.h                                       **//
//**                                   Directx�p�\����                                   **//
//*****************************************************************************************//

#ifndef Class_DxStruct_Header
#define Class_DxStruct_Header

#include "DxFunction.h"
#include <windows.h>
#include <wrl.h>//Microsoft::WRL
#include <dxgi1_4.h>
#include <string.h>

#define LIGHT_PCS 256
#define INSTANCE_PCS_3D 256
#define INSTANCE_PCS_2D 256
#define MAX_BONES 256
#define RELEASE(p)    if(p){p->Release();  p=nullptr;}
#define S_DELETE(p)   if(p){delete p;      p=nullptr;}
#define ARR_DELETE(p) if(p){delete[] p;    p=nullptr;}
#define ARRAY_SIZE(a) (sizeof(a)/sizeof(a[0]))
#define ALIGNMENT_TO(alignment, val) ((val + alignment - 1) & ~(alignment - 1))

//�V�F�[�_�[�󂯓n���p�o�b�t�@3D�p
struct CONSTANT_BUFFER {

	MATRIX World[INSTANCE_PCS_3D];
	MATRIX WVP[INSTANCE_PCS_3D];
	VECTOR4 C_Pos;       //���_�ʒu
	VECTOR4 viewUp;      //���_�����
	VECTOR4 AddObjColor;//�I�u�W�F�N�g�̐F�ω��p

	//�O���[�o���A���r�G���g
	VECTOR4 GlobalAmbientLight;

	//�|�C���g���C�g
	VECTOR4 pLightPos[LIGHT_PCS];//xyz:Pos, w:�I���I�t
	VECTOR4 pLightColor[LIGHT_PCS];
	VECTOR4 pLightst[LIGHT_PCS];//�����W, ����1, ����2, ����3
	VECTOR4 numLight;//x:���C�g��

	//�f�B���N�V���i�����C�g
	VECTOR4 dDirection;
	VECTOR4 dLightColor;
	VECTOR4 dLightst;//x:�I���I�t

	//�t�H�O
	VECTOR4  FogAmo_Density; //�t�H�O��x, �t�H�O�̖��xy, onoffz
	VECTOR4  FogColor;   //�t�H�O�̐F

	//x:�f�B�X�v���C�g�����g�}�b�s���O�̋N����
	//y:divide�z��
	//z:shininess
	VECTOR4  DispAmount;

	//divide�z�� x:distance, y:divide
	VECTOR4 Divide[16];

	//UV���W�ړ��p
	VECTOR4 pXpYmXmY;
};

struct CONSTANT_BUFFER2 {
	VECTOR4 vDiffuse;//�f�B�t���[�Y�F
	VECTOR4 vSpeculer;//�X�؃L�����F
	VECTOR4 vAmbient;//�A���r�G���g
	VECTOR4 uvSwitch;//.x�̂�
};

//�R���X�^���g�o�b�t�@2D�p
struct CONSTANT_BUFFER2D {
	VECTOR4 Pos[INSTANCE_PCS_2D];
	VECTOR4 Color[INSTANCE_PCS_2D];
	VECTOR4 sizeXY[INSTANCE_PCS_2D];
	VECTOR4 WidHei;//�E�C���h�Ewh
};

//�R���X�^���g�o�b�t�@�p�[�e�B�N���p
struct CONSTANT_BUFFER_P {
	MATRIX  WV;
	MATRIX  Proj;
	VECTOR4 size;//x�p�[�e�B�N���傫��, y�p�[�e�B�N���������t���O, z�X�s�[�h
};

//�|�C���g���C�g
struct PointLight {
	VECTOR4 LightPos[LIGHT_PCS];   //xyz:Pos, w:�I���I�t
	VECTOR4 LightColor[LIGHT_PCS];//�F
	VECTOR4 Lightst[LIGHT_PCS];  //�����W, ����1, ����2, ����3
	int     LightPcs;     //���C�g��
};

//���s����
struct DirectionLight {
	VECTOR4 Direction;  //����
	VECTOR4 LightColor;//�F
	float onoff;
};

//�t�H�O
struct Fog {
	VECTOR4  FogColor;//�t�H�O�̐F
	float    Amount;  //�t�H�O��
	float    Density;//���x
	float    on_off;
};

//���_3DTexture����
struct VertexBC {
	VECTOR3 Pos;       //�ʒu
	VECTOR4 color;   //�F
};

//���_2D
struct MY_VERTEX2 {
	VECTOR3 Pos;       
	VECTOR4 color;
	VECTOR2 tex;    
};

//�p�[�e�B�N�����_
struct PartPos {
	VECTOR3 CurrentPos; //�`��Ɏg��
	VECTOR3 PosSt;     //�J�n�ʒu
	VECTOR3 PosEnd;   //�I���ʒu
	VECTOR4 Col;
};

//���_3DTexture�L��
struct Vertex {
	VECTOR3 Pos;       //�ʒu
	VECTOR3 normal;   //�@��
	VECTOR2 tex;    //�e�N�X�`�����W
};

//Mesh.obj
struct VertexM {
	VECTOR3 Pos;       //�ʒu
	VECTOR3 normal;   //�@��
	VECTOR3 geoNormal;
	VECTOR2 tex;    //�e�N�X�`�����W
};

//�ȉ��X�L�����b�V��
struct MY_VERTEX_S {
	VECTOR3 vPos = {};//���_
	VECTOR3 vNorm = {};//�@��
	VECTOR3 vGeoNorm = {};//�W�I���g���@��
	VECTOR2 vTex0 = {};//UV���W0
	VECTOR2 vTex1 = {};//UV���W1
	UINT bBoneIndex[4] = {};//�{�[���@�ԍ�
	float bBoneWeight[4] = {};//�{�[���@�d��
};

struct MY_MATERIAL_S {
	VECTOR4 diffuse = {};
	VECTOR4 specular = {};
	VECTOR4 ambient = {};
	CHAR difUvName[255] = {};
	CHAR norUvName[255] = {};
	CHAR speUvName[255] = {};
	int diftex_no = -1;
	int nortex_no = -1;
	int spetex_no = -1;
};

struct BONE{
	MATRIX mBindPose;//�����|�[�Y
	MATRIX mNewPose;//���݂̃|�[�Y

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

class InternalTexture {
public:
	UCHAR* byteArr = nullptr;
	char* texName = nullptr; //�t�@�C����
	DXGI_FORMAT format = {};
	int width = 0;
	LONG_PTR RowPitch = 0;
	int height = 0;

	void setParameter(DXGI_FORMAT Format, int Width, LONG_PTR rowPitch, int Height) {
		format = Format;
		width = Width;
		RowPitch = rowPitch;
		height = Height;
	}
	void setName(char* name) {
		int ln = (int)strlen(name) + 1;
		texName = new char[ln];
		memcpy(texName, name, sizeof(char) * ln);
	}
	void setData(UCHAR* ByteArr) {
		byteArr = new UCHAR[RowPitch * height];
		memcpy(byteArr, ByteArr, sizeof(UCHAR) * RowPitch * height);
	}
	~InternalTexture() {
		ARR_DELETE(byteArr);
		ARR_DELETE(texName);
	}
};

//Wave
struct WaveData
{
	float sinWave;
	float theta;
};

struct CONSTANT_BUFFER_WAVE {
	VECTOR4 wHei_divide;//x:waveHeight, y:������
	float speed = 0.0f;
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
	int specular;
};

struct MovieTexture {
	bool m_on = false;
	int width = 8;
	int height = 8;
	int resIndex = -1;
};

//�|�X�g�G�t�F�N�g
struct CONSTANT_BUFFER_PostMosaic {
	VECTOR4 mosaicSize;//x
	VECTOR4 blur;//xy:���W, z:����
};

struct DivideArr {
	float distance;
	float divide;
};

struct VERTEX_DXR {
	VECTOR3 Pos = {};//���_
	VECTOR3 Nor = {};//�@��
	VECTOR2 Tex[2] = {};//UV���W
};

#endif