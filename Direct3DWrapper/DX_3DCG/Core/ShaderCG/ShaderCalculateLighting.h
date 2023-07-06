///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                ShaderCalculateLighting.hlsl                                           //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

char* ShaderCalculateLighting =
//////////////////////////////////////ライト計算/////////////////////////////////////////////////////////
"struct LightOut\n"
"{\n"
"    float3 Diffuse: COLOR;\n"
"    float3 Speculer: COLOR;\n"
"};\n"

"LightOut LightCom(float3 SpeculerCol, float3 Diffuse, float3 Ambient, float3 Nor, \n"
"                  float3 wPos, float3 lightCol, float3 eyePos, float3 lightVec, float distAtten, float shininess)\n"
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
"    float angleAttenSpe = pow(saturate(dot(eyeVec, reflectVec)), shininess);\n"

//ディフェーズ出力
"    Out.Diffuse = distAtten * lightCol * angleAttenDif * Diffuse + Ambient;\n"
//スペキュラ出力
"    Out.Speculer = distAtten * lightCol * angleAttenSpe * SpeculerCol;\n"
"    return Out;\n"
"}\n"
//////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////ポイントライト計算(ライト数分ループさせて使用する)////////////////////////////////
"LightOut PointLightCom(float3 SpeculerCol, float3 Diffuse, float3 Ambient, float3 Nor, \n"
"                       float4 lightPos, float3 wPos, float4 lightSt, float3 lightCol, float3 eyePos, float shininess)\n"
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
"                      wPos, lightCol, eyePos, lightVec, distAtten, shininess);\n"

"    }\n"
"    return Out;\n"
"}\n"
//////////////////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////平行光源計算/////////////////////////////////////////////////////////
"LightOut DirectionalLightCom(float3 SpeculerCol, float3 Diffuse, float3 Ambient, float3 Nor, \n"
"                             float4 DlightSt, float3 Dir, float3 DCol, float3 wPos, float3 eyePos, float shininess)\n"
"{\n"
//出力用
"    LightOut Out = (LightOut)0;\n"

"    if(DlightSt.x == 1.0f)\n"
"    {\n"

//光源計算
"       Out = LightCom(SpeculerCol, Diffuse, Ambient, Nor, \n"
"                      wPos, DCol, eyePos, -Dir, 1.0f, shininess);\n"

"    }\n"
"    return Out;\n"
"}\n";
////////////////////////////////////////////////////////////////////////////////////////////////////////