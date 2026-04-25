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
    payload.EmissiveIndex = -1;
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
    else
    {
        payload.mNo = NONE;
    }
}

///////////////////////////////////////////ハッシュ関数///////////////////////////////////////////
uint pcg_hash(uint input)
{
    uint state = input * 747796405u + 2891336453u;
    uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    return (word >> 22u) ^ word;
}

///////////////////////////////////////////ランダム少数////////////////////////////////////////////
float Rand(inout uint seed)
{
    seed = pcg_hash(seed);
    return float(seed) * (1.0 / 4294967296.0);
}

///////////////////////////////////////////ランダムベクトル////////////////////////////////////////
float3 RandomVector(in float3 v, in float area, inout uint Seed)
{
    float rand1 = Rand(Seed);
    float rand2 = Rand(Seed);

//ランダムなベクトルを生成
    float z = area * rand1 - 1.0f;
    float phi = PI * (2.0f * rand2 - 1.0f);
    float sq = sqrt(1.0f - z * z);
    float x = sq * cos(phi);
    float y = sq * sin(phi);
    float3 randV = float3(x, y, z);

    return -localToWorld(v, randV);
}

////////////////////////////////////コサイン重要度サンプリング////////////////////////////////////
float3 SampleHemisphereCosine(float3 N, inout uint seed)
{
    float r1 = Rand(seed);
    float r2 = Rand(seed);

    // 極座標からローカル座標への変換（コサイン重み付き）
    float phi = 2.0f * PI * r1;
    float cosTheta = sqrt(r2);
    float sinTheta = sqrt(1.0f - r2);

    float3 localDir;
    localDir.x = cos(phi) * sinTheta;
    localDir.y = sin(phi) * sinTheta;
    localDir.z = cosTheta;

    return localToWorld(N, localDir);
}

