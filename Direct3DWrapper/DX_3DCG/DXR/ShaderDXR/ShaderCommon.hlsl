///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                      ShaderCommon.hlsl                                                //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "ShaderGlobalParameters.hlsl"
#include "ShaderLocalParameters.hlsl"
#include "../../Core/ShaderCG/ShaderNormalTangent.hlsl"

///////////////////////////////////////////traceRay////////////////////////////////////////////////
void traceRay(in uint RecursionCnt, 
              in uint RayFlags, 
              in uint HitGroupIndex, 
              in uint MissShaderIndex, 
              in RayDesc ray, 
              inout RayPayload payload)
{
	ray.TMin = TMin_TMax.x;
	ray.TMax = TMin_TMax.y;
	payload.color = float3(0.0f, 0.0f, 0.0f);
	payload.RecursionCnt = RecursionCnt + 1;
	payload.EmissiveIndex = 0;
	payload.reTry = false;
	payload.hit = false;

	if (RecursionCnt <= maxRecursion)
	{
		bool loop = true;
		while (loop)
		{
			ray.Origin = payload.hitPosition;
			TraceRay(gRtScene, RayFlags, 0xFF, HitGroupIndex, 0, MissShaderIndex, ray, payload);
			loop = payload.reTry;
		}
	}
}

///////////////////////////////////////////ランダムfloat///////////////////////////////////////////
float Rand_float(in float2 v2, inout uint Seed)
{
    Seed++;
    return sin(dot(v2, float2(12.9898, 78.233)) * (SeedFrame % 100 + 1) * 0.001 + Seed + SeedFrame) * 43758.5453;
}

///////////////////////////////////////////ランダム整数////////////////////////////////////////////
uint Rand_integer(inout uint Seed)
{
    float2 index = (float2) DispatchRaysIndex().xy;
    return (uint) (abs(Rand_float(index, Seed)));
}

///////////////////////////////////////////ランダム少数////////////////////////////////////////////
float Rand_frac(in float2 v2, inout uint Seed)
{
    return frac(Rand_float(v2, Seed));
}

///////////////////////////////////////////ランダムベクトル////////////////////////////////////////
float3 RandomVector(in float3 v, in float area, inout uint Seed)
{
    float2 index = (float2) DispatchRaysIndex().xy;
    float rand1 = Rand_frac(index, Seed);
    float rand2 = Rand_frac(index + 0.5f, Seed);

//ランダムなベクトルを生成
    float z = area * rand1 - 1.0f;
    float phi = PI * (2.0f * rand2 - 1.0f);
    float sq = sqrt(1.0f - z * z);
    float x = sq * cos(phi);
    float y = sq * sin(phi);
    float3 randV = float3(x, y, z);

    return -localToWorld(v, randV);
}

///////////////////////////////////////////Material識別////////////////////////////////////////////
bool materialIdent(uint matNo, uint MaterialBit)
{
	return (matNo & MaterialBit) == MaterialBit;
}

///////////////////////////////////////////MaterialID取得//////////////////////////////////////////
uint getMaterialID()
{
	uint instanceID = InstanceID();
	return (instanceID >> 16) & 0x0000ffff;
}

////////////////////////////////////////InstancingID取得///////////////////////////////////////////
uint getInstancingID()
{
	uint instanceID = InstanceID();
	return instanceID & 0x0000ffff;
}

///////////////////////////////////////ヒット位置取得/////////////////////////////////////////////
float3 HitWorldPosition()
{
//     原点           現在のヒットまでの距離      レイを飛ばした方向
	return WorldRayOrigin() + RayTCurrent() * WorldRayDirection();
}

///////////////////////////////////////マテリアルCB取得////////////////////////////////////////////
MaterialCB getMaterialCB()
{
    return material[getMaterialID()];
}

