///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                      ShaderLocalParameters.hlsl                                       //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

#define NONREFLECTION  64 //0b1000000
#define METALLIC       32 //0b0100000
#define EMISSIVE       16 //0b0010000
#define TRANSLUCENCE   8  //0b0001000
#define NEE            4  //0b0000100
#define NEE_PATHTRACER 2  //0b0000010
#define NONE           1  //0b0000001

RWTexture2D<float4> gOutput : register(u0, space0);
RWTexture2D<float> gDepthOut : register(u1, space0);
RWTexture2D<float> gInstanceIdMap : register(u2, space0);
RWTexture2D<float4> gNormalMap : register(u3, space0);
RWTexture2D<float> gPrevDepthOut : register(u4, space0);
RWTexture2D<float4> gPrevNormalMap : register(u5, space0);

StructuredBuffer<uint> Indices[] : register(t0, space1);//無制限配列の場合,別なレジスタ空間にした方が(・∀・)ｲｲ!! みたい

struct MaterialCB
{
	float4 Diffuse;
	float4 Speculer;
	float4 Ambient;
	float shininess;
	float RefractiveIndex; //屈折率
	float roughness; //粗さ
	uint materialNo;
};

ConstantBuffer<MaterialCB> material[] : register(b1, space3);

struct WVPCB
{
    matrix wvp;
    matrix world;
    float4 AddObjColor;
};

ConstantBuffer<WVPCB> wvp[] : register(b2, space4);

Texture2D g_texDiffuse[] : register(t0, space10);
Texture2D g_texNormal[] : register(t1, space11);
Texture2D g_texSpecular[] : register(t2, space12);

struct Vertex
{
	float3 Pos;
	float3 normal;
	float3 tangent;
	float2 tex[2];
};

struct Vertex3
{
	Vertex v[3];
};

StructuredBuffer<Vertex> Vertices[] : register(t3, space13);