////////////////////////////////////GGX重要度サンプリング//////////////////////////////////////////
float3 SampleGGX(float3 N, float roughness, inout uint seed)
{
    float a = roughness * roughness;
    float2 Xi = float2(Rand(seed), Rand(seed));
    
    //角度のサンプリング (GGXの分布に基づいた重み付け)
    float phi = 2.0f * PI * Xi.x;
    float cosTheta = sqrt((1.0f - Xi.y) / (1.0f + (a * a - 1.0f) * Xi.y));
    float sinTheta = sqrt(1.0f - cosTheta * cosTheta);
    
    float3 localH;
    localH.x = cos(phi) * sinTheta;
    localH.y = sin(phi) * sinTheta;
    localH.z = cosTheta;
    
    return localToWorld(N, localH);
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

///////////////////////////////////////////MaterialID取得2/////////////////////////////////////////
uint getMaterialID2(uint InstanceID)
{
    return (InstanceID >> 16) & 0x0000ffff;
}

////////////////////////////////////////InstancingID取得///////////////////////////////////////////
uint getInstancingID()
{
	uint instanceID = InstanceID();
	return instanceID & 0x0000ffff;
}

////////////////////////////////////////InstancingID取得2//////////////////////////////////////////
uint getInstancingID2(uint InstanceID)
{
    return InstanceID & 0x0000ffff;
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

///////////////////////////////////////マテリアルCB取得2///////////////////////////////////////////
MaterialCB getMaterialCB2(uint InstanceID)
{
    return material[getMaterialID2(InstanceID)];
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

///////////////////////////////////////頂点取得2///////////////////////////////////////////////////
Vertex3 getVertex2(uint InstanceID, uint PrimitiveIndex)
{
    uint indicesPerTriangle = 3;
    uint baseIndex = PrimitiveIndex * indicesPerTriangle;
    uint materialID = getMaterialID2(InstanceID);

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
    return F0 + (1 - F0) * pow(saturate(1 - dotVH), 5.0);
}

///////////////////////////////////////////FresnelSchlick2/////////////////////////////////////////
float3 FresnelSchlick2(float3 outDir, float3 difTexColor, float3 speTexColor, float3 H)
{
    const uint materialID = getMaterialID();
    const MaterialCB mcb = material[materialID];
    const float3 Specular = mcb.Specular.xyz * speTexColor;
    const float3 Diffuse = mcb.Diffuse.xyz * difTexColor;
    const float metallic = mcb.metallic;
    
    float3 dief0 = float3(0.04f, 0.04f, 0.04f) * Specular;
    float3 metf0 = Diffuse;

    float3 F0 = lerp(dief0, metf0, metallic);
    
    return FresnelSchlick(max(dot(outDir, H), 0), F0);
}

///////////////////////////////////////////FresnelSchlick3/////////////////////////////////////////
float FresnelSchlick3(float3 outDir, float3 difTexColor, float3 speTexColor, float3 H)
{
    const float3 f = FresnelSchlick2(outDir, difTexColor, speTexColor, H);
    return (f.x + f.y + f.z) / 3.0f;
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

///////////////////////////////////////////DiffuseBRDF_PDF////////////////////////////////////////
float3 DiffuseBRDF_PDF(float3 inDir, float3 difTexColor, float3 normal, out float PDF)
{
    const uint materialID = getMaterialID();
    const MaterialCB mcb = material[materialID];
    const float3 Diffuse = mcb.Diffuse.xyz * difTexColor;

    if (dot(normal, inDir) <= 0)
    {
        PDF = 1.0f;
        return float3(0, 0, 0);
    }

    const float dotNL = abs(dot(normal, inDir));
   
    const float3 BRDF = DiffuseBRDF(Diffuse);
    PDF = CosinePDF(dotNL);

    if (PDF <= 0)
    {
        PDF = 1.0f;
        return float3(0, 0, 0);
    }

    return BRDF;
}

///////////////////////////////////////////SpecularBRDF_PDF///////////////////////////////////////
float3 SpecularBRDF_PDF(float3 inDir, float3 outDir, float3 difTexColor, float3 speTexColor, float3 normal, out float PDF)
{
    const uint materialID = getMaterialID();
    const MaterialCB mcb = material[materialID];
    const float roughness = mcb.roughness;

    if (dot(normal, inDir) <= 0)
    {
        PDF = 1.0f;
        return float3(0, 0, 0);
    }

    const float3 H = normalize(inDir + outDir);
    
    const float dotNH = abs(dot(normal, H));
    const float dotVH = abs(dot(outDir, H));

    const float3 F = FresnelSchlick2(outDir, difTexColor, speTexColor, H);

    const float NDF = GGX_Distribution(normal, H, roughness);
    const float G = GGX_GeometrySmith(normal, outDir, inDir, roughness);
    
    float3 BRDF;
	
    if (roughness <= 0.0f)
    {
        BRDF = FullySpecularBRDF(F, inDir, normal);
        PDF = 1.0f;
    }
    else
    {
        BRDF = SpecularBRDF(NDF, G, F, outDir, inDir, normal);
        PDF = GGX_PDF(NDF, dotNH, dotVH);
    }

    if (PDF <= 0)
    {
        PDF = 1.0f;
        return float3(0, 0, 0);
    }

    return BRDF;
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

///////////////////////////////////////////RefractionBTDF_PDF///////////////////////////////////////
float3 RefractionBTDF_PDF(float3 inDir, float3 outDir, float3 difTexColor, float3 speTexColor,
                          float3 N, float in_eta, float out_eta, out float PDF)
{
    float3 H = N;
    const float dotNH = abs(dot(N, H));
    const float dotVH = abs(dot(outDir, H));

    const uint materialID = getMaterialID();
    const MaterialCB mcb = material[materialID];
    const float roughness = mcb.roughness;

    const float3 F = FresnelSchlick2(outDir, difTexColor, speTexColor, H);

    float NDF = GGX_Distribution(N, H, roughness);
    float G = GGX_GeometrySmith(N, outDir, inDir, roughness);
    
    float3 BTDF;
    
    if (roughness <= 0.0f)
    {
        BTDF = FullyRefractionBTDF(F, inDir, N, in_eta, out_eta);
        PDF = 1.0f;
    }
    else
    {
        BTDF = RefractionBTDF(NDF, G, F, outDir, inDir, N, H, in_eta, out_eta);
        PDF = GGX_PDF(NDF, dotNH, dotVH);
    }

    if (PDF <= 0)
    {
        PDF = 1.0f;
        return float3(0, 0, 0);
    }

    return BTDF;
}

///////////////////////////////////////////BSDF////////////////////////////////////////////////////
float3 BSDF(int bsdf_f, float3 inDir, float3 outDir, float3 difTexColor, float3 speTexColor, float3 N,
            float in_eta, float out_eta, out float PDF)
{
    float3 bsdf;

    if (bsdf_f == 0)
    {
        bsdf = RefractionBTDF_PDF(inDir, outDir, difTexColor, speTexColor, N, in_eta, out_eta, PDF);
    }
    else if (bsdf_f == 1)
    {
        bsdf = SpecularBRDF_PDF(inDir, outDir, difTexColor, speTexColor, N, PDF);
    }
    else if (bsdf_f == 2)
    {
        bsdf = DiffuseBRDF_PDF(inDir, difTexColor, N, PDF);
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

////////////////////////////////////短形ライトサンプリング/////////////////////////////////////////
float3 sampleRectLight(uint InstanceID, uint InstancingID, inout uint Seed)
{
    Vertex3 v3 = getVertex2(InstanceID, 0);
    
    float4x4 w = wvp[InstancingID].world;
    float3 p0 = v3.v[0].Pos;
    float3 p1 = v3.v[1].Pos;
    float3 p2 = v3.v[2].Pos;
    
    // p0-----p1
    // |      |
    // |      |
    // p3-----p2
    
    float3 edge1 = p1 - p0;
    float3 edge2 = p2 - p1;
    
    float rand1 = Rand(Seed);
    float rand2 = Rand(Seed);
    
    float4 local_v = float4(p0 + rand1 * edge1 + rand2 * edge2, 1);
    float4 global_v = mul(local_v, w);
    return global_v.xyz;
}

////////////////////////////////////短形ライトArea////////////////////////////////////////////////
float RectLightArea(uint InstanceID, uint InstancingID)
{
    Vertex3 v3 = getVertex2(InstanceID, 0);
    
    float3x3 w = wvp[InstancingID].world;
    float3 p0 = v3.v[0].Pos;
    float3 p1 = v3.v[1].Pos;
    float3 p2 = v3.v[2].Pos;
    
    float3 up = p1 - p0;
    float3 vp = p2 - p1;
    
    float3 global_u = mul(up, w);
    float3 global_v = mul(vp, w);
    
    return length(cross(global_u, global_v));
}

////////////////////////////////////球体ライトサンプリング面積/////////////////////////////////////
float3 sampleSphereLightArea(int emIndex, float3 hitPosition, inout uint Seed)
{
    float u = Rand(Seed);
    float v = Rand(Seed);
    
    //float z = -2.0f * u + 1.0f;//全球
    float z = u; //半球
    float y = sqrt(max(1.0f - z * z, 0.0f)) * sin(2.0f * PI * v);
    float x = sqrt(max(1.0f - z * z, 0.0f)) * cos(2.0f * PI * v);
    
    float global_r = emissiveNo[emIndex].y * 0.5f;
    float3 spherePos = emissivePosition[emIndex].xyz;
    float3 centerToHit = normalize(hitPosition - spherePos);
    
    return global_r * localToWorld(centerToHit, float3(x, y, z)) + spherePos;
}

///////////////////////////////////球体ライトサンプリング立体角////////////////////////////////////
float3 sampleSphereLightSolidAngle(int emIndex, float3 hitPosition, inout uint Seed)
{
    float global_r = emissiveNo[emIndex].y * 0.5f;
    float3 spherePos = emissivePosition[emIndex].xyz;
    
    float3 L = spherePos - hitPosition;
    float distSq = dot(L, L);
    float dist = sqrt(distSq);
    float3 L_norm = L / dist;

    // ヒット点から見た球の「広がり角」の最大値を計算
    float sinThetaMax2 = (global_r * global_r) / distSq;
    float cosThetaMax = sqrt(max(0.0f, 1.0f - sinThetaMax2));

    // 円錐内での一様サンプリング
    float u1 = Rand(Seed);
    float u2 = Rand(Seed);
    
    float cosTheta = 1.0f - u1 * (1.0f - cosThetaMax);
    float sinTheta = sqrt(max(0.0f, 1.0f - cosTheta * cosTheta));
    float phi = 2.0f * PI * u2;

    // ローカル座標（コーンの方向）
    float3 dir = float3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);

    // 方向ベクトルをワールド座標へ（L_normを中央軸にする）
    float3 worldDir = localToWorld(L_norm, dir);

    // 球面上の点を正確に計算
    // レイと球の交差判定の解（近い方の点）を求める
    float b = dot(L, worldDir);
    float c = distSq - global_r * global_r;
    float t = b - sqrt(max(0.0f, b * b - c));

    return hitPosition + worldDir * t;
}

////////////////////////////////////球体ライトサンプリング////////////////////////////////////////
float3 sampleSphereLight(int emIndex, float3 hitPosition, inout uint Seed)
{
    uint emInstanceID = (uint) emissiveNo[emIndex].x;
    MaterialCB emMcb = getMaterialCB2(emInstanceID);
    
    float3 output = float3(0, 0, 0);
    
    if (emMcb.NeeLightSampleType == AREA)
    {
        output = sampleSphereLightArea(emIndex, hitPosition, Seed);
    }
    else
    {
        output = sampleSphereLightSolidAngle(emIndex, hitPosition, Seed);
    }
    return output;
}

///////////////////////////////////球体ライトArea/////////////////////////////////////////////////
float SphereLightArea(int emIndex)
{
    float global_r = emissiveNo[emIndex].y * 0.5f;
    return 2 * PI * global_r * global_r; //半球
}

///////////////////////////////////球体ライト立体角///////////////////////////////////////////////
float SphereLightSolidAngle(int emIndex, float3 hitPosition)
{
    float global_r = emissiveNo[emIndex].y * 0.5f;
    float3 spherePos = emissivePosition[emIndex].xyz;
    float3 L = spherePos - hitPosition;
    float distSq = dot(L, L);
    float sinThetaMax2 = (global_r * global_r) / distSq;
    float cosThetaMax = sqrt(max(0.0f, 1.0f - sinThetaMax2));
    return 2.0f * PI * (1.0f - cosThetaMax);
}

///////////////////////////////////////////othersLightArea/////////////////////////////////////////
float othersLightArea(int emIndex)
{
    float x = emissiveNo[emIndex].y;
    float y = emissiveNo[emIndex].z;
    float z = emissiveNo[emIndex].w;
    
    return 2 * (x * y + y * z + x * z);
}

///////////////////////////////////////////IBL_Area////////////////////////////////////////////////
float IBL_Area()
{
    return 4 * PI * IBL_size * IBL_size;
}

///////////////////////////////////////////LightArea///////////////////////////////////////////////
float LightArea(int emIndex, float3 hitPosition)
{
    uint emInstanceID = (uint) emissiveNo[emIndex].x;
    MaterialCB emMcb = getMaterialCB2(emInstanceID);
    uint emInstancingID = getInstancingID2(emInstanceID);
    uint NeeLightType = emMcb.NeeLightType;
    
    float Area = 0.0f;
    
    if (NeeLightType == RECTANGLE)
    {
        Area = RectLightArea(emInstanceID, emInstancingID);
    }
    else if (NeeLightType == SPHERE)
    {
        if (emMcb.NeeLightSampleType == AREA)
        {
            Area = SphereLightArea(emIndex);
        }
        else
        {
            Area = SphereLightSolidAngle(emIndex, hitPosition);
        }
    }
    else
    {
        Area = othersLightArea(emIndex);
    }
    return Area;
}

///////////////////////////////////////////AllLightArea////////////////////////////////////////////
float AllLightArea(float3 hitPosition)
{
    int NumEmissive = (int) numEmissive.x;
    float sumSize = 0.0f;
    for (int j = 0; j < NumEmissive; j++)
    {
        sumSize += LightArea(j, hitPosition);
    }
    
    if (useImageBasedLighting)
    {
        sumSize += IBL_Area();
    }
    
    return sumSize;
}

///////////////////////////////////////////LightPDF////////////////////////////////////////////////
float LightPDF(int emIndex, float3 hitPosition)
{
    float lightArea = 0.0f;
    if (emIndex >= 0)
    {
        lightArea = LightArea(emIndex, hitPosition);
    }
    else if (useImageBasedLighting)
    {
        lightArea = IBL_Area();
    }
    
    float choicePDF = lightArea / AllLightArea(hitPosition);
    float lightPDF = 1.0f / lightArea;
    return choicePDF * lightPDF;
}

