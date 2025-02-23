///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                ShaderCommonParameters.hlsl                                            //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////共通パラメーター////////////////////////////////////////////////////////
Texture2D g_texDiffuse : register(t0, space0);
Texture2D g_texNormal : register(t1, space0);
Texture2D g_texSpecular : register(t2, space0);
SamplerState g_samLinear : register(s0, space0);

struct WVPCB
{
    matrix wvp;
    matrix world;
    float4 ObjCol;
    float4 pXpYmXmY;
};

ConstantBuffer<WVPCB> wvpCb[] : register(b0, space1);

cbuffer global : register(b0, space0)
{
//視点
    float4 g_C_Pos;
//x:ディスプ起伏量, y:divide配列数, z:shininess, w:Smooth範囲
    float4 g_DispAmount;
//x:Smooth比率(初期値0.999f)
    float4 g_SmoothRatio;
//divide配列 x:distance, y:divide
    float4 g_divide[16];
}

cbuffer global_3 : register(b3, space0)
{
//グローバルアンビエント
	float4 g_GlobalAmbientLight;
//xyz:光源位置, w:オンオフ
	float4 g_LightPos[256];
//ライト色
	float4 g_LightColor[256];
//レンジ, 減衰1, 減衰2, 減衰3
	float4 g_Lightst[256];
//x:ライト個数
	float4 g_numLight;
//平行光源方向
	float4 g_DLightDirection;
//平行光源色
	float4 g_DLightColor;
//x:平行光源オンオフ
	float4 g_DLightst;
//フォグ量x, 密度y, onoffz
	float4 g_FogAmo_Density;
//フォグ色
	float4 g_FogColor;
};

cbuffer global_4 : register(b4, space0)
{
//DXR用
	float4 g_instanceID; //x:ID, y:1.0f on 0.0f off
}

struct VS_OUTPUT
{
	float4 Pos : POSITION;
	float3 Nor : NORMAL;
	float3 Tan : TANGENT;
	float3 GNor : GEO_NORMAL;
	float2 Tex0 : TEXCOORD0;
	float2 Tex1 : TEXCOORD1;
	uint instanceID : SV_InstanceID;
};

struct GS_Mesh_INPUT
{
	float4 Pos : POSITION;
	float3 Nor : NORMAL;
	float3 Tan : TANGENT;
	float2 Tex0 : TEXCOORD0;
	float2 Tex1 : TEXCOORD1;
	float3 AddNor : POSITION1;
	uint instanceID : SV_InstanceID;
};

struct PS_INPUT
{
	float4 Pos : SV_POSITION;
	float4 wPos : POSITION;
	float3 Nor : NORMAL;
	float2 Tex0 : TEXCOORD0;
	float2 Tex1 : TEXCOORD1;
	float3 tangent : TANGENT;
	uint instanceID : SV_InstanceID;
};

struct HS_CONSTANT_OUTPUT
{
	float factor[3] : SV_TessFactor;
	float inner_factor : SV_InsideTessFactor;
};

struct HS_OUTPUT
{
	float4 Pos : POSITION;
	float3 Nor : NORMAL;
	float3 Tan : TANGENT;
	float3 GNor : GEO_NORMAL;
	float2 Tex0 : TEXCOORD0;
	float2 Tex1 : TEXCOORD1;
	uint instanceID : SV_InstanceID;
};
//////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////DS後法線再計算/////////////////////////////////////////////////////////
void getNearTexAndHeight(float2 tex, inout float2 NearTex[4], inout float4 NearHeight[4])
{
	float sm = g_DispAmount.w;
	NearTex[0] = float2(saturate(tex.x - sm), saturate(tex.y - sm));
	NearTex[1] = float2(saturate(tex.x + sm), saturate(tex.y - sm));
	NearTex[2] = float2(saturate(tex.x + sm), saturate(tex.y + sm));
	NearTex[3] = float2(saturate(tex.x - sm), saturate(tex.y + sm));

	float di = g_DispAmount.x;
	NearHeight[0] = g_texDiffuse.SampleLevel(g_samLinear, NearTex[0], 0) * di;
	NearHeight[1] = g_texDiffuse.SampleLevel(g_samLinear, NearTex[1], 0) * di;
	NearHeight[2] = g_texDiffuse.SampleLevel(g_samLinear, NearTex[2], 0) * di;
	NearHeight[3] = g_texDiffuse.SampleLevel(g_samLinear, NearTex[3], 0) * di;
}

float3 getSmoothPreparationVec(float2 NearTex[4], float4 NearHeight[4], float addHeight[4])
{
	float hei0 = (NearHeight[0].x + NearHeight[0].y + NearHeight[0].z) / 3.0f + addHeight[0];
	float hei1 = (NearHeight[1].x + NearHeight[1].y + NearHeight[1].z) / 3.0f + addHeight[1];
	float hei2 = (NearHeight[2].x + NearHeight[2].y + NearHeight[2].z) / 3.0f + addHeight[2];
	float hei3 = (NearHeight[3].x + NearHeight[3].y + NearHeight[3].z) / 3.0f + addHeight[3];

	float3 v0 = float3(NearTex[0], hei0);
	float3 v1 = float3(NearTex[1], hei1);
	float3 v2 = float3(NearTex[2], hei2);
	float3 v3 = float3(NearTex[3], hei3);

	float3 vecX0 = normalize(v0 - v1);
	float3 vecY0 = normalize(v0 - v2);
	float3 vecX1 = normalize(v0 - v2);
	float3 vecY1 = normalize(v0 - v3);
	float3 cv0 = cross(vecX0, vecY0);
	float3 cv1 = cross(vecX1, vecY1);
	return (cv0 + cv1) * 0.5f;
}

float3 NormalRecalculationSmoothPreparation(float2 tex)
{
	float2 nTex[4];
	float4 nHei[4];
	getNearTexAndHeight(tex, nTex, nHei);

	float dummy[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	return getSmoothPreparationVec(nTex, nHei, dummy);
}

GS_Mesh_INPUT NormalRecalculationSmooth(GS_Mesh_INPUT input)
{
	GS_Mesh_INPUT output;
	output = input;

	float ratio = g_SmoothRatio.x;
	float3 tmpNor = output.Nor;
	output.Nor = output.AddNor * ratio + output.Nor * (1.0f - ratio);
	float3 difN = output.Nor - tmpNor;
	output.Tan += difN;

	return output;
}

void NormalRecalculationEdge(inout GS_Mesh_INPUT Input[3])
{
	float3 vecX = normalize(Input[0].Pos.xyz - Input[1].Pos.xyz);
	float3 vecY = normalize(Input[0].Pos.xyz - Input[2].Pos.xyz);
	float3 vecNor = cross(vecX, vecY);

	float3 differenceN[3];
	differenceN[0] = vecNor - Input[0].Nor;
	differenceN[1] = vecNor - Input[1].Nor;
	differenceN[2] = vecNor - Input[2].Nor;

	Input[0].Nor = vecNor;
	Input[1].Nor = vecNor;
	Input[2].Nor = vecNor;
	Input[0].Tan += differenceN[0];
	Input[1].Tan += differenceN[1];
	Input[2].Tan += differenceN[2];
}
