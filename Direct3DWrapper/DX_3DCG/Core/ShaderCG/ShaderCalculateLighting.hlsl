///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                ShaderCalculateLighting.hlsl                                           //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////ライト計算OutPut/////////////////////////////////////////////////////
struct LightOut
{
	float3 Diffuse : COLOR;
	float3 Speculer : COLOR;
};
/////////////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////ランバート拡散反射/////////////////////////////////////////////////
float3 Lambert(float distAtten, float3 lightCol, float3 Diffuse, float3 Ambient, float3 Nor, float3 LVec)
{
//角度減衰率ディフェーズ
//内積で計算するので光源のベクトルを逆にする
	float angleAttenDif = saturate(dot(-LVec, Nor));

	return distAtten * lightCol * angleAttenDif * Diffuse + Ambient;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////フォン鏡面反射/////////////////////////////////////////////////////
float3 Phong(float distAtten, float3 lightCol, float3 SpeculerCol, float3 Nor, float3 LVec,
             float3 wPos, float3 eyePos, float shininess)
{
//視線ベクトル
	float3 eyeVec = normalize(wPos - eyePos);
//反射ベクトル
	float3 reflectVec = normalize(reflect(LVec, Nor));
//角度減衰率スペキュラ
	float angleAttenSpe = pow(saturate(dot(reflectVec, -eyeVec)), shininess);

	return distAtten * lightCol * angleAttenSpe * SpeculerCol;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////ライト計算/////////////////////////////////////////////////////////
LightOut LightCom(float3 SpeculerCol, float3 Diffuse, float3 Ambient, float3 Nor,
                  float3 wPos, float3 lightCol, float3 eyePos, float3 lightVec, float distAtten, float shininess)
{
//出力用
	LightOut Out = (LightOut) 0;

//ライトベクトル正規化
	float3 LVec = normalize(lightVec);

//ディフェーズ出力
	Out.Diffuse = Lambert(distAtten, lightCol, Diffuse, Ambient, Nor, LVec);
//スペキュラ出力
	Out.Speculer = Phong(distAtten, lightCol, SpeculerCol, Nor, LVec, wPos, eyePos, shininess);
	return Out;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////ポイントライト計算(ライト数分ループさせて使用する)////////////////////////////////
LightOut PointLightCom(float3 SpeculerCol, float3 Diffuse, float3 Ambient, float3 Nor,
                       float4 lightPos, float3 wPos, float4 lightSt, float3 lightCol, float3 eyePos, float shininess)
{
//出力用
	LightOut Out = (LightOut) 0;

//ライトベクトル (頂点の位置 - 点光源の位置)
	float3 lightVec = wPos - lightPos.xyz;

//頂点から光源までの距離を計算
	float distance = length(lightVec);

//ライトオフ, レンジより外は飛ばす
	if (lightPos.w == 1.0f && distance < lightSt.x)
	{

//距離減衰率         
		float distAtten = 1.0f /
                        (lightSt.y +
                         lightSt.z * distance +
                         lightSt.w * distance * distance);

//光源計算
		Out = LightCom(SpeculerCol, Diffuse, Ambient, Nor,
                      wPos, lightCol, eyePos, lightVec, distAtten, shininess);

	}
	return Out;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////ポイントライト計算, 距離減衰無し///////////////////////////////////////////////////
LightOut PointLightComNoDistance(float3 SpeculerCol, float3 Diffuse, float3 Ambient, float3 Nor, 
                                 float4 lightPos, float3 wPos, float3 lightCol, float3 eyePos, float shininess)
{
//ライトベクトル (頂点の位置 - 点光源の位置)
	float3 lightVec = wPos - lightPos.xyz;

//距離減衰率         
	float distAtten = 1.0f;

//光源計算
	return LightCom(SpeculerCol, Diffuse, Ambient, Nor,
                    wPos, lightCol, eyePos, lightVec, distAtten, shininess);
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////平行光源計算/////////////////////////////////////////////////////////
LightOut DirectionalLightCom(float3 SpeculerCol, float3 Diffuse, float3 Ambient, float3 Nor, 
                             float4 DlightSt, float3 Dir, float3 DCol, float3 wPos, float3 eyePos, float shininess)
{
//出力用
	LightOut Out = (LightOut) 0;

	if (DlightSt.x == 1.0f)
	{

//光源計算
		Out = LightCom(SpeculerCol, Diffuse, Ambient, Nor,
                      wPos, DCol, eyePos, Dir, 1.0f, shininess);

	}
	return Out;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////