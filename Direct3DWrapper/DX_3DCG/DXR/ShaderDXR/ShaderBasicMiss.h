
///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                           ShaderBasicMiss.hlsl                                        //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

char* ShaderBasicMiss =

"float3 getSkyLight(float3 dir)"
"{\n"
"    float2 uv = float2(atan2(dir.z, dir.x) / 2.0f / PI + 0.5f, acos(dir.y) / PI);\n"
"    return g_texImageBasedLighting.SampleLevel(g_samLinear, uv, 0.0);\n"
"}\n"

"[shader(\"miss\")]\n"
"void basicMiss(inout RayPayload payload)\n"
"{\n"
"    payload.color = float3(0.0, 0.0, 0.0);\n"
"    payload.hit = false;\n"
"    payload.reTry = false;\n"

"    if(useImageBasedLighting){\n"
"       payload.color = getSkyLight(WorldRayDirection());\n"
"       payload.hitPosition = WorldRayOrigin() + RayTCurrent() * WorldRayDirection();\n"
"       payload.hit = true;\n"
"    }\n"
"}\n";
