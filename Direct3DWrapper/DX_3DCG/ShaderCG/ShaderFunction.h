///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                       ShaderFunction.hlsl                                             //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

char *ShaderFunction =
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
"}\n"
//////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////ポイントライト計算(ライト数分ループさせて使用する)////////////////////////////////
"float3 PointLightCom(float4 SpeculerCol, float4 Diffuse, float3 Nor, float4 shadow, float4 lightPos, float4 wPos, float4 lightSt, float4 lightCol, float4 CPos)\n"
"{\n"
//出力用Col
"    float3 Col = { 0.0f, 0.0f, 0.0f };\n"
//頂点から光源までの距離を計算
"    float distance = length(lightPos.xyz - wPos.xyz);\n"

//ライトオフ, レンジ×3より外は飛ばす
"    if (lightSt.w == 1.0f && distance < lightSt.x * 3){\n"

//ライトベクトル
"       float3 L = normalize(lightPos.xyz - wPos.xyz);\n"
"       float NL = saturate(dot(Nor, L));"

//視線ベクトル
"       float3 EyeVec = normalize(CPos.xyz - wPos.xyz);\n"
//スぺキュラ
"       float3 Reflect = normalize(2 * NL * Nor - L);\n"
"       float specular = pow(saturate(dot(Reflect, EyeVec)), 4);\n"

//デフォルト減衰率
"       float attenuation = 2.0f;\n"
//レンジ外減衰率増減適用
"       if (distance > lightSt.x){ attenuation = lightSt.z; }\n"
//減衰計算           
"       float r = lightSt.y / (pow(distance, attenuation) * 0.001f);\n"

//法線,ライト方向から陰影作成
"       Col = (max(NL, shadow.x) * Diffuse.xyz + specular * SpeculerCol.xyz) * lightCol.xyz * r;\n"
"    }\n"
"    return Col;\n"
"}\n"
//////////////////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////平行光源計算/////////////////////////////////////////////////////////
"float3 DirectionalLightCom(float4 SpeculerCol, float4 Diffuse, float3 Nor, float4 DlightSt, float4 Dir, float4 DCol, float4 wPos, float4 CPos)\n"
"{\n"
//出力用Col
"    float3 Col = { 0.0f, 0.0f, 0.0f };\n"

"    if(DlightSt.y == 1.0f)\n"
"    {\n"

//ライトベクトル
"       float3 L = normalize(Dir.xyz);\n"
"       float NL = max(saturate(dot(Nor, L)), DlightSt.z);\n"

//視線ベクトル
"       float3 EyeVec = normalize(CPos.xyz - wPos.xyz);\n"
//スぺキュラ
"       float3 Reflect = normalize(2 * NL * Nor - L);\n"
"       float specular = pow(saturate(dot(Reflect, EyeVec)), 4);\n"

"       Col = DCol.xyz * DlightSt.x * (Diffuse.xyz * NL + specular * SpeculerCol.xyz);\n"
"    }\n"
"    return Col;\n"
"}\n"
//////////////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////共通パラメーター////////////////////////////////////////////////////////
"Texture2D g_texColor : register(t0);\n"
"Texture2D g_texNormal : register(t1);\n"
"SamplerState g_samLinear : register(s0);\n"

"cbuffer global : register(b0)\n"
"{\n"
"    matrix g_World[256]; \n"
"    matrix g_WVP[256];\n"
//視点
"    float4 g_C_Pos;\n"
//オブジェクト追加カラー
"    float4 g_ObjCol;\n"
//光源位置
"    float4 g_LightPos[256];\n"
//ライト色
"    float4 g_LightColor[256];\n"
//レンジ, 明るさ, 減衰の大きさ, オンオフ
"    float4 g_Lightst[256];\n"
//影の下限値x, ライト個数y
"    float4 g_ShadowLow_Lpcs;\n"
//平行光源方向
"    float4 g_DLightDirection;\n"
//平行光源色
"    float4 g_DLightColor;\n"
//平行光源明るさx,オンオフy, 影の下限値z
"    float4 g_DLightst;\n"
//フォグ量x, 密度y, onoffz
"    float4 g_FogAmo_Density;\n"
//フォグ色
"    float4 g_FogColor;\n"
//ディスプ起伏量x
"    float4 g_DispAmount;\n"
//UV座標移動用
"    float4 g_pXpYmXmY;\n"
"};\n"

//マテリアル毎の色
"cbuffer global_1 : register(b1)\n"
"{\n"
"    float4 g_Diffuse;\n"
"    float4 g_Speculer; \n"
"    float4 g_Ambient;\n"
"};\n"

//DS, PSで使用
"struct PS_INPUT\n"
"{\n"
"    float4 Pos        : SV_POSITION;\n"
"    float4 wPos       : POSITION;\n"
"    float3 Nor        : NORMAL;\n"
"    float2 Tex        : TEXCOORD;\n"
"};\n";
/////////////////////////////////////////////////////////////////////////////////////////////////////////