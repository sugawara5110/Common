///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                          ShaderSkinMesh_D.hlsl                                        //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

cbuffer global_bones : register(b1, space0) //ボーンのポーズ行列が入る
{
	matrix g_mConstBoneWorld[256];
};

//スキニング後の頂点・法線が入る
struct Skin
{
	float4 Pos : POSITION;
	float3 Nor : NORMAL;
	float3 Tan : TANGENT;
	float3 GNor : GEO_NORMAL;
};
//バーテックスバッファーの入力
struct VSSkinIn
{
	float3 Pos : POSITION; //頂点   
	float3 Nor : NORMAL; //法線
	float3 Tan : TANGENT; //接ベクトル
	float3 GNor : GEO_NORMAL; //ジオメトリ法線
	float2 Tex0 : TEXCOORD0; //テクスチャー座標0
	float2 Tex1 : TEXCOORD1; //テクスチャー座標1
	uint4 Bones : BONE_INDEX; //ボーンのインデックス
	float4 Weights : BONE_WEIGHT; //ボーンの重み
};

//指定した番号のボーンのポーズ行列を返す　サブ関数（バーテックスシェーダーで使用）
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
	float3 GNor = Input.GNor;
//ボーン0
	uint iBone = Input.Bones.x;
	float fWeight = Input.Weights.x;
	matrix m = FetchBoneMatrix(iBone);
	Output.Pos += fWeight * mul(Pos, m);
	Output.Nor += fWeight * mul(Nor, (float3x3) m);
	Output.Tan += fWeight * mul(Tan, (float3x3) m);
	Output.GNor += fWeight * mul(GNor, (float3x3) m);
//ボーン1
	iBone = Input.Bones.y;
	fWeight = Input.Weights.y;
	m = FetchBoneMatrix(iBone);
	Output.Pos += fWeight * mul(Pos, m);
	Output.Nor += fWeight * mul(Nor, (float3x3) m);
	Output.Tan += fWeight * mul(Tan, (float3x3) m);
	Output.GNor += fWeight * mul(GNor, (float3x3) m);
//ボーン2
	iBone = Input.Bones.z;
	fWeight = Input.Weights.z;
	m = FetchBoneMatrix(iBone);
	Output.Pos += fWeight * mul(Pos, m);
	Output.Nor += fWeight * mul(Nor, (float3x3) m);
	Output.Tan += fWeight * mul(Tan, (float3x3) m);
	Output.GNor += fWeight * mul(GNor, (float3x3) m);
//ボーン3
	iBone = Input.Bones.w;
	fWeight = Input.Weights.w;
	m = FetchBoneMatrix(iBone);
	Output.Pos += fWeight * mul(Pos, m);
	Output.Nor += fWeight * mul(Nor, (float3x3) m);
	Output.Tan += fWeight * mul(Tan, (float3x3) m);
	Output.GNor += fWeight * mul(GNor, (float3x3) m);

	return Output;
}

//****************************************バーテックスシェーダー****************************************************//
VS_OUTPUT VS(VSSkinIn input, uint instanceID : SV_InstanceID)
{
	VS_OUTPUT output = (VS_OUTPUT) 0;

	Skin vSkinned = SkinVert(input);

	output.Pos = vSkinned.Pos;
	output.Nor = vSkinned.Nor;
	output.Tan = vSkinned.Tan;
	output.GNor = vSkinned.GNor;
	output.instanceID = instanceID;
	output.Tex0 = input.Tex0;
	output.Tex1 = input.Tex1;

	return output;
}
//****************************************バーテックスシェーダー****************************************************//
