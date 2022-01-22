///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                ShaderCommonParameters.hlsl                                            //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

char* ShaderCommonParameters =
//////////////////////////////////共通パラメーター////////////////////////////////////////////////////////
"Texture2D g_texDiffuse : register(t0, space0);\n"
"Texture2D g_texNormal : register(t1, space0);\n"
"Texture2D g_texSpecular : register(t2, space0);\n"
"SamplerState g_samLinear : register(s0, space0);\n"

"struct WVPCB {\n"
"    matrix wvp;\n"
"    matrix world;\n"
//オブジェクト追加カラー
"    float4 ObjCol;\n"
"};\n"
"ConstantBuffer<WVPCB> wvpCb[] : register(b0, space1);\n"

"cbuffer global : register(b0, space0)\n"
"{\n"
//視点
"    float4 g_C_Pos;\n"
//グローバルアンビエント
"    float4 g_GlobalAmbientLight;\n"
//xyz:光源位置, w:オンオフ
"    float4 g_LightPos[256];\n"
//ライト色
"    float4 g_LightColor[256];\n"
//レンジ, 減衰1, 減衰2, 減衰3
"    float4 g_Lightst[256];\n"
//x:ライト個数
"    float4 g_numLight;\n"
//平行光源方向
"    float4 g_DLightDirection;\n"
//平行光源色
"    float4 g_DLightColor;\n"
//x:平行光源オンオフ
"    float4 g_DLightst;\n"
//フォグ量x, 密度y, onoffz
"    float4 g_FogAmo_Density;\n"
//フォグ色
"    float4 g_FogColor;\n"
//x:ディスプ起伏量, y:divide配列数, z:shininess, w:Smooth範囲
"    float4 g_DispAmount;\n"
//x:Smooth比率(初期値0.999f)
"    float4 g_SmoothRatio;\n"
//divide配列 x:distance, y:divide
"    float4 g_divide[16];\n"
//UV座標移動用
"    float4 g_pXpYmXmY;\n"
"};\n"

//マテリアル毎の色
"cbuffer global_1 : register(b1, space0)\n"
"{\n"
"    float4 g_Diffuse;\n"
"    float4 g_Speculer; \n"
"    float4 g_Ambient;\n"
"};\n"

"cbuffer global_2 : register(b3, space0)\n"
"{\n"
//DXR用
"    float4 g_instanceID;\n"//x:ID, y:1.0f on 0.0f off
"};\n"

"struct VS_OUTPUT\n"
"{\n"
"    float4 Pos    : POSITION;\n"
"    float3 Nor    : NORMAL;\n"
"    float3 Tan    : TANGENT;\n"
"    float3 GNor   : GEO_NORMAL;\n"
"    float2 Tex0   : TEXCOORD0;\n"
"    float2 Tex1   : TEXCOORD1;\n"
"    uint   instanceID : SV_InstanceID;\n"
"};\n"

"struct GS_Mesh_INPUT\n"
"{\n"
"    float4 Pos   : POSITION;\n"
"    float3 Nor   : NORMAL;\n"
"    float3 Tan   : TANGENT;\n"
"    float2 Tex0  : TEXCOORD0;\n"
"    float2 Tex1  : TEXCOORD1;\n"
"    float3 AddNor: POSITION1;\n"
"    uint   instanceID : SV_InstanceID;\n"
"};\n"

"struct PS_INPUT\n"
"{\n"
"    float4 Pos      : SV_POSITION;\n"
"    float4 wPos     : POSITION;\n"
"    float3 Nor      : NORMAL;\n"
"    float2 Tex0     : TEXCOORD0;\n"
"    float2 Tex1     : TEXCOORD1;\n"
"    float3 tangent  : TANGENT;\n"
"    uint   instanceID : SV_InstanceID;\n"
"};\n"

"struct HS_CONSTANT_OUTPUT\n"
"{\n"
"	 float factor[3]    : SV_TessFactor;\n"
"	 float inner_factor : SV_InsideTessFactor;\n"
"};\n"

"struct HS_OUTPUT\n"
"{\n"
"    float4 Pos   : POSITION;\n"
"    float3 Nor   : NORMAL;\n"
"    float3 Tan   : TANGENT;\n"
"    float3 GNor  : GEO_NORMAL;\n"
"    float2 Tex0  : TEXCOORD0;\n"
"    float2 Tex1  : TEXCOORD1;\n"
"    uint   instanceID : SV_InstanceID;\n"
"};\n"
//////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////DS後法線再計算/////////////////////////////////////////////////////////
"void getNearTexAndHeight(float2 tex, inout float2 NearTex[4], inout float4 NearHeight[4])\n"
"{\n"
"   float sm = g_DispAmount.w;\n"
"   NearTex[0] = float2(saturate(tex.x - sm), saturate(tex.y - sm));\n"
"   NearTex[1] = float2(saturate(tex.x + sm), saturate(tex.y - sm));\n"
"   NearTex[2] = float2(saturate(tex.x + sm), saturate(tex.y + sm));\n"
"   NearTex[3] = float2(saturate(tex.x - sm), saturate(tex.y + sm));\n"

