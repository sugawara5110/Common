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

//////////////////////////////////////ライト計算/////////////////////////////////////////////////////////
"struct LightOut\n"
"{\n"
"    float3 Diffuse: COLOR;\n"
"    float3 Speculer: COLOR;\n"
"};\n"
"LightOut LightCom(float3 SpeculerCol, float3 Diffuse, float3 Ambient, float3 Nor, \n"
"                  float3 wPos, float3 lightCol, float3 eyePos, float3 lightVec, float distAtten)\n"
"{\n"
     //出力用
"    LightOut Out = (LightOut)0;\n"

     //ライトベクトル正規化
"    float3 LVec = normalize(lightVec);\n"
     //角度減衰率ディフェーズ
"    float angleAttenDif = saturate(dot(LVec, Nor));"

     //視線ベクトル
"    float3 eyeVec = normalize(eyePos - wPos);\n"
     //反射ベクトル
"    float3 reflectVec = reflect(-LVec, Nor);\n"
     //角度減衰率スペキュラ
"    float angleAttenSpe = pow(saturate(dot(eyeVec, reflectVec)), 4);\n"//4:shininess

     //ディフェーズ出力
"    Out.Diffuse = distAtten * lightCol * (angleAttenDif * Diffuse + Ambient);\n"
     //スペキュラ出力
"    Out.Speculer = distAtten * lightCol * angleAttenSpe * SpeculerCol;\n"
"    return Out;\n"
"}\n"
//////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////ポイントライト計算(ライト数分ループさせて使用する)////////////////////////////////
"LightOut PointLightCom(float3 SpeculerCol, float3 Diffuse, float3 Ambient, float3 Nor, \n"
"                       float4 lightPos, float3 wPos, float4 lightSt, float3 lightCol, float3 eyePos)\n"
"{\n"
     //出力用
"    LightOut Out = (LightOut)0;\n"

     //ライトベクトル
"    float3 lightVec = lightPos.xyz - wPos;\n"

     //頂点から光源までの距離を計算
"    float distance = length(lightVec);\n"

     //ライトオフ, レンジより外は飛ばす
"    if (lightPos.w == 1.0f && distance < lightSt.x){\n"

        //距離減衰率         
"       float distAtten = 1.0f / \n"
"                        (lightSt.y + \n"
"                         lightSt.z * distance + \n"
"                         lightSt.w * distance * distance);\n"
        
        //光源計算
"       Out = LightCom(SpeculerCol, Diffuse, Ambient, Nor, \n"
"                      wPos, lightCol, eyePos, lightVec, distAtten);\n"

"    }\n"
"    return Out;\n"
"}\n"
//////////////////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////平行光源計算/////////////////////////////////////////////////////////
"LightOut DirectionalLightCom(float3 SpeculerCol, float3 Diffuse, float3 Ambient, float3 Nor, \n"
"                             float4 DlightSt, float3 Dir, float3 DCol, float3 wPos, float3 eyePos)\n"
"{\n"
     //出力用
"    LightOut Out = (LightOut)0;\n"

"    if(DlightSt.x == 1.0f)\n"
"    {\n"

        //光源計算
"       Out = LightCom(SpeculerCol, Diffuse, Ambient, Nor, \n"
"                      wPos, DCol, eyePos, -Dir, 1.0f);\n"

"    }\n"
"    return Out;\n"
"}\n"
//////////////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////共通パラメーター////////////////////////////////////////////////////////
"Texture2D g_texDiffuse : register(t0);\n"
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