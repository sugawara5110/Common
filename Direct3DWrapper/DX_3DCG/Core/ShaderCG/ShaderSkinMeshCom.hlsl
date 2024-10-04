///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                       ShaderSkinMeshCom.hlsl                                          //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

cbuffer global_bones : register(b0, space0) //ボーンのポーズ行列が入る
{
	matrix g_mConstBoneWorld[256];
};

//スキニング後の頂点・法線が入る
struct Skin
{
	float4 Pos;
	float3 Nor;
	float3 Tan;
};
//バーテックスバッファーの入力
struct VSSkinIn
{
	float3 Pos; //頂点   
	float3 Nor; //法線
	float3 Tan; //接ベクトル
	float3 gNor; //フォーマット合わせるため
	float2 Tex0; //テクスチャー座標0
	float2 Tex1; //テクスチャー座標1
	uint4 Bones; //ボーンのインデックス
	float4 Weights; //ボーンの重み
};
struct DXR_INPUT
{
	float3 Pos;
	float3 Nor;
	float3 Tan;
	float2 Tex0;
	float2 Tex1;
};

StructuredBuffer<VSSkinIn> VerticesSkin : register(t0, space0);
RWStructuredBuffer<DXR_INPUT> VerticesDXR : register(u0, space0);

//指定した番号のボーンのポーズ行列を返す
matrix FetchBoneMatrix(uint iBone)
{
	return g_mConstBoneWorld[iBone];
}

//頂点をスキニング
Skin SkinVert(VSSkinIn Input)
{
	Skin Output = (Skin) 0;

	float4 Pos = float4(Input.Pos, 1);
	float3 Nor = Input.Nor;
	float3 Tan = Input.Tan;
//ボーン0
	uint iBone = Input.Bones.x;
	float fWeight = Input.Weights.x;
	matrix m = FetchBoneMatrix(iBone);
	Output.Pos += fWeight * mul(Pos, m);
	Output.Nor += fWeight * mul(Nor, (float3x3) m);
	Output.Tan += fWeight * mul(Tan, (float3x3) m);
//ボーン1
	iBone = Input.Bones.y;
	fWeight = Input.Weights.y;
	m = FetchBoneMatrix(iBone);
	Output.Pos += fWeight * mul(Pos, m);
	Output.Nor += fWeight * mul(Nor, (float3x3) m);
	Output.Tan += fWeight * mul(Tan, (float3x3) m);
//ボーン2
	iBone = Input.Bones.z;
	fWeight = Input.Weights.z;
	m = FetchBoneMatrix(iBone);
	Output.Pos += fWeight * mul(Pos, m);
	Output.Nor += fWeight * mul(Nor, (float3x3) m);
	Output.Tan += fWeight * mul(Tan, (float3x3) m);
//ボーン3
	iBone = Input.Bones.w;
	fWeight = Input.Weights.w;
	m = FetchBoneMatrix(iBone);
	Output.Pos += fWeight * mul(Pos, m);
	Output.Nor += fWeight * mul(Nor, (float3x3) m);
	Output.Tan += fWeight * mul(Tan, (float3x3) m);

	return Output;
}

//****************************************メッシュ頂点**************************************************************//
[numthreads(1, 1, 1)]
void VSSkinCS(int2 id : SV_DispatchThreadID)
{
	int verID = id.x;

	DXR_INPUT output = (DXR_INPUT) 0;

	VSSkinIn input = VerticesSkin[verID];

	Skin vSkinned = SkinVert(input);

	output.Pos = vSkinned.Pos.xyz;
	output.Nor = vSkinned.Nor;
	output.Tan = vSkinned.Tan;
	output.Tex0 = input.Tex0;
	output.Tex1 = input.Tex1;

	VerticesDXR[verID] = output;
}
//****************************************メッシュ頂点**************************************************************//

