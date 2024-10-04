///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                  ShaderNormalTangent.hlsl                                             //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

struct NormalTangent
{
	float3 normal : NORMAL;
	float3 tangent : TANGENT;
};

NormalTangent GetTangent(float3 normal, float3x3 world, float3 tangent)
{
	NormalTangent Out = (NormalTangent) 0;

	Out.normal = mul(normal, world);
	Out.normal = normalize(Out.normal);
	Out.tangent = mul(tangent, world);
	Out.tangent = normalize(Out.tangent);

	return Out;
}

void getTangentBinormal(in float3 N, out float3 T, out float3 B)
{
	float3 a = abs(N);
	uint xm = ((a.x - a.y) < 0 && (a.x - a.z) < 0) ? 1 : 0;
	uint ym = (a.y - a.z) < 0 ? (1 ^ xm) : 0;
	uint zm = 1 ^ (xm | ym);
	B = cross(N, float3(xm, ym, zm));
	T = cross(B, N);
}

float3 worldToLocal(float3 N, float3 localVec)
{
	float3 T;
	float3 B;
	getTangentBinormal(N, T, B);
	return normalize(float3(dot(T, localVec), dot(B, localVec), dot(N, localVec)));
}

float3 localToWorld(float3 N, float3 localVec)
{
	float3 T;
	float3 B;
	getTangentBinormal(N, T, B);
	return normalize(T * localVec.x + B * localVec.y + N * localVec.z);
}

float3 normalTexConvert(float3 norT, float3 normal, float3 tangent)
{
	float3 N = normal;
	float3 T = tangent;
	float3 B = cross(T, N);
	return normalize(T * norT.x + B * norT.y + N * norT.z);
}

float3 GetNormal(float3 norTex, float3 normal, float3 tangent)
{
	float3 norT = norTex * 2.0f - 1.0f;
	return normalTexConvert(norT, normal, tangent);
}