///////////////////////////////////////頂点取得////////////////////////////////////////////////////
Vertex3 getVertex()
{
	uint indicesPerTriangle = 3;
	uint baseIndex = PrimitiveIndex() * indicesPerTriangle;
	uint materialID = getMaterialID();

	Vertex3 ver =
	{
		Vertices[materialID][Indices[materialID][baseIndex + 0]],
        Vertices[materialID][Indices[materialID][baseIndex + 1]],
        Vertices[materialID][Indices[materialID][baseIndex + 2]]
	};
	return ver;
}

///////////////////////////////////////深度値取得//////////////////////////////////////////////////
float getDepth(in BuiltInTriangleIntersectionAttributes attr, Vertex3 v3)
{
	float3 vertex[3] =
	{
		v3.v[0].Pos,
        v3.v[1].Pos,
        v3.v[2].Pos
	};

	float3 v = vertex[0] +
        attr.barycentrics.x * (vertex[1] - vertex[0]) +
        attr.barycentrics.y * (vertex[2] - vertex[0]);
	float4 ver = float4(v, 1.0f);

	matrix m = wvp[getInstancingID()].wvp;
	float4 ver2 = mul(ver, m);
	return ver2.z / ver2.w;
}

////////////////Hitの重心を使用して、頂点属性から補間されたhit位置の属性を取得(法線)///////////////
float3 getCenterNormal(in float3 vertexNormal[3], in BuiltInTriangleIntersectionAttributes attr)
{
	return vertexNormal[0] +
        attr.barycentrics.x * (vertexNormal[1] - vertexNormal[0]) +
        attr.barycentrics.y * (vertexNormal[2] - vertexNormal[0]);
}

////////////////////////////////法線取得//////////////////////////////////////////////////////////
float3 getNormal(in BuiltInTriangleIntersectionAttributes attr, Vertex3 v3)
{
	float3 vertexNormals[3] =
	{
		v3.v[0].normal,
        v3.v[1].normal,
        v3.v[2].normal
	};
	return getCenterNormal(vertexNormals, attr);
}
////////////////////////////////接ベクトル取得//////////////////////////////////////////////////////////
float3 get_tangent(in BuiltInTriangleIntersectionAttributes attr, Vertex3 v3)
{
	float3 vertexTangents[3] =
	{
		v3.v[0].tangent,
        v3.v[1].tangent,
        v3.v[2].tangent
	};
	return getCenterNormal(vertexTangents, attr);
}

////////////////Hitの重心を使用して、頂点属性から補間されたhit位置の属性を取得(UV)////////////////
float2 getCenterUV(in float2 vertexUV[3], in BuiltInTriangleIntersectionAttributes attr)
{
	return vertexUV[0] +
        attr.barycentrics.x * (vertexUV[1] - vertexUV[0]) +
        attr.barycentrics.y * (vertexUV[2] - vertexUV[0]);
}

////////////////////////////////UV取得//////////////////////////////////////////////////////////
float2 getUV(in BuiltInTriangleIntersectionAttributes attr, in uint uvNo, Vertex3 v3)
{
	float2 vertexUVs[3] =
	{
		v3.v[0].tex[uvNo],
        v3.v[1].tex[uvNo],
        v3.v[2].tex[uvNo]
	};
	return getCenterUV(vertexUVs, attr);
}

/////////////////////////////ノーマルテクスチャから法線取得/////////////////////////////////////
float3 getNormalMap(in float3 normal, in float2 uv, in float3 tangent)
{
	NormalTangent tan;
	uint instancingID = getInstancingID();
	uint materialID = getMaterialID();
//接ベクトル計算
	tan = GetTangent(normal, (float3x3) wvp[getInstancingID()].world, tangent); //ShaderCG内関数
//法線テクスチャ
	float4 Tnor = g_texNormal[materialID].SampleLevel(g_samLinear, uv, 0.0);
//ノーマルマップでの法線出力
	return GetNormal(Tnor.xyz, tan.normal, tan.tangent); //ShaderCG内関数
}

