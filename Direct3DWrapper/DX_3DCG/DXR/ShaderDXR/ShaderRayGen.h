///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                           ShaderRayGen.hlsl                                           //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

char* ShaderRayGen =

//共通
"void rayGenIn()\n"
"{\n"
"    uint2 index = DispatchRaysIndex().xy;\n"//この関数を実行しているピクセル座標を取得
"    uint2 dim = DispatchRaysDimensions();\n"//画面全体の幅と高さを取得
"    float2 screenPos = (index + 0.5f) / dim * 2.0f - 1.0f;\n"

"    Seed = SeedFrame;\n"

"    RayDesc ray;\n"
//光線の原点, ここが光線スタート, 視線から始まる
"    ray.Origin = cameraPosition.xyz;\n"
//光線の方向
"    float4 world = mul(float4(screenPos.x, -screenPos.y, 0, 1), projectionToWorld);\n"
"    world.xyz /= world.w;\n"
"    ray.Direction = normalize(world.xyz - ray.Origin);\n"

"    RayPayload payload;\n"
//TraceRay(AccelerationStructure, 
//         RAY_FLAG, 
//         InstanceInclusionMask, 
//         HitGroupIndex, 使用するhitShaderのIndex
//         MultiplierForGeometryContributionToShaderIndex, 
//         MissShaderIndex, 使用するmissShaderのIndex
//         RayDesc, 
//         Payload);
//この関数からRayがスタートする
//payloadに各hit, miss シェーダーで計算された値が格納される
"    payload.RecursionCnt = 0;\n"
"    payload.hitPosition = ray.Origin;\n"
"    gDepthOut[index] = 1.0f;\n"
"    payload.depth = 1.0f;\n"
"    payload.normal = float3(0.0f, 0.0f, 0.0f);\n"
"    payload.hitInstanceId = -1;\n"
"    payload.throughput = float3(1.0f, 1.0f, 1.0f);\n"
"    payload.mNo = NONREFLECTION;\n"

"    traceRay(payload.RecursionCnt, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, 0, 0, ray, payload);\n"

"    gDepthOut[index] = payload.depth;\n"

"    gNormalMap[index] = float4(payload.normal, 1.0f);\n"

"    gInstanceIdMap[index] = payload.hitInstanceId;\n"

"    float3 col = payload.color;\n"
"    float3 colsatu;\n"
"    colsatu.x = saturate(col.x);\n"
"    colsatu.y = saturate(col.y);\n"
"    colsatu.z = saturate(col.z);\n"

"    if(traceMode != 0){\n"
"       float4 prev_screenPos4 = mul(float4(payload.hitPosition, 1.0f), prevViewProjection);\n"
"       float2 prev_screenPos = (prev_screenPos4.xy / prev_screenPos4.z * payload.depth);\n"
"       prev_screenPos.y *= -1.0f;\n"
"       uint2 prevInd = (prev_screenPos + 1.0f) * dim * 0.5f;\n"
"       float prevDepth = gPrevDepthOut[prevInd];\n"
"       float3 prevNor = gPrevNormalMap[prevInd].xyz;\n"
"       float crruentDepth = gDepthOut[index];\n"
"       float3 crruentNor = gNormalMap[index].xyz;\n"

"       float frameReset = frameReset_DepthRange_NorRange.x;\n"
"       float DepthRange = frameReset_DepthRange_NorRange.y;\n"
"       float NorRange = frameReset_DepthRange_NorRange.z;\n"

"       if(abs(prevDepth - crruentDepth) <= DepthRange && dot(prevNor, crruentNor) >= NorRange){\n"
"          gFrameIndexMap[index]++;\n"
"       }\n"
"       else{\n"
"          gFrameIndexMap[index] = 0;\n"
"       }\n"

"       if(frameReset == 1.0f)gFrameIndexMap[index] = 0;\n"

"       const float CMA_Ratio = 1.0f / ((float)gFrameIndexMap[index] + 1.0f);\n"
"       float3 prev = gOutput[index];\n"
"       float3 le = lerp(prev, colsatu, CMA_Ratio);\n"
"       gOutput[index] = float4(le, 1.0f);\n"
"    }\n"
"    else{\n"
"       gOutput[index] = float4(colsatu, 1.0f);\n"
"    }\n"
"}\n"

//通常
"[shader(\"raygeneration\")]\n"
"void rayGen()\n"
"{\n"
"    rayGenIn();\n"
"}\n"

//InstanceIdMapテスト用
"[shader(\"raygeneration\")]\n"
"void rayGen_InstanceIdMapTest()\n"
"{\n"
"    rayGenIn();\n"
"    uint2 index = DispatchRaysIndex(); \n"
"    int id = gInstanceIdMap[index];\n"
"    int numInstance = numEmissive.y;\n"
"    float3 ret = float3(0.0f, 0.0f, 0.0f);\n"

"    int step = 4096 / numInstance;\n"
"    int color3 = step;\n"

"    for(int i = 0; i < numInstance; i++){\n"
"       int r = color3 >> 8;\n"
"       int g = (color3 >> 4) & 0x00f;\n"
"       int b = color3 & 0x00f;\n"
"       float rf = float(r) / 15.0f;\n"
"       float gf = float(g) / 15.0f;\n"
"       float bf = float(b) / 15.0f;\n"
"       if(id == i){\n"
"         ret = float3(rf, gf, bf);\n"
"         break;\n"
"       }\n"
"       color3 += step;\n"
"    }\n"

"    gOutput[index] = float4(ret, 1.0f);\n"
"}\n";
