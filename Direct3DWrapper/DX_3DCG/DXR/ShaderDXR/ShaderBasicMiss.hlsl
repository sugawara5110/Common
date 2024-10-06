
///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                           ShaderBasicMiss.hlsl                                        //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "ShaderGlobalParameters.hlsl"

float3 getSkyLight(float3 dir)
{
	float2 uv = float2(atan2(dir.z, dir.x) / 2.0f / PI + 0.5f, acos(dir.y) / PI);
	return g_texImageBasedLighting.SampleLevel(g_samLinear, uv, 0.0);
}

[shader("miss")]
void basicMiss(inout RayPayload payload)
{
	payload.color = float3(0.0, 0.0, 0.0);
	payload.hit = false;
	payload.reTry = false;

	if (useImageBasedLighting)
	{
		payload.color = getSkyLight(WorldRayDirection());
		payload.hitPosition = WorldRayOrigin() + RayTCurrent() * WorldRayDirection();
		payload.hit = true;
	}
}
