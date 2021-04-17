//*****************************************************************************************//
//**                                                                                     **//
//**                   �@�@�@           DxStruct.h                                       **//
//**                                   Directx�p�\����                                   **//
//*****************************************************************************************//

#ifndef Class_DxStruct_Header
#define Class_DxStruct_Header

#include "../../../../CoordTf/CoordTf.h"
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

	CoordTf::MATRIX World[INSTANCE_PCS_3D];
	CoordTf::MATRIX WVP[INSTANCE_PCS_3D];
	CoordTf::VECTOR4 C_Pos;       //���_�ʒu
	CoordTf::VECTOR4 AddObjColor;//�I�u�W�F�N�g�̐F�ω��p

	//�O���[�o���A���r�G���g
	CoordTf::VECTOR4 GlobalAmbientLight;

	//�|�C���g���C�g
	CoordTf::VECTOR4 pLightPos[LIGHT_PCS];//xyz:Pos, w:�I���I�t
	CoordTf::VECTOR4 pLightColor[LIGHT_PCS];
	CoordTf::VECTOR4 pLightst[LIGHT_PCS];//�����W, ����1, ����2, ����3
	CoordTf::VECTOR4 numLight;//x:���C�g��

	//�f�B���N�V���i�����C�g
	CoordTf::VECTOR4 dDirection;
	CoordTf::VECTOR4 dLightColor;
	CoordTf::VECTOR4 dLightst;//x:�I���I�t

	//�t�H�O
	CoordTf::VECTOR4  FogAmo_Density; //�t�H�O��x, �t�H�O�̖��xy, onoffz
	CoordTf::VECTOR4  FogColor;   //�t�H�O�̐F

	//x:�f�B�X�v���C�g�����g�}�b�s���O�̋N����
	//y:divide�z��
	//z:shininess
	CoordTf::VECTOR4  DispAmount;

	//divide�z�� x:distance, y:divide
	CoordTf::VECTOR4 Divide[16];

	//UV���W�ړ��p
	CoordTf::VECTOR4 pXpYmXmY;
};

struct CONSTANT_BUFFER2 {
	CoordTf::VECTOR4 vDiffuse;//�f�B�t���[�Y�F
	CoordTf::VECTOR4 vSpeculer;//�X�؃L�����F
	CoordTf::VECTOR4 vAmbient;//�A���r�G���g
};

struct cbInstanceID {
	CoordTf::VECTOR4 instanceID;//x:ID, y:1.0f on 0.0f off
};

//�R���X�^���g�o�b�t�@2D�p
struct CONSTANT_BUFFER2D {
	CoordTf::VECTOR4 Pos[INSTANCE_PCS_2D];
	CoordTf::VECTOR4 Color[INSTANCE_PCS_2D];
	CoordTf::VECTOR4 sizeXY[INSTANCE_PCS_2D];
	CoordTf::VECTOR4 WidHei;//�E�C���h�Ewh
};

//�R���X�^���g�o�b�t�@�p�[�e�B�N���p
struct CONSTANT_BUFFER_P {
	CoordTf::MATRIX  WV;
	CoordTf::MATRIX  invRot;
	CoordTf::MATRIX  Proj;
	CoordTf::VECTOR4 size;//x�p�[�e�B�N���傫��, y�p�[�e�B�N���������t���O, z�X�s�[�h
	CoordTf::VECTOR4 AddObjColor;//�I�u�W�F�N�g�̐F�ω��p
};

//�|�C���g���C�g
struct PointLight {
	CoordTf::VECTOR4 LightPos[LIGHT_PCS];   //xyz:Pos, w:�I���I�t
	CoordTf::VECTOR4 LightColor[LIGHT_PCS];//�F
	CoordTf::VECTOR4 Lightst[LIGHT_PCS];  //�����W, ����1, ����2, ����3
	int     LightPcs;     //���C�g��
};

//���s����
struct DirectionLight {
	CoordTf::VECTOR4 Direction;  //����
	CoordTf::VECTOR4 LightColor;//�F
	float onoff;
};

//�t�H�O
struct Fog {
	CoordTf::VECTOR4  FogColor;//�t�H�O�̐F
	float    Amount;  //�t�H�O��
	float    Density;//���x
	float    on_off;
};

//���_3DTexture����
struct VertexBC {
	CoordTf::VECTOR3 Pos;     //�ʒu
	CoordTf::VECTOR4 color;   //�F
};

//���_2D
struct MY_VERTEX2 {
	CoordTf::VECTOR3 Pos;
	CoordTf::VECTOR4 color;
	CoordTf::VECTOR2 tex;
};

//�p�[�e�B�N�����_
struct PartPos {
	CoordTf::VECTOR3 CurrentPos; //�`��Ɏg��
	CoordTf::VECTOR3 PosSt;     //�J�n�ʒu
	CoordTf::VECTOR3 PosEnd;   //�I���ʒu
};

//���_3DTexture�L��
struct Vertex {
	CoordTf::VECTOR3 Pos;    //�ʒu
	CoordTf::VECTOR3 normal; //�@��
	CoordTf::VECTOR2 tex;    //�e�N�X�`�����W
};

//Mesh.obj
struct VertexM {
	CoordTf::VECTOR3 Pos;      //�ʒu
	CoordTf::VECTOR3 normal;   //�@��
	CoordTf::VECTOR3 tangent;  //�ڃx�N�g��
	CoordTf::VECTOR3 geoNormal;
	CoordTf::VECTOR2 tex;      //�e�N�X�`�����W
};

//�ȉ��X�L�����b�V��
struct MY_VERTEX_S {
	CoordTf::VECTOR3 vPos = {};//���_
	CoordTf::VECTOR3 vNorm = {};//�@��
	CoordTf::VECTOR3 vTangent;  //�ڃx�N�g��
	CoordTf::VECTOR3 vGeoNorm = {};//�W�I���g���@��
	CoordTf::VECTOR2 vTex0 = {};//UV���W0
	CoordTf::VECTOR2 vTex1 = {};//UV���W1
	UINT bBoneIndex[4] = {};//�{�[���@�ԍ�
	float bBoneWeight[4] = {};//�{�[���@�d��
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
	CoordTf::MATRIX mBindPose;//�����|�[�Y
	CoordTf::MATRIX mNewPose;//���݂̃|�[�Y

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
	CoordTf::VECTOR4 wHei_divide;//x:waveHeight, y:������
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

//�|�X�g�G�t�F�N�g
struct CONSTANT_BUFFER_PostMosaic {
	CoordTf::VECTOR4 mosaicSize;//x
	CoordTf::VECTOR4 blur;//xy:���W, z:����, w:�s���g�������[��
	float focusRange;
};

struct DivideArr {
	float distance;
	float divide;
};

struct VERTEX_DXR {
	CoordTf::VECTOR3 Pos = {};//���_
	CoordTf::VECTOR3 Nor = {};//�@��
	CoordTf::VECTOR3 Tangent = {};//�ڃx�N�g��
	CoordTf::VECTOR2 Tex[2] = {};//UV���W
};

#endif