"   float di = g_DispAmount.x;\n"
"   NearHeight[0] = g_texDiffuse.SampleLevel(g_samLinear, NearTex[0], 0) * di;\n"
"   NearHeight[1] = g_texDiffuse.SampleLevel(g_samLinear, NearTex[1], 0) * di;\n"
"   NearHeight[2] = g_texDiffuse.SampleLevel(g_samLinear, NearTex[2], 0) * di;\n"
"   NearHeight[3] = g_texDiffuse.SampleLevel(g_samLinear, NearTex[3], 0) * di;\n"
"}\n"

"float3 getSmoothPreparationVec(float2 NearTex[4], float4 NearHeight[4], float addHeight[4])\n"
"{\n"
"   float hei0 = (NearHeight[0].x + NearHeight[0].y + NearHeight[0].z) / 3.0f + addHeight[0];\n"
"   float hei1 = (NearHeight[1].x + NearHeight[1].y + NearHeight[1].z) / 3.0f + addHeight[1];\n"
"   float hei2 = (NearHeight[2].x + NearHeight[2].y + NearHeight[2].z) / 3.0f + addHeight[2];\n"
"   float hei3 = (NearHeight[3].x + NearHeight[3].y + NearHeight[3].z) / 3.0f + addHeight[3];\n"

"   float3 v0 = float3(NearTex[0], hei0);\n"
"   float3 v1 = float3(NearTex[1], hei1);\n"
"   float3 v2 = float3(NearTex[2], hei2);\n"
"   float3 v3 = float3(NearTex[3], hei3);\n"

"   float3 vecX0 = normalize(v0 - v1);\n"
"   float3 vecY0 = normalize(v0 - v2);\n"
"   float3 vecX1 = normalize(v0 - v2);\n"
"   float3 vecY1 = normalize(v0 - v3);\n"
"   float3 cv0 = cross(vecX0, vecY0);\n"
"   float3 cv1 = cross(vecX1, vecY1);\n"
"   return (cv0 + cv1) * 0.5f;\n"
"}\n"

"float3 NormalRecalculationSmoothPreparation(float2 tex)\n"
"{\n"
"   float2 nTex[4];\n"
"   float4 nHei[4];\n"
"   getNearTexAndHeight(tex, nTex, nHei);\n"

"   float dummy[4] = {0.0f, 0.0f, 0.0f, 0.0f};\n"
"   return getSmoothPreparationVec(nTex, nHei, dummy);\n"
"}\n"

"GS_Mesh_INPUT NormalRecalculationSmooth(GS_Mesh_INPUT input)\n"
"{\n"
"   GS_Mesh_INPUT output;\n"
"   output = input;\n"

"   float ratio = g_SmoothRatio.x;\n"
"   float3 tmpNor = output.Nor;\n"
"   output.Nor = output.AddNor * ratio + output.Nor * (1.0f - ratio);\n"
"   float3 difN = output.Nor - tmpNor;\n"
"   output.Tan += difN;\n"

"   return output;\n"
"}\n"

"void NormalRecalculationEdge(inout GS_Mesh_INPUT Input[3])\n"
"{\n"
"   float3 vecX = normalize(Input[0].Pos.xyz - Input[1].Pos.xyz);\n"
"   float3 vecY = normalize(Input[0].Pos.xyz - Input[2].Pos.xyz);\n"
"   float3 vecNor = cross(vecX, vecY);\n"

"   float3 differenceN[3];\n"
"   differenceN[0] = vecNor - Input[0].Nor;\n"
"   differenceN[1] = vecNor - Input[1].Nor;\n"
"   differenceN[2] = vecNor - Input[2].Nor;\n"

"   Input[0].Nor = vecNor;\n"
"   Input[1].Nor = vecNor;\n"
"   Input[2].Nor = vecNor;\n"
"   Input[0].Tan += differenceN[0];\n"
"   Input[1].Tan += differenceN[1];\n"
"   Input[2].Tan += differenceN[2];\n"
"}\n"
//////////////////////////////////////////////////////////////////////////////////////////////////////////
 
////////////////////////////////フォグ計算(テクスチャに対して計算)////////////////////////////////////////
"float4 FogCom(float4 FogCol, float4 Fog, float4 CPos, float4 wPos, float4 Tex)\n"
"{\n"
"    float fd;\n"//距離
"    float ff;\n"//フォグファクター
"    if(Fog.z == 1.0f){\n"
"       fd = length(CPos.xyz - wPos.xyz) * 0.01f;\n"//距離計算, 0.01は補正値
"       ff = pow(2.71828, -fd * Fog.y);\n"//フォグファクター計算(変化量)
"       ff *= Fog.x;\n"//フォグ全体の量(小さい方が多くなる)
"       ff = saturate(ff);\n"
"       if(Tex.w > 0.3f){\n"
"         Tex = ff * Tex + (1.0f - ff) * FogCol;\n"
"       }\n"
"    }\n"
"   return Tex;\n"
"}\n";
//////////////////////////////////////////////////////////////////////////////////////////////////////////