//////////////////////////////////////ピクセル値取得///////////////////////////////////////////
//////////////ディフェーズ
float4 getDifPixel(in BuiltInTriangleIntersectionAttributes attr, Vertex3 v3)
{
	uint materialID = getMaterialID();
//UV計算
	float2 UV = getUV(attr, 0, v3);
//ピクセル値
	float4 difTex = g_texDiffuse[materialID].SampleLevel(g_samLinear, UV, 0.0);
	float4 add = wvp[getInstancingID()].AddObjColor;
	difTex.x = saturate(difTex.x + add.x);
	difTex.y = saturate(difTex.y + add.y);
	difTex.z = saturate(difTex.z + add.z);
	difTex.w = saturate(difTex.w + add.w);
	return difTex;
}
//////////////ノーマル
float3 getNorPixel(in BuiltInTriangleIntersectionAttributes attr, Vertex3 v3)
{
	uint materialID = getMaterialID();
//UV計算
	float2 UV = getUV(attr, 0, v3);
//法線の計算
	float3 triangleNormal = getNormal(attr, v3);
//接ベクトル計算
	float3 tan = get_tangent(attr, v3);
//ノーマルマップからの法線出力
	return getNormalMap(triangleNormal, UV, tan);
}
//////////////スペキュラ
float3 getSpePixel(in BuiltInTriangleIntersectionAttributes attr, Vertex3 v3)
{
	uint materialID = getMaterialID();
//UV計算
	float2 UV = getUV(attr, 1, v3);
//ピクセル値
	float4 spe = g_texSpecular[materialID].SampleLevel(g_samLinear, UV, 0.0);
	return spe.xyz;
}

///////////////////////////////////////////LightPDF////////////////////////////////////////////////
float LightPDF(uint emIndex)
{
	float NumEmissive = (float) numEmissive.x;
	float emSize = emissiveNo[emIndex].y;
	return 1.0f / (NumEmissive * emSize);
}

///////////////////////////////////////////IBL_PDF////////////////////////////////////////////////
float IBL_PDF()
{
	return 1.0f / (4 * PI);
}

///////////////////////////////////////////radiusPDF///////////////////////////////////////////////
float radiusPDF()
{
	return 1.0f / (2 * PI);
}

///////////////////////////////////////////DiffuseBRDF/////////////////////////////////////////////
float3 DiffuseBRDF(float3 diffuse)
{
	return diffuse / PI;
}

///////////////////////////////////////////CosinePDF///////////////////////////////////////////////
float CosinePDF(float dotNL)
{
	return dotNL / PI;
}

///////////////////////////////////////////GGX_PDF/////////////////////////////////////////////////
float GGX_PDF(float NDF, float dotNH, float dotVH)
{
	return NDF * dotNH / (4 * dotVH);
}

///////////////////////////////////////////GGX_GeometrySchlick/////////////////////////////////////
float GGX_GeometrySchlick(float dotNX, float roughness)
{
	float a = roughness * roughness;
	float k = a / 2.0f;
	return dotNX / (dotNX * (1 - k) + k);
}

///////////////////////////////////////////GGX_Distribution////////////////////////////////////////
float GGX_Distribution(float3 N, float3 H, float roughness)
{
	float a = roughness * roughness;
	float a2 = a * a;
	float dotNH = max(0.0, dot(N, H));
	float dotNH2 = dotNH * dotNH;

	float d = (dotNH2 * (a2 - 1.0f) + 1.0f);
	d *= PI * d;

	return a2 / d;
}

///////////////////////////////////////////GGX_GeometrySmith///////////////////////////////////////
float GGX_GeometrySmith(float3 N, float3 V, float3 L, float roughness)
{
	float dotNV = abs(dot(N, V));
	float dotNL = abs(dot(N, L));
	return GGX_GeometrySchlick(dotNV, roughness) * GGX_GeometrySchlick(dotNL, roughness);
}

///////////////////////////////////////////FresnelSchlick/////////////////////////////////////////
float3 FresnelSchlick(float dotVH, float3 F0)
{
	return F0 + (1 - F0) * pow(1 - dotVH, 5.0);
}

///////////////////////////////////////////SpecularBRDF///////////////////////////////////////////
float3 SpecularBRDF(float D, float G, float3 F, float3 V, float3 L, float3 N)
{
	float dotNL = abs(dot(N, L));
	float dotNV = abs(dot(N, V));
	return (D * G * F) / (4 * dotNV * dotNL + 0.001f);
}
float3 FullySpecularBRDF(float3 F, float3 L, float3 N)
{
    float dotNL = abs(dot(N, L));
    return F / dotNL;
}

///////////////////////////////////////////DiffSpeBSDF/////////////////////////////////////////////
float3 DiffSpeBSDF(float3 inDir, float3 outDir, float3 difTexColor, float3 speTexColor, float3 normal, out float PDF)
{
    const uint materialID = getMaterialID();
    const MaterialCB mcb = material[materialID];
    const float3 Diffuse = mcb.Diffuse.xyz;
    const float3 Speculer = mcb.Speculer.xyz;
    const float roughness = mcb.roughness;

    if (dot(normal, inDir) <= 0)
    {
        PDF = 1.0f;
        return float3(0, 0, 0);
    }

    const float sum_diff = Diffuse.x + Diffuse.y + Diffuse.z;
    const float sum_spe = Speculer.x + Speculer.y + Speculer.z;
    const float sum = sum_diff + sum_spe;
    const float diff_threshold = sum_diff / sum;
    const float spe_threshold = sum_spe / sum;

    const float3 H = normalize(inDir + outDir);

    const float dotNL = abs(dot(normal, inDir));
    const float dotNH = abs(dot(normal, H));
    const float dotVH = abs(dot(outDir, H));

    float3 F0 = 0.08.xxx;
    F0 = lerp(F0 * speTexColor, difTexColor, (spe_threshold).xxx);

    const float NDF = GGX_Distribution(normal, H, roughness);
    const float G = GGX_GeometrySmith(normal, outDir, inDir, roughness);
    const float3 F = FresnelSchlick(max(dot(outDir, H), 0), F0);
    const float3 kD = (1 - F) * (1 - spe_threshold);

    float3 speBRDF;
    float spePDF;
	
    if (roughness <= 0.0f)
    {
        speBRDF = FullySpecularBRDF(F, inDir, normal);
        spePDF = 1.0f;
    }
    else
    {
        speBRDF = SpecularBRDF(NDF, G, F, outDir, inDir, normal);
        spePDF = GGX_PDF(NDF, dotNH, dotVH);
    }
	
    const float3 diffBRDF = DiffuseBRDF(difTexColor);
    const float diffPDF = CosinePDF(dotNL);
    const float3 sumBSDF = (diffBRDF * kD + speBRDF) * dotNL;
    const float sumPDF = diff_threshold * diffPDF + spe_threshold * spePDF;

    if (sumPDF <= 0)
    {
        PDF = 1.0f;
        return float3(0, 0, 0);
    }

    PDF = sumPDF;
    return sumBSDF;
}

///////////////////////////////////////////RefractionBTDF///////////////////////////////////////////
float3 RefractionBTDF(float D, float G, float3 F, float3 V, float3 L, float3 N, float3 H, float in_eta, float out_eta)
{
    const float dotNL = abs(dot(N, L));
    const float dotNV = abs(dot(N, V));
    const float dotHL = abs(dot(H, L));
    const float dotHV = abs(dot(H, V));

    const float a = dotHL * dotHV / (dotNL * dotNV);
    const float3 b = out_eta * out_eta * (1 - F) * G * D;
    const float c = pow((in_eta * dotHL + out_eta * dotHV), 2) + 0.001f;
    return a * b / c;
}
float3 FullyRefractionBTDF(float3 F, float3 L, float3 N, float in_eta, float out_eta)
{
    const float dotNL = abs(dot(N, L));
   
    const float a = out_eta * out_eta / (in_eta * in_eta);
    const float3 b = 1 - F;
    const float c = 1.0f / dotNL;
    return a * b * c;
}

