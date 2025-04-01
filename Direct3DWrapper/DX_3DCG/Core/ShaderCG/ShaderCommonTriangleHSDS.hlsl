///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                 ShaderCommonTriangleHSDS.hlsl                                         //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "ShaderCommonParameters.hlsl"
#include "ShaderNormalTangent.hlsl"

//***************************************ハルシェーダーコンスタント*************************************************//
HS_CONSTANT_OUTPUT HSConstant(InputPatch<VS_OUTPUT, 3> ip, uint pid : SV_PrimitiveID)
{
    HS_CONSTANT_OUTPUT output = (HS_CONSTANT_OUTPUT) 0;

    uint instanceID = ip[0].instanceID;

//ワールド変換
    float4 wPos = mul(ip[0].Pos, wvpCb[instanceID].world);
//頂点から現在地までの距離を計算
    float distance = length(g_C_Pos.xyz - wPos.xyz);

//距離でポリゴン数決定
    float divide = 2.0f;
    for (int i = 0; i < g_DispAmount.y; i++)
    {
        if (distance < g_divide[i].x)
        {
            divide = g_divide[i].y;
        }
    }

    output.factor[0] = divide;
    output.factor[1] = divide;
    output.factor[2] = divide;
//u 縦の分割数（横のラインを何本ひくか）
    output.inner_factor = divide;
//divideが2  →   3 *  6頂点
//divideが4  →   3 * 24
//divideが8  →   3 * 96
    return output;
}
//***************************************ハルシェーダーコンスタント*************************************************//

//***************************************ハルシェーダー*************************************************************//
[domain("tri")]
[partitioning("integer")]
[outputtopology("triangle_cw")]//裏表cw, ccw
[outputcontrolpoints(3)]
[patchconstantfunc("HSConstant")]
HS_OUTPUT HS(InputPatch<VS_OUTPUT, 3> ip, uint cpid : SV_OutputControlPointID, uint pid : SV_PrimitiveID)
{
	HS_OUTPUT output = (HS_OUTPUT) 0;
	output.Pos = ip[cpid].Pos;
	output.Nor = ip[cpid].Nor;
	output.Tan = ip[cpid].Tan;
	output.GNor = ip[cpid].GNor;
	output.Tex0 = ip[cpid].Tex0;
	output.Tex1 = ip[cpid].Tex1;
	output.instanceID = ip[cpid].instanceID;
	return output;
}
//***************************************ハルシェーダー*************************************************************//

//**************************************ドメインシェーダー**********************************************************//
//三角形は重心座標系  (UV.x + UV.y + UV.z) == 1.0f が成り立つ
[domain("tri")]
GS_Mesh_INPUT DS(HS_CONSTANT_OUTPUT In, float3 UV : SV_DomaInLocation, const OutputPatch<HS_OUTPUT, 3> patch)
{
	GS_Mesh_INPUT output = (GS_Mesh_INPUT) 0;

//UV座標計算
	output.Tex0 = patch[0].Tex0 * UV.x + patch[1].Tex0 * UV.y + patch[2].Tex0 * UV.z;
	output.Tex1 = patch[0].Tex1 * UV.x + patch[1].Tex1 * UV.y + patch[2].Tex1 * UV.z;

//画像から高さを算出
	float4 height = g_texDiffuse.SampleLevel(g_samLinear, output.Tex0, 0);
	float hei = (height.x + height.y + height.z) / 3 * g_DispAmount.x;

//法線ベクトル
	output.Nor = patch[0].Nor * UV.x + patch[1].Nor * UV.y + patch[2].Nor * UV.z;

//接ベクトル
	output.Tan = patch[0].Tan * UV.x + patch[1].Tan * UV.y + patch[2].Tan * UV.z;

//pos座標計算
	output.Pos = patch[0].Pos * UV.x + patch[1].Pos * UV.y + patch[2].Pos * UV.z;

//ローカル法線の方向にhei分頂点移動(コントロールポイント位置で処理を分ける)
	if (UV.x == 0.0f || UV.y == 0.0f || UV.z == 0.0f)//どれかの要素が0.0fの場合端に有る状態
	{
		float3 geoDir = patch[0].GNor * UV.x + patch[1].GNor * UV.y + patch[2].GNor * UV.z;
		output.Pos.xyz += hei * geoDir; //端はジオメトリ法線使用(クラッキング対策)
	}
	else
	{
		output.Pos.xyz += hei * output.Nor;
	}

//Smooth用
	output.AddNor = NormalRecalculationSmoothPreparation(output.Tex0);
	output.AddNor = normalTexConvert(output.AddNor, output.Nor, output.Tan);

	output.instanceID = patch[0].instanceID;

	return output;
}
//**************************************ドメインシェーダー**********************************************************//