///////////////////////////////////////////RefSpeBSDF//////////////////////////////////////////////
float3 RefSpeBSDF(float3 inDir, float3 outDir, float4 difTexColor, float3 speTexColor, float3 N, float3 H, 
                  float in_eta, float out_eta, out float PDF)
{
    const float Alpha = difTexColor.w;
    const float speRatio = Alpha;

    const float dotNL = abs(dot(N, inDir));
    const float dotNV = abs(dot(N, outDir));
    const float dotNH = abs(dot(N, H));
    const float dotVH = abs(dot(outDir, H));
    const float dotLH = abs(dot(inDir, H));

    const uint materialID = getMaterialID();
    const MaterialCB mcb = material[materialID];
    const float3 Diffuse = mcb.Diffuse.xyz * difTexColor.xyz;
    const float3 Speculer = mcb.Speculer.xyz * speTexColor;
    const float roughness = mcb.roughness;

    float3 F0 = 0.08.xxx * Speculer;
    float3 F = FresnelSchlick(max(dot(H, outDir), 0), F0);

    float NDF = GGX_Distribution(N, H, roughness);
    float G = GGX_GeometrySmith(N, outDir, inDir, roughness);
    
    float3 speBRDF;
    float spePDF;
    float3 refrBTDF;
    float refrPDF;
    
    if (roughness <= 0.0f)
    {
        speBRDF = FullySpecularBRDF(F, inDir, N);
        spePDF = 1.0f;
        refrBTDF = FullyRefractionBTDF(F, inDir, N, in_eta, out_eta);
        refrPDF = 1.0f;
    }
    else
    {
        speBRDF = SpecularBRDF(NDF, G, F, outDir, inDir, N);
        spePDF = GGX_PDF(NDF, dotNH, dotVH);
        refrBTDF = RefractionBTDF(NDF, G, F, outDir, inDir, N, H, in_eta, out_eta);
        refrPDF = GGX_PDF(NDF, dotNH, dotVH);
    }

    const float3 sumBSDF = (speBRDF + refrBTDF * Diffuse) * dotNL;
    const float sumPDF = speRatio * spePDF + (1 - speRatio) * refrPDF;

    if (sumPDF <= 0)
    {
        PDF = 1.0f;
        return float3(0, 0, 0);
    }

    PDF = sumPDF;
    return sumBSDF;
}

///////////////////////////////////////////BSDF////////////////////////////////////////////////////
float3 BSDF(bool bsdf_f, float3 inDir, float3 outDir, float4 difTexColor, float3 speTexColor, float3 N,
            float in_eta, float out_eta, out float PDF)
{
    float3 bsdf;

    if (bsdf_f)
    {
        const float3 H = normalize(-inDir + outDir);
        bsdf = RefSpeBSDF(-inDir, outDir, difTexColor, speTexColor, N, H, in_eta, out_eta, PDF);
    }
    else
    {
        bsdf = DiffSpeBSDF(inDir, outDir, difTexColor.xyz, speTexColor, N, PDF);
    }
    return bsdf;
}

////////////////////////////////////////////SkyLight////////////////////////////////////////////////
float3 getSkyLight(float3 dir)
{
    float2 uv = float2(atan2(dir.z, dir.x) / 2.0f / PI + 0.5f, acos(dir.y) / PI);
    float4 ret = g_texImageBasedLighting.SampleLevel(g_samLinear, uv, 0.0);
    return ret.xyz